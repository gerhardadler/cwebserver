#define _POSIX_C_SOURCE 200809L

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "utils.h"

typedef struct {
    char *extension;
    char *mime_type;
} MimeEntry;

char *get_mime_type(const char *ext);

void load_mime_types();