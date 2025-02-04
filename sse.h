#ifndef SSE_H
#define SSE_H

#define CLOSE_CLIENT(cfd)                                                      \
  shutdown(cfd, SHUT_RDWR);                                                    \
  close(cfd);

void *handle_sse(void *arg);

#endif // SSE_H
