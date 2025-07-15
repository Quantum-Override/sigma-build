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
   cJSON *dir = cJSON_GetObjectItemCaseSensitive(json, CONFIG_FIELD_DIR);
   // If dir is not specified, use the current working directory
   string cwd = NULL;
   if (!cJSON_IsString(dir)) {
      Directories.get_wd(&cwd);
      if (!cwd) {
         Logger.debug(stderr, LOG_NORMAL, DBG_ERROR,
                      "Failed to get current working directory.\n");
         cJSON_Delete(json);
         goto loadExit;
      }
   } else {
      string raw_dir = strdup(dir->valuestring);
      if (!raw_dir) {
         Logger.debug(stderr, LOG_NORMAL, DBG_ERROR,
                      "Failed to allocate memory for working directory.\n");
         cJSON_Delete(json);
         goto loadExit;
      }
      cwd = resolve_vars(raw_dir);
      free(raw_dir);
      if (!cwd) {
         Logger.debug(stderr, LOG_NORMAL, DBG_ERROR,
                      "Failed to resolve working directory: %s\n", dir->valuestring);
         cJSON_Delete(json);
         goto loadExit;
      }
   }
   (*config)->cwd = strdup(cwd); // Set the current working directory
   free(cwd);                    // Free the raw directory string after resolving

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

      // we can check the target for a NULL cwd here; if NULL, assign context cwd
      if (!(*config)->targets[i]->cwd) {
         (*config)->targets[i]->cwd = strdup((*config)->cwd);
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
   char *raw_build_dir = NULL, *raw_out_dir = NULL, *raw_output = NULL;

   // Initialize - no need for NULL assignments since Resources.alloc uses calloc
   cJSON *name = cJSON_GetObjectItemCaseSensitive(target_json, CONFIG_TARGET_NAME);
   if (!cJSON_IsString(name)) goto fail;
   cJSON *type = cJSON_GetObjectItemCaseSensitive(target_json, CONFIG_TARGET_TYPE);
   if (!cJSON_IsString(type)) goto fail;

   cJSON *dir = cJSON_GetObjectItemCaseSensitive(target_json, CONFIG_TARGET_DIR);
   // if we have a dir, resolve it
   if (cJSON_IsString(dir)) {
      string raw_dir = strdup(dir->valuestring);
      if (!raw_dir) {
         Logger.debug(stderr, LOG_NORMAL, DBG_ERROR,
                      "Failed to allocate memory for working directory.\n");
         goto fail;
      }
      target->cwd = resolve_vars(raw_dir);
      free(raw_dir);
      if (!target->cwd) {
         Logger.debug(stderr, LOG_NORMAL, DBG_ERROR,
                      "Failed to resolve working directory: %s\n", dir->valuestring);
         goto fail;
      }
   } else {
      // NULL is fine, it will use the config's cwd
      target->cwd = NULL;
   }

   target->name = strdup(name->valuestring);
   target->type = strdup(type->valuestring);
   if (!target->name || !target->type) goto fail;

   if (strcmp(target->type, TARGET_TYPE_OP) == 0) {
      cJSON *commands = cJSON_GetObjectItemCaseSensitive(target_json, CONFIG_TARGET_COMMANDS);
      target->commands = load_platform_commands(commands);
      if (!target->commands) goto fail;
      return target;
   }

   // Handle executable target
   cJSON *sources = cJSON_GetObjectItemCaseSensitive(target_json, CONFIG_TARGET_SOURCES);
   target->sources = load_string_array(sources);
   if (!target->sources) goto fail;

   cJSON *build_dir = cJSON_GetObjectItemCaseSensitive(target_json, CONFIG_TARGET_BUILD_DIR);
   if (cJSON_IsString(build_dir)) {
      raw_build_dir = strdup(build_dir->valuestring);
      target->build_dir = raw_build_dir ? resolve_vars(raw_build_dir) : NULL;
      free(raw_build_dir);
      if (!target->build_dir) goto fail;
   }

   cJSON *compiler = cJSON_GetObjectItemCaseSensitive(target_json, CONFIG_TARGET_COMPILER);
   if (cJSON_IsString(compiler)) {
      target->compiler = strdup(compiler->valuestring);
      if (!target->compiler) goto fail;
   }

   target->c_flags = load_string_array(
       cJSON_GetObjectItemCaseSensitive(target_json, CONFIG_TARGET_COMPILER_FLAGS));
   target->ld_flags = load_string_array(
       cJSON_GetObjectItemCaseSensitive(target_json, CONFIG_TARGET_LINKER_FLAGS));

   cJSON *out_dir = cJSON_GetObjectItemCaseSensitive(target_json, CONFIG_TARGET_OUTDIR);
   if (cJSON_IsString(out_dir)) {
      raw_out_dir = strdup(out_dir->valuestring);
      target->out_dir = raw_out_dir ? resolve_vars(raw_out_dir) : NULL;
      free(raw_out_dir);
   } else {
      target->out_dir = target->build_dir;
   }

   cJSON *output = cJSON_GetObjectItemCaseSensitive(target_json, CONFIG_TARGET_OUTPUT);
   if (cJSON_IsString(output)) {
      raw_output = strdup(output->valuestring);
      target->output = raw_output ? resolve_vars(raw_output) : NULL;
      free(raw_output);
   } else {
      target->output = strdup(target->name);
   }
   if (!target->output) goto fail;

   return target;

fail:
   free(raw_build_dir);
   free(raw_out_dir);
   free(raw_output);
   Resources.dispose_target(target);

   return NULL;
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