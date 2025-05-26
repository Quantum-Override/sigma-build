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

struct build_config_s; // Forward declaration of build_config_s structure

typedef struct build_config_s *BuildConfig; // BuildConfig is a pointer to the build_config_s structure

/**
 * @brief IParser interface.
 * @details Provides an interface for parsing JSON files.
 *          This interface defines the functions that can be used to parse JSON data
 *          and retrieve configuration information from JSON files.
 */
typedef struct IParser
{
   /**
    * @brief Parses a JSON file and returns the build configuration.
    * @param filename :the name of the JSON file to parse
    * @return :a pointer to the BuildConfig structure containing the parsed configuration
    */
   BuildConfig (*parse)(const char *filename);
   void (*dispose)(BuildConfig);
} IParser;

extern struct IParser JParse; // Global JSON parser instance

#endif // JSON_PARSER_H