/* include/core.h: Core header file for the project
 *
 * David Boarman
 * 2025-05-25
 *
 * SIGMABUILD_VERSION "0.00.01"
 *
 * This file includes standard libraries and project-specific headers.
 */
#pragma once

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef void *object;   // Generic object type for pointers to any data structure
typedef char *string;   // String type for character arrays
typedef uintptr_t addr; // An address type for pointers

struct cli_state_s;     // Forward declaration of CLIState structure
struct cli_options_s;   // Forward declaration of CLIOptions structure
struct build_context_s; // Forward declaration of BuildContext structure
struct build_config_s;  // Forward declaration of build_config_s structure
struct build_target_s;  // Forward declaration of BuildTarget structure

typedef struct cli_state_s *CLIState;         // CLIState is the structure that holds the state of the command line interface
typedef struct cli_options_s *CLIOptions;     // CLIOptions is the structure that holds the command line options
typedef struct build_context_s *BuildContext; // BuildContext is the structure that holds the build context for the application
typedef struct build_config_s *BuildConfig;   // BuildConfig is a pointer to the build_config_s structure
typedef struct build_target_s *BuildTarget;   // BuildTarget is a pointer to the build_target_s structure

#define SB_TRUE 1                // Boolean true value
#define SB_FALSE 0               // Boolean false value
#define SB_NULL NULL             // Null pointer definition
#define SB_VERSION "0.01.03.001" // Project version - MAJOR.MINOR.REVISION.BUILD

/**
 * @brief Enumeration for log levels.
 * @details This enumeration defines the different levels of logging configured
 *          for the target project. It differes from DebugLevel in that it is used
 *          to control the verbosity of logging output, while DebugLevel is used for
 *          controlling debug output in the code.
 */
typedef enum { LOG_NONE = 0,    // No logging
               LOG_NORMAL = 1,  // Normal logging
               LOG_VERBOSE = 2, // Verbose logging
} LogLevel;
/**
 * @brief Enumeration for debug levels.
 * @details This enumeration defines the different levels of debug output that can
 *          be enabled in the code. It is used to control the granularity of debug
 *          information printed during execution.
 */
typedef enum { DBG_DEBUG,   // Debug level logging
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
typedef enum { CLI_SUCCESS = 0, // No error
               CLI_FAILURE = 1, // Unknown error
               // CLI Parser Error Codes
               CLI_ERR_PARSE_INVALID_ARG = 1001, // Invalid argument provided
               CLI_ERR_PARSE_MISSING_OPTION,     // Required option is missing
               CLI_ERR_PARSE_INVALID_CONFIG,     // Invalid or NULL configuration file specified
               CLI_ERR_PARSE_MISSING_CONFIG,     // Configuration file is missing
               CLI_ERR_PARSE_UNKNOWN_OPTION,     // Unknown option provided
               CLI_ERR_PARSE_FAILED,             // Failed to parse command line arguments
               // Loader Error Codes
               LOADER_ERR_INVALID_FORMAT = 2001, // Invalid JSON format
               LOADER_ERR_MISSING_FIELD,         // Required field is missing in JSON
               LOADER_ERR_INVALID_FIELD,         // Invalid field in JSON
               LOADER_ERR_UNKNOWN_FIELD,         // Unknown field in JSON
               LOADER_ERR_INVALID_VAR,           // Invalid variable reference
               LOADER_ERR_PARSE_FAILED,          // Failed to parse JSON
               LOADER_ERR_FILE_NOT_FOUND,        // JSON file not found
               LOADER_ERR_FILE_READ,             // Error reading JSON file
               LOADER_ERR_FILE_EMPTY,            // Empty JSON file
               LOADER_ERR_LOAD_CONFIG,           // Failed to load configuration file
               // Builder Error Codes
               BUILD_ERR_BUILD_TARGET = 3001, // Build target failed
} CLIErrorCode;
/**
 * @brief CLI options structure.
 * @details This structure holds the options parsed from the command line arguments.
 *          It includes flags for showing help, about information, and any custom
 *          options that may be specified by the user.
 */
typedef struct cli_options_s {
   int show_help;          // Flag to indicate if help should be displayed
   int show_about;         // Flag to indicate if about information should be displayed
   string config_file;     // Path to the configuration file
   string original_path;   // Original cwd at launch
   string target_name;     // Name of the target to build
   LogLevel log_level;     // Logging level for the application
   DebugLevel debug_level; // Debug level for the application
   int is_verbose;         // Flag for verbose logging (only observed with --about && --help)
   FILE *log_stream;       // Stream for logging output
} cli_options_s;
/**
 * @brief CLI state structure.
 * @details This structure holds the state of the command line interface, including
 *          the current command, arguments, and options parsed from the command line.
 */
typedef struct cli_state_s {
   char **argv;        // Array of arguments for the command
   int argc;           // Number of arguments
   CLIOptions options; // Options parsed from the command line
   CLIErrorCode error; // Error code for any parsing errors
} cli_state_s;

/**
 * @brief A context structure for the current build.
 */
typedef struct build_context_s {
   LogLevel log_level;       // Current logging level
   DebugLevel debug_level;   // Current debug level
   const char *project_name; // Name of the project being built
   string cwd;               // Current working directory for the build context
   FILE *log_stream;         // Stream for logging output
   string current_target;    // Name of the current target being built
   string config_file;       // Configuration being used
   BuildConfig config;       // Current Build Configuration
   object data;              // Pointer to any additional data structure
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
typedef struct ILogger {
   /**
    * @brief Returns the current log stream
    * @return :the current log stream
    */
   FILE *(*log_stream)(void); // Returns the current log stream
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
typedef struct IApplication {
   /**
    * @brief Initializes the application with the given context
    * @param argc :the number of command line arguments
    * @param argv :the command line arguments
    */
   void (*init)(int, char **);
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
 * @brief Global resource management interface.
 */
typedef struct IResources {
   /**
    * @brief Allocates memory for an object of the specified size
    * @param addr :the address to allocate memory for
    * @param size :the size of the memory to allocate
    * @return :1 if allocation was successful; otherwise, 0
    */
   int (*alloc)(addr *, size_t);
   /**
    * @brief Disposes of a BuildConfig object
    * @param config :the BuildConfig object to dispose of
    */
   void (*dispose_config)(BuildConfig);
   /**
    * @brief Disposes of a BuildTarget object
    * @param target :the BuildTarget object to dispose of
    */
   void (*dispose_target)(BuildTarget);
} IResources;
/**
 * @brief Global file interface
 */
typedef struct IFiles {
   /**
    * @brief Reads the contents of a file into a buffer
    * @param path :the path to the file to read
    * @param out :the output buffer to store the file contents (NULL terminated)
    * @return :the number of bytes read from the file
    */
   size_t (*read)(const char *, char **); // Read file contents to buffer returning number of bytes read
   /**
    * @brief Checks if a file exists at the given path
    * @param path :the path to the file to check
    * @return :SB_TRUE if the file exists; otherwise, SB_FALSE
    */
   int (*file_exists)(const char *); // Check if a file exists at the given path
   /**
    * @brief Get the file's relative path
    * @param path :the path to the file to get the relative path for
    * @param out :the output buffer to store the relative path (NULL terminated)
    * @return :SB_TRUE if successful; otherwise, SB_FALSE
    */
   int (*file_path)(const char *, string *); // Get the file's relative path
} IFiles;
/**
 * @brief Global directory interface
 */
typedef struct IDirectories {
   /**
    * @brief Get the current working directory
    * @param out :the output buffer to store the current working directory (NULL terminated)
    */
   void (*get_wd)(string *);
   /**
    * @brief Change the current working directory
    * @param path :the path to change the current working directory to
    * @return :SB_TRUE if successful; otherwise, SB_FALSE
    */
   int (*set_wd)(const char *);
} IDirectories;
/**
 * @brief Global paths interface
 */
typedef struct IPaths {
   /**
    * @brief Gets the absolute path of a given relative path
    * @param relative_path :the relative path to convert to an absolute path
    * @param out :the output buffer to store the absolute path (NULL terminated)
    * @return :SB_TRUE if successful; otherwise, SB_FALSE
    */
   int (*get_path)(const char *, string *);
   /**
    * @brief Gets the file name from a given path
    * @param path :the path to extract the file name from
    * @return :SB_TRUE if successful; otherwise, SB_FALSE
    */
   int (*get_file_name)(const char **); // Get the file name from a given path
} IPaths;

/**
 * @brief Global Logger Interface
 * @details This Interface is used to log messages throughout the application.
 */
extern const ILogger Logger;
/**
 * @brief Global Application Interface
 * @details This Interface is used to manage the Application lifecycle.
 */
extern const IApplication App;
/**
 * @brief Global Resources Interface
 * @details This Interface is used to manage resources such as configurations and targets.
 */
extern const IResources Resources;
/**
 * @brief Global Files Interface
 * @details This Interface is used to handle basic file operations
 */
extern const IFiles Files;
/**
 * @brief Global Directories Interface
 * @details This Interface is used to handle basic directory operations
 */
extern const IDirectories Directories;
/**
 * @brief Global Paths Interface
 * @details This Interface is used to handle basic path operations
 */
extern const IPaths Paths;
