/* src/core/json_parser.c
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

#define CONFIG_LOADER_VERSION "0.00.02"

static const char *loader_get_version(void) {
   return CONFIG_LOADER_VERSION; // Return the version of the JSON parser
}

// Forward declaration of of loader functions
static char **load_string_array(cJSON *array);
static BuildTarget load_target(cJSON *target_json);
static char *replace_vars(const char *);
static void loader_cleanup(void);

/* Load configuration for Build */
static int loader_load_config(const char *filename, BuildConfig *config) {
   if (!config || !(*config)) {
      Logger.debug(stderr, LOG_NORMAL, DBG_ERROR, "BuildConfig must be allocated: %s\n",
                   filename);
      goto loadExit;
   }
   if (!filename) {
      Logger.debug(stderr, LOG_NORMAL, DBG_ERROR, "Configuration file must not be null\n");
      goto loadExit;
   }

   FILE *fp = fopen(filename, "r");
   if (!fp) {
      Logger.debug(stderr, LOG_NORMAL, DBG_ERROR, "Failed to open config: %s\n",
                   filename);
      goto loadExit;
   }
   fseek(fp, 0, SEEK_END);
   long size = ftell(fp);
   fseek(fp, 0, SEEK_SET);
   char *buffer = malloc(size + 1);
   if (!buffer) {
      fclose(fp);
      Logger.debug(stderr, LOG_NORMAL, DBG_ERROR,
                   "Memory allocation failed for config buffer.\n");
      goto loadExit;
   }
   // Read the file content into the buffer
   size_t read = fread(buffer, 1, size, fp);
   // if bytes read is 0 or < size, handle error
   if (read < size) {
      free(buffer);
      fclose(fp);
      Logger.debug(stderr, LOG_NORMAL, DBG_ERROR,
                   "Failed to read config file: %s\n", filename);
      goto loadExit;
   }
   // Null-terminate the buffer
   buffer[size] = '\0';
   fclose(fp);

   cJSON *json = cJSON_Parse(buffer);
   free(buffer);
   if (!json) {
      Logger.debug(stderr, LOG_NORMAL, DBG_ERROR, "JSON load error: %s\n",
                   cJSON_GetErrorPtr());
      goto loadExit;
   }

   cJSON *name = cJSON_GetObjectItemCaseSensitive(json, "name");
   cJSON *log_file = cJSON_GetObjectItemCaseSensitive(json, "log_file");
   cJSON *targets = cJSON_GetObjectItemCaseSensitive(json, "targets");
   cJSON *variables = cJSON_GetObjectItemCaseSensitive(json, "variables");
   VarTable.load(variables);

   (*config)->name = cJSON_IsString(name) ? strdup(name->valuestring) : NULL;
   (*config)->log_file =
       cJSON_IsString(log_file) ? strdup(log_file->valuestring) : NULL;

   // Load targets
   int target_count = cJSON_IsArray(targets) ? cJSON_GetArraySize(targets) : 0;
   (*config)->targets = malloc((target_count + 1) * sizeof(BuildTarget));
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
         free(config);
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

   int count = cJSON_GetArraySize(array);
   char **result = malloc((count + 1) * sizeof(char *));

   for (int i = 0; i < count; i++) {
      cJSON *item = cJSON_GetArrayItem(array, i);
      char *raw = cJSON_IsString(item) ? strdup(item->valuestring) : strdup("");
      result[i] = replace_vars(raw);

      free(raw);
   }

   result[count] = NULL;
   return result;
}
/* Load build target */
static BuildTarget load_target(cJSON *target_json) {
   BuildTarget target = malloc(sizeof(struct build_target_s));
   if (!target) {
      Logger.debug(stderr, LOG_NORMAL, DBG_ERROR,
                   "Failed to allocate memory for build target.\n");
      return NULL;
   }
   cJSON *name = cJSON_GetObjectItemCaseSensitive(target_json, "name");
   cJSON *type = cJSON_GetObjectItemCaseSensitive(target_json, "type");
   cJSON *sources = cJSON_GetObjectItemCaseSensitive(target_json, "sources");
   cJSON *build_dir = cJSON_GetObjectItemCaseSensitive(target_json, "build_dir");
   cJSON *compiler = cJSON_GetObjectItemCaseSensitive(target_json, "compiler");
   cJSON *c_flags =
       cJSON_GetObjectItemCaseSensitive(target_json, "compiler_flags");
   cJSON *ld_flags =
       cJSON_GetObjectItemCaseSensitive(target_json, "linker_flags");

   target->name = cJSON_IsString(name) ? strdup(name->valuestring) : NULL;
   target->type = cJSON_IsString(type) ? strdup(type->valuestring) : NULL;
   target->sources = load_string_array(sources);
   char *raw_build_dir =
       cJSON_IsString(build_dir) ? strdup(build_dir->valuestring) : NULL;
   target->build_dir = raw_build_dir ? replace_vars(raw_build_dir) : NULL;
   target->compiler =
       cJSON_IsString(compiler) ? strdup(compiler->valuestring) : NULL;
   target->c_flags = load_string_array(c_flags);
   target->ld_flags = load_string_array(ld_flags);

   if (!target->name || !target->type || !target->sources ||
       !target->build_dir || !target->compiler) {
      Logger.debug(stderr, LOG_NORMAL, DBG_ERROR,
                   "Missing required fields in build target.\n");
      free(target->name);
      free(target->type);
      for (char **src = target->sources; src && *src; src++)
         free(*src);
      free(target->sources);
      free(target->build_dir);
      free(target->compiler);
      for (char **flag = target->c_flags; flag && *flag; flag++)
         free(*flag);
      free(target->c_flags);
      for (char **flag = target->ld_flags; flag && *flag; flag++)
         free(*flag);
      free(target->ld_flags);
      free(target);
      return NULL;
   }
   return target;
}
/* Raplces variable symbols with the value in VarTable */
static char *replace_vars(const char *input) {
   if (!input) return strdup("");
   char *result = strdup(input);
   char *temp = NULL;
   char *start = result;

   while ((start = strchr(start, '{'))) {
      char *end = strchr(start, '}');
      if (!end) break;
      *end = '\0';
      char *key = start + 1;
      char *value;

      if (VarTable.lookup(key, &value)) {
         size_t prefix_len = start - result;
         size_t suffix_len = strlen(end + 1);
         temp = malloc(prefix_len + strlen(value) + suffix_len + 1);
         strncpy(temp, result, prefix_len);
         temp[prefix_len] = '\0';
         strcat(temp, value);
         strcat(temp, end + 1);
         free(result);
         result = temp;
         temp = NULL;
      }

      start = result + (end - result) + 1;
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