/* src/sigbuild.c
 *
 * Sigma.Build
 * A build system for C projects. (it make work for C++ projects too)
 *
 * David Boarman
 * 2025-05-25
 *
 */

#include "core.h"
#include "cli_parser.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <time.h>

#define SIGMABUILD_VERSION "0.00.01"
#define SIGMABUILD_NAME "Sigma.Build"

CLIState cli_state = NULL;   // Global variable to hold the current CLI state
BuildContext context = NULL; // Global variable to hold the current build context

void cli_init(int, char **);

static void log_message(FILE *, const char *, va_list);
static void log_debug(DebugLevel, const char *, ...);
void writef(const char *, ...);
void writelnf(const char *, ...);
void fwritef(FILE *, const char *, ...);
void fwritelnf(FILE *, const char *, ...);
void fdebugf(FILE *, LogLevel, DebugLevel, const char *, ...);

// For dynamic log level annotation
static const char *DBG_LEVELS[] = {
    "DEBUG",
    "INFO",
    "WARNING",
    "ERROR",
    "FATAL",
    NULL,
};

// General Helper Functions
// Function to get the current timestamp in a specified format
void get_timestamp(char *buffer, const char *format)
{
   time_t now = time(NULL);
   strftime(buffer, 32, format, localtime(&now));
}

// CLI App Functions
// This function initializes the CLI state with the provided arguments
void cli_init(int argc, char **args)
{
   // initialize the CLI state
   cli_state = (CLIState)malloc(sizeof(struct cli_state_s));
   if (!cli_state)
   {
      fdebugf(stderr, LOG_NORMAL, DBG_ERROR, "Failed to allocate memory for CLI state.\n");
      exit(EXIT_FAILURE);
   }
   cli_state->argv = args;
   cli_state->argc = argc;
   cli_state->options = (CLIOptions)malloc(sizeof(struct cli_options_s));
   if (!cli_state->options)
   {
      fdebugf(stderr, LOG_NORMAL, DBG_ERROR, "Failed to allocate memory for CLI options.\n");
      free(cli_state);
      exit(EXIT_FAILURE);
   }
   cli_state->options->show_help = 0;
   cli_state->options->show_version = 0;
   cli_state->options->config_file = NULL;     // No config file by default
   cli_state->options->log_level = LOG_NORMAL; // Default log level
   cli_state->options->debug_level = DBG_INFO; // Default debug level

   // Parse command line arguments
   CLI.parse_args(argc, args, &cli_state->options, &cli_state->error);
   if (cli_state->error != CLI_SUCCESS)
   {
      // Handle parsing errors
      switch (cli_state->error)
      {
      case CLI_ERROR_PARSE_INVALID_ARG:
         fdebugf(stderr, LOG_NORMAL, DBG_ERROR, "Invalid argument provided.\n");
         break;
      case CLI_ERROR_PARSE_MISSING_OPTION:
         fdebugf(stderr, LOG_NORMAL, DBG_ERROR, "Required option is missing.\n");
         break;
      case CLI_ERROR_PARSE_UNKNOWN_OPTION:
         fdebugf(stderr, LOG_NORMAL, DBG_ERROR, "Unknown option provided.\n");
         break;
      case CLI_ERROR_PARSE_FAILED:
         fdebugf(stderr, LOG_NORMAL, DBG_ERROR, "Failed to parse command line arguments.\n");
         break;
      default:
         fdebugf(stderr, LOG_NORMAL, DBG_ERROR, "An unknown error occurred during parsing.\n");
         break;
      }
      exit(EXIT_FAILURE);
   }
}

/* Helper function to write formatted output to the log stream */
// base logging function
static void log_message(FILE *stream, const char *fmt, va_list args)
{
   vfprintf(stream, fmt, args);
   fflush(stream);
}
// This function is used to write formatted messages to the log stream
void writef(const char *fmt, ...)
{
   va_list args;
   va_start(args, fmt);

   log_message(context->log_stream, fmt, args);

   va_end(args);
}
// This function is used to write formatted messages with a newline to the log stream
void writelnf(const char *fmt, ...)
{
   va_list args;
   va_start(args, fmt);

   // append a newline to the format string
   char msg[1024];
   snprintf(msg, sizeof(msg), "%s\n", fmt);
   log_message(context->log_stream, msg, args);

   va_end(args);
}
// This function is used to write formatted messages to the given stream
void fwritef(FILE *stream, const char *fmt, ...)
{
   va_list args;
   va_start(args, fmt);

   stream = stream ? stream : stdout; // Default to stdout if no stream is provided
   log_message(stream, fmt, args);

   va_end(args);
}
// This function is used to write formatted messages with a newline to the given stream
void fwritelnf(FILE *stream, const char *fmt, ...)
{
   va_list args;
   va_start(args, fmt);

   stream = stream ? stream : stdout; // Default to stdout if no stream is provided
   char msg[1024];
   snprintf(msg, sizeof(msg), "%s\n", fmt);
   log_message(stream, msg, args);

   va_end(args);
}
// Debug logging function
void fdebugf(FILE *stream, LogLevel log_level, DebugLevel debug_level, const char *fmt, ...)
{
   if (log_level == LOG_NONE)
   {
      return; // No output for NONE
   }

   va_list args;
   va_start(args, fmt);

   if (log_level == LOG_NORMAL)
   {
      char dbg_label[16];
      char dbg_msg[256];
      if (debug_level > DBG_INFO)
      {
         // Only ERROR+ for LOG_MINIMAL: add debug label based on debug level
         if (debug_level >= DBG_ERROR)
         {
            snprintf(dbg_label, sizeof(dbg_label), "[%s]", DBG_LEVELS[debug_level]);
         }
         else
         {
            // debug label unknown
            snprintf(dbg_label, sizeof(dbg_label), "[UNKNOWN]");
         }

         // prepend label to message
         snprintf(dbg_msg, sizeof(dbg_msg), "%-10s %s", dbg_label, fmt);
         log_message(stream, dbg_msg, args);
      }
      else
      {
         // Minimal: No debug label
         log_message(stream, fmt, args);
      }
   }
   else if (log_level == LOG_VERBOSE && debug_level >= context->debug_level)
   {
      char dbg_label[16];
      char dbg_msg[16];
      if (debug_level >= 0 && debug_level < sizeof(DBG_LEVELS) / sizeof(DBG_LEVELS[0]) - 1)
      {
         snprintf(dbg_label, sizeof(dbg_label), "[%s]", DBG_LEVELS[debug_level]);
      }
      else
      {
         snprintf(dbg_label, sizeof(dbg_label), "[UNKNOWN]");
      }
      // prepend label to message
      snprintf(dbg_msg, sizeof(dbg_msg), "%-10s %s", dbg_label, fmt);
      log_message(stream, dbg_msg, args);
   }

   va_end(args);
}

// Global logger instance
const ILogger Logger = {
    .write = writef,
    .writeln = writelnf,
    .fwrite = fwritef,
    .fwriteln = fwritelnf,
    .debug = fdebugf,
};
// Global application instance
const IApplication App = {
    .init = cli_init,
    .run = NULL,     // To be set by the application
    .cleanup = NULL, // To be set by the application
};