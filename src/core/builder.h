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

#include "core.h"

typedef struct build_target_s
{
   string name;      // Name of the build target
   string type;      // Type of the build target (e.g., executable, library)
   string *sources;  // Array of source files for the build targets
   string build_dir; // Directory where the build output will be placed
   string compiler;  // Compiler to use for building the target
   string *c_flags;  // Array of compiler flags for the target
   string *ld_flags; // Array of linker flags for the target
} build_target_s;

/**
 * @brief IBuilder interface.
 * @details Provides an interface for building the application from the command line interface.
 *          This interface defines the functions that can be used to build the application
 *          based on the provided build target.
 */
typedef struct IBuilder
{
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