#ifndef REQUEST_H
#define REQUEST_H

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_HEADER_NAME 256
#define MAX_HEADER_VALUE 1024
#define MAX_HEADERS 50

struct RequestInfo {
    char method[8];
    char target[PATH_MAX];
    char path[PATH_MAX];
    char server_path[PATH_MAX];  // Resolved path on the server
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

// Parses the HTTP request from client_fd, returns populated RequestData.
struct RequestData parse_request(int client_fd, const char *serve_path);

// Validates the parsed request, returns 1 if valid, 0 otherwise.
int validate_request_data(int client_fd,
                          const struct RequestData *request_data);

// Writes the requested file to the client if valid.
void write_request(int client_fd, struct RequestData request_data);

#endif  // REQUEST_H
