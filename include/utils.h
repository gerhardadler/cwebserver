#ifndef UTILS_H
#define UTILS_H

#include <arpa/inet.h>
#include <ctype.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

// Converts a single hex character to its numeric value.
// Returns -1 if the character is not a valid hex digit.
char from_hex(char c);

// Decodes a URL-encoded string. Caller must free the returned string.
char *url_decode(const char *src);

// Sanitizes a path by removing "." and resolving ".." components.
// Caller must free the returned string.
char *sanitize_path(const char *path);

// Combines directory and relative paths into out_path with a specified buffer
// length.
void combine_paths(char *out_path, size_t out_path_len, const char *dir_path,
                   const char *relative_path);

// Returns 1 if filepath is a readable regular file, 0 otherwise.
int is_readable_file(const char *filepath);

// Returns 1 if filepath is a readable dir, 0 otherwise.
int is_readable_dir(const char *filepath);

// Resolves a hostname or IP string to a struct in_addr.
// Returns 1 on success, 0 on failure.
int resolve_address(struct in_addr *out_addr, const char *input);

// Gets the directory of the program executable
char *get_program_directory();

// Get extension from filename
const char *get_filename_ext(const char *filename);

// Get name from filename
char *get_filename_name(const char *filename);

// Get index file in directory, and set out_path. Returns 0 if found, and -1
// elsewise.
int get_index_file(char *out_path, size_t out_path_len, char *dirname);

#endif  // UTILS_H