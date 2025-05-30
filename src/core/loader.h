/* src/core/json_parser.h
 *
 * Sigma.Build Configuration Loader
 * A JSON-based configuration loader for Sigma.Build.
 *
 * David Boarman
 * 2025-05-25
 *
 * CONFIG_LOADER_VERSION "0.00.02"
 *
 * This file provides an interface for loading the JSON-based configuration used for the Sigma.Build toolchain.
 *
 */

#ifndef CONFIG_LOADER_H
#define CONFIG_LOADER_H

#include "core.h"
#include "builder.h"

/**
 * @brief ILoader interface.
 * @details Provides an interface for parsing JSON files.
 *          This interface defines the functions that can be used to load JSON data
 *          and retrieve configuration information from JSON files.
 */
typedef struct ILoader
{
   /**
    * @brief Get the version of the JSON parser.
    * @return :a string representing the version of the JSON parser
    * @details This function returns the version of the JSON parser being used.
    */
   const char *(*get_version)(void);
   /**
    * @brief Parses a JSON file and returns the build configuration.
    * @param filename :the name of the JSON file to load
    * @return :a pointer to the BuildConfig structure containing the parsed configuration
    */
   BuildConfig (*load_config)(const char *filename);
   /**
    * @brief Clean up resources used by the loader.
    * @details This function is used to clean up any resources used by the loader.
    */
   void (*cleanup)(void);
} ILoader;

extern const ILoader Loader; // Global JSON parser instance

#endif // CONFIG_LOADER_H