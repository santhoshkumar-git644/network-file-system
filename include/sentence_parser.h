#ifndef SENTENCE_PARSER_H
#define SENTENCE_PARSER_H

#include "common.h"

// Define a sentence as an array of words
typedef struct {
    char words[MAX_BUFFER_SIZE][MAX_FILENAME];
    int word_count;
    char delimiter; // ., !, or ?
} Sentence;

typedef struct {
    Sentence sentences[100]; // Assume max 100 sentences for this assignment
    int sentence_count;
} ParsedFile;

void parse_file_content(const char* content, ParsedFile* parsed);
void reconstruct_file_content(ParsedFile* parsed, char* content);

#endif // SENTENCE_PARSER_H
