#include "system.h"

#include "test.h"
#include "mongo.h"

#include "debug.h"

static const char *db = "test";

int main(int argc, char *argv[])
{
    const char * test_server = (argc > 1 ? argv[1] : TEST_SERVER);

    mongo conn[1];

    INIT_SOCKETS_FOR_WINDOWS;
    CONN_CLIENT_TEST;

    mongo_cmd_drop_db( conn, db );

    ASSERT( mongo_cmd_authenticate( conn, db, "user", "password" ) == MONGO_ERROR );
    mongo_cmd_add_user( conn, db, "user", "password" );
    ASSERT( mongo_cmd_authenticate( conn, db, "user", "password" ) == MONGO_OK );

    mongo_cmd_drop_db( conn, db );
    mongo_destroy( conn );
    return 0;
}
