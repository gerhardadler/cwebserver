#define _POSIX_C_SOURCE 200809L

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "utils.h"

#define MAX_LINE_LENGTH 1024
#define MAX_EXTENSIONS 10

typedef struct {
    char *extension;
    char *mime_type;
} MimeEntry;

MimeEntry *entries = NULL;
size_t entry_count = 0;

void add_entry(const char *mime_type, const char *ext) {
    entries = realloc(entries, sizeof(MimeEntry) * (entry_count + 1));
    entries[entry_count].mime_type = strdup(mime_type);
    entries[entry_count].extension = strdup(ext);
    entry_count++;
}

char *get_mime_type(const char *ext) {
    for (size_t i = 0; i < entry_count; i++) {
        if (strcmp(entries[i].extension, ext) == 0) {
            return entries[i].mime_type;
        }
    }

    return "application/octet-stream";
}

void parse_line(char *line) {
    if (line[0] == '#' || isspace(line[0])) return;

    // end the line on the first '#' (if inline comments)
    char *first_pound = strchr(line, '#');
    if (first_pound != NULL) {
        *first_pound = '\0';
    }

    char *tokens[MAX_EXTENSIONS + 1];  // mime type + extension
    int count = 0;

    char *token = strtok(line, " \t\n\r");
    while (token && count < MAX_EXTENSIONS + 1) {
        tokens[count++] = token;
        token = strtok(NULL, " \t\n\r");
    }

    if (count >= 2) {
        char *mime_type = tokens[0];
        for (int i = 1; i < count; i++) {
            add_entry(mime_type, tokens[i]);
        }
    }
}

void load_mime_types() {
    char mime_path[PATH_MAX];

    snprintf(mime_path, sizeof(mime_path), "%s/mime.types",
             get_program_directory());

    FILE *fp = fopen(mime_path, "r");
    if (!fp) {
        perror("fopen");
        return;
    }

    char line[MAX_LINE_LENGTH];
    while ((fgets(line, sizeof(line), fp))) {
        parse_line(line);
    }

    fclose(fp);
}