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

#define CLI_PARSER_VERSION "0.00.01"

#define OPT_SHOW_HELP "--help"     // Option to show help
#define OPT_SHOW_ABOUT "--about"   // Option to show version information
#define OPT_CONFIG_FILE "--config" // Option to specify a configuration file
#define OPT_LOG_LEVEL "--log="     // Option to set the log level (0-2)
#define OPT_LOG_VERBOSE "-v"       // Option for verbose logging (only observed with --about && --help

void cli_parse_args(int argc, char **argv, CLIOptions *options, CLIErrorCode *error)
{
   CLIOptions opts = *options;
   *error = CLI_SUCCESS; // Initialize error code to success

   for (int i = 1; i < argc; i++)
   {
      if (strcmp(argv[i], OPT_SHOW_HELP) == 0)
      {
         // Display help message
         opts->show_help = 1;
         *error = CLI_SUCCESS; // No error, just show about information

         // zero out other options
         opts->show_help = 0;          // Reset help flag
         opts->config_file = NULL;     // Reset config file
         opts->log_level = LOG_NONE;   // Reset log level to default
         opts->debug_level = DBG_INFO; // Reset debug level to default
      }
      else if (strcmp(argv[i], OPT_SHOW_ABOUT) == 0)
      {
         // Display version information
         opts->show_about = 1;
         *error = CLI_SUCCESS; // No error, just show about information

         // zero out other options
         opts->show_help = 0;          // Reset help flag
         opts->config_file = NULL;     // Reset config file
         opts->log_level = LOG_NONE;   // Reset log level to default
         opts->debug_level = DBG_INFO; // Reset debug level to default
      }
      else if (strncmp(argv[i], OPT_LOG_LEVEL, strlen(OPT_LOG_LEVEL)) == 0)
      {
         // Set the log level based on the provided option
         int level = atoi(argv[i] + strlen(OPT_LOG_LEVEL));
         if (level < 0 || level > 2)
         {
            opts->show_about = 0;
            opts->show_help = 1;

            *error = CLI_ERROR_PARSE_INVALID_ARG; // Invalid log level
            return;
         }

         opts->log_level = (LogLevel)level; // Set the log level
      }
      else if (strcmp(argv[i], OPT_LOG_VERBOSE) == 0)
      {
         // Set the log level to verbose
         opts->is_verbose = 1; // Set log level to verbose
      }
      else if (strncmp(argv[i], OPT_CONFIG_FILE, strlen(OPT_CONFIG_FILE)) == 0)
      {
         // Set the configuration file path
         if (i + 1 < argc)
         {
            char config_file = strdup(argv[i + 1]); // Get the next argument as the config file
            // validate the config file path
            if (config_file == NULL)
            {
               *error = CLI_ERROR_PARSE_FAILED; // Memory allocation failed
               return;
            }
            else
            {
               FILE *file = NULL;
               if (!(file = fopen(config_file, "r"))) // Check if the file exists
               {
                  free(config_file); // Free the allocated memory for config file

                  *error = CLI_ERROR_PARSE_INVALID_CONFIG; // Invalid or NULL configuration file
                  return;
               }

               opts->config_file = config_file; // Set the configuration file path
               fclose(file);                    // Close the file after checking
               ++i;                             // Move to the next argument

               continue;
            }
         }
         else
         {
            *error = CLI_ERROR_PARSE_MISSING_CONFIG; // Missing value for config file option
            return;
         }
      }
      else
      {
         *error = CLI_ERROR_PARSE_UNKNOWN_OPTION; // Unknown option provided
         return;
      }
   }
}

const struct ICLI CLI = {
    .parse_args = cli_parse_args, // Assign the CLI initialization function
};