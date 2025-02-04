#include "server.h"
#include <cstdio>

int main(void) {
  const char *host = "127.0.0.1";
  int port = 8080;

  int result = run_server(host, port);
  if (result != 0) {
    fprintf(stderr, "Server failed to start or encountered an error.\n");
  }

  return result;
}
