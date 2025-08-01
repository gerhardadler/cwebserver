#define _POSIX_C_SOURCE 200809L

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "mime.h"
#include "utils.h"

#define MAX_HEADER_NAME 256
#define MAX_HEADER_VALUE 1024
#define MAX_HEADERS 50

struct RequestInfo {
    char method[8];
    char target[PATH_MAX];

    char path[PATH_MAX];
    char server_path[PATH_MAX];  // path on the server (serve_path + path)

    char query_string[PATH_MAX];
    char protocol[16];
};

struct Header {
    char name[MAX_HEADER_NAME];
    char value[MAX_HEADER_VALUE];
};

struct RequestData {
    struct RequestInfo info;
    struct Header headers[MAX_HEADERS];
    int header_count;
};
struct RequestData parse_request(int client_fd, const char *serve_path) {
    struct RequestData request_data;
    memset(&request_data, 0, sizeof(request_data));  // initialize

    char buffer[8192];
    unsigned int total_read = 0;

    while (total_read < sizeof(buffer) - 1) {
        int bytes = read(client_fd, buffer + total_read,
                         sizeof(buffer) - 1 - total_read);
        if (bytes <= 0) break;  // client closed or error
        total_read += bytes;
        buffer[total_read] = '\0';
        if (strstr(buffer, "\r\n\r\n"))
            break;  // end of headers (TODO: handle body)
    }

    char *line = strtok(buffer, "\r\n");
    int state = 0;  // status

    while (line != NULL && *line != '\0') {
        switch (state) {
            case 0:  // parse status
                sscanf(line, "%7s %255s %15s", request_data.info.method,
                       request_data.info.target, request_data.info.protocol);

                char *decoded = url_decode(request_data.info.target);

                char *sanitized = sanitize_path(decoded);
                free(decoded);

                strcpy(request_data.info.path, sanitized);
                free(sanitized);

                combine_paths(request_data.info.server_path, PATH_MAX,
                              serve_path, request_data.info.path);

                if (is_readable_dir(request_data.info.server_path)) {
                    // out_path and dirname cannot point to the same memory
                    // location, so we need to copy the server path
                    char *copied_server_path =
                        strdup(request_data.info.server_path);
                    get_index_file(request_data.info.server_path, PATH_MAX,
                                   copied_server_path);
                    free(copied_server_path);
                }

                state = 1;
                break;
            case 1:
                char *colon = strchr(line, ':');
                if (!colon) break;
                *colon = '\0';
                char *name = line;
                char *value = colon + 1;

                // trim leading spaces in value
                while (*value == ' ') value++;

                strncpy(request_data.headers[request_data.header_count].name,
                        name, MAX_HEADER_NAME - 1);
                strncpy(request_data.headers[request_data.header_count].value,
                        value, MAX_HEADER_VALUE - 1);
                request_data.header_count++;

                if (request_data.header_count >= MAX_HEADERS) {
                    state = 2;
                }
                break;
            case 2:
                break;
                // TODO, parse body
        }
        line = strtok(NULL, "\r\n");
    }

    return request_data;
}
int validate_request_data(int client_fd,
                          const struct RequestData *request_data) {
    if (strcmp(request_data->info.protocol, "HTTP/1.1") != 0) {
        dprintf(client_fd,
                "HTTP/1.1 505 HTTP Version Not Supported\r\n"
                "Content-Length: 41\r\n"
                "Content-Type: text/html\r\n"
                "\r\n"
                "<h1>505 HTTP Version Not Supported</h1>\r\n");
        return 0;
    }

    if (!is_readable_file(request_data->info.server_path)) {
        dprintf(client_fd,
                "HTTP/1.1 404 Not Found\r\n"
                "Content-Length: 23\r\n"
                "Content-Type: text/html\r\n"
                "\r\n"
                "<h1>404 Not Found</h1>\r\n");
        return 0;
    }
    return 1;
}

void write_request(int client_fd, struct RequestData request_data) {
    FILE *fp = fopen(request_data.info.server_path, "rb");
    if (!fp) {
        perror("fopen");
        return;
    }

    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    rewind(fp);

    const char *server_path_ext =
        get_filename_ext(request_data.info.server_path);

    char *mime_type = get_mime_type(server_path_ext);

    dprintf(client_fd,
            "HTTP/1.1 200 OK\r\n"
            "Content-Length: %ld\r\n"
            "Content-Type: %s\r\n"
            "\r\n",
            file_size, mime_type);

    char buffer[4096];
    size_t bytes;
    while ((bytes = fread(buffer, 1, sizeof(buffer), fp)) > 0) {
        write(client_fd, buffer, bytes);
    }

    fclose(fp);
}