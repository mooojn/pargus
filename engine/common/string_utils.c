#include "common/string_utils.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *pargus_strdup(const char *text)
{
    size_t len;
    char *copy;

    if (!text) {
        return NULL;
    }

    len = strlen(text);
    copy = (char *)malloc(len + 1);
    if (!copy) {
        return NULL;
    }

    memcpy(copy, text, len + 1);
    return copy;
}

int pargus_ends_with_ci(const char *text, const char *suffix)
{
    size_t text_len;
    size_t suffix_len;
    size_t offset;

    if (!text || !suffix) {
        return 0;
    }

    text_len = strlen(text);
    suffix_len = strlen(suffix);
    if (suffix_len > text_len) {
        return 0;
    }

    offset = text_len - suffix_len;
    for (size_t i = 0; i < suffix_len; i++) {
        unsigned char a = (unsigned char)text[offset + i];
        unsigned char b = (unsigned char)suffix[i];
        if (tolower(a) != tolower(b)) {
            return 0;
        }
    }

    return 1;
}

const char *pargus_basename(const char *path)
{
    const char *last_slash;
    const char *last_backslash;
    const char *base;

    if (!path) {
        return "";
    }

    last_slash = strrchr(path, '/');
    last_backslash = strrchr(path, '\\');
    base = path;

    if (last_slash && last_slash + 1 > base) {
        base = last_slash + 1;
    }
    if (last_backslash && last_backslash + 1 > base) {
        base = last_backslash + 1;
    }

    return base;
}

int pargus_join_path(char *out, size_t out_size, const char *dir, const char *name)
{
    size_t len;
    int needs_sep;

    if (!out || !dir || !name || out_size == 0) {
        return 0;
    }

    len = strlen(dir);
    needs_sep = len > 0 && dir[len - 1] != '/' && dir[len - 1] != '\\';

    if (snprintf(out, out_size, needs_sep ? "%s/%s" : "%s%s", dir, name) >= (int)out_size) {
        return 0;
    }

    return 1;
}

