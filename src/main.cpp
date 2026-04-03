/*
 * Main entry point for the Shader Editor application.
 *
 * pretty much just instanciates an App that wrapps all the logic
 */

#include "app/app.h"

int main() {
  App app;
  if (!app.initialize(1000, 800, "Shader Editor")) {
    return 1;
  }
  app.run();
  return 0;
}
