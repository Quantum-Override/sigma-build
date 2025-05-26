/* include/core.h: Core header file for the project
 *
 * David Boarman
 * 2025-05-25
 *
 * SIGMABUILD_VERSION "0.00.01"
 *
 * This file includes standard libraries and project-specific headers.
 */
#ifndef CORE_H
#define CORE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef void *object; // Generic object type for pointers to any data structure
typedef char *string; // String type for character arrays

struct cli_state_s;     // Forward declaration of CLIState structure
struct cli_options_s;   // Forward declaration of CLIOptions structure
struct build_context_s; // Forward declaration of BuildContext structure

typedef struct cli_state_s *CLIState;         // CLIState is the structure that holds the state of the command line interface
typedef struct cli_options_s *CLIOptions;     // CLIOptions is the structure that holds the command line options
typedef struct build_context_s *BuildContext; // BuildContext is the structure that holds the build context for the application

/**
 * @brief Enumeration for log levels.
 * @details This enumeration defines the different levels of logging configured
 *          for the target project. It differes from DebugLevel in that it is used
 *          to control the verbosity of logging output, while DebugLevel is used for
 *          controlling debug output in the code.
 */
typedef enum
{
   LOG_NONE = 0,    // No logging
   LOG_NORMAL = 1,  // Normal logging
   LOG_VERBOSE = 2, // Verbose logging
} LogLevel;
/**
 * @brief Enumeration for debug levels.
 * @details This enumeration defines the different levels of debug output that can
 *          be enabled in the code. It is used to control the granularity of debug
 *          information printed during execution.
 */
typedef enum
{
   DBG_DEBUG,   // Debug level logging
   DBG_INFO,    // Info level logging
   DBG_WARNING, // Warning level logging
   DBG_ERROR,   // Error (non-fatal) level logging
   DBG_FATAL,   // Fatal error level logging
} DebugLevel;
/**
 * @brief CLI Error Codes.
 * @details This enumeration defines the error codes that can be returned by the
 *          command line interface parser. These codes indicate various types of
 *          errors that can occur during command line parsing, such as invalid
 *          arguments, missing required options, or other parsing errors.
 */
typedef enum
{
   CLI_SUCCESS = 0, // No error
   // CLI Parser Error Codes
   CLI_ERROR_PARSE_INVALID_ARG = 1001, // Invalid argument provided
   CLI_ERROR_PARSE_MISSING_OPTION,     // Required option is missing
   CLI_ERROR_PARSE_INVALID_CONFIG,     // Invalid or NULL configuration file specified
   CLI_ERROR_PARSE_MISSING_CONFIG,     // Configuration file is missing
   CLI_ERROR_PARSE_UNKNOWN_OPTION,     // Unknown option provided
   CLI_ERROR_PARSE_FAILED,             // Failed to parse command line arguments
   // JSON Parser Error Codes
   JSON_ERROR_INVALID_FORMAT = 2001, // Invalid JSON format
   JSON_ERROR_MISSING_FIELD,         // Required field is missing in JSON
   JSON_ERROR_INVALID_FIELD,         // Invalid field in JSON
   JSON_ERROR_UNKNOWN_FIELD,         // Unknown field in JSON
   JSON_ERROR_PARSE_FAILED,          // Failed to parse JSON
   JSON_ERROR_FILE_NOT_FOUND,        // JSON file not found
   JSON_ERROR_FILE_READ,             // Error reading JSON file
   JSON_ERROR_FILE_EMPTY,            // Empty JSON file
} CLIErrorCode;
/**
 * @brief CLI options structure.
 * @details This structure holds the options parsed from the command line arguments.
 *          It includes flags for showing help, about information, and any custom
 *          options that may be specified by the user.
 */
typedef struct cli_options_s
{
   int show_help;          // Flag to indicate if help should be displayed
   int show_about;         // Flag to indicate if about information should be displayed
   string config_file;     // Path to the configuration file
   LogLevel log_level;     // Logging level for the application
   DebugLevel debug_level; // Debug level for the application
   int is_verbose;         // Flag for verbose logging (only observed with --about && --help)
} cli_options_s;
/**
 * @brief CLI state structure.
 * @details This structure holds the state of the command line interface, including
 *          the current command, arguments, and options parsed from the command line.
 */
typedef struct cli_state_s
{
   char **argv;        // Array of arguments for the command
   int argc;           // Number of arguments
   CLIOptions options; // Options parsed from the command line
   CLIErrorCode error; // Error code for any parsing errors
} cli_state_s;

/**
 * @brief A context structure for the current build.
 */
typedef struct build_context_s
{
   LogLevel log_level;          // Current logging level
   DebugLevel debug_level;      // Current debug level
   const char *project_name;    // Name of the project being built
   FILE *log_stream;            // Stream for logging output
   char *current_target;        // Name of the current target being built
   char *current_configuration; // Current configuration being used
   object data;                 // Pointer to any additional data structure
} build_context_s;

/**
 * @brief Formats the current time into a buffer using the specified format
 * @param buffer :output buffer for the timestamp (at least 32 chars)
 * @param format :strftime format string (e.g., "%Y-%m-%dT%H:%M:%S")
 */
void get_timestamp(char *, const char *);

/**
 * @brief Logger interface for writing messages to the context log stream.
 * @details This interface provides functions to write formatted messages to the
 *          context log stream or to a specified stream. It also includes
 *          debug logging functionality that can be controlled by the log level
 *          and debug level settings.
 */
typedef struct ILogger
{
   /**
    * @brief Writes a formatted message to the current test set's log stream
    * @param fmt :the format message to display
    * @param ... :the variable arguments for the format message
    */
   void (*write)(const char *, ...);
   /**
    * @brief Writes a formatted message with newline to the current test set's log stream
    * @param fmt :the format message to display
    * @param ... :the variable arguments for the format message
    */
   void (*writeln)(const char *, ...);
   /**
    * @brief Writes a formatted message to the specified stream
    * @param stream :the output stream to write to
    * @param fmt :the format message to display
    * @param ... :the variable arguments for the format message
    */
   void (*fwrite)(FILE *, const char *, ...);
   /**
    * @brief Writes a formatted message with newline to the specified stream
    * @param stream :the output stream to write to
    * @param fmt :the format message to display
    * @param ... :the variable arguments for the format message
    */
   void (*fwriteln)(FILE *, const char *, ...);
   /**
    * @brief Debug logging function
    * @param stream :the output stream to write to
    * @param log_level :the log level
    * @param debug_level :the debug level
    * @param fmt :the format message to display
    * @param ... :the variable arguments for the format message
    */
   void (*debug)(FILE *, LogLevel, DebugLevel, const char *, ...); // Debug logging function
} ILogger;
/**
 * @brief General application interface.
 */
typedef struct IApplication
{
   /**
    * @brief Initializes the application with the given context
    * @param argc :the number of command line arguments
    * @param argv :the command line arguments
    */
   void (*init)(int, char **);
   /**
    * @brief Load the configuration file for the application
    * @return :0 on success, non-zero on failure
    * @details This function loads the configuration file specified in the command line
    *          options. If no configuration file is specified, it uses a default configuration.
    *          If the configuration file is invalid or cannot be loaded, it returns an error code.
    *          The configuration file is expected to be in JSON format.
    */
   int (*load_config)(void);
   /**
    * @brief Runs the application with the given context
    */
   void (*run)(void);
   /**
    * @brief Cleans up resources used by the application
    */
   void (*cleanup)(void);
   /**
    * @brief Gets the message for a given CLI error code
    * @param code :the CLI error code
    * @return :the error message corresponding to the CLI error code
    */
   const char *(*get_err_msg)(CLIErrorCode);
} IApplication;

/**
 * @brief Global logger instance
 * @details This instance is used to log messages throughout the application.
 */
extern const ILogger Logger; // Global logger instance
/**
 * @brief Global application instance
 * @details This instance is used to manage the application lifecycle.
 */
extern const IApplication App; // Global application instance

#endif // CORE_H