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
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <time.h>

#define SIGMABUILD_VERSION "0.00.01"
#define SIGMABUILD_NAME "Sigma.Build"

CLIState cli_state = NULL;   // Global variable to hold the current CLI state
BuildContext context = NULL; // Global variable to hold the current build context

void cli_init(int, char **);
void cli_init_context(void);
int cli_load_config(void);
void cli_run(void);
static void cli_cleanup(void);
const char *cli_get_err_msg(CLIErrorCode);

static void log_message(FILE *, const char *, va_list);
static void log_debug(DebugLevel, const char *, ...);
void writef(const char *, ...);
void writelnf(const char *, ...);
void fwritef(FILE *, const char *, ...);
void fwritelnf(FILE *, const char *, ...);
void fdebugf(FILE *, LogLevel, DebugLevel, const char *, ...);

// For dynamic log level annotation
static const char *DEBUG_LEVELS[] = {
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
   // Register the cleanup function to be called on exit
   if (atexit(cli_cleanup) != 0)
   {
      perror("Failed to register cleanup function");
      exit(EXIT_FAILURE);
   }

   // initialize the CLI context & state
   cli_init_context();
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
   cli_state->options->show_about = 0;
   cli_state->options->config_file = NULL;     // No config file by default
   cli_state->options->log_level = LOG_NORMAL; // Default log level
   cli_state->options->debug_level = DBG_INFO; // Default debug level

   // Parse command line arguments
   CLI.parse_args(argc, args, &cli_state->options, &cli_state->error);
   if (cli_state->error != CLI_SUCCESS)
   {
      // Handle parsing errors
      fdebugf(stderr, LOG_NORMAL, DBG_ERROR, "Error parsing command line arguments: %s\n",
              cli_get_err_msg(cli_state->error));
      exit(EXIT_FAILURE);
   }
   // Update build context with the parsed options
   context->log_level = cli_state->options->log_level;                     // Set log level from options
   context->debug_level = cli_state->options->debug_level;                 // Set debug level from options
   context->log_stream = cli_state->options->is_verbose ? stdout : stderr; // Set log stream based on verbosity
   context->project_name = SIGMABUILD_NAME;                                // Set default project name
   context->current_target = NULL;                                         // No current target by default
   context->current_configuration = cli_state->options->config_file;       // Set current configuration from options
   context->data = NULL;                                                   // No additional data by default
}
// This function initializes the build context with default values
void cli_init_context(void)
{
   // Allocate memory for the build context
   context = (BuildContext)malloc(sizeof(struct build_context_s));
   if (!context)
   {
      fdebugf(stderr, LOG_NORMAL, DBG_ERROR, "Failed to allocate memory for build context.\n");
      exit(EXIT_FAILURE);
   }

   // Initialize the build context with default values
   context->log_level = LOG_NONE;           // Default log level is NONE
   context->debug_level = DBG_INFO;         // Default debug level is INFO
   context->log_stream = stdout;            // Default log stream is stdout
   context->current_target = NULL;          // No current target by default
   context->project_name = SIGMABUILD_NAME; // Default project name
   context->current_configuration = NULL;   // No current configuration by default
   context->data = NULL;                    // No additional data by default
}
int cli_load_config(void)
{
   writelnf("Loading configuration file: %s", cli_state->options->config_file ? cli_state->options->config_file : "None");
   if (cli_state->options->config_file == NULL)
   {
      fdebugf(stderr, LOG_NORMAL, DBG_ERROR, "Configuration file is missing.\n");
      return CLI_ERROR_PARSE_MISSING_CONFIG; // Return error if config file is missing
   }
   // Here you would typically load the configuration file
   // For now, we just simulate a successful load
   if (strcmp(cli_state->options->config_file, "invalid.json") == 0)
   {
      fdebugf(stderr, LOG_NORMAL, DBG_ERROR, "Invalid configuration file specified: %s\n", cli_state->options->config_file);
      return CLI_ERROR_PARSE_INVALID_CONFIG; // Return error if config file is invalid
   }

   // If the configuration file is valid, we can proceed
   writelnf("Configuration file loaded successfully: %s", cli_state->options->config_file);

   return CLI_SUCCESS; // Return success if config file is loaded successfully
}
// Run the build application
void cli_run(void)
{
   writelnf("Starting Sigma.Build");
}
// Cleanup function to free resources allocated during the CLI initialization
static void cli_cleanup(void)
{
   if (cli_state)
   {
      if (cli_state->options)
      {
         free(cli_state->options);
      }
      free(cli_state);
   }
   if (context)
   {
      // Free any resources allocated in the context
      free(context);
   }
}

// Get the error message for a given CLIErrorCode
const char *cli_get_err_msg(CLIErrorCode code)
{
   switch (code)
   {
   case CLI_SUCCESS:
      return "No error";
   case CLI_ERROR_PARSE_INVALID_ARG:
      return "Invalid argument provided";
   case CLI_ERROR_PARSE_MISSING_OPTION:
      return "Required option is missing";
   case CLI_ERROR_PARSE_INVALID_CONFIG:
      return "Invalid or NULL configuration file specified";
   case CLI_ERROR_PARSE_MISSING_CONFIG:
      return "Configuration file is missing";
   case CLI_ERROR_PARSE_UNKNOWN_OPTION:
      return "Unknown option provided";
   case CLI_ERROR_PARSE_FAILED:
      return "Failed to parse command line arguments";
   case JSON_ERROR_INVALID_FORMAT:
      return "Invalid JSON format";
   case JSON_ERROR_MISSING_FIELD:
      return "Required field is missing in JSON";
   case JSON_ERROR_INVALID_FIELD:
      return "Invalid field in JSON";
   case JSON_ERROR_UNKNOWN_FIELD:
      return "Unknown field in JSON";
   case JSON_ERROR_PARSE_FAILED:
      return "Failed to parse JSON";
   case JSON_ERROR_FILE_NOT_FOUND:
      return "JSON file not found";
   case JSON_ERROR_FILE_READ:
      return "Error reading JSON file";
   case JSON_ERROR_FILE_EMPTY:
      return "Empty JSON file";
   default:
      return "Unknown error code"; // Default case for unknown error codes
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
   // Suppress non-error messages if context or caller specify LOG_NONE.
   if ((log_level == LOG_NONE || context->log_level == LOG_NONE) && debug_level < DBG_ERROR)
   {
      return; // No output for NONE
   }

   va_list args;
   va_start(args, fmt);

   // build debug label
   char dbg_label[16] = "[UNKNOWN]";
   if (debug_level >= 0 && debug_level < sizeof(DEBUG_LEVELS) / sizeof(DEBUG_LEVELS[0]) - 1)
   {
      snprintf(dbg_label, sizeof(dbg_label), "[%s]", DEBUG_LEVELS[debug_level]);
   }
   //  build the message
   char msg[1024];
   int use_label = (log_level == LOG_VERBOSE || context->log_level == LOG_VERBOSE || debug_level >= DBG_ERROR);
   snprintf(msg, sizeof(msg), use_label ? "%-10s %s" : "%s", use_label ? dbg_label : "", fmt);

   // Log if:
   // 1. context->log_level == LOG_VERBOSE (log everything)
   // 2. log_level != LOG_NONE && log_level >= context->log_level && debug_level >= context->debug_level
   // 3. debug_level >= DBG_ERROR (always log errors)
   if (context->log_level == LOG_VERBOSE ||
       (log_level != LOG_NONE && log_level >= context->log_level && debug_level >= context->debug_level) ||
       debug_level >= DBG_ERROR)
   {
      log_message(stream, msg, args);
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
    .run = cli_run,
    .cleanup = cli_cleanup,
    .get_err_msg = cli_get_err_msg,
};