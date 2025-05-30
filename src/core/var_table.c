/* src/core/var_table.c
 * Sigma.Build Variable Lookup
 * Manages variable key-value pairs for placeholder substitution.
 *
 * David Boarman
 * 2025-05-27
 */
#include "var_table.h"
#include <string.h>
#include <stdlib.h>

static char **keys = NULL;
static char **values = NULL;
static int var_count = 0;

// Forward declaration
static void table_cleanup(void);
static int table_lookup_key(const char *, char **);

/* Load variables into table */
static void table_load_vars(cJSON *variables)
{
    table_cleanup(); // Clear existing cache
    if (!variables || !cJSON_IsObject(variables))
    {
        keys = malloc(sizeof(char *));
        values = malloc(sizeof(char *));
        keys[0] = NULL;
        values[0] = NULL;
        var_count = 0;
        return;
    }

    var_count = cJSON_GetArraySize(variables);
    keys = malloc((var_count + 1) * sizeof(char *));
    values = malloc((var_count + 1) * sizeof(char *));
    int i = 0;
    cJSON *var;
    cJSON_ArrayForEach(var, variables)
    {
        keys[i] = strdup(var->string);
        values[i] = strdup(var->valuestring);
        i++;
    }
    keys[i] = NULL;
    values[i] = NULL;
}
/* No frills lookup */
static int table_lookup_key(const char *key, char **value)
{
    (*value) = NULL;
    for (int i = 0; i < var_count; i++)
    {
        if (keys[i] && strcmp(keys[i], key) == 0)
        {
            (*value) = values[i];
        }
    }

    return (*value) != NULL;
}
/* Dispose of the table */
static void table_cleanup(void)
{
    if (keys)
    {
        for (int i = 0; i < var_count; i++)
        {
            free(keys[i]);
            free(values[i]);
        }
        free(keys);
        free(values);
        keys = NULL;
        values = NULL;
        var_count = 0;
    }

    Logger.fwriteln(stdout, "VarLookup cleanup complete.");
}

const IVarTable VarTable = {
    .load = table_load_vars,
    .lookup = table_lookup_key,
    .dispose = table_cleanup,
};