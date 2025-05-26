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

/**
 * @brief CLIOptions structure.
 * @details Provides an interface for parsing command line options.
 */
typedef struct ICLI
{
   void (*parse_args)(int argc, char **, CLIOptions *, CLIErrorCode *); // Function to parse command line arguments
} ICLI;

extern const ICLI CLI; // Global CLI instance

#endif // CLI_PARSER_H
