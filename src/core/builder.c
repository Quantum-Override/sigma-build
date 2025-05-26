/* src/core/builder.c
 * Sigma.Build CLI Builder
 *
 * David Boarman
 * 2025-05-25
 *
 * This file provides the implementation of the IBuilder interface for building
 * the Sigma.Build CLI application. It includes functions for initializing the
 * CLI state, loading the configuration file, running the application, and cleaning
 * up resources used by the CLI.
 */

#include "builder.h"

#define CLI_BUILDER_VERSION "0.00.01"

// Function to return the version of the builder
const char *get_builder_version()
{
   return CLI_BUILDER_VERSION; // Return the version of the builder
}
// Function to build the specified target
int builder_build_target(BuildTarget target)
{
   if (!target || !target->name)
   {
      Logger.debug(stderr, LOG_NORMAL, DBG_ERROR, "Invalid build target specified.\n");
      return -1; // Return error if target is NULL or has no name
   }

   // Simulate building the target
   Logger.writeln("Building target: %s", target->name);

   // Here you would typically perform the actual build process
   // For now, we just simulate a successful build
   return 0; // Return success
}

const IBuilder Builder = {
    .get_version = get_builder_version,
    .build = builder_build_target,
};