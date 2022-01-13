#include "system.h"

#include "test.h"
#include "mongo.h"

#include "debug.h"

static int increment( void ) {
    static int i = 1000;
    i++;
    return i;
}

static int fuzz( void ) {
    return 50000;
}

/* Test custom increment and fuzz functions. */
int main(int argc, char *argv[])
{
    const char * test_server = (argc > 1 ? argv[1] : TEST_SERVER);

    bson_oid_t o;
    int res;

    bson_set_oid_inc( increment );
    bson_set_oid_fuzz( fuzz );

    bson_oid_gen( &o );
    bson_big_endian32( &res, &( o.ints[2] ) );

    ASSERT( o.ints[1] == 50000 );
    ASSERT( res == 1001 );

    return 0;
}
