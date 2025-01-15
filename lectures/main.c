#include "reverse.h"

int main()
{
    char str[] = "hello"; // Use a mutable array
    printf("Original: %s\n", str);
    reverse(str);
    printf("Reversed: %s\n", str);

    return 0;
}