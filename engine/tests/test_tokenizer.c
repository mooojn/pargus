#include "nlp/stopwords.h"
#include "nlp/tokenizer.h"

#include <stdio.h>
#include <string.h>

static int contains_token(const TokenList *tokens, const char *needle)
{
    for (int i = 0; i < tokens->count; i++) {
        if (strcmp(tokens->tokens[i], needle) == 0) {
            return 1;
        }
    }
    return 0;
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
    StopwordSet stopwords;
    TokenList tokens;
    int ok = 1;

    stopword_set_init(&stopwords);
    ok = ok && stopword_set_add(&stopwords, "the");
    ok = ok && stopword_set_add(&stopwords, "and");

    ok = ok && tokenize_text("The QUICK, running runners and tested files.", &stopwords, &tokens);
    ok = ok && expect(!contains_token(&tokens, "the"), "stopword removed");
    ok = ok && expect(!contains_token(&tokens, "and"), "second stopword removed");
    ok = ok && expect(contains_token(&tokens, "quick"), "uppercase normalized");
    ok = ok && expect(contains_token(&tokens, "runn"), "ing suffix normalized");
    ok = ok && expect(contains_token(&tokens, "runner"), "plural suffix normalized");
    ok = ok && expect(contains_token(&tokens, "test"), "ed suffix normalized");
    token_list_free(&tokens);

    ok = ok && tokenize_text("... !!!", &stopwords, &tokens);
    ok = ok && expect(tokens.count == 0, "empty punctuation document has no tokens");
    token_list_free(&tokens);

    stopword_set_free(&stopwords);
    return ok ? 0 : 1;
}

