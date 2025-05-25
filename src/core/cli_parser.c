/* src/core/cli_parser.c
 *
 * Sigma.Build CLI Parser
 * A command line interface parser for Sigma.Build.
 *
 * David Boarman
 * 2025-05-25
 */

#include "cli_parser.h"

#define CLI_PARSER_VERSION "0.00.01"

#define OPT_SHOW_HELP "--help"     // Option to show help
#define OPT_SHOW_VERSION "--about" // Option to show version information
#define OPT_LOG_LEVEL "--log="     // Option to set the log level (0-2)

void cli_parse_args(int argc, char **argv, CLIOptions *options, CLIErrorCode *error)
{
   CLIOptions opts = *options;
   for (int i = 1; i < argc; i++)
   {
      // Handle options starting with '-'
      if (strcmp(argv[i], OPT_SHOW_HELP) == 0)
      {
         // Display help message
         opts->show_help = 1;
      }
      else if (strcmp(argv[i], OPT_SHOW_VERSION) == 0)
      {
         // Display version information
         opts->show_version = 1;
      }
      else if (strncmp(argv[i], OPT_LOG_LEVEL, strlen(OPT_LOG_LEVEL)) == 0)
      {
         // Set the log level based on the provided option
         int level = atoi(argv[i] + strlen(OPT_LOG_LEVEL));
         if (level < 0 || level > 2)
         {
            opts->show_version = 0;
            opts->show_help = 1;
            *error = CLI_ERROR_PARSE_INVALID_ARG; // Invalid log level

            return;
         }
         else
         {
            opts->log_level = (LogLevel)level; // Set the log level
         }
      }
   }
}

const struct ICLI CLI = {
    .parse_args = cli_parse_args, // Assign the CLI initialization function
};