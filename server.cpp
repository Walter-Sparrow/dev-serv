#include "server.h"
#include "file_utils.h"
#include "sse.h"
#include <arpa/inet.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <pthread.h>
#include <sys/socket.h>
#include <unistd.h>

#define LISTEN_BACKLOG 10
#define PATH_MAX 1024

static const char *client_sse_script =
    "\n<script>\n"
    "  var eventSource = new EventSource('/sse');\n"
    "  eventSource.onmessage = function(event) {\n"
    "    const data = JSON.parse(event.data);\n"
    "    if (!data.reload) return;\n"
    "    console.log('[INFO]: Reloading...');\n"
    "    location.reload();\n"
    "  };\n"
    "</script>\n";

static void serve_file(int cfd, const char *base_dir, const char *path) {
  char file_path[PATH_MAX];
  snprintf(file_path, sizeof(file_path), "%s%s", base_dir, path);

  FILE *fp = fopen(file_path, "r");
  if (!fp) {
    fprintf(stderr, "File not found: %s\n", file_path);
    CLOSE_CLIENT(cfd);
    return;
  }

  char content_type[128] = {0};
  if (content_type_from_path(file_path, content_type) < 0) {
    fprintf(stderr, "Unsupported file type: %s\n", file_path);
    fclose(fp);
    CLOSE_CLIENT(cfd);
    return;
  }

  fseek(fp, 0, SEEK_END);
  long file_size = ftell(fp);
  fseek(fp, 0, SEEK_SET);

  char *file_buf = (char *)malloc(file_size + strlen(client_sse_script) + 2);
  if (!file_buf) {
    fprintf(stderr, "Memory allocation error\n");
    fclose(fp);
    CLOSE_CLIENT(cfd);
    return;
  }

  fread(file_buf, 1, file_size, fp);
  fclose(fp);
  file_buf[file_size] = '\0';

  if (strcmp(content_type, "text/html") == 0) {
    char *head_tag = strstr(file_buf, "<head>");
    if (head_tag) {
      char *insert_pos = head_tag + strlen("<head>");
      size_t script_len = strlen(client_sse_script);
      size_t tail_len = strlen(insert_pos);

      memmove(insert_pos + script_len, insert_pos, tail_len + 1);
      memcpy(insert_pos, client_sse_script, script_len);
    }
  }

  size_t final_size = strlen(file_buf);
  char response_header[1024];
  snprintf(response_header, sizeof(response_header),
           "HTTP/1.1 200 OK\r\n"
           "Content-Type: %s\r\n"
           "Content-Length: %zu\r\n"
           "\r\n",
           content_type, final_size);

  write(cfd, response_header, strlen(response_header));
  write(cfd, file_buf, final_size);

  free(file_buf);
  CLOSE_CLIENT(cfd);
}

int run_server(const char *host, int port) {
  char cwd[PATH_MAX];
  if (!getcwd(cwd, sizeof(cwd))) {
    perror("getcwd");
    return 1;
  }

  int sfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sfd < 0) {
    perror("socket");
    return -1;
  }

  struct sockaddr_in server_addr;
  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(port);
  inet_aton(host, &server_addr.sin_addr);

  if (bind(sfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
    perror("bind");
    close(sfd);
    return -1;
  }

  if (listen(sfd, LISTEN_BACKLOG) < 0) {
    perror("listen");
    close(sfd);
    return -1;
  }

  printf("Listening on %s:%d\n", host, port);

  char request_buf[2048];
  for (;;) {
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    int cfd = accept(sfd, (struct sockaddr *)&client_addr, &client_addr_len);
    if (cfd < 0) {
      perror("accept");
      continue;
    }

    int set = 1;
    int set_result =
        setsockopt(cfd, SOL_SOCKET, SO_NOSIGPIPE, &set, sizeof(set));
    if (set_result < 0) {
      perror("setsockopt");
      CLOSE_CLIENT(cfd);
      continue;
    }

    ssize_t n = read(cfd, request_buf, sizeof(request_buf) - 1);
    if (n <= 0) {
      CLOSE_CLIENT(cfd);
      continue;
    }
    request_buf[n] = '\0';

    char *proto_line = strchr(request_buf, '\n');
    if (proto_line) {
      *proto_line = '\0';
    }
    char *method_s = strtok(request_buf, " ");
    char *path = strtok(NULL, " ");

    if (!method_s || !path) {
      CLOSE_CLIENT(cfd);
      continue;
    }

    if (strcmp(path, "/sse") == 0) {
      int *client_fd = (int *)malloc(sizeof(int));
      if (!client_fd) {
        fprintf(stderr, "Error allocating memory for client_fd\n");
        CLOSE_CLIENT(cfd);
        continue;
      }
      *client_fd = cfd;
      pthread_t thread_id;
      if (pthread_create(&thread_id, NULL, handle_sse, client_fd) != 0) {
        fprintf(stderr, "Error creating SSE thread\n");
        free(client_fd);
        CLOSE_CLIENT(cfd);
        continue;
      }
      pthread_detach(thread_id);
      continue;
    }

    if (strcmp(path, "/") == 0) {
      path = (char *)"/index.html";
    }

    if (strrchr(path, '.') == NULL) {
      strcat(path, ".html");
    }

    printf("Serving request for: %s\n", path);
    serve_file(cfd, cwd, path);
  }

  close(sfd);
  return 0;
}
