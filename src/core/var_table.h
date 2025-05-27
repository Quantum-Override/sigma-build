/* src/core/var_lookup.h
 * Sigma.Build Variable Lookup
 * Manages variable key-value pairs for placeholder substitution.
 *
 * David Boarman
 * 2025-05-27
 */
#ifndef VAR_TABLE_H
#define VAR_TABLE_H

#include "core.h"
#include <lib/cjson/cJSON.h>

typedef struct IVarTable {
    void (*init)(cJSON *variables);
    char* (*lookup)(const char *key);
    void (*cleanup)(void);
} IVarLookup;

extern const IVarTable VarTable;

#endif // VAR_TABLE_H