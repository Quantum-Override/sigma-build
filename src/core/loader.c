/* src/core/loader.c
 *
 * Sigma.Build JSON Parser
 * A JSON parser for Sigma.Build.
 *
 * David Boarman
 * 2025-05-25
 *
 */

#include "loader.h"
#include "cJSON.h"
#include "var_table.h"
#include <stdlib.h>
#include <string.h>

#define CONFIG_LOADER_VERSION "0.00.02.002"

static const char *loader_get_version(void) {
   return CONFIG_LOADER_VERSION; // Return the version of the JSON parser
}

// Forward declaration of of loader functions
static char **load_string_array(cJSON *array);
static BuildTarget load_target(cJSON *target_json);
static char **load_platform_commands(cJSON *);
static char *resolve_vars(const char *);
static void loader_cleanup(void);

/* Load configuration for Build */
static int loader_load_config(const char *filename, BuildConfig *config) {
   if (!filename) {
      Logger.debug(stderr, LOG_NORMAL, DBG_ERROR, "Configuration file name cannot be null.\n");
      goto loadExit;
   }
   if (!config) {
      Logger.debug(stderr, LOG_NORMAL, DBG_ERROR, "Invalid configuration structure.\n");
      goto loadExit;
   }
   char *buffer;
   size_t bytes_read = 0;
   if ((bytes_read = Files.read(filename, &buffer) <= 0)) {
      Logger.debug(stderr, LOG_NORMAL, DBG_ERROR, "Failed to read configuration from file: %s\n", filename);
      return LOADER_ERR_FILE_READ;
   }
   cJSON *json = cJSON_Parse(buffer);
   free(buffer);
   if (!json) {
      Logger.debug(stderr, LOG_NORMAL, DBG_ERROR, "JSON load error: %s\n",
                   cJSON_GetErrorPtr());
      goto loadExit;
   }

   cJSON *name = cJSON_GetObjectItemCaseSensitive(json, CONFIG_FIELD_NAME);
   cJSON *log_file = cJSON_GetObjectItemCaseSensitive(json, CONFIG_FIELD_LOG_FILE);
   cJSON *targets = cJSON_GetObjectItemCaseSensitive(json, CONFIG_FIELD_TARGETS);
   cJSON *variables = cJSON_GetObjectItemCaseSensitive(json, CONFIG_FIELD_VARIABLES);
   cJSON *default_target = cJSON_GetObjectItemCaseSensitive(json, CONFIG_FIELD_DEFAULT_TARGET);
   VarTable.load(variables);

   (*config)->name = cJSON_IsString(name) ? strdup(name->valuestring) : NULL;
   char *raw_log_file =
       cJSON_IsString(log_file) ? strdup(log_file->valuestring) : NULL;
   if (raw_log_file) {
      (*config)->log_file = resolve_vars(raw_log_file);
      free(raw_log_file);
   }
   (*config)->default_target =
       cJSON_IsString(default_target) ? strdup(default_target->valuestring) : NULL;
   // Load targets
   int target_count = cJSON_IsArray(targets) ? cJSON_GetArraySize(targets) : 0;
   addr targets_addr;
   if (!Resources.alloc(&targets_addr, (target_count + 1) * sizeof(BuildTarget))) {
      Logger.debug(stderr, LOG_NORMAL, DBG_ERROR,
                   "Failed to allocate memory for targets array.\n");
      cJSON_Delete(json);
      VarTable.dispose();
      goto loadExit;
   }
   (*config)->targets = (BuildTarget *)targets_addr;

   for (int i = 0; i < target_count; i++) {
      cJSON *target_json = cJSON_GetArrayItem(targets, i);
      (*config)->targets[i] = load_target(target_json);
      if (!(*config)->targets[i]) {
         for (int j = 0; j < i; j++) {
            Resources.dispose_target((*config)->targets[j]);
         }
         free((*config)->targets);
         free((*config)->name);
         free((*config)->log_file);
         for (char **var = (*config)->variables; var && *var; var++)
            free(*var);
         free((*config)->variables);
         free((*config));
         cJSON_Delete(json);
         VarTable.dispose();

         goto loadExit;
      }
   }
   (*config)->targets[target_count] = NULL;

   cJSON_Delete(json);
   Logger.fwriteln(stdout, "Parsed config: %s", filename);

loadExit:
   return (*config) != NULL;
}
/* Load string array */
static char **load_string_array(cJSON *array) {
   if (!array || !cJSON_IsArray(array))
      return NULL;

   addr result_addr;
   int count = cJSON_GetArraySize(array);

   // Allocate pointer array using resources_alloc
   if (!Resources.alloc(&result_addr, (count + 1) * sizeof(char *))) {
      return NULL;
   }
   char **result = (char **)result_addr;

   // Process each array element
   for (int i = 0; i < count; i++) {
      cJSON *item = cJSON_GetArrayItem(array, i);
      char *raw = cJSON_IsString(item) ? strdup(item->valuestring) : strdup("");
      result[i] = resolve_vars(raw); // Transfers ownership to result[i]
      free(raw);                     // Free the intermediate copy
   }

   result[count] = NULL; // NULL-terminate the array
   return result;
}
/* Load build target */
static BuildTarget load_target(cJSON *target_json) {
   addr target_addr;
   if (!Resources.alloc(&target_addr, sizeof(struct build_target_s))) {
      Logger.debug(stderr, LOG_NORMAL, DBG_ERROR,
                   "Failed to allocate memory for build target.\n");
      return NULL;
   }
   BuildTarget target = (BuildTarget)target_addr;

   cJSON *name = cJSON_GetObjectItemCaseSensitive(target_json, CONFIG_TARGET_NAME);
   cJSON *type = cJSON_GetObjectItemCaseSensitive(target_json, CONFIG_TARGET_TYPE);

   target->name = cJSON_IsString(name) ? strdup(name->valuestring) : NULL;
   target->type = cJSON_IsString(type) ? strdup(type->valuestring) : NULL;
   if (strcmp(target->type, TARGET_TYPE_OP) == 0) {
      // If the target type is "clean", we don't need to load other fields
      target->sources = NULL;
      target->build_dir = NULL;
      target->compiler = NULL;
      target->c_flags = NULL;
      target->ld_flags = NULL;

      // parse commands
      cJSON *commands = cJSON_GetObjectItemCaseSensitive(target_json, CONFIG_TARGET_COMMANDS);
      target->commands = load_platform_commands(commands);

      return target; // Return early for clean targets
   }

   // otherwise our only other type for now is executable
   cJSON *sources = cJSON_GetObjectItemCaseSensitive(target_json, CONFIG_TARGET_SOURCES);
   cJSON *build_dir = cJSON_GetObjectItemCaseSensitive(target_json, CONFIG_TARGET_BUILD_DIR);
   cJSON *compiler = cJSON_GetObjectItemCaseSensitive(target_json, CONFIG_TARGET_COMPILER);
   cJSON *c_flags =
       cJSON_GetObjectItemCaseSensitive(target_json, CONFIG_TARGET_COMPILER_FLAGS);
   cJSON *ld_flags =
       cJSON_GetObjectItemCaseSensitive(target_json, CONFIG_TARGET_LINKER_FLAGS);
   cJSON *out_dir =
       cJSON_GetObjectItemCaseSensitive(target_json, CONFIG_TARGET_OUTDIR);
   cJSON *output =
       cJSON_GetObjectItemCaseSensitive(target_json, CONFIG_TARGET_OUTPUT);

   target->sources = load_string_array(sources);
   char *raw_build_dir =
       cJSON_IsString(build_dir) ? strdup(build_dir->valuestring) : NULL;
   target->build_dir = raw_build_dir ? resolve_vars(raw_build_dir) : NULL;
   target->compiler =
       cJSON_IsString(compiler) ? strdup(compiler->valuestring) : NULL;
   target->c_flags = load_string_array(c_flags);
   target->ld_flags = load_string_array(ld_flags);
   char *raw_out_dir =
       cJSON_IsString(out_dir) ? strdup(out_dir->valuestring) : NULL;
   target->out_dir = raw_out_dir ? resolve_vars(raw_out_dir) : target->build_dir;
   char *raw_output =
       cJSON_IsString(output) ? strdup(output->valuestring) : NULL;
   target->output = raw_output ? resolve_vars(raw_output) : strdup(target->name);

   if (!target->name || !target->type || !target->sources ||
       !target->build_dir || !target->compiler) {
      Logger.debug(stderr, LOG_NORMAL, DBG_ERROR,
                   "Missing required fields in build target.\n");
      Resources.dispose_target(target);

      return NULL;
   }
   return target;
}
/* Load platform commands*/
static char **load_platform_commands(cJSON *commands) {
   if (!commands || !cJSON_IsObject(commands)) {
      Logger.debug(stderr, LOG_NORMAL, DBG_ERROR, "Invalid or missing commands.\n");
      return NULL; // No commands to load
   }
   char platform[8] =
#ifdef _WIN32
       "windows";
#else
       "linux";
#endif
   cJSON *platform_commands = cJSON_GetObjectItemCaseSensitive(commands, platform);
   if (!platform_commands || !cJSON_IsArray(platform_commands)) {
      Logger.debug(stderr, LOG_NORMAL, DBG_ERROR,
                   "No commands found for platform: %s\n", platform);
      return NULL;
   }

   return load_string_array(platform_commands);
}
/* Raplaces variable symbols with the value in VarTable */
static char *resolve_vars(const char *input) {
   if (!input) return strdup("");

   char *result = strdup(input);
   if (!result) return NULL; // Handle allocation failure

   char *start = result;
   while ((start = strchr(start, '{'))) {
      char *end = strchr(start, '}');
      if (!end) break; // No closing brace, stop

      // Extract the key (between '{' and '}')
      *end = '\0';
      char *key = start + 1;

      // Look up the value
      char *value;
      if (!VarTable.lookup(key, &value)) {
         *end = '}'; // Restore the original character
         start = end + 1;
         continue; // Skip if key not found
      }

      // Calculate lengths
      size_t prefix_len = start - result;
      size_t value_len = strlen(value);
      size_t suffix_len = strlen(end + 1);

      // Allocate new buffer
      addr buffer_addr;
      if (!Resources.alloc(&buffer_addr, prefix_len + value_len + suffix_len + 1)) {
         free(result);
         return NULL; // Allocation failed
      }
      char *buffer = (char *)buffer_addr;

      // Construct the new string
      memcpy(buffer, result, prefix_len);
      memcpy(buffer + prefix_len, value, value_len);
      memcpy(buffer + prefix_len + value_len, end + 1, suffix_len + 1); // +1 for null terminator

      // Update pointers and free old memory
      free(result);
      result = buffer;
      start = result + prefix_len + value_len; // Continue after the replaced part
   }

   return result;
}

/* Loader clean up resources */
static void loader_cleanup(void) {
   VarTable.dispose();
}

const ILoader Loader = {
    .load_config = loader_load_config,
    .get_version = loader_get_version,
    .cleanup = loader_cleanup,
};