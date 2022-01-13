/* this file should be removed when mongo_connect changes from deprecated to eliminated */

#include "system.h"

#include "test.h"
#include "mongo.h"

#include "debug.h"

int main(int argc, char *argv[])
{
    const char * test_server = (argc > 1 ? argv[1] : TEST_SERVER);
    mongo conn[1];
    char version[10];

    INIT_SOCKETS_FOR_WINDOWS;

    if( mongo_connect( conn, TEST_SERVER, 27017 ) != MONGO_OK ) {
        printf( "failed to connect\n" );
        exit( 1 );
    }

    /* mongo_connect should print a warning to stderr that it is deprecated */

    ASSERT( conn->write_concern == (void*)0 ); /* write_concern should be 0 for backwards compatibility */

    mongo_destroy( conn );
    return 0;
}
