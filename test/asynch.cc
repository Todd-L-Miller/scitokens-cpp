#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/select.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include "../src/scitokens.h"

void
usage( const char * self ) {
    fprintf( stderr, "usage: %s encoded-scitoken\n", self );
}

void
print_claim( SciToken & token, const char * claim ) {
    char * value;
    char * error;
    int rv = scitoken_get_claim_string(
        token, claim, & value, & error
    );
    if( rv != 0 ) {
        fprintf( stderr, "scitoken_get_claim_string('%s') failed: %s\n", claim, error );
        // exit( -2 );
        return;
    }
    fprintf( stdout, "%s = %s\n", claim, value );
}


int
main( int argc, char ** argv) {
    if( argc < 2 ) { usage(argv[0]); exit(-1); }
    const char * encoded = argv[1];

    int rv;
    char * error;
    SciToken token;

/*
    // Synchronous.
    rv = scitoken_deserialize( encoded, & token, NULL, & error );
    if( rv != 0 ) {
        fprintf( stderr, "scitoken_deserialize() failed: %s\n", error );
        exit( -2 );
    }
    // scitoken_destroy( token );
*/

    // The asynchronous API doesn't work like the synchronous API, and
    // requires that deserialization profile be set before it starts
    // working.  This is probably a bug, but there's another bug where
    // the default value for the profile causes a throw.  *sigh*

    // Asynchronous API.
    SciTokenStatus status;
    rv = scitoken_deserialize_start(
        encoded, & token, NULL, & status, & error
    );
    if( rv != 0 ) {
        fprintf( stderr, "scitoken_deserialize_start() failed: %s\n", error );
        exit( -2 );
    }
    if( status == NULL ) {
        fprintf( stderr, "scitoken_deserialize_start() returned a token\n" );
        exit( 1 );
    }


    do {
        fd_set * read_fds = NULL;
        rv = scitoken_status_get_read_fd_set( & status, & read_fds, & error );
        if( rv != 0 ) {
            fprintf( stderr, "scitoken_status_get_read_fd_set() failed: %s\n", error );
            exit( -2 );
        }

        fd_set * write_fds = NULL;
        rv = scitoken_status_get_write_fd_set( & status, & write_fds, & error );
        if( rv != 0 ) {
            fprintf( stderr, "scitoken_status_get_write_fd_set() failed: %s\n", error );
            exit( -2 );
        }

        fd_set * except_fds = NULL;
        rv = scitoken_status_get_exc_fd_set( & status, & except_fds, & error );
        if( rv != 0 ) {
            fprintf( stderr, "scitoken_status_get_exc_fd_set() failed: %s\n", error );
            exit( -2 );
        }

        int max_fds;
        rv = scitoken_status_get_max_fd( & status, & max_fds, & error );
        if( rv != 0 ) {
            fprintf( stderr, "scitoken_status_get_max_fds() failed: %s\n", error );
            exit( -2 );
        }

        struct timeval time_out{1, 0};
        int s = select( max_fds + 1, read_fds, write_fds, except_fds, & time_out );
        if( s == -1 ) {
            fprintf( stderr, "select() failed: %s (%d)\n", strerror(errno), errno );
            exit( -4 );
        } else if( s == 0 ) {
            fprintf( stderr, "select() timed out, checking for progress.\n" );
        }

        fprintf( stderr, "Calling scitoken_deserialize_continue()...\n" );
        rv = scitoken_deserialize_continue( & token, & status, & error );
        if( rv != 0 ) {
            fprintf( stderr, "scitoken_deserialize_continue() failed: %s\n", error );
            exit( -3 );
        }
    } while( status != NULL );


    print_claim(token, "ver");
    print_claim(token, "aud");
    print_claim(token, "iss");
    // Not a string.
    // print_claim(token, "exp");
    // Not a string.
    // print_claim(token, "iat");
    // Not a string.
    // print_claim(token, "nbf");
    print_claim(token, "jti");

    scitoken_destroy( token );
    return 0;
}
