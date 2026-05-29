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
#include <pthread.h>
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

#ifdef _WIN32
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
#endif

static int load_document_at(Document *doc, int doc_id, const char *path)
{
    memset(doc, 0, sizeof(*doc));
    doc->doc_id = doc_id;
    doc->path = pargus_strdup(path);
    doc->filename = pargus_strdup(pargus_basename(path));

    if (!doc->path || !doc->filename || !read_file_content(path, &doc->content, &doc->length)) {
        document_free(doc);
        return 0;
    }

    return 1;
}

#ifdef _WIN32
static int add_document(DocumentList *docs, const char *path)
{
    Document doc;

    if (!load_document_at(&doc, docs->count, path)) {
        return 0;
    }

    if (!document_list_push(docs, doc)) {
        document_free(&doc);
        fprintf(stderr, "Out of memory storing document metadata\n");
        return 0;
    }

    return 1;
}
#endif

#ifdef _WIN32
int read_documents_from_dir(const char *input_dir, const EngineConfig *config, DocumentList *docs)
{
    char pattern[1024];
    char path[1024];
    WIN32_FIND_DATAA find_data;
    HANDLE handle;

    (void)config;
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
typedef struct {
    char **items;
    int count;
    int capacity;
} PathList;

typedef struct {
    const PathList *paths;
    DocumentList *docs;
    int next_index;
    int failed;
    pthread_mutex_t mutex;
} ReaderThreadState;

static void path_list_init(PathList *list)
{
    list->items = NULL;
    list->count = 0;
    list->capacity = 0;
}

static void path_list_free(PathList *list)
{
    if (!list) {
        return;
    }
    for (int i = 0; i < list->count; i++) {
        free(list->items[i]);
    }
    free(list->items);
    path_list_init(list);
}

static int path_list_push(PathList *list, const char *path)
{
    char **grown;
    int new_capacity;

    if (list->count == list->capacity) {
        new_capacity = list->capacity == 0 ? 16 : list->capacity * 2;
        grown = (char **)realloc(list->items, (size_t)new_capacity * sizeof(char *));
        if (!grown) {
            return 0;
        }
        list->items = grown;
        list->capacity = new_capacity;
    }

    list->items[list->count] = pargus_strdup(path);
    if (!list->items[list->count]) {
        return 0;
    }
    list->count++;
    return 1;
}

static int compare_paths(const void *left, const void *right)
{
    const char *a = *(char * const *)left;
    const char *b = *(char * const *)right;

    return strcmp(a, b);
}

static int discover_document_paths(const char *input_dir, PathList *paths)
{
    DIR *dir;
    struct dirent *entry;
    char path[1024];
    struct stat st;

    path_list_init(paths);
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
            path_list_free(paths);
            fprintf(stderr, "Document path is too long: %s\n", entry->d_name);
            return 0;
        }
        if (stat(path, &st) != 0 || !S_ISREG(st.st_mode)) {
            continue;
        }
        if (!path_list_push(paths, path)) {
            closedir(dir);
            path_list_free(paths);
            fprintf(stderr, "Out of memory storing document path\n");
            return 0;
        }
    }

    closedir(dir);

    if (paths->count == 0) {
        fprintf(stderr, "No .txt documents found in: %s\n", input_dir);
        return 0;
    }

    qsort(paths->items, (size_t)paths->count, sizeof(char *), compare_paths);
    return 1;
}

static int document_list_allocate(DocumentList *docs, int count)
{
    document_list_init(docs);
    docs->items = (Document *)calloc((size_t)count, sizeof(Document));
    if (!docs->items && count > 0) {
        return 0;
    }
    docs->count = count;
    docs->capacity = count;
    return 1;
}

static void *reader_worker(void *arg)
{
    ReaderThreadState *state = (ReaderThreadState *)arg;

    for (;;) {
        int index;

        pthread_mutex_lock(&state->mutex);
        if (state->failed || state->next_index >= state->paths->count) {
            pthread_mutex_unlock(&state->mutex);
            break;
        }
        index = state->next_index++;
        pthread_mutex_unlock(&state->mutex);

        if (!load_document_at(&state->docs->items[index], index, state->paths->items[index])) {
            pthread_mutex_lock(&state->mutex);
            state->failed = 1;
            pthread_mutex_unlock(&state->mutex);
            break;
        }
    }

    return NULL;
}

static int load_documents_serial(const PathList *paths, DocumentList *docs)
{
    if (!document_list_allocate(docs, paths->count)) {
        fprintf(stderr, "Out of memory allocating document list\n");
        return 0;
    }

    for (int i = 0; i < paths->count; i++) {
        if (!load_document_at(&docs->items[i], i, paths->items[i])) {
            document_list_free(docs);
            return 0;
        }
    }

    return 1;
}

static int load_documents_pthreads(const PathList *paths, const EngineConfig *config, DocumentList *docs)
{
    pthread_t *threads;
    ReaderThreadState state;
    int thread_count = config->threads;
    int ok = 1;

    if (thread_count < 1) {
        thread_count = 1;
    }
    if (thread_count > paths->count) {
        thread_count = paths->count;
    }
    if (thread_count <= 1) {
        return load_documents_serial(paths, docs);
    }

    if (!document_list_allocate(docs, paths->count)) {
        fprintf(stderr, "Out of memory allocating document list\n");
        return 0;
    }

    memset(&state, 0, sizeof(state));
    state.paths = paths;
    state.docs = docs;
    if (pthread_mutex_init(&state.mutex, NULL) != 0) {
        document_list_free(docs);
        fprintf(stderr, "Failed to initialize Pthreads reader mutex\n");
        return 0;
    }

    threads = (pthread_t *)calloc((size_t)thread_count, sizeof(pthread_t));
    if (!threads) {
        pthread_mutex_destroy(&state.mutex);
        document_list_free(docs);
        fprintf(stderr, "Out of memory allocating reader threads\n");
        return 0;
    }

    for (int i = 0; i < thread_count; i++) {
        if (pthread_create(&threads[i], NULL, reader_worker, &state) != 0) {
            pthread_mutex_lock(&state.mutex);
            state.failed = 1;
            pthread_mutex_unlock(&state.mutex);
            thread_count = i;
            ok = 0;
            break;
        }
    }

    for (int i = 0; i < thread_count; i++) {
        pthread_join(threads[i], NULL);
    }

    if (state.failed) {
        ok = 0;
    }

    free(threads);
    pthread_mutex_destroy(&state.mutex);

    if (!ok) {
        document_list_free(docs);
        return 0;
    }

    return 1;
}

int read_documents_from_dir(const char *input_dir, const EngineConfig *config, DocumentList *docs)
{
    PathList paths;
    int ok;

    if (!discover_document_paths(input_dir, &paths)) {
        return 0;
    }

    if (config && config->mode == PARGUS_MODE_PTHREADS) {
        ok = load_documents_pthreads(&paths, config, docs);
    } else {
        ok = load_documents_serial(&paths, docs);
    }

    path_list_free(&paths);
    return ok;
}
#endif
