/* src/core/cli_parser.h
 *  Header for the Sigma.Build CLI parser.
 *
 * David Boarman
 * 2025-05-25
 *
 * SIGMABUILD_VERSION "0.00.01"
 * CLI_PARSER_VERSION "0.00.01"
 *
 * This file provides an interface for parsing command line options
 * used in the Sigma.Build project. It defines the CLIOptions structure
 * and the ICLI interface for parsing command line arguments.
 */
#ifndef CLI_PARSER_H
#define CLI_PARSER_H

#include "core.h"

#define OPT_SHOW_HELP "--help"     // Option to show help
#define OPT_SHOW_ABOUT "--about"   // Option to show version information
#define OPT_BUILD_CONFIG "--build" // Option to specify a build configuration file
#define OPT_LOG_LEVEL "--log="     // Option to set the log level (0-2)
#define OPT_LOG_VERBOSE "-v"       // Option for verbose logging (only observed with --about && --help)

/**
 * @brief CLIOptions structure.
 * @details Provides an interface for parsing command line options.
 */
typedef struct ICLI {
   /**
    * @brief Get the version of the CLI parser.
    * @return :the version of the CLI parser as a string
    * @details This function returns the version of the CLI parser.
    */
   const char *(*get_version)(void); // Function to get the version of the CLI parser
   /**
    * @brief Initializes the CLI parser with command line arguments.
    * @param argc :the number of command line arguments
    * @param argv :the command line arguments
    */
   void (*parse_args)(int argc, char **, CLIOptions *, CLIErrorCode *); // Function to parse command line arguments
} ICLI;

extern const ICLI CLI; // Global CLI instance

#endif // CLI_PARSER_H
