/* src/core/builder.h
 * Header for the Sigma.Build CLI builder.
 *
 * David Boarman
 * 2025-05-25
 *
 * SIGMABUILD_VERSION "0.00.01"
 * CLI_BUILDER_VERSION "0.00.01"
 *
 * This file provides an interface for building the Sigma.Build CLI application.
 * It defines the BuildContext structure and the IBuilder interface for building
 * the application from the command line interface.
 */

#ifndef BUILDER_H
#define BUILDER_H

#include "sbuild.h"

typedef struct build_target_s {
   string name;      // Name of the build target
   string type;      // Type of the build target (e.g., executable, library)
   string cwd;       // Current working directory for the build target
   string *sources;  // Array of source files for the build targets
   string build_dir; // Directory where the build output will be placed
   string out_dir;   // Directory where the final output will be placed (optional - if NULL, will use build_dir)
   string compiler;  // Compiler to use for building the target
   string *c_flags;  // Array of compiler flags for the target
   string *ld_flags; // Array of linker flags for the target
   string *commands; // Array of custom commands to run
   string output;    // Output file name for the target (optional)
} build_target_s;

typedef struct build_config_s {
   string name;           // Name of the build configuration
   string cwd;            // Current working directory for the build configuration
   string log_file;       // Log file for the build configuration
   BuildTarget *targets;  // Array of build targets for the configuration
   string *variables;     // Array of key-value pairs for configuration variables
   string default_target; // Default target to build if none is specified
} build_config_s;

/**
 * @brief IBuilder interface.
 * @details Provides an interface for building the application from the command line interface.
 *          This interface defines the functions that can be used to build the application
 *          based on the provided build target.
 */
typedef struct IBuilder {
   /**
    * @brief Gets the version of the builder.
    * @return :the version of the builder as a string
    * @details This function returns the version of the builder.
    */
   const char *(*get_version)(void); // Function to get the version of the builder
   /**
    * @brief Builds the application for the specified target.
    * @param target :the build target to build the application for
    * @return :0 on success, non-zero on failure
    */
   int (*build)(BuildTarget);
} IBuilder;

extern const IBuilder Builder; // Global Builder instance

#endif // BUILDER_H