#define _POSIX_C_SOURCE 200809L

#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>

#include "mime.h"
#include "request.h"
#include "utils.h"

#define QUEUED_REQUESTS 10

struct CliArguments {
    char *host;
    int port;
    char *serve_path;
};

struct ThreadArgs {
    int client_fd;
    char *serve_path;
};

struct CliArguments parse_cli_arguments(int argc, char *argv[]) {
    struct CliArguments cli_arguments;
    cli_arguments.host = "localhost";
    cli_arguments.port = 8080;
    cli_arguments.serve_path = "./";

    for (int i = 1; i < argc - 1; i++) {
        char *arg = argv[i];
        char *next_arg = argv[i + 1];

        if (strcmp(arg, "-h") == 0) {
            cli_arguments.host = next_arg;
        } else if (strcmp(arg, "-p") == 0) {
            cli_arguments.port = atoi(next_arg);
            if (cli_arguments.port < 1 || cli_arguments.port > 65535) {
                fprintf(stderr, "Invalid port: %s\n", next_arg);
                exit(EXIT_FAILURE);
            }
        } else if (strcmp(arg, "-d") == 0) {
            cli_arguments.serve_path = next_arg;
        }
    }
    return cli_arguments;
}

struct RequestData handle_request(int client_fd, const char *serve_path) {
    struct RequestData request_data = parse_request(client_fd, serve_path);

    if (validate_request_data(client_fd, &request_data)) {
        write_request(client_fd, request_data);
    }

    close(client_fd);
    return request_data;
}

void *thread_handle_request(void *arg) {
    struct ThreadArgs *args = (struct ThreadArgs *)arg;

    int client_fd = args->client_fd;
    char *serve_path = args->serve_path;
    free(args);  // free memory allocated for args

    struct RequestData request_data = handle_request(client_fd, serve_path);

    printf("%s %s\n", request_data.info.method, request_data.info.target);

    pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
    struct CliArguments cli_arguments = parse_cli_arguments(argc, argv);

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));

    addr.sin_family = AF_INET;
    addr.sin_port = htons(cli_arguments.port);
    resolve_address(&addr.sin_addr, cli_arguments.host);

    // load mime types, so they can be used later in the program
    load_mime_types();

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, QUEUED_REQUESTS) < 0) {
        perror("listen failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);

    printf("Starting server on %s:%d, serving from %s\n", cli_arguments.host,
           cli_arguments.port, cli_arguments.serve_path);
    fflush(stdout);

    while (1) {
        int client_fd =
            accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
        if (client_fd < 0) {
            perror("accept failed");
            continue;
        }

        pthread_t tid;
        struct ThreadArgs *args = malloc(sizeof(struct ThreadArgs));
        if (!args) {
            perror("malloc failed");
            close(client_fd);
            continue;
        }

        args->client_fd = client_fd;
        args->serve_path =
            cli_arguments.serve_path;  // assumed to be constant/shared

        if (pthread_create(&tid, NULL, thread_handle_request, args) != 0) {
            perror("pthread_create failed");
            close(client_fd);
            free(args);
            continue;
        }

        // Option 1: Detach thread so it cleans up after itself
        pthread_detach(tid);
    }

    close(server_fd);
}
