/***************************************************************
 * hw2.c -- Reference solution sketch for the Knight's Tour using
 *          fork(), waitpid(), pipe(), and dynamic memory allocation.
 *
 * NOTE:  - No square brackets: Use pointer arithmetic instead.
 *        - We rely on #ifdef QUIET / NO_PARALLEL for minimal or
 *          non-parallel operation as requested.
 *        - This is a minimal (but complete) demonstration. In practice,
 *          you might want to split functionality into multiple functions.
 *        - Watch for lines that might inadvertently contain '[' or ']'.
 *
 * USAGE:
 *   ./hw2.out <m> <n> <start_row> <start_col>
 *
 * Example outputs (PID values will differ):
 *   PID 100: Solving the knight's tour problem for a 3x3 board
 *   PID 100: Starting at row 0 and column 0 (move #1)
 *   PID 100: 2 possible moves after move #1; creating 2 child processes...
 *   PID 101: Dead end at move #8
 *   PID 102: Dead end at move #8
 *   PID 100: Search complete; best solution(s) visited 8 squares out of 9
 ***************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <string.h>

/* For error printing */
#include <errno.h>

/*-------------------------------------------------------------------*
 * Knight's Moves in a clockwise order (starting from row-2,col+1).
 * We cannot use an array like move_row[8], move_col[8], so store them
 * statically and access via pointer arithmetic.
 *-------------------------------------------------------------------*/
static int kRowOffsets[8] = { -2, -1,  1,  2,  2,  1, -1, -2 };
static int kColOffsets[8] = {  1,  2,  2,  1, -1, -2, -2, -1 };

/* A small helper to handle error printing and exit. */
static void error_and_exit( const char *msg )
{
    /* If errno was set by a previous failing call, we can use perror. */
    if( errno != 0 ) {
        perror( msg );
    } else {
        /* Otherwise just print a formatted message to stderr. */
        fprintf( stderr, "ERROR: %s\n", msg );
    }
    exit( EXIT_FAILURE );
}

/*-------------------------------------------------------------------*
 * Validate inputs and usage.
 *-------------------------------------------------------------------*/
static void validate_args_or_die( long m, long n, long r, long c )
{
    if( m < 3 || n < 3 ) {
        fprintf(stderr, "ERROR: Invalid argument(s)\n");
        fprintf(stderr, "USAGE: ./hw2.out <m> <n> <r> <c>\n");
        exit(EXIT_FAILURE);
    }
    if( r < 0 || r >= m || c < 0 || c >= n ) {
        fprintf(stderr, "ERROR: Invalid argument(s)\n");
        fprintf(stderr, "USAGE: ./hw2.out <m> <n> <r> <c>\n");
        exit(EXIT_FAILURE);
    }
}

/*-------------------------------------------------------------------*
 * Write exactly 1 byte to pipe indicating an open ('O') or closed ('C')
 * solution was found, to be read by the top-level parent.
 *-------------------------------------------------------------------*/
static void write_solution_to_pipe( int pipe_fd, char sol_type )
{
    /* Typically you would want to check for partial writes, but
       writing a single byte in a well-formed environment should succeed. */
    if( write( pipe_fd, &sol_type, 1 ) == -1 ) {
        /* Not fatal to the entire process, but we do want to know. */
        perror("write to pipe failed");
    }
}

/*-------------------------------------------------------------------*
 * Find all valid knight moves from (row, col). Return the number of
 * valid moves found, and store them in 'moves' array. We cannot
 * use bracket indexing, so we do pointer arithmetic.
 *
 * moves[] is effectively an array of pairs: (row, col).
 *
 * Remember: board is allocated as:
 *    board = calloc(m, sizeof(int*));
 *    *(board + i) = calloc(n, sizeof(int));
 * so to reference board[row][col] we do:
 *    *(*(board + row) + col)
 *-------------------------------------------------------------------*/
static int find_valid_moves(
    int cur_row, int cur_col,
    int m, int n,
    int **board,
    int *moves /* capacity for 16 integers: row,col, row,col, ... */
)
{
    int count = 0;
    for( int i = 0; i < 8; i++ )
    {
        int nr = cur_row + *(kRowOffsets + i);
        int nc = cur_col + *(kColOffsets + i);

        /* Check if in-bounds. */
        if( nr >= 0 && nr < m && nc >= 0 && nc < n )
        {
            /* Check if not visited yet. */
            if( *(*(board + nr) + nc) == 0 )
            {
                /* Store the move. We do not do moves[count][0], etc. */
                /* Each move takes two slots: row, col. */
                *(moves + (2 * count) + 0) = nr;
                *(moves + (2 * count) + 1) = nc;
                count++;
            }
        }
    }
    return count;
}

/*-------------------------------------------------------------------*
 * Recursively attempt to solve the Knight's Tour from the current
 * position (cur_row, cur_col), having made 'move_number' so far.
 * 
 * Return value = maximum coverage found in this branch (number of
 * squares visited).
 *
 * If we detect a full coverage = m*n, we check for open vs closed
 * (did we end on the starting square?). Then we write to the pipe
 * and exit immediately with coverage = m*n.
 *
 * If multiple moves are found, each move is assigned to a child
 * process. We gather their exit statuses, compute the max coverage,
 * and return it to our parent.
 *
 * If zero moves remain but coverage < m*n, we have a dead end.
 *-------------------------------------------------------------------*/
static int knight_tour(
    int cur_row, int cur_col,
    int move_number,
    int start_row, int start_col,
    int m, int n,
    int **board,
    int pipe_fd_write  /* write-end of the pipe for solutions */
)
{
    /* If we've visited all squares, we have a solution. */
    if( move_number == (m * n) )
    {
        /* Check for closed or open:
         *   A "closed" tour means we ended on the same (row,col) that we started.
         */
        char sol_type;
        if( cur_row == start_row && cur_col == start_col ) {
            sol_type = 'C'; /* closed */
        } else {
            sol_type = 'O'; /* open */
        }

#ifndef QUIET
        printf("PID %d: Found a %s knight's tour; notifying top-level parent\n",
               getpid(),
               (sol_type == 'C') ? "closed" : "open");
#endif

        /* Notify top-level parent via pipe. */
        write_solution_to_pipe( pipe_fd_write, sol_type );

        /* Return coverage = m*n as exit status. */
        return (m * n);
    }

    /* Otherwise, gather possible moves. */
    int local_moves[16]; /* up to 8 moves, each having 2 int coords => 16 ints */
    int num_moves = find_valid_moves( cur_row, cur_col, m, n, board, local_moves );

    /* If no moves left but we haven't visited all squares => dead end. */
    if( num_moves == 0 )
    {
#ifndef QUIET
        printf("PID %d: Dead end at move #%d\n", (int)getpid(), move_number);
#endif
        return move_number; /* coverage is 'move_number' squares visited */
    }

    /* If exactly 1 move, continue in THIS process (no new fork). */
    if( num_moves == 1 )
    {
        int nr = *(local_moves + 0);
        int nc = *(local_moves + 1);

        /* Mark visited. */
        *(*(board + nr) + nc) = move_number + 1;

        /* Recurse in the same process. */
        int coverage = knight_tour( nr, nc,
                                    move_number + 1,
                                    start_row, start_col,
                                    m, n, board, pipe_fd_write );

        /* Backtrack before returning. */
        *(*(board + nr) + nc) = 0;
        return coverage;
    }

    /* If multiple moves, we create child processes for each. */
#ifndef QUIET
    printf("PID %d: %d possible moves after move #%d; creating %d child processes...\n",
           (int)getpid(), num_moves, move_number, num_moves );
#endif

    int max_coverage = move_number; /* track best coverage from children */

    /* We store child pids in dynamically-allocated memory so we don't use brackets. */
    pid_t *child_pids = calloc( num_moves, sizeof(pid_t) );
    if( child_pids == NULL ) {
        error_and_exit("calloc child_pids failed");
    }

    /* Fork for each move. */
    for( int i = 0; i < num_moves; i++ )
    {
        /* Extract next move. */
        int nr = *(local_moves + (2*i) + 0);
        int nc = *(local_moves + (2*i) + 1);

        pid_t pid = fork();
        if( pid < 0 ) {
            error_and_exit("fork() failed");
        }

        if( pid == 0 ) {
            /* CHILD process: mark visited in child’s copy of board. */
            *(*(board + nr) + nc) = move_number + 1;

            /* Recurse. */
            int coverage = knight_tour( nr, nc,
                                        move_number + 1,
                                        start_row, start_col,
                                        m, n, board, pipe_fd_write );
            /* Immediately exit with coverage as status. */
            _exit( coverage & 0xFF ); 
            /* The &0xFF is a common trick so we don’t exceed 8 bits for exit code.
               For larger boards, coverage might exceed 255, but we'll keep it
               simple here. You may store coverage differently if needed. */
        }
        else {
            /* PARENT process: record child pid. */
            *(child_pids + i) = pid;

#ifdef NO_PARALLEL
            /* If NO_PARALLEL is defined, we block-wait right away. */
            int status;
            pid_t cpid = waitpid( pid, &status, 0 );
            if( cpid < 0 ) {
                error_and_exit("waitpid failed");
            }
            if( WIFEXITED(status) ) {
                int c = WEXITSTATUS(status);
                if( c > max_coverage ) {
                    max_coverage = c;
                }
            }
#endif
        }
    } /* end for each move */

#ifndef NO_PARALLEL
    /* If NO_PARALLEL is NOT defined, we wait for all children here. */
    for( int i = 0; i < num_moves; i++ )
    {
        int status;
        pid_t cpid = waitpid( *(child_pids + i), &status, 0 );
        if( cpid < 0 ) {
            error_and_exit("waitpid failed");
        }
        if( WIFEXITED(status) ) {
            int c = WEXITSTATUS(status);
            if( c > max_coverage ) {
                max_coverage = c;
            }
        }
    }
#endif

    free(child_pids);
    /* Return parent's best coverage from these branches. */
    return max_coverage;
}

/*-------------------------------------------------------------------*
 * MAIN
 *-------------------------------------------------------------------*/
int main( int argc, char **argv )
{
    if( argc != 5 ) {
        fprintf(stderr, "ERROR: Invalid argument(s)\n");
        fprintf(stderr, "USAGE: ./hw2.out <m> <n> <r> <c>\n");
        return EXIT_FAILURE;
    }

    /* Parse command-line arguments. Use strtol instead of brackets. */
    char *endp;
    long m = strtol( *(argv + 1), &endp, 10 );
    long n = strtol( *(argv + 2), &endp, 10 );
    long r = strtol( *(argv + 3), &endp, 10 );
    long c = strtol( *(argv + 4), &endp, 10 );

    /* Validate. */
    validate_args_or_die( m, n, r, c );

    /* Create pipe for child -> top-level parent communication. */
    int pipefd[2];
    if( pipe(pipefd) < 0 ) {
        error_and_exit("pipe() failed");
    }

    /* We'll only write from child processes, so the top-level parent
       should read from pipefd[0], and pass pipefd[1] downward. */
    int read_end  = *(pipefd + 0);
    int write_end = *(pipefd + 1);

    /* Dynamically allocate the board with calloc. */
    int **board = calloc( m, sizeof(int*) );
    if( board == NULL ) {
        error_and_exit("calloc for board failed");
    }

    for( int i = 0; i < m; i++ )
    {
        *(board + i) = calloc( n, sizeof(int) );
        if( *(board + i) == NULL ) {
            error_and_exit("calloc for board row failed");
        }
    }

    /* Mark the starting square as visited: move #1. */
    *(*(board + r) + c) = 1;

#ifndef QUIET
    printf("PID %d: Solving the knight's tour problem for a %ldx%ld board\n",
           (int)getpid(), m, n );
    printf("PID %d: Starting at row %ld and column %ld (move #1)\n",
           (int)getpid(), r, c );
#endif

    /* Now call our solver routine. */
    pid_t child = fork();
    if( child < 0 ) {
        error_and_exit("fork() failed");
    }
    if( child == 0 ) {
        /* This is the "process tree root" child that does the real search.
           Why a child? So that the top-level parent remains around to read
           from the pipe even if the search tries to do additional forks, etc.
           (Alternatively, you can do it in the same process, but be mindful
           of how you close pipe ends.) */

        /* We do not read from the pipe in children. Close read end. */
        if( close( read_end ) < 0 ) {
            perror("close read_end in child failed");
        }

        /* Launch the recursive solver. */
        int coverage = knight_tour(
            (int)r, (int)c,
            1,
            (int)r, (int)c,
            (int)m, (int)n,
            board,
            write_end
        );

        /* Exit code is coverage & 0xFF to stay within 8 bits. */
        _exit( coverage & 0xFF );
    }
    else {
        /* Top-level parent process. */
        /* We do not write to the pipe from here, so close write end. */
        if( close( write_end ) < 0 ) {
            perror("close write_end in parent failed");
        }

        /* Wait for the "root" child that leads the search. */
        int status;
        pid_t w = waitpid(child, &status, 0);
        if( w < 0 ) {
            error_and_exit("waitpid failed in parent");
        }

        int best_coverage = 0;
        if( WIFEXITED(status) ) {
            best_coverage = WEXITSTATUS(status);
        }

        /* Now read from the pipe to count how many open and closed tours. */
        int open_count = 0;
        int closed_count = 0;

        {
            char buffer;
            /* Repeatedly read 1 byte until EOF. */
            while( 1 ) {
                ssize_t rc = read( read_end, &buffer, 1 );
                if( rc == 0 ) {
                    /* EOF reached: no more child writes. */
                    break;
                }
                else if( rc < 0 ) {
                    /* If interrupted or error, break or handle. */
                    if( errno == EINTR ) continue; 
                    perror("read from pipe failed");
                    break;
                }
                else {
                    /* rc == 1: we got a solution type. */
                    if( buffer == 'O' ) open_count++;
                    else if( buffer == 'C' ) closed_count++;
                }
            }
        }

        /* All child processes done, we can close the read end. */
        if( close( read_end ) < 0 ) {
            perror("close read_end after reading failed");
        }

        /* Final output. If at least one tour was found, print the open/closed counts.
           Otherwise, print the best coverage. */
        if( (open_count + closed_count) > 0 ) {
#ifndef QUIET
            /* In QUIET mode, we print only the final line (with the same format). */
#endif
            printf("PID %d: Search complete; found %d open tours and %d closed tours\n",
                   (int)getpid(), open_count, closed_count );
        }
        else {
            /* No full tours found; just print best coverage. */
#ifndef QUIET
            printf("PID %d: Search complete; best solution(s) visited %d squares out of %ld\n",
                   (int)getpid(), best_coverage, m*n );
#else
            /* Even in QUIET mode, we do show the final line. */
            printf("PID %d: Search complete; best solution(s) visited %d squares out of %ld\n",
                   (int)getpid(), best_coverage, m*n );
#endif
        }
    }

    /* Free all memory (the parent can do so). */
    for( int i = 0; i < m; i++ ) {
        free( *(board + i) );
    }
    free( board );

    return EXIT_SUCCESS;
}
