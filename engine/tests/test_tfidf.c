#include "common/types.h"
#include "config/config.h"
#include "nlp/stopwords.h"
#include "nlp/tfidf.h"

#include <math.h>
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

int main(void)
{
    Document docs_storage[2];
    DocumentList docs;
    StopwordSet stopwords;
    EngineConfig config;
    TfidfCorpus corpus;
    double common;
    double rare;
    int ok = 1;

    memset(docs_storage, 0, sizeof(docs_storage));
    docs_storage[0].doc_id = 0;
    docs_storage[0].filename = copy_text("a.txt");
    docs_storage[0].path = copy_text("a.txt");
    docs_storage[0].content = copy_text("common rare");
    docs_storage[0].length = strlen(docs_storage[0].content);

    docs_storage[1].doc_id = 1;
    docs_storage[1].filename = copy_text("b.txt");
    docs_storage[1].path = copy_text("b.txt");
    docs_storage[1].content = copy_text("common");
    docs_storage[1].length = strlen(docs_storage[1].content);

    docs.items = docs_storage;
    docs.count = 2;
    docs.capacity = 2;

    stopword_set_init(&stopwords);
    config_init_defaults(&config);
    config.serial_mode = 1;

    ok = ok && build_tfidf_corpus(&docs, &stopwords, &config, &corpus);
    ok = ok && expect(corpus.total_tokens == 3, "total token count");
    ok = ok && expect(corpus.vocabulary.count == 2, "vocabulary count");

    common = fabs(tfidf_value_for_term(&corpus, 0, "common"));
    rare = fabs(tfidf_value_for_term(&corpus, 0, "rare"));
    ok = ok && expect(rare > common, "rare term has stronger TF-IDF weight than common term");

    tfidf_corpus_free(&corpus);
    stopword_set_free(&stopwords);

    for (int i = 0; i < 2; i++) {
        free(docs_storage[i].filename);
        free(docs_storage[i].path);
        free(docs_storage[i].content);
    }

    return ok ? 0 : 1;
}
