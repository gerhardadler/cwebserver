#define _POSIX_C_SOURCE 200809L

#include <arpa/inet.h>
#include <ctype.h>
#include <dirent.h>
#include <libgen.h>
#include <limits.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

char from_hex(char c) {
    // '0' == 48
    // '1' == 49
    // '1' - '0' == 1
    if ('0' <= c && c <= '9') return c - '0';
    if ('a' <= c && c <= 'f') return c - 'a' + 10;
    if ('A' <= c && c <= 'F') return c - 'A' + 10;
    return -1;
}

char *url_decode(const char *src) {
    // remember to free
    char *out = malloc(strlen(src) + 1);
    if (!out) return NULL;
    char *current_out = out;

    while (*src) {
        if (*src == '%' && isxdigit(*(src + 1)) && isxdigit(*(src + 2))) {
            *current_out++ = from_hex(*(src + 1)) << 4 | from_hex(*(src + 2));
            src += 3;
        } else if (*src == '+') {
            *current_out++ = ' ';
            src++;
        } else {
            *current_out++ = *src++;
        }
    }
    *current_out = '\0';
    return out;
}

char *sanitize_path(const char *path) {
    char *out = malloc(PATH_MAX);
    if (!out) return NULL;

    char *segments[256];
    int seg_count = 0;

    char *copy = strdup(path);
    char *token = strtok(copy, "/");

    while (token) {
        if (strcmp(token, ".") == 0 || strcmp(token, "") == 0) {
            // skip
        } else if (strcmp(token, "..") == 0 && seg_count > 0) {
            segments[seg_count] = NULL;
            seg_count--;
        } else {
            segments[seg_count] = token;
            seg_count++;
        }
        token = strtok(NULL, "/");
    }

    out[0] = '\0';  // make it an empty C string
    for (int i = 0; i < seg_count; i++) {
        if (i != 0) {
            strcat(out, "/");
        }
        strcat(out, segments[i]);
    }

    free(copy);
    return out;
}

void combine_paths(char *out_path, size_t out_path_len, const char *dir_path,
                   const char *relative_path) {
    size_t dir_path_len = strlen(dir_path);

    if (dir_path_len == 0) {
        snprintf(out_path, out_path_len, "%s", relative_path);
    } else if (dir_path[dir_path_len - 1] == '/') {
        snprintf(out_path, out_path_len, "%s%s", dir_path, relative_path);
    } else {
        snprintf(out_path, out_path_len, "%s/%s", dir_path, relative_path);
    }
}

int is_readable_file(const char *filepath) {
    if (access(filepath, R_OK) != 0) {
        return 0;
    }

    struct stat st;
    if (stat(filepath, &st) != 0) {
        return 0;
    }

    if (!S_ISREG(st.st_mode)) {
        return 0;
    }

    return 1;
}

int is_readable_dir(const char *filepath) {
    if (access(filepath, R_OK) != 0) {
        return 0;
    }

    struct stat st;
    if (stat(filepath, &st) != 0) {
        return 0;
    }

    if (!S_ISDIR(st.st_mode)) {
        return 0;
    }

    return 1;
}

int resolve_address(struct in_addr *out_addr, const char *input) {
    // Try parsing as numeric IP first
    if (inet_pton(AF_INET, input, out_addr) == 1) {
        return 1;
    }

    // If not a numeric IP, try resolving as hostname
    struct hostent *he = gethostbyname(input);
    if (he == NULL || he->h_addrtype != AF_INET) {
        return 0;
    }

    memcpy(out_addr, he->h_addr_list[0], sizeof(struct in_addr));
    return 1;
}

char *get_program_directory() {
    static char path[PATH_MAX];

    ssize_t len = readlink("/proc/self/exe", path, sizeof(path) - 1);
    if (len == -1) {
        perror("readlink");
        return NULL;
    }
    path[len] = '\0';  // Null-terminate

    // Get directory of the executable
    char *dir = dirname(path);

    return dir;
}

const char *get_filename_ext(const char *filename) {
    const char *dot = strrchr(filename, '.');
    if (!dot || dot == filename) return "";
    return dot + 1;
}

char *get_filename_name(const char *filename) {
    char *filename_out = strdup(filename);
    char *dot = strrchr(filename_out, '.');
    if (!dot || dot == filename_out) {
        *filename_out = '\0';
        return filename_out;
    };
    *dot = '\0';
    return filename_out;
}

int get_index_file(char *out_path, size_t out_path_len, char *dirname) {
    DIR *dir = opendir(dirname);
    if (!dir) return -1;

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        char *filename_name = get_filename_name(entry->d_name);
        int is_name_index = strcmp(filename_name, "index") == 0;
        free(filename_name);

        if (is_name_index) {
            closedir(dir);

            // Compose full path
            if (dirname[strlen(dirname) - 1] == '/') {
                snprintf(out_path, out_path_len, "%s%s", dirname,
                         entry->d_name);
            } else {
                snprintf(out_path, out_path_len, "%s/%s", dirname,
                         entry->d_name);
            }
            return 0;
        }
    }

    closedir(dir);
    return -1;  // No index file found
}