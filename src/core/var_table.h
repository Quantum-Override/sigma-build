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
#include <cJSON.h>

/**
 * @brief IVarTable interface.
 * @details Provides an interface for managing variable key-value pairs.
 *          This interface defines the functions that can be used to load,
 *          lookup, and dispose of variables in the variable table.
 */
typedef struct IVarTable
{
    /**
     * @brief Loads variables from a cJSON object.
     * @param json :the cJSON object containing the variables
     * @details This function loads variables from the provided cJSON object
     *          and populates the variable table.
     */
    void (*load)(cJSON *);
    /**
     * @brief Looks up a variable by its key.
     * @param key :the key of the variable to look up
     * @param value :the key's value as a string
     * @return :return 1 if key found; otherwise, 0
     * @details This function retrieves the value associated with the given key
     *          from the variable table.
     */
    int (*lookup)(const char *, char **);
    /**
     * @brief Disposes of the variable table.
     * @details This function cleans up and frees any resources used by the variable table.
     */
    void (*dispose)(void); //  treat this module as an object
} IVarTable;

extern const IVarTable VarTable;

#endif // VAR_TABLE_H