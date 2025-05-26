/* src/core/json_parser.h
 *
 * Sigma.Build JSON Parser
 * A JSON parser for Sigma.Build.
 *
 * David Boarman
 * 2025-05-25
 *
 * JSON_PARSER_VERSION "0.00.01"
 *
 * This file provides an interface for parsing JSON files used in the Sigma.Build project.
 * The SigmaJay Parser is used to read configuration files and other JSON data structures relevant
 * to the Sigma.Build project.
 */

#ifndef JSON_PARSER_H
#define JSON_PARSER_H

#include "core.h"
#include "builder.h"

typedef struct build_config_s
{
   string name;          // Name of the build configuration
   string log_file;      // Log file for the build configuration
   BuildTarget *targets; // Array of build targets for the configuration
   string *variables;    // Array of key-value pairs for configuration variables
} build_config_s;

/**
 * @brief IParser interface.
 * @details Provides an interface for parsing JSON files.
 *          This interface defines the functions that can be used to parse JSON data
 *          and retrieve configuration information from JSON files.
 */
typedef struct IParser
{
   /**
    * @brief Get the version of the JSON parser.
    * @return :a string representing the version of the JSON parser
    * @details This function returns the version of the JSON parser being used.
    */
   const char *(*get_version)(void);
   /**
    * @brief Parses a JSON file and returns the build configuration.
    * @param filename :the name of the JSON file to parse
    * @return :a pointer to the BuildConfig structure containing the parsed configuration
    */
   BuildConfig (*parse)(const char *filename);
   void (*dispose)(BuildConfig);
} IParser;

extern const IParser JParse; // Global JSON parser instance

#endif // JSON_PARSER_H