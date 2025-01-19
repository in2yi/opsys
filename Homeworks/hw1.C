#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "hw1.h"

int main() {
    char input[] = "RPI) is a private research university in Troy, New York, United Sta";
    char **words = tokenize(input);
    
    if (words) {
        for (size_t i = 0; words[i]; i++) {
            printf("%s\n", words[i]);
        }
        free(words);
    }
    
    return 0;
}


