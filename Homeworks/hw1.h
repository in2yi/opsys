#ifndef REVERSE_H
#define REVERSE_H

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

char **tokenize(char *string) {
    if (!string) return NULL;
    
    char **tokens = NULL;
    size_t count = 0;
    char *ptr = string;
    char *start = NULL;
    
    while (*ptr) {
        if (isalpha(*ptr)) {
            if (!start) {
                start = ptr;
            }
        } else {
            if (start) {
                if (ptr - start >= 2) { // Ensure at least two characters for a valid word
                    *ptr = '\0';
                    char **temp = (char **)realloc(tokens, (count + 2) * sizeof(char *));
                    if (!temp) {
                        free(tokens);
                        return NULL;
                    }
                    tokens = temp;
                    tokens[count++] = start;
                    tokens[count] = NULL;
                }
                start = NULL;
            }
        }
        ptr++;
    }
    
    // Handle last word if the string ends with a valid word
    if (start && (ptr - start >= 2)) {
        char **temp = (char **)realloc(tokens, (count + 2) * sizeof(char *));
        if (!temp) {
            free(tokens);
            return NULL;
        }
        tokens = temp;
        tokens[count++] = start;
        tokens[count] = NULL;
    }
    
    return tokens;
}


#endif