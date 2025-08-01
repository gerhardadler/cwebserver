#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <string.h>

#include "mime.h"
#include "request.h"
#include "utils.h"

struct CliArguments {
    char *host;
    int port;
    char *serve_path;
};

struct CliArguments parse_cli_arguments(int argc, char *argv[]) {
    struct CliArguments cli_arguments;
    cli_arguments.host = "localhost";
    cli_arguments.port = 8080;
    cli_arguments.serve_path = "";

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

    if (listen(server_fd, 10) < 0) {
        perror("listen failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);

    while (1) {
        int client_fd =
            accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
        if (client_fd < 0) {
            perror("accept failed");
            continue;
        }

        pid_t pid = fork();
        if (pid == 0) {        // child process
            close(server_fd);  // child doesn't need listening socket

            struct RequestData request_data =
                parse_request(client_fd, cli_arguments.serve_path);
            printf("%s %s\n", request_data.info.method,
                   request_data.info.target);

            if (validate_request_data(client_fd, &request_data)) {
                write_request(client_fd, request_data);
            }

            close(client_fd);
            exit(EXIT_FAILURE);
        } else if (pid > 0) {
            close(client_fd);  // parent shouldn't have accepted
                               // socket
        } else {
            perror("fork failed");
            close(client_fd);
        }
    }

    close(server_fd);
}
