#include "nlp/lsh.h"

#include <stdlib.h>

typedef struct {
    unsigned int key;
    int doc_id;
    int band;
} BandEntry;

void pair_list_init(PairList *list)
{
    list->pairs = NULL;
    list->count = 0;
    list->capacity = 0;
}

void pair_list_free(PairList *list)
{
    if (!list) {
        return;
    }
    free(list->pairs);
    pair_list_init(list);
}

int pair_list_contains(const PairList *list, int a, int b)
{
    int lo = a < b ? a : b;
    int hi = a < b ? b : a;

    if (lo == hi) {
        return 1;
    }

    for (int i = 0; i < list->count; i++) {
        if (list->pairs[i].a == lo && list->pairs[i].b == hi) {
            return 1;
        }
    }

    return 0;
}

int pair_list_add_unique(PairList *list, int a, int b)
{
    DocPair *grown;
    int new_capacity;
    int lo = a < b ? a : b;
    int hi = a < b ? b : a;

    if (lo == hi || pair_list_contains(list, lo, hi)) {
        return 1;
    }

    if (list->count == list->capacity) {
        new_capacity = list->capacity == 0 ? 32 : list->capacity * 2;
        grown = (DocPair *)realloc(list->pairs, (size_t)new_capacity * sizeof(DocPair));
        if (!grown) {
            return 0;
        }
        list->pairs = grown;
        list->capacity = new_capacity;
    }

    list->pairs[list->count].a = lo;
    list->pairs[list->count].b = hi;
    list->count++;
    return 1;
}

static unsigned int band_hash(const MinHashSignature *signature, int start, int rows, int band)
{
    unsigned int hash = 2166136261u ^ (unsigned int)band;

    for (int i = 0; i < rows; i++) {
        hash ^= signature->values[start + i] + 0x9e3779b9u + (hash << 6) + (hash >> 2);
    }

    return hash;
}

static int compare_band_entries(const void *left, const void *right)
{
    const BandEntry *a = (const BandEntry *)left;
    const BandEntry *b = (const BandEntry *)right;

    if (a->band != b->band) {
        return a->band < b->band ? -1 : 1;
    }
    if (a->key != b->key) {
        return a->key < b->key ? -1 : 1;
    }
    return a->doc_id - b->doc_id;
}

int build_lsh_candidates(const MinHashCorpus *minhash, const EngineConfig *config, PairList *pairs)
{
    int bands = config->bands;
    int rows = config->rows;
    int entry_count;
    BandEntry *entries;

    pair_list_init(pairs);

    if (bands <= 0 || rows <= 0 || bands * rows > minhash->length) {
        bands = 20;
        rows = 5;
    }
    if (bands * rows > minhash->length) {
        rows = minhash->length / bands;
    }
    if (rows <= 0) {
        rows = 1;
        bands = minhash->length;
    }

    entry_count = minhash->count * bands;
    entries = (BandEntry *)calloc((size_t)entry_count, sizeof(BandEntry));
    if (!entries) {
        return 0;
    }

    for (int doc_id = 0; doc_id < minhash->count; doc_id++) {
        for (int band = 0; band < bands; band++) {
            int index = doc_id * bands + band;
            int start = band * rows;
            entries[index].doc_id = doc_id;
            entries[index].band = band;
            entries[index].key = band_hash(&minhash->signatures[doc_id], start, rows, band);
        }
    }

    qsort(entries, (size_t)entry_count, sizeof(BandEntry), compare_band_entries);

    for (int i = 0; i < entry_count;) {
        int j = i + 1;
        while (j < entry_count && entries[j].band == entries[i].band && entries[j].key == entries[i].key) {
            j++;
        }

        if (j - i > 1) {
            for (int a = i; a < j; a++) {
                for (int b = a + 1; b < j; b++) {
                    if (!pair_list_add_unique(pairs, entries[a].doc_id, entries[b].doc_id)) {
                        free(entries);
                        pair_list_free(pairs);
                        return 0;
                    }
                }
            }
        }

        i = j;
    }

    free(entries);
    return 1;
}

