/*
 * luuid.c -- minimum OSSP uuid based UUID extension for RPM Lua
 * Copyright (C) 2008 Ralf S. Engelschall <rse@engelschall.com>
 * Licensed under LGPL.
 */

/* NOTICE: This is NOT a general Lua binding for OSSP uuid!
   Instead it provides just the minimum functionality to generate a
   UUID (output in string representation) and to retrieve a textual
   description of a UUID (input in string representation). */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lua.h"
#include "lauxlib.h"
#include "luuid.h"

#ifdef LUA_USE_UUID

#include "uuid.h"

/*  Lua method: <uuid> = uuid.generate(<version>[, <uuid-namespace>, <data>])  */
static int luuid_generate(lua_State *L)
{
    uuid_t *uuid;
    uuid_t *uuid_ns = NULL;
    const char *uuid_ns_str;
    const char *data = NULL;
    uuid_rc_t rc;
    int version;
    char result_buf[UUID_LEN_STR+1];
    char *result_ptr;
    size_t result_len;

    /*  fetch arguments  */
    if (lua_isnumber(L, 1))
        version = lua_tointeger(L, 1);
    else
        return luaL_argerror(L, 1, "UUID version number");
    if (!(version == 1 || (version >= 3 && version <= 5)))
        return luaL_argerror(L, 1, "UUID version number");
    if (version == 3 || version == 5) {
        if (lua_isstring(L, 2))
            uuid_ns_str = lua_tostring(L, 2);
        else
            return luaL_argerror(L, 2, "namespace UUID");
        if (lua_isstring(L, 3))
            data = lua_tostring(L, 3);
        else
            return luaL_argerror(L, 3, "plaintext data");
        if ((rc = uuid_create(&uuid_ns)) != UUID_RC_OK)
            return luaL_error(L, "failed to create UUID NS object: %s", uuid_error(rc));
        if ((rc = uuid_load(uuid_ns, uuid_ns_str)) != UUID_RC_OK)
            if ((rc = uuid_import(uuid_ns, UUID_FMT_STR, uuid_ns_str, strlen(uuid_ns_str))) != UUID_RC_OK)
                return luaL_error(L, "failed to import UUID NS object: %s", uuid_error(rc));
    }

    /*  create UUID object  */
    if ((rc = uuid_create(&uuid)) != UUID_RC_OK)
        return luaL_error(L, "failed to create UUID object: %s", uuid_error(rc));

    /*  generate UUID  */
    if (version == 1)
        rc = uuid_make(uuid, UUID_MAKE_V1);
    else if (version == 3)
        rc = uuid_make(uuid, UUID_MAKE_V3, uuid_ns, data);
    else if (version == 4)
        rc = uuid_make(uuid, UUID_MAKE_V4);
    else if (version == 5)
        rc = uuid_make(uuid, UUID_MAKE_V5, uuid_ns, data);
    if (rc != UUID_RC_OK) {
        uuid_destroy(uuid);
        if (uuid_ns != NULL)
            uuid_destroy(uuid_ns);
        return luaL_error(L, "failed to make UUID object: %s", uuid_error(rc));
    }

    /*  export UUID in string representation  */
    result_ptr = result_buf;
    result_len = sizeof(result_buf);
    if ((rc = uuid_export(uuid, UUID_FMT_STR, &result_ptr, &result_len)) != UUID_RC_OK) {
        uuid_destroy(uuid);
        if (uuid_ns != NULL)
            uuid_destroy(uuid_ns);
        return luaL_error(L, "failed to export UUID object: %s", uuid_error(rc));
    }

    /*  destroy UUID object(s)  */
    uuid_destroy(uuid);
    if (uuid_ns != NULL)
        uuid_destroy(uuid_ns);

    /*  provide result  */
    lua_pushstring(L, result_buf);
    return 1;
}

/*  Lua method: <description> = uuid.describe(<uuid>)  */
static int luuid_describe(lua_State *L)
{
    uuid_t *uuid;
    const char *uuid_str;
    uuid_rc_t rc;
    char *result_ptr;

    /*  fetch arguments  */
    if (lua_isstring(L, 1))
        uuid_str = lua_tostring(L, 1);
    else
        return luaL_argerror(L, 1, "UUID string representation");

    /*  create UUID object  */
    if ((rc = uuid_create(&uuid)) != UUID_RC_OK)
        return luaL_error(L, "failed to create UUID object: %s", uuid_error(rc));

    /*  import UUID  */
    if ((rc = uuid_import(uuid, UUID_FMT_STR, uuid_str, strlen(uuid_str))) != UUID_RC_OK)
        return luaL_error(L, "failed to import UUID object: %s", uuid_error(rc));

    /*  export UUID in text description  */
    result_ptr = NULL;
    if ((rc = uuid_export(uuid, UUID_FMT_TXT, &result_ptr, NULL)) != UUID_RC_OK) {
        uuid_destroy(uuid);
        return luaL_error(L, "failed to export UUID object: %s", uuid_error(rc));
    }

    /*  destroy UUID object(s)  */
    uuid_destroy(uuid);

    /*  provide result  */
    lua_pushstring(L, result_ptr);
    return 1;
}

/*  Lua uuid.* method dispatch table  */
static const luaL_Reg uuid_meta[] = {
    { "generate",  luuid_generate },
    { "describe",  luuid_describe },
    { NULL,        NULL }
};

#endif

/*  Lua extension initialization  */
LUALIB_API int luaopen_uuid(lua_State *L)
{
#ifdef LUA_USE_UUID
    luaL_newlib(L, uuid_meta);
#endif
    return 1;
}

