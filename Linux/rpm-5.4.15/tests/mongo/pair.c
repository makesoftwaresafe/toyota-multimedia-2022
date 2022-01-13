#include "system.h"

#include "test.h"
#include "mongo.h"

#include "debug.h"

static mongo_connection conn[1];
static mongo_connection_options left;
static mongo_connection_options right;

int main(int argc, char *argv[])
{
    const char * test_server = (argc > 1 ? argv[1] : TEST_SERVER);

    strncpy(left.host, test_server, 255);

    left.host[254] = '\0';
    left.port = 27017;

    strncpy(right.host, "0.0.0.0", 255);
    right.port = 12345;

    ASSERT(mongo_connect_pair(conn, &left, &right) == mongo_conn_success);
    ASSERT(conn->left_opts->port == 27017);
    ASSERT(conn->right_opts->port == 12345);
    ASSERT(mongo_cmd_ismaster(conn, NULL));

    mongo_destroy(conn);

    ASSERT(mongo_connect_pair(conn, &right, &left) == mongo_conn_success);
    ASSERT(conn->left_opts->port == 27017); /* should have swapped left and right */
    ASSERT(conn->right_opts->port == 12345);
    ASSERT(mongo_cmd_ismaster(conn, NULL));

    ASSERT(mongo_reconnect(conn) == mongo_conn_success);
    ASSERT(conn->left_opts->port == 27017); /* should have swapped left and right */
    ASSERT(conn->right_opts->port == 12345);
    ASSERT(mongo_cmd_ismaster(conn, NULL));

    mongo_destroy(conn);
    return 0;
}
