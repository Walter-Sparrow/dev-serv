#include "file_utils.h"
#include <cstring>

int content_type_from_path(const char *path, char *content_type) {
  const char *ext = strrchr(path, '.');
  if (ext == NULL) {
    return -1;
  }

  const char *exts[] = {".html", ".css", ".js"};
  const char *content_types[] = {"text/html", "text/css",
                                 "application/javascript"};

  size_t count = sizeof(exts) / sizeof(exts[0]);
  for (size_t i = 0; i < count; i++) {
    if (strcmp(ext, exts[i]) == 0) {
      strcpy(content_type, content_types[i]);
      return 0;
    }
  }

  return -1;
}
