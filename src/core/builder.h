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

struct build_target_s; // Forward declaration of BuildTarget structure

typedef struct build_target_s *BuildTarget; // BuildTarget is a pointer to the build_target_s structure

/**
 * @brief IBuilder interface.
 * @details Provides an interface for building the application from the command line interface.
 *          This interface defines the functions that can be used to build the application
 *          based on the provided build target.
 */
typedef struct IBuilder
{
   /**
    * @brief Builds the application for the specified target.
    * @param target :the build target to build the application for
    * @return :0 on success, non-zero on failure
    */
   int (*build)(BuildTarget);
} IBuilder;

#endif // BUILDER_H