#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "incorrect number of arguments\n");
        exit(EXIT_FAILURE);
    }

    int n = atoi(*(argv + 1));
    if (n <= 0) {
        fprintf(stderr, "cant be negative\n");
        exit(EXIT_FAILURE);
    }

    int fd = open(*(argv + 2), O_RDONLY);
    if (fd == -1) {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }

    char *Wstring = malloc(n);
    if (Wstring == NULL) {
        perror("Error allocating memory");
        close(fd);
        exit(EXIT_FAILURE);
    }
    // printf("first Wstring: %s\n", Wstring);

    ssize_t bytes_read;
    int first = 1;

    while ((bytes_read = read(fd, Wstring, n)) == n) {
        if (first) {
            first = 0;
        }
        else {
            write(STDOUT_FILENO, "|", 1);
        }
        write(STDOUT_FILENO, Wstring, n);
        // printf("current Wstring: %s\n", Wstring);
        if (lseek(fd, n, SEEK_CUR) == -1) {
            perror("Error seeking file");
            free(Wstring);
            close(fd);
            exit(EXIT_FAILURE);
        }
    }
    if (bytes_read == n - 1) {
        Wstring[bytes_read] = '\0'; 
        // printf("last Wstring: %s\n", Wstring);
        write(STDOUT_FILENO, "|", 1);
        write(STDOUT_FILENO, Wstring, bytes_read);
    } 

    write(STDOUT_FILENO, "\n", 1);

    free(Wstring);

    if (close(fd) == -1) {
        perror("Error closing file");
        exit(EXIT_FAILURE);
    }

    return 0;
}
