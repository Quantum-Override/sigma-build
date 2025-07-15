/* src/sigbuild.c
 *
 * Sigma.Build
 * A build system for C projects. (it make work for C++ projects too)
 *
 * David Boarman
 * 2025-05-25
 *
 */

#include "sbuild.h"
#include "core/builder.h"
#include "core/cli_parser.h"
#include "core/loader.h"
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
#define PATH_MAX MAX_PATH
#else
#include <limits.h>
#include <unistd.h>
#endif

// Version and name definitions
#define SIGMABUILD_VERSION "0.00.03.001"
#define SIGMABUILD_NAME "Sigma.Build"

// Define a maximum path length for file operations
#define PATH_MAX 4096

CLIState cli_state = NULL;   // Global variable to hold the current CLI state
BuildContext context = NULL; // Global variable to hold the current build context

// CLI declarations
void cli_init(int, char **);
void cli_init_context(void);
void cli_init_state(int, char **);
int cli_load_config(void);
void cli_run(void);
static void cli_cleanup(void);
const char *cli_get_err_msg(CLIErrorCode);
void cli_display_help(void);
void cli_display_about(void);

// Logger declarations
static void logger_log_message(FILE *, const char *, va_list);
FILE *logger_get_log_stream(void);
void logger_writef(const char *, ...);
void logger_writelnf(const char *, ...);
void logger_fwritef(FILE *, const char *, ...);
void logger_fwritelnf(FILE *, const char *, ...);
void logger_fdebugf(FILE *, LogLevel, DebugLevel, const char *, ...);

// Resources declarations
int resources_alloc(addr *, size_t);
void resources_dispose_config(BuildConfig);
void resources_dispose_target(BuildTarget);

// Files declarations
size_t files_read_file(const char *, char **);
int files_file_exists(const char *);
int files_get_file_path(const char *path, string *out);

// Directories declarations
void directories_get_wd(string *);
int directories_set_wd(const char *);

// Paths declarations
int paths_get_absolute_path(const char *, string *);
int paths_get_file_name(const char **);

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
void get_timestamp(char *buffer, const char *format) {
   time_t now = time(NULL);
   strftime(buffer, 32, format, localtime(&now));
}
// Function to get the target configuration by name
BuildTarget get_target(const char *name) {
   if (!context || !context->config || !context->config->targets) {
      logger_fdebugf(stderr, LOG_NORMAL, DBG_ERROR, "No configuration loaded or no targets defined.\n");
      return NULL; // Return NULL if no configuration or targets are available
   }
   for (BuildTarget *target = context->config->targets; target && *target; target++) {
      if (strcmp((*target)->name, name) == 0) {
         return *target; // Return the target if found
      }
   }

   logger_fdebugf(stderr, LOG_NORMAL, DBG_ERROR, "Target '%s' not found in configuration.\n", name);
   return NULL; // Return NULL if target not found
}

// CLI App Functions
// This function initializes the CLI state with the provided arguments
void cli_init(int argc, char **args) {
   // Register the cleanup function to be called on exit
   if (atexit(cli_cleanup) != 0) {
      perror("Failed to register cleanup function");
      exit(EXIT_FAILURE);
   }

   // initialize the CLI context & state
   cli_init_context();
   cli_init_state(argc, args);
   // Parse command line arguments
   CLI.parse_args(argc, args, &cli_state->options, &cli_state->error);
   if (cli_state->error != CLI_SUCCESS) {
      // Handle parsing errors
      logger_fdebugf(stderr, LOG_NORMAL, DBG_ERROR, "Error parsing command line arguments: %s\n",
                     cli_get_err_msg(cli_state->error));
      exit(EXIT_FAILURE);
   } else {
      // validate config_file
      if (!Files.file_exists(cli_state->options->config_file)) {
         logger_fdebugf(stderr, LOG_NORMAL, DBG_ERROR, "Configuration file does not exist: %s\n",
                        cli_state->options->config_file);
         exit(EXIT_FAILURE);
      }
      string cfg_dir = NULL;
      files_get_file_path(cli_state->options->config_file, &cfg_dir);

      context->config_file = cli_state->options->config_file; // Set the config file in the context
      paths_get_file_name((const char **)&context->config_file);

      // check if cwd and the config file paths are the same
      if (strcmp(cli_state->options->original_path, cfg_dir) != 0) {
         // since the paths don't match, we need to change the cwd to match the config file path
         if (!directories_set_wd(cfg_dir)) {
            free(cfg_dir);
            cli_state->options->log_stream = stderr;         // Set log stream to stderr for error messages
            cli_state->error = CLI_ERR_PARSE_INVALID_CONFIG; // Failed to change directory to config file path
            return;
         }
         free(cli_state->options->original_path);     // Free the original path
         cli_state->options->original_path = cfg_dir; // Set the original path to the config file path
      }
   }
   // Update build context with the parsed options
   context->log_level = cli_state->options->log_level;     // Set log level from options
   context->debug_level = cli_state->options->debug_level; // Set debug level from options
   context->cwd = cli_state->options->original_path;       // Set current working directory from options
   context->log_stream = cli_state->options->log_stream;   // Set log stream based on verbosity
   context->project_name = SIGMABUILD_NAME;                // Set default project name
}
// This function initializes the build context with default values
void cli_init_context(void) {
   // Allocate memory for the build context
   addr context_addr;
   if (!resources_alloc(&context_addr, sizeof(struct build_context_s))) {
      logger_fdebugf(stderr, LOG_NORMAL, DBG_ERROR, "Failed to allocate memory for build context.\n");
      exit(EXIT_FAILURE);
   }
   context = (BuildContext)context_addr; // Cast the allocated address to BuildContext

   // Initialize the build context with default values
   context->log_level = LOG_NONE;           // Default log level is NONE
   context->debug_level = DBG_INFO;         // Default debug level is INFO
   context->log_stream = stdout;            // Default log stream is stdout
   context->project_name = SIGMABUILD_NAME; // Default project name
}
// This function initializes the CLI state with default values
void cli_init_state(int argc, char **args) {
   addr cli_state_addr; // Pointer to hold the address of CLIState
   if (!resources_alloc(&cli_state_addr, sizeof(struct cli_state_s))) {
      logger_fdebugf(stderr, LOG_NORMAL, DBG_ERROR, "Failed to allocate memory for CLI state.\n");
      exit(EXIT_FAILURE);
   }
   cli_state = (CLIState)cli_state_addr; // Cast the allocated address to CLIState
   cli_state->argv = args;
   cli_state->argc = argc;

   addr options_addr; // Pointer to hold the address of CLIOptions
   if (!resources_alloc(&options_addr, sizeof(struct cli_options_s))) {
      logger_fdebugf(stderr, LOG_NORMAL, DBG_ERROR, "Failed to allocate memory for CLI options.\n");
      free(cli_state);
      exit(EXIT_FAILURE);
   }
   cli_state->options = (CLIOptions)options_addr; // Cast the allocated address to CLIOptions
   // Initialize CLI options with default values
   cli_state->options->show_help = 0;
   cli_state->options->show_about = 0;
   cli_state->options->log_level = LOG_NORMAL;             // Default log level
   cli_state->options->debug_level = DBG_INFO;             // Default debug level
   cli_state->options->is_verbose = 0;                     // Verbose logging is off by default
   cli_state->options->log_stream = stdout;                // Default log stream is stdout
   cli_state->error = CLI_SUCCESS;                         // Initialize error code to success
   directories_get_wd(&cli_state->options->original_path); // Get the original working directory
}
// Load the configuration file specified in the command line options
int cli_load_config(void) {
   logger_writelnf("Loading configuration file: %s", cli_state->options->config_file ? cli_state->options->config_file : "None");
   if (cli_state->options->config_file == NULL) {
      logger_fdebugf(stderr, LOG_NORMAL, DBG_ERROR, "Configuration file is missing.\n");
      return LOADER_ERR_PARSE_FAILED; // Return error if config file is missing
   }

   // Load configuration
   addr config_addr;
   if (!resources_alloc(&config_addr, sizeof(struct build_config_s))) {
      logger_fdebugf(stderr, LOG_NORMAL, DBG_ERROR, "Failed to allocate memory for build configuration.\n");
      return LOADER_ERR_PARSE_FAILED; // Return error if config allocation fails
   }
   context->config = (BuildConfig)config_addr;
   if (!Loader.load_config(cli_state->options->config_file, &context->config)) {
      logger_fdebugf(stderr, LOG_NORMAL, DBG_ERROR, "Failed to load configuration from file: %s\n", cli_state->options->config_file);
      return LOADER_ERR_PARSE_FAILED; // Return error if config file could not be parsed
   }
   // Set the current configuration in the context
   context->config_file = cli_state->options->config_file; // Set the current configuration file
   if (context->config->log_file && cli_state->options->log_stream != stderr) {
      // modify log_sream
      FILE *log_file = fopen(context->config->log_file, "w");
      if (!log_file) {
         logger_fdebugf(stderr, LOG_NORMAL, DBG_ERROR, "Failed to open log file: %s\n", context->config->log_file);
      } else {
         context->log_stream = log_file;
      }
   } else {
      context->log_stream = cli_state->options->log_stream;
   }

   return CLI_SUCCESS; // Return success if config file is loaded successfully
}
// Run the build application
void cli_run(void) {
   if (cli_state->options->show_help) {
      cli_display_help();
      return;
   } else if (cli_state->options->show_about) {
      cli_display_about();
      return;
   }
   // Load the configuration file if specified
   if (cli_load_config() != CLI_SUCCESS) {
      logger_fdebugf(stderr, LOG_NORMAL, DBG_ERROR, "Failed to load configuration file: %s\n",
                     cli_get_err_msg(LOADER_ERR_LOAD_CONFIG));
      exit(EXIT_FAILURE);
   }
   BuildConfig config = context->config; // Get the loaded configuration from the context

   // if we have a target in cli_options, override the default target
   context->current_target = cli_state->options->target_name
                                 ? cli_state->options->target_name
                                 : config->default_target; // Set the default target from options

   BuildTarget target = get_target(context->current_target);
   if (!target) {
      exit(EXIT_FAILURE); // Exit if the target is not found
   } else if (Builder.build(target) != 0) {
      logger_fdebugf(stderr, LOG_NORMAL, DBG_ERROR, "%s: %s\n",
                     cli_get_err_msg(BUILD_ERR_BUILD_TARGET), target->name);
      exit(EXIT_FAILURE); // Exit if any build target fails
   }
}
// Cleanup function to free resources allocated during the CLI initialization
static void cli_cleanup(void) {
   static int is_disposed = 0; // Flag to ensure cleanup is only done once
   if (is_disposed) {
      return;
   }

   // set the working directory back to the original path
   if (cli_state && cli_state->options && cli_state->options->original_path) {
      if (!directories_set_wd(cli_state->options->original_path)) {
         logger_fdebugf(stderr, LOG_NORMAL, DBG_ERROR, "Failed to change directory back to original path: %s\n",
                        cli_state->options->original_path);
      }
   }

   if (context) {
      // Free any resources allocated in the context
      if (context->config) {
         resources_dispose_config((BuildConfig)context->config);
         context->config = NULL;
      }
      // free(context->current_target);

      if (context->log_stream != stdout && context->log_stream != stderr) {
         fclose(context->log_stream);
      }
      free(context);
      context = NULL; // Set to NULL after freeing
   }

   if (cli_state) {
      if (cli_state->options) {
         if (cli_state->options->config_file) {
            free(cli_state->options->config_file);
            cli_state->options->config_file = NULL;
         }
         if (cli_state->options->target_name) {
            free(cli_state->options->target_name);
            cli_state->options->target_name = NULL;
         }
         free(cli_state->options->original_path);  // Free the original path string
         cli_state->options->original_path = NULL; // Set to NULL after freeing

         free(cli_state->options);
         cli_state->options = NULL; // Set to NULL after freeing
      }
      free(cli_state);
      cli_state = NULL; // Set to NULL after freeing
   }

   Loader.cleanup();

   is_disposed = 1; // Set the flag to indicate cleanup has been done
   logger_fdebugf(logger_get_log_stream(), LOG_NORMAL, DBG_INFO, "Cleanup completed for Sigma.Build.\n");
}

// Get the error message for a given CLIErrorCode
const char *cli_get_err_msg(CLIErrorCode code) {
   switch (code) {
   case CLI_SUCCESS:
      return "No error";
   case CLI_ERR_PARSE_INVALID_ARG:
      return "Invalid argument provided";
   case CLI_ERR_PARSE_MISSING_OPTION:
      return "Required option is missing";
   case CLI_ERR_PARSE_INVALID_CONFIG:
      return "Invalid or NULL configuration file specified";
   case CLI_ERR_PARSE_MISSING_CONFIG:
      return "Configuration file is missing";
   case CLI_ERR_PARSE_UNKNOWN_OPTION:
      return "Unknown option provided";
   case CLI_ERR_PARSE_FAILED:
      return "Failed to parse command line arguments";
   case LOADER_ERR_INVALID_FORMAT:
      return "Invalid configuration format";
   case LOADER_ERR_MISSING_FIELD:
      return "Required field is missing in JSON";
   case LOADER_ERR_INVALID_FIELD:
      return "Invalid field in JSON";
   case LOADER_ERR_UNKNOWN_FIELD:
      return "Unknown field in JSON";
   case LOADER_ERR_PARSE_FAILED:
      return "Failed to load JSON";
   case LOADER_ERR_FILE_NOT_FOUND:
      return "JSON file not found";
   case LOADER_ERR_FILE_READ:
      return "Error reading JSON file";
   case LOADER_ERR_FILE_EMPTY:
      return "Empty JSON file";
   case LOADER_ERR_LOAD_CONFIG:
      return "Failed to load configuration file";
   case BUILD_ERR_BUILD_TARGET:
      return "Build target failed";
   default:
      return "Unknown error code"; // Default case for unknown error codes
   }
}
// Display help information for the CLI application
void cli_display_help(void) {
   char *app = strrchr(cli_state->argv[0], '/');
   app = app ? app + 1 : cli_state->argv[0]; // Get the application name from the path
   // Build options string
   char options[128];
   snprintf(options, sizeof(options), "[%s]|[%s]|[%s <file>]|[%s0-2]",
            OPT_SHOW_HELP, OPT_SHOW_ABOUT, OPT_BUILD_CONFIG, OPT_LOG_LEVEL);

   logger_fwritelnf(stdout, "Usage: %s %s", app, options);
   logger_fwritelnf(stdout, "Options:");
   logger_fwritelnf(stdout, "  %-25s Show this help message", OPT_SHOW_HELP);
   logger_fwritelnf(stdout, "  %-25s Show version information", OPT_SHOW_ABOUT);
   logger_fwritelnf(stdout, "  %-9s%-16s Specify the configuration file with optional target", OPT_BUILD_CONFIG, "<file>[:target]");
   logger_fwritelnf(stdout, "  %-6s%-19s Set the log level", OPT_LOG_LEVEL, "(0-2)");
}
// Display application and optional component versions
void cli_display_about(void) {
   if (cli_state->options->is_verbose) {
      logger_fwritelnf(stdout, "Sigma.Build is a build system for C projects.");
      logger_fwritelnf(stdout, "%-15s                    %s", "David Boarman", "05-25-2025");
      logger_fwritelnf(stdout, "Components:");
      logger_fwritelnf(stdout, "  - %-15s%26s", "Core Library", SIGMABUILD_VERSION);
      logger_fwritelnf(stdout, "  - %-15s%26s", "CLI Parser", CLI.get_version());
      logger_fwritelnf(stdout, "  - %-15s%26s", "JSON Loader", Loader.get_version());
      logger_fwritelnf(stdout, "  - %-15s%26s", "Builder", Builder.get_version());
   } else {
      // display simple version - trim last part of the version string.xxx
      char *version = strdup(SIGMABUILD_VERSION);
      char *dot = strrchr(version, '.');
      if (dot) {
         *dot = '\0'; // Trim the last part of the version string
      }
      logger_fwritelnf(stdout, "%s v.%s", SIGMABUILD_NAME, version);
      free(version);
   }
}

/* Logger functions to format output */
// base logging function
static void logger_log_message(FILE *stream, const char *fmt, va_list args) {
   vfprintf(stream, fmt, args);
   fflush(stream);
}
// Get the log stream based on the current context
FILE *logger_get_log_stream(void) {
   if (context && context->log_stream) {
      return context->log_stream; // Return the log stream from the context
   }

   return stdout; // Default to stdout if no log stream is set
}
// This function is used to write formatted messages to the log stream
void logger_writef(const char *fmt, ...) {
   va_list args;
   va_start(args, fmt);

   logger_log_message(context->log_stream, fmt, args);

   va_end(args);
}
// This function is used to write formatted messages with a newline to the log stream
void logger_writelnf(const char *fmt, ...) {
   va_list args;
   va_start(args, fmt);

   // append a newline to the format string
   char msg[1024];
   snprintf(msg, sizeof(msg), "%s\n", fmt);
   logger_log_message(context->log_stream, msg, args);

   va_end(args);
}
// This function is used to write formatted messages to the given stream
void logger_fwritef(FILE *stream, const char *fmt, ...) {
   va_list args;
   va_start(args, fmt);

   stream = stream ? stream : stdout; // Default to stdout if no stream is provided
   logger_log_message(stream, fmt, args);

   va_end(args);
}
// This function is used to write formatted messages with a newline to the given stream
void logger_fwritelnf(FILE *stream, const char *fmt, ...) {
   va_list args;
   va_start(args, fmt);

   stream = stream ? stream : stdout; // Default to stdout if no stream is provided
   char msg[1024];
   snprintf(msg, sizeof(msg), "%s\n", fmt);
   logger_log_message(stream, msg, args);

   va_end(args);
}
// Debug logging function
void logger_fdebugf(FILE *stream, LogLevel log_level, DebugLevel debug_level, const char *fmt, ...) {
   // Suppress non-error messages if context or caller specify LOG_NONE.
   if (log_level <= LOG_NORMAL || ((context ? context->log_level == LOG_NONE : 0) && debug_level < DBG_ERROR)) {
      return; // No output for non-error messages when log level is LOG_NONE
   }

   va_list args;
   va_start(args, fmt);

   // build debug label
   char dbg_label[16] = "[UNKNOWN]";
   if (debug_level >= 0 && debug_level < sizeof(DEBUG_LEVELS) / sizeof(DEBUG_LEVELS[0]) - 1) {
      snprintf(dbg_label, sizeof(dbg_label), "[%s]", DEBUG_LEVELS[debug_level]);
   }
   //  build the message
   char msg[1024];
   int use_label = (log_level == LOG_VERBOSE || context ? context->log_level == LOG_VERBOSE : 1 || debug_level >= DBG_ERROR);
   snprintf(msg, sizeof(msg), use_label ? "%-10s %s" : "%s", use_label ? dbg_label : "", fmt);

   // Log if:
   // 1. context->log_level == LOG_VERBOSE (log everything)
   // 2. log_level != LOG_NONE && log_level >= context->log_level && debug_level >= context->debug_level
   // 3. debug_level >= DBG_ERROR (always log errors)
   if ((context ? context->log_level == LOG_VERBOSE : 1) ||
       (log_level != LOG_NONE && log_level >= (context ? context->log_level : LOG_NORMAL) &&
        debug_level >= (context ? context->debug_level : DBG_DEBUG)) ||
       debug_level >= DBG_ERROR) {
      logger_log_message(stream, msg, args);
   }

   va_end(args);
}

/* Resource functions */
// This function allocates memory for an object of a given size
int resources_alloc(addr *out, size_t size) {
   if (!out || size == 0) {
      logger_fdebugf(stderr, LOG_NORMAL, DBG_ERROR, "Invalid allocation request: NULL object or size is zero.\n");
      return SB_FALSE;
   }
   object obj = calloc(1, size);
   if (!obj) {
      logger_fdebugf(stderr, LOG_NORMAL, DBG_ERROR, "Memory allocation failed: %s\n", strerror(errno));
      return SB_FALSE;
   }
   (*out) = (addr)obj; // Cast the allocated memory to addr type

   return SB_TRUE;
}
// This function disposes of the configuration resources
void resources_dispose_config(BuildConfig config) {
   if (!config)
      return; // Nothing to dispose of

   free(config->name);
   free(config->log_file);
   free(config->cwd);
   for (char **var = config->variables; var && *var; var++)
      free(*var);
   free(config->variables);

   // Dispose of each target
   for (BuildTarget *target = config->targets; target && *target; target++) {
      Resources.dispose_target(*target);
   }
   free(config->targets);
   free(config); // Finally, free the config itself
}
// This function disposes of the target resources
void resources_dispose_target(BuildTarget target) {
   if (!target)
      return; // Nothing to dispose of

   free(target->name);
   free(target->type);
   free(target->cwd);
   for (char **src = target->sources; src && *src; src++)
      free(*src);
   free(target->sources);
   free(target->build_dir);
   free(target->compiler);
   for (char **flag = target->c_flags; flag && *flag; flag++)
      free(*flag);
   free(target->c_flags);
   for (char **flag = target->ld_flags; flag && *flag; flag++)
      free(*flag);
   free(target->ld_flags);
   free(target->out_dir);
   if (target->output) free(target->output);
   if (target->commands) {
      for (char **cmd = target->commands; *cmd; cmd++) {
         free(*cmd); // Free individual command strings
      }
      free(target->commands); // Free the array itself
   }
   free(target);
}

/* Files functions */
// This function reads the contents of a file into a buffer
size_t files_read_file(const char *filename, char **buffer) {
   // return number of bytes read ...
   if (!filename || !buffer) {
      logger_fdebugf(stderr, LOG_NORMAL, DBG_ERROR, "Invalid file read request: NULL filename or buffer.\n");
      return 0;
   }
   FILE *file = fopen(filename, "rb");
   if (!file) {
      logger_fdebugf(stderr, LOG_NORMAL, DBG_ERROR, "Failed to open file: %s\n", filename);
      return 0; // Return 0 if file could not be opened
   }
   fseek(file, 0, SEEK_END);
   long size = ftell(file);
   fseek(file, 0, SEEK_SET);
   if (size <= 0) {
      fclose(file);
      logger_fdebugf(stderr, LOG_NORMAL, DBG_ERROR, "File is empty or could not determine size: %s\n", filename);
      return 0; // Return 0 if file is empty or size could not be determined
   }
   addr buffer_addr;
   if (!resources_alloc(&buffer_addr, size + 1)) {
      fclose(file);
      logger_fdebugf(stderr, LOG_NORMAL, DBG_ERROR, "Memory allocation failed for file buffer.\n");
      return 0; // Return 0 if memory allocation fails
   }
   *buffer = (char *)buffer_addr;

   size_t bytes_read = fread(*buffer, 1, size, file);
   fclose(file);
   if (bytes_read < size) {
      free(*buffer);
      logger_fdebugf(stderr, LOG_NORMAL, DBG_ERROR, "Failed to read entire file: %s\n", filename);
      return 0; // Return 0 if not all bytes were read
   }
   (*buffer)[size] = '\0'; // Null-terminate the buffer

   return size;
}
// Function to determine if a file exists
int files_file_exists(const char *filename) {
   if (!filename) {
      logger_fdebugf(stderr, LOG_NORMAL, DBG_ERROR, "Invalid file existence check: NULL filename.\n");
      return SB_FALSE; // Return 0 if filename is NULL
   }

   FILE *file = fopen(filename, "rb");
   if (!file) {
      return SB_FALSE; // File does not exist
   }

   fclose(file);
   return SB_TRUE; // File exists
}
// Function to get the relative path of a file
int files_get_file_path(const char *path, string *out) {
   if (!path || !out) {
      logger_fdebugf(stderr, LOG_NORMAL, DBG_ERROR, "Invalid file path request: NULL path or output buffer.\n");
      return SB_FALSE;
   }

   // Find the last '/' in the path
   const char *last_slash = strrchr(path, '/');

   if (last_slash) {
      // Calculate length of directory portion (including the '/')
      size_t dir_len = last_slash - path + 1;

      // Allocate and copy the directory portion
      *out = malloc(dir_len + 1);
      if (!*out) {
         logger_fdebugf(stderr, LOG_NORMAL, DBG_ERROR, "Memory allocation failed for file path.\n");
         return SB_FALSE;
      }

      strncpy(*out, path, dir_len);
      (*out)[dir_len] = '\0';

      // Ensure it starts with './' if it's a relative path
      if (strncmp(*out, "./", 2) != 0 && strncmp(*out, "/", 1) != 0) {
         char *temp = malloc(strlen(*out) + 3);
         if (!temp) {
            free(*out);
            *out = NULL;
            return SB_FALSE;
         }
         strcpy(temp, "./");
         strcat(temp, *out);
         free(*out);
         *out = temp;
      }
   } else {
      // No directory component, just return "./"
      *out = strdup("./");
   }

   return *out ? SB_TRUE : SB_FALSE;
}

/* Directories functions */
// This function gets the current working directory
void directories_get_wd(string *out) {
   if (!out) {
      logger_fdebugf(stderr, LOG_NORMAL, DBG_ERROR, "Invalid directory request: NULL output buffer.\n");
      return;
   }

#ifdef _WIN32
   char *cwd = _getcwd(NULL, 0); // Windows version
   const char sep = '\\';
#else
   char *cwd = getcwd(NULL, 0); // POSIX version
   const char sep = '/';
#endif

   if (!cwd) {
      logger_fdebugf(stderr, LOG_NORMAL, DBG_ERROR, "Failed to get current working directory.\n");
      return;
   }

   // Check if we need to append separator
   size_t len = strlen(cwd);
   if (len == 0 || cwd[len - 1] != sep) {
      // Reallocate with space for separator
      char *tmp = realloc(cwd, len + 2); // +1 sep, +1 null
      if (!tmp) {
         logger_fdebugf(stderr, LOG_NORMAL, DBG_ERROR, "Memory allocation failed.\n");
         free(cwd);
         return;
      }
      tmp[len] = sep;      // Add separator
      tmp[len + 1] = '\0'; // Add null terminator
      cwd = tmp;
   }

   *out = cwd; // Make a copy (caller must free)
}
// This function sets the current working directory to the specified path
int directories_set_wd(const char *path) {
   if (!path) {
      logger_fdebugf(stderr, LOG_NORMAL, DBG_ERROR, "Invalid directory request: NULL path.\n");
      return SB_FALSE; // Return false if path is NULL
   }

   if (chdir(path) != 0) {
      logger_fdebugf(stderr, LOG_NORMAL, DBG_ERROR, "Failed to change directory: %s\n", path);
      return SB_FALSE; // Return false if chdir fails
   }

   return SB_TRUE; // Return true if successful
}

/* Paths functions */
// This function gets the absolute path of a file or directory
int paths_get_absolute_path(const char *rel_path, char **abs_path) {
   if (!rel_path || !abs_path) {
      logger_fdebugf(stderr, LOG_NORMAL, DBG_ERROR, "Invalid path request: NULL relative path or output buffer.\n");
      return SB_FALSE; // Return false if rel_path or abs_path is NULL
   }

   char *cwd = NULL;
   directories_get_wd(&cwd); // Get the current working directory
   if (!cwd) {
      return SB_FALSE; // Return false if cwd could not be obtained
   }

   size_t len = strlen(cwd) + strlen(rel_path) + 2; // +1 for separator, +1 for null terminator
   *abs_path = malloc(len);
   if (!*abs_path) {
      free(cwd);
      logger_fdebugf(stderr, LOG_NORMAL, DBG_ERROR, "Memory allocation failed for absolute path.\n");
      return SB_FALSE; // Return false if memory allocation fails
   }

   snprintf(*abs_path, len, "%s/%s", cwd, rel_path); // Construct the absolute path
   free(cwd);                                        // Free the cwd buffer

   return SB_TRUE; // Return true if successful
}
// Function to get the file name from a given path
int paths_get_file_name(const char **path) {
   if (!path || !*path) {
      logger_fdebugf(stderr, LOG_NORMAL, DBG_ERROR, "Invalid path request: NULL path.\n");
      return SB_FALSE;
   }

   const char *file_name = strrchr(*path, '/');
   if (file_name) {
      file_name++;       // Move past the '/'
      *path = file_name; // Modify the pointer to point to filename
   }
   // If no '/' found, leave path as is (it's already just the filename)

   logger_fdebugf(stderr, LOG_NORMAL, DBG_INFO, "Extracted file name: %s\n", *path);
   return SB_TRUE;
}

// Global Logger Interface
const ILogger Logger = {
    .log_stream = logger_get_log_stream,
    .write = logger_writef,
    .writeln = logger_writelnf,
    .fwrite = logger_fwritef,
    .fwriteln = logger_fwritelnf,
    .debug = logger_fdebugf,
};
// Global Application Interface
const IApplication App = {
    .init = cli_init,
    .run = cli_run,
    .cleanup = cli_cleanup,
    .get_err_msg = cli_get_err_msg,
};
// Global Resources Interface
const IResources Resources = {
    .alloc = resources_alloc,
    .dispose_config = resources_dispose_config,
    .dispose_target = resources_dispose_target,
};
// Global Files Interface
const IFiles Files = {
    .read = files_read_file, // No file reading function defined
    .file_exists = files_file_exists,
    .file_path = files_get_file_path,
};
// Global Directories Interface
const IDirectories Directories = {
    .get_wd = directories_get_wd,
    .set_wd = directories_set_wd,
};
// Global Paths Interface
const IPaths Paths = {
    .get_path = paths_get_absolute_path,  // Get the absolute path of a file or directory
    .get_file_name = paths_get_file_name, // Get the file name from a given
};