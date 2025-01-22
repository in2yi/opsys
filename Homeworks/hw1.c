#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include "hw1.h"

#define MAX_FILE_SIZE 1048576

int hash(const char *word, int cache_size) {
    int sum = 0;
    while (*word) {
        sum += tolower((unsigned char)*word);
        word++;
    }
    return sum % cache_size;
}

// main function
void process_file(const char *filename, char **cache, int cache_size) {
    int fd = open(filename, O_RDONLY);
    if (fd == -1) {
        perror("ERROR: Cannot open file");
        exit(EXIT_FAILURE);
    }

    char *buffer = (char *)calloc(MAX_FILE_SIZE, sizeof(char));
    if (buffer == NULL) {
        perror("ERROR: Memory allocation failed");
        exit(EXIT_FAILURE);
    }

    ssize_t bytes_read = read(fd, buffer, MAX_FILE_SIZE - 1);
    if (bytes_read == -1) {
        perror("ERROR: File read error");
        free(buffer);
        exit(EXIT_FAILURE);
    }
    *(buffer + bytes_read) = '\0';

    close(fd);
    //tokenize
    char **tokens = tokenize(buffer);
    if (tokens == NULL) {
        perror("ERROR: Tokenization failed");
        free(buffer);
        exit(EXIT_FAILURE);
    }

    // Process each token
    for (size_t i = 0; *(tokens + i) != NULL; i++) {
        char *original_word = *(tokens + i);
        char *lower_word = (char *)calloc(129, sizeof(char));
        if (lower_word == NULL) {
            perror("ERROR: Memory allocation failed");
            exit(EXIT_FAILURE);
        }

        for (size_t j = 0; j < strlen(original_word) && j < 128; j++) {
            *(lower_word + j) = tolower((unsigned char)*(original_word + j));
        }

        *(lower_word + strlen(original_word)) = '\0';

        int index = hash(lower_word, cache_size);

        if (*(cache + index) == NULL) {

            *(cache + index) = (char *)calloc(strlen(lower_word) + 1, sizeof(char));

            if (*(cache + index) == NULL) {
                perror("ERROR: Memory allocation failed");
                exit(EXIT_FAILURE);
            }

            strcpy(*(cache + index), lower_word);
            printf("Word \"%s\" ==> %d (calloc)\n", original_word, index);
        } else if (strcmp(*(cache + index), lower_word) != 0) {

            *(cache + index) = (char *)realloc(*(cache + index), strlen(lower_word) + 1);

            if (*(cache + index) == NULL) {
                perror("ERROR: Memory reallocation failed");
                exit(EXIT_FAILURE);
            }

            strcpy(*(cache + index), lower_word);
            printf("Word \"%s\" ==> %d (realloc)\n", original_word, index);
        } else {
            printf("Word \"%s\" ==> %d (nop)\n", original_word, index);
        }
        free(lower_word);
    }

    free(buffer);
    free(tokens);
}

int main(int argc, char **argv) {
    if (argc < 3) {
        fprintf(stderr, "ERROR: Usage: %s <cache_size> <file1> [file2 ...]\n", *(argv + 0));
        return EXIT_FAILURE;
    }

    char *endptr;
    long cache_size = strtol(*(argv + 1), &endptr, 10);
    if (*endptr != '\0' || cache_size <= 0) {
        fprintf(stderr, "ERROR: Invalid cache size\n");
        return EXIT_FAILURE;
    }

    char **cache = (char **)calloc(cache_size, sizeof(char *));
    if (cache == NULL) {
        perror("ERROR: Memory allocation failed");
        return EXIT_FAILURE;
    }

    for (int i = 2; i < argc; i++) {
        process_file(*(argv + i), cache, cache_size);
    }

    printf("\nCache:\n");
    for (int i = 0; i < cache_size; i++) {
        if (*(cache + i) != NULL) {
            printf("[%d] ==> \"%s\"\n", i, *( cache + i ));
            free(*( cache + i )); 
        }
    }
    free(cache);

    return EXIT_SUCCESS;
}
