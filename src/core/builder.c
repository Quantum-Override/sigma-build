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

   Logger.writeln("Building target: %s", target->name);

       // Compile each source file
    char obj_files[2048] = "";
    for (char **src = target->sources; src && *src; src++) {
        char *src_copy = strdup(*src);
        char *base = strdup(*src);
        char *slash = base;
        while ((slash = strchr(slash, '/'))) *slash = '_';
        char *dot = strrchr(base, '.');
        if (dot) *dot = '\0';
        char obj_path[1024];
        snprintf(obj_path, sizeof(obj_path), "%s%s.o", target->build_dir, base);
        char c_flags[512] = "";
        for (char **flag = target->c_flags; flag && *flag; flag++) {
            strcat(c_flags, *flag);
            strcat(c_flags, " ");
        }
        char cmd[2048];
        snprintf(cmd, sizeof(cmd), "%s %s-Iinclude -Ilib/cjson -o %s %s",
                 target->compiler, c_flags, obj_path, *src);
        Logger.debug(stdout, LOG_VERBOSE, DBG_INFO, "Executing: %s", cmd);
        if (system(cmd) != 0) {
            Logger.debug(stderr, LOG_NORMAL, DBG_ERROR, "Failed to compile %s", *src);
            free(src_copy);
            free(base);
            return -1;
        }
        strcat(obj_files, obj_path);
        strcat(obj_files, " ");
        free(src_copy);
        free(base);
    }

    // Link object files
    char ld_flags[512] = "";
    for (char **flag = target->ld_flags; flag && *flag; flag++) {
        strcat(ld_flags, *flag);
        strcat(ld_flags, " ");
    }
    char link_cmd[4096];
    snprintf(link_cmd, sizeof(link_cmd), "%s %s-o %s%s %s",
             target->compiler, ld_flags, target->build_dir, target->name, obj_files);
    Logger.debug(stdout, LOG_VERBOSE, DBG_INFO, "Executing: %s", link_cmd);
    if (system(link_cmd) != 0) {
        Logger.debug(stderr, LOG_NORMAL, DBG_ERROR, "Failed to link target: %s", target->name);
        return -1;
    }

   return 0; // Return success
}

const IBuilder Builder = {
    .get_version = get_builder_version,
    .build = builder_build_target,
};