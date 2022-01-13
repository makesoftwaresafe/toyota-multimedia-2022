#include "system.h"

#include "test.h"
#include "mongo.h"

#include "debug.h"

int main(int argc, char *argv[])
{
    const char * test_server = (argc > 1 ? argv[1] : TEST_SERVER);
    
    mongo conn[1];
    bson b;
    const char *sock_path = "/tmp/mongodb-27017.sock";
    const char *ns = "test.c.unix_socket";
    const char *col = "c.unix_socket";

    ASSERT( mongo_client( conn, sock_path, -1 ) == MONGO_OK );

    mongo_cmd_drop_collection( conn, "test", col, NULL );

    bson_init( &b );
    bson_append_new_oid( &b, "_id" );
    bson_append_string( &b, "foo", "bar" );
    bson_append_int( &b, "x", 1);
    bson_finish( &b );

    ASSERT( mongo_insert( conn, ns, &b, NULL ) == MONGO_OK );

    mongo_cmd_drop_collection( conn, "test", col, NULL );

    bson_destroy( &b );
    mongo_destroy( conn );

    return 0;
}
