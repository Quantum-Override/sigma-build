/* src/core/cli_parser.c
 *
 * Sigma.Build CLI Parser
 * A command line interface parser for Sigma.Build.
 *
 * David Boarman
 * 2025-05-25
 */

#include "cli_parser.h"
#include <string.h>

#define CLI_PARSER_VERSION "0.00.02.001"

// Function to get the version of the CLI parser
const char *cli_parser_get_version(void) {
   return CLI_PARSER_VERSION; // Return the version of the CLI parser
}
// Function to parse command line arguments
void cli_parse_args(int argc, char **argv, CLIOptions *options, CLIErrorCode *error) {
   *error = CLI_SUCCESS; // Initialize error code to success

   for (int i = 1; i < argc; i++) {
      if (strcmp(argv[i], OPT_SHOW_HELP) == 0) {
         // Display help message
         (*options)->show_help = 1;
         *error = CLI_SUCCESS; // No error, just show about information

         // zero out other options
         (*options)->show_about = 0;         // Reset about flag
         (*options)->config_file = NULL;     // Reset config file
         (*options)->log_level = LOG_NORMAL; // Reset log level to default
         (*options)->debug_level = DBG_INFO; // Reset debug level to default
      } else if (strcmp(argv[i], OPT_SHOW_ABOUT) == 0) {
         // Display version information
         (*options)->show_about = 1;
         *error = CLI_SUCCESS; // No error, just show about information

         // zero out other options
         (*options)->show_help = 0;          // Reset help flag
         (*options)->config_file = NULL;     // Reset config file
         (*options)->log_level = LOG_NORMAL; // Reset log level to default
         (*options)->debug_level = DBG_INFO; // Reset debug level to default
      } else if (strncmp(argv[i], OPT_LOG_LEVEL, strlen(OPT_LOG_LEVEL)) == 0) {
         // Set the log level based on the provided option
         int level = atoi(argv[i] + strlen(OPT_LOG_LEVEL));
         if (level < 0 || level > 2) {
            (*options)->show_about = 0;
            (*options)->show_help = 1;

            (*options)->log_stream = stderr;    // Set log stream to stderr for error messages
            *error = CLI_ERR_PARSE_INVALID_ARG; // Invalid log level
            return;
         }

         (*options)->log_level = (LogLevel)level; // Set the log level
      } else if (strncmp(argv[i], OPT_DBG_LEVEL, strlen(OPT_DBG_LEVEL)) == 0) {
         // Set the debug level based on the provided option
         int level = atoi(argv[i] + strlen(OPT_DBG_LEVEL));
         if (level < 0 || level > 4) {
            (*options)->show_about = 0;
            (*options)->show_help = 1;

            (*options)->log_stream = stderr;    // Set log stream to stderr for error messages
            *error = CLI_ERR_PARSE_INVALID_ARG; // Invalid debug level
            return;
         }

         (*options)->debug_level = (DebugLevel)level; // Set the debug level
      } else if (strcmp(argv[i], OPT_LOG_VERBOSE) == 0) {
         // Set the log level to verbose
         (*options)->is_verbose = 1; // Set log level to verbose
      } else if (strncmp(argv[i], OPT_BUILD_CONFIG, strlen(OPT_BUILD_CONFIG)) == 0) {
         // Set the configuration file path
         if (i + 1 < argc) {
            char *arg = strdup(argv[i + 1]); // Get the next argument: `config.json[:target]`
            if (!arg) {
               (*options)->log_stream = stderr; // Set log stream to stderr for error messages
               *error = CLI_ERR_PARSE_FAILED;   // Memory allocation failed
               return;
            }
            char *colon = strchr(arg, ':'); // Check for a colon in the argument
            char *config_file = arg;
            char *target_name = NULL;
            if (colon) {
               *colon = '\0'; // Split the string at the colon
               target_name = strdup(colon + 1);
               if (!target_name) {
                  free(arg);
                  (*options)->log_stream = stderr;
                  *error = CLI_ERR_PARSE_FAILED;
                  return;
               }
            }
            // validate the config file path
            FILE *file = NULL;
            if (!(file = fopen(config_file, "r"))) {
               free(config_file);
               free(target_name);
               (*options)->log_stream = stderr;
               *error = CLI_ERR_PARSE_INVALID_CONFIG;
               return;
            }

            (*options)->config_file = config_file; // Set the configuration file path
            (*options)->target_name = target_name; // Set the target name if provided
            fclose(file);                          // Close the file after checking
            ++i;                                   // Move to the next argument
         } else {
            (*options)->log_stream = stderr;       // Set log stream to stderr for error messages
            *error = CLI_ERR_PARSE_MISSING_CONFIG; // Missing value for config file option
            return;
         }
      } else {
         (*options)->log_stream = stderr;       // Set log stream to stderr for error messages
         *error = CLI_ERR_PARSE_UNKNOWN_OPTION; // Unknown option provided
         return;
      }
   }
}

const struct ICLI CLI = {
    .get_version = cli_parser_get_version, // Function to get the version of the CLI parser
    .parse_args = cli_parse_args,          // Assign the CLI initialization function
};