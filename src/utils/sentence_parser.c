#include "sentence_parser.h"
#include <ctype.h>

void parse_file_content(const char* content, ParsedFile* parsed) {
    parsed->sentence_count = 0;
    
    int i = 0;
    int len = strlen(content);
    
    Sentence current_sentence;
    current_sentence.word_count = 0;
    current_sentence.delimiter = '.'; // Default
    
    char current_word[MAX_FILENAME];
    int word_idx = 0;
    
    while (i < len) {
        char c = content[i];
        
        if (c == ' ' || c == '\n' || c == '\r' || c == '.' || c == '!' || c == '?') {
            if (word_idx > 0) {
                current_word[word_idx] = '\0';
                strncpy(current_sentence.words[current_sentence.word_count], current_word, MAX_FILENAME);
                current_sentence.word_count++;
                word_idx = 0;
            }
            
            if (c == '.' || c == '!' || c == '?') {
                current_sentence.delimiter = c;
                parsed->sentences[parsed->sentence_count] = current_sentence;
                parsed->sentence_count++;
                current_sentence.word_count = 0;
            }
        } else {
            if (word_idx < MAX_FILENAME - 1) {
                current_word[word_idx++] = c;
            }
        }
        i++;
    }
    
    // Add any remaining sentence without delimiter
    if (current_sentence.word_count > 0 || word_idx > 0) {
        if (word_idx > 0) {
            current_word[word_idx] = '\0';
            strncpy(current_sentence.words[current_sentence.word_count], current_word, MAX_FILENAME);
            current_sentence.word_count++;
        }
        current_sentence.delimiter = '\0'; // No delimiter
        parsed->sentences[parsed->sentence_count] = current_sentence;
        parsed->sentence_count++;
    }
}

void reconstruct_file_content(ParsedFile* parsed, char* content) {
    content[0] = '\0';
    
    for (int i = 0; i < parsed->sentence_count; i++) {
        Sentence* s = &parsed->sentences[i];
        for (int j = 0; j < s->word_count; j++) {
            strcat(content, s->words[j]);
            // If it's not the last word in sentence, or there's a delimiter coming right after, wait, normally spaces are after words unless punctuation.
            // Simplified: Add space after every word unless it's the last word and we have a delimiter.
            if (j < s->word_count - 1) {
                strcat(content, " ");
            }
        }
        if (s->delimiter != '\0') {
            int len = strlen(content);
            content[len] = s->delimiter;
            content[len+1] = ' ';
            content[len+2] = '\0';
        }
    }
}
