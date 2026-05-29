#include "common/types.h"
#include "config/config.h"
#include "nlp/lsh.h"
#include "nlp/minhash.h"
#include "nlp/stopwords.h"
#include "nlp/tfidf.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *copy_text(const char *text)
{
    size_t len = strlen(text);
    char *copy = (char *)malloc(len + 1);
    if (copy) {
        memcpy(copy, text, len + 1);
    }
    return copy;
}

static int expect(int condition, const char *message)
{
    if (!condition) {
        fprintf(stderr, "FAIL: %s\n", message);
        return 0;
    }
    return 1;
}

static void free_docs(Document *docs, int count)
{
    for (int i = 0; i < count; i++) {
        free(docs[i].filename);
        free(docs[i].path);
        free(docs[i].content);
    }
}

static int signatures_equal(const MinHashSignature *a, const MinHashSignature *b)
{
    if (a->length != b->length) {
        return 0;
    }
    for (int i = 0; i < a->length; i++) {
        if (a->values[i] != b->values[i]) {
            return 0;
        }
    }
    return 1;
}

int main(void)
{
    Document docs_storage[3];
    DocumentList docs;
    StopwordSet stopwords;
    EngineConfig config;
    TfidfCorpus tfidf;
    MinHashCorpus minhash_a;
    MinHashCorpus minhash_b;
    PairList pairs;
    int ok = 1;

    memset(docs_storage, 0, sizeof(docs_storage));
    docs_storage[0].doc_id = 0;
    docs_storage[0].filename = copy_text("a.txt");
    docs_storage[0].path = copy_text("a.txt");
    docs_storage[0].content = copy_text("alpha beta gamma delta epsilon zeta eta theta");
    docs_storage[0].length = strlen(docs_storage[0].content);

    docs_storage[1].doc_id = 1;
    docs_storage[1].filename = copy_text("b.txt");
    docs_storage[1].path = copy_text("b.txt");
    docs_storage[1].content = copy_text("alpha beta gamma delta epsilon zeta eta theta");
    docs_storage[1].length = strlen(docs_storage[1].content);

    docs_storage[2].doc_id = 2;
    docs_storage[2].filename = copy_text("c.txt");
    docs_storage[2].path = copy_text("c.txt");
    docs_storage[2].content = copy_text("network compiler memory process scheduler kernel cache thread");
    docs_storage[2].length = strlen(docs_storage[2].content);

    docs.items = docs_storage;
    docs.count = 3;
    docs.capacity = 3;

    stopword_set_init(&stopwords);
    config_init_defaults(&config);
    config.serial_mode = 1;
    config.bands = 20;
    config.rows = 5;

    ok = ok && build_tfidf_corpus(&docs, &stopwords, &config, &tfidf);
    ok = ok && build_minhash_corpus(&tfidf, &config, &minhash_a);
    ok = ok && build_minhash_corpus(&tfidf, &config, &minhash_b);

    ok = ok && expect(signatures_equal(&minhash_a.signatures[0], &minhash_b.signatures[0]), "minhash signature is deterministic");
    ok = ok && expect(signatures_equal(&minhash_a.signatures[0], &minhash_a.signatures[1]), "identical documents have matching signatures");

    ok = ok && build_lsh_candidates(&minhash_a, &config, &pairs);
    ok = ok && expect(pair_list_contains(&pairs, 0, 1), "identical documents become candidates");

    ok = ok && pair_list_add_unique(&pairs, 1, 0);
    ok = ok && pair_list_add_unique(&pairs, 0, 1);
    ok = ok && expect(pairs.count <= 3, "duplicate pair insertions are ignored");

    pair_list_free(&pairs);
    minhash_corpus_free(&minhash_b);
    minhash_corpus_free(&minhash_a);
    tfidf_corpus_free(&tfidf);
    stopword_set_free(&stopwords);
    free_docs(docs_storage, 3);
    return ok ? 0 : 1;
}

