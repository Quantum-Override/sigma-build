/* /src/main.c
 *
 * Sigma.Build
 *
 * David Boarman
 * 2025-05-25
 *
 * An extensible build system for C projects and encoding various file types for embedding
 * into a C codebase.
 */

#include "core.h"

int main(int argc, char **argv)
{
   App.init(argc, argv); // Initialize the application with command line arguments
   App.run();            // Run the application
   App.cleanup();        // Clean up resources used by the application

   return 0;
}
