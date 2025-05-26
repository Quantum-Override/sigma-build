/* src/core/json_parser.c
 *
 * Sigma.Build JSON Parser
 * A JSON parser for Sigma.Build.
 *
 * David Boarman
 * 2025-05-25
 *
 */

#include "json_parser.h"
// #include <cjson/cJSON.h>
#include <stdlib.h> // For NULL definition

#define JSON_PARSER_VERSION "0.00.01"

BuildConfig parser_build_config(const char *filename)
{
   Logger.fwriteln(stderr, "JSON Parser: Parsing file %s", filename);

   BuildConfig config = malloc(sizeof(struct build_config_s));
   if (!config)
   {
      Logger.fwriteln(stderr, "JSON Parser: Failed to allocate memory for build configuration.");
      return NULL; // Return NULL if memory allocation fails
   }

   Logger.debug(stdout, LOG_VERBOSE, DBG_INFO, "JSON Parser: Successfully allocated memory for bootstrap build configuration.");
   // Here you would typically use a JSON library to parse the file
   config->name = strdup("bootstrap_build");        // Example default value
   config->log_file = strdup("logs/bootstrap.log"); // Example default value

   return config;
}

void parser_dispose_config(BuildConfig config)
{
   if (config)
   {
      free(config->name);
      free(config->log_file);
      free(config);
   }
}

const IParser JParse = {
    .parse = parser_build_config,     // Initialize the parse function to NULL
    .dispose = parser_dispose_config, // Dispose of the build configuration
};
