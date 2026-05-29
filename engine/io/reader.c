#include "io/reader.h"

#include "common/string_utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <dirent.h>
#include <sys/stat.h>
#endif

void document_free(Document *doc)
{
    if (!doc) {
        return;
    }
    free(doc->path);
    free(doc->filename);
    free(doc->content);
    memset(doc, 0, sizeof(*doc));
}

void document_list_init(DocumentList *list)
{
    list->items = NULL;
    list->count = 0;
    list->capacity = 0;
}

void document_list_free(DocumentList *list)
{
    if (!list) {
        return;
    }
    for (int i = 0; i < list->count; i++) {
        document_free(&list->items[i]);
    }
    free(list->items);
    document_list_init(list);
}

static int document_list_push(DocumentList *list, Document doc)
{
    Document *grown;
    int new_capacity;

    if (list->count == list->capacity) {
        new_capacity = list->capacity == 0 ? 8 : list->capacity * 2;
        grown = (Document *)realloc(list->items, (size_t)new_capacity * sizeof(Document));
        if (!grown) {
            return 0;
        }
        list->items = grown;
        list->capacity = new_capacity;
    }

    list->items[list->count++] = doc;
    return 1;
}

static int read_file_content(const char *path, char **content_out, size_t *length_out)
{
    FILE *file;
    long size;
    char *buffer;
    size_t read_count;

    file = fopen(path, "rb");
    if (!file) {
        fprintf(stderr, "Failed to open document: %s\n", path);
        return 0;
    }

    if (fseek(file, 0, SEEK_END) != 0) {
        fclose(file);
        fprintf(stderr, "Failed to seek document: %s\n", path);
        return 0;
    }

    size = ftell(file);
    if (size < 0) {
        fclose(file);
        fprintf(stderr, "Failed to measure document: %s\n", path);
        return 0;
    }

    rewind(file);

    buffer = (char *)malloc((size_t)size + 1);
    if (!buffer) {
        fclose(file);
        fprintf(stderr, "Out of memory reading: %s\n", path);
        return 0;
    }

    read_count = fread(buffer, 1, (size_t)size, file);
    fclose(file);

    if (read_count != (size_t)size) {
        free(buffer);
        fprintf(stderr, "Failed to read full document: %s\n", path);
        return 0;
    }

    buffer[size] = '\0';
    *content_out = buffer;
    *length_out = (size_t)size;
    return 1;
}

static int add_document(DocumentList *docs, const char *path)
{
    Document doc;

    memset(&doc, 0, sizeof(doc));
    doc.doc_id = docs->count;
    doc.path = pargus_strdup(path);
    doc.filename = pargus_strdup(pargus_basename(path));

    if (!doc.path || !doc.filename || !read_file_content(path, &doc.content, &doc.length)) {
        document_free(&doc);
        return 0;
    }

    if (!document_list_push(docs, doc)) {
        document_free(&doc);
        fprintf(stderr, "Out of memory storing document metadata\n");
        return 0;
    }

    return 1;
}

#ifdef _WIN32
int read_documents_from_dir(const char *input_dir, DocumentList *docs)
{
    char pattern[1024];
    char path[1024];
    WIN32_FIND_DATAA find_data;
    HANDLE handle;

    document_list_init(docs);

    if (!pargus_join_path(pattern, sizeof(pattern), input_dir, "*.txt")) {
        fprintf(stderr, "Input path is too long\n");
        return 0;
    }

    handle = FindFirstFileA(pattern, &find_data);
    if (handle == INVALID_HANDLE_VALUE) {
        fprintf(stderr, "No .txt documents found in: %s\n", input_dir);
        return 0;
    }

    do {
        if (!(find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
            if (!pargus_join_path(path, sizeof(path), input_dir, find_data.cFileName)) {
                FindClose(handle);
                document_list_free(docs);
                fprintf(stderr, "Document path is too long: %s\n", find_data.cFileName);
                return 0;
            }
            if (!add_document(docs, path)) {
                FindClose(handle);
                document_list_free(docs);
                return 0;
            }
        }
    } while (FindNextFileA(handle, &find_data));

    FindClose(handle);

    if (docs->count == 0) {
        fprintf(stderr, "No .txt documents found in: %s\n", input_dir);
        return 0;
    }

    return 1;
}
#else
int read_documents_from_dir(const char *input_dir, DocumentList *docs)
{
    DIR *dir;
    struct dirent *entry;
    char path[1024];
    struct stat st;

    document_list_init(docs);
    dir = opendir(input_dir);
    if (!dir) {
        fprintf(stderr, "Failed to open input directory: %s\n", input_dir);
        return 0;
    }

    while ((entry = readdir(dir)) != NULL) {
        if (!pargus_ends_with_ci(entry->d_name, ".txt")) {
            continue;
        }
        if (!pargus_join_path(path, sizeof(path), input_dir, entry->d_name)) {
            closedir(dir);
            document_list_free(docs);
            fprintf(stderr, "Document path is too long: %s\n", entry->d_name);
            return 0;
        }
        if (stat(path, &st) != 0 || !S_ISREG(st.st_mode)) {
            continue;
        }
        if (!add_document(docs, path)) {
            closedir(dir);
            document_list_free(docs);
            return 0;
        }
    }

    closedir(dir);

    if (docs->count == 0) {
        fprintf(stderr, "No .txt documents found in: %s\n", input_dir);
        return 0;
    }

    return 1;
}
#endif

