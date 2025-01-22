#ifndef REVERSE_H
#define REVERSE_H

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include <stdlib.h>
#include <ctype.h>

char ** tokenize( char * string ) {
    printf("string begining : %s\n", string);
    if (string == NULL) {
        return NULL;
    }
    size_t capacity = 10; 
    size_t count = 0;  

    char ** tokens = (char **)calloc(capacity, sizeof(char *));
    if (tokens == NULL) {
        return NULL;
    }

    char *ptr = string;

    while (*ptr) {
        while (*ptr && !isalpha((unsigned char)*ptr)) {
            ptr++;
        }

        char *start = ptr;

        while (*ptr && isalpha((unsigned char)*ptr)) {
            ptr++;
        }

        size_t length = ptr - start;

        if (length >= 2) {
            *(tokens + count) = start;
            count++;

            if (*ptr) {
                *ptr = '\0';
                ptr++;
            }

            //resize if neeeded
            if (count >= capacity) {
                size_t new_capacity = capacity * 2;
                char ** temp = (char **)realloc(tokens, new_capacity * sizeof(char *));

                if (temp == NULL) {
                    free(tokens);
                    return NULL;
                }

                tokens = temp;
                capacity = new_capacity;
            }
        } else {
            if (*ptr) {
                ptr++;
            }
        }

        printf("Original string for each step:\n");
        for (size_t i = 0; i < strlen(string); i++) {
            if (string[i] == '\0') {
                printf("\\0");
            } else {
                printf("%c", string[i]);
            }
        }
    }
    tokens[count] = NULL;
    printf("Original string after tokenization (with '\\0' characters):\n");
    for (size_t i = 0; i < strlen(string); i++) {
        if (string[i] == '\0') {
            printf("\\0");
        } else {
            printf("%c", string[i]);
        }
    }
    printf("\n\n");
    return tokens;
}

#endif 
