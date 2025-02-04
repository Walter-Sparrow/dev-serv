#include "sse.h"
#include <CoreFoundation/CoreFoundation.h>
#include <CoreServices/CoreServices.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sys/socket.h>
#include <unistd.h>

static void fsevents_callback(ConstFSEventStreamRef streamRef,
                              void *clientCallBackInfo, size_t numEvents,
                              void *eventPaths,
                              const FSEventStreamEventFlags eventFlags[],
                              const FSEventStreamEventId eventIds[]) {
  char **paths = (char **)eventPaths;
  int cfd = *(int *)clientCallBackInfo;

  for (size_t i = 0; i < numEvents; i++) {
    printf("Change detected: %s\n", paths[i]);
  }

  char message[] = "data: {\"reload\": true}\n\n";
  if (write(cfd, message, strlen(message)) < 0) {
    fprintf(stderr, "Error writing SSE message\n");
  }
}

void *handle_sse(void *arg) {
  int cfd = *(int *)arg;
  free(arg);

  const char sse_response[] = "HTTP/1.1 200 OK\r\n"
                              "Content-Type: text/event-stream\r\n"
                              "Cache-Control: no-cache\r\n"
                              "\r\n";

  if (write(cfd, sse_response, strlen(sse_response)) < 0) {
    fprintf(stderr, "Error writing SSE response\n");
    return NULL;
  };

  char cwd[PATH_MAX];
  if (getcwd(cwd, sizeof(cwd)) == NULL) {
    return NULL;
  }

  CFStringRef pathToWatch = CFStringCreateWithCString(kCFAllocatorDefault, cwd,
                                                      kCFStringEncodingUTF8);

  CFArrayRef pathsToWatch = CFArrayCreate(NULL, (const void **)&pathToWatch, 1,
                                          &kCFTypeArrayCallBacks);

  FSEventStreamContext context = {0};
  context.info = &cfd;

  CFAbsoluteTime latency = 0.1;
  FSEventStreamRef stream = FSEventStreamCreate(
      NULL, &fsevents_callback, &context, pathsToWatch,
      kFSEventStreamEventIdSinceNow, latency, kFSEventStreamCreateFlagNone);

  if (!stream) {
    fprintf(stderr, "Failed to create FSEventStream\n");
    CFRelease(pathsToWatch);
    CLOSE_CLIENT(cfd);
    return NULL;
  }

  dispatch_queue_t queue =
      dispatch_queue_create("com.dev-serv", DISPATCH_QUEUE_SERIAL);
  FSEventStreamSetDispatchQueue(stream, queue);

  if (!FSEventStreamStart(stream)) {
    fprintf(stderr, "Failed to start FSEventStream\n");
    FSEventStreamInvalidate(stream);
    FSEventStreamRelease(stream);
    CFRelease(pathsToWatch);
    CLOSE_CLIENT(cfd);
    return NULL;
  }

  char heartbeat[] = "data: {\"heartbeat\": true}\n\n";
  while (write(cfd, heartbeat, strlen(heartbeat)) > 0) {
    sleep(1);
  };

  printf("Client disconnected\n");

  FSEventStreamInvalidate(stream);
  FSEventStreamRelease(stream);
  CFRelease(pathsToWatch);

  CLOSE_CLIENT(cfd);
  return NULL;
}
