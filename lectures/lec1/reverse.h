#ifndef REVERSE_H
#define REVERSE_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

char * reverse( char * s )
{
    char * buffer = (char *) malloc( strlen( s ) + 1 );
    int i, len = strlen( s );
    *(buffer + len)= '\0';

    for ( i = 0 ; i < len ; i++ ) * (buffer + i) = * (s + (len - i - 1));
    for ( i = 0 ; i <= len ; i++ ) * (s + i) = * (buffer + i);

    free( buffer );
    
    return s;
}


#endif