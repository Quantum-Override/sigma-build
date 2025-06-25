/* src/core/var_table.c
 * Sigma.Build Variable Lookup
 * Manages variable key-value pairs for placeholder substitution.
 *
 * David Boarman
 * 2025-05-27
 */
#include "var_table.h"
#include <stdlib.h>
#include <string.h>

static char **keys = NULL;
static char **values = NULL;
static int var_count = 0;

// Forward declaration
static void table_cleanup(void);
static int table_lookup_key(const char *, char **);

/* Load variables into table */
static void table_load_vars(cJSON *variables) {
   table_cleanup(); // Clear existing cache
   addr keys_addr, values_addr;

   if (!variables || !cJSON_IsObject(variables)) {
      // Allocate NULL-terminated empty arrays
      if (Resources.alloc(&keys_addr, sizeof(char *)))
         keys = (char **)keys_addr;
      if (Resources.alloc(&values_addr, sizeof(char *)))
         values = (char **)values_addr;

      if (keys) keys[0] = NULL;
      if (values) values[0] = NULL;
      var_count = 0;
      return;
   }

   var_count = cJSON_GetArraySize(variables);

   // Allocate arrays using Resources.alloc
   if (!Resources.alloc(&keys_addr, (var_count + 1) * sizeof(char *))) {
      Logger.debug(stderr, LOG_NORMAL, DBG_ERROR, "Failed to allocate keys array\n");
      return;
   }
   keys = (char **)keys_addr;
   if (!Resources.alloc(&values_addr, (var_count + 1) * sizeof(char *))) {
      Logger.debug(stderr, LOG_NORMAL, DBG_ERROR, "Failed to allocate values array\n");
      free(keys); // Cleanup partial allocation
      keys = NULL;
      return;
   }
   values = (char **)values_addr;

   // Populate arrays
   int i = 0;
   cJSON *var;
   cJSON_ArrayForEach(var, variables) {
      keys[i] = strdup(var->string);        // Still uses strdup for strings
      values[i] = strdup(var->valuestring); // (Could wrap with Resources.alloc later)
      i++;
   }
   keys[i] = NULL;
   values[i] = NULL;
}
/* Poor man's lookup */
static int table_lookup_key(const char *key, char **value) {
   (*value) = NULL;
   for (int i = 0; i < var_count; i++) {
      if (keys[i] && strcmp(keys[i], key) == 0) {
         (*value) = values[i];
         break;
      }
   }

   if ((*value) == NULL) {
      Logger.debug(stderr, LOG_NORMAL, DBG_ERROR, "Unknown Variable: %s\n", key);
   }
   return (*value) != NULL;
}
/* Dispose of the table */
static void table_cleanup(void) {
   if (keys) {
      for (int i = 0; i < var_count; i++) {
         free(keys[i]);
         free(values[i]);
      }
      free(keys);
      free(values);
      keys = NULL;
      values = NULL;
      var_count = 0;
   }
}

const IVarTable VarTable = {
    .load = table_load_vars,
    .lookup = table_lookup_key,
    .dispose = table_cleanup,
};