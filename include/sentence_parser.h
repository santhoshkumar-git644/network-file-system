#ifndef SENTENCE_PARSER_H
#define SENTENCE_PARSER_H

#include "common.h"

// Max words per sentence and max chars per word
#define MAX_WORDS_PER_SENTENCE 256
#define MAX_WORD_LEN 64
#define MAX_SENTENCES 100

// Define a sentence as an array of words
typedef struct {
    char words[MAX_WORDS_PER_SENTENCE][MAX_WORD_LEN];
    int word_count;
    char delimiter; // ., !, or ?
} Sentence;

typedef struct {
    Sentence sentences[MAX_SENTENCES];
    int sentence_count;
} ParsedFile;

void parse_file_content(const char* content, ParsedFile* parsed);
void reconstruct_file_content(ParsedFile* parsed, char* content);

#endif // SENTENCE_PARSER_H
