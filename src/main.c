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

#include "sbuild.h"

int main(int argc, char **argv) {
   App.init(argc, argv); // Initialize the application with command line arguments
   App.run();            // Run the application

   // This is *technically* optional because clean up is registered `atexit`
   App.cleanup(); // Clean up resources used by the application

   return 0;
}
