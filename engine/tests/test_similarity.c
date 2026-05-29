#include "common/types.h"
#include "config/config.h"
#include "nlp/lsh.h"
#include "nlp/similarity.h"
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

static void free_docs(Document *docs, int count)
{
    for (int i = 0; i < count; i++) {
        free(docs[i].filename);
        free(docs[i].path);
        free(docs[i].content);
    }
}

int main(void)
{
    Document docs_storage[3];
    DocumentList docs;
    StopwordSet stopwords;
    EngineConfig config;
    TfidfCorpus tfidf;
    PairList pairs;
    SimilarityResults similarity;
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
    docs_storage[2].content = copy_text("compiler memory kernel scheduler process cache thread network");
    docs_storage[2].length = strlen(docs_storage[2].content);

    docs.items = docs_storage;
    docs.count = 3;
    docs.capacity = 3;

    stopword_set_init(&stopwords);
    config_init_defaults(&config);
    config.serial_mode = 1;
    config.sim_threshold = 0.75;

    pair_list_init(&pairs);
    ok = ok && build_tfidf_corpus(&docs, &stopwords, &config, &tfidf);
    ok = ok && pair_list_add_unique(&pairs, 0, 1);
    ok = ok && pair_list_add_unique(&pairs, 0, 2);
    ok = ok && compute_similarity_results(&tfidf, &pairs, &config, &similarity);

    ok = ok && expect(cosine_similarity(&tfidf.vectors[0], &tfidf.vectors[1]) > 0.999, "identical vectors have cosine near one");
    ok = ok && expect(rabin_karp_similarity(&tfidf.tokens[0], &tfidf.tokens[1], 5) > 0.999, "identical token streams have fingerprint overlap near one");
    ok = ok && expect(similarity_matrix_get(&similarity, 0, 1) > 0.999, "identical documents score near one");
    ok = ok && expect(similarity_matrix_get(&similarity, 0, 2) < 0.25, "different documents score low");
    ok = ok && expect(fabs(similarity_matrix_get(&similarity, 0, 1) - similarity_matrix_get(&similarity, 1, 0)) < 0.000001, "matrix is symmetric");
    ok = ok && expect(similarity_matrix_get(&similarity, 0, 0) == 1.0, "matrix diagonal is one");
    ok = ok && expect(similarity.flagged_count == 1, "thresholded flagged pair count is correct");

    similarity_results_free(&similarity);
    pair_list_free(&pairs);
    tfidf_corpus_free(&tfidf);
    stopword_set_free(&stopwords);
    free_docs(docs_storage, 3);
    return ok ? 0 : 1;
}
