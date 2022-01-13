/** \ingroup lib
 * \file lib/rpmluaext.c
 */

#if defined(RPM_VENDOR_OPENPKG) || defined(RPM_LUA_EXTENSIONS_BASED_ON_RPMLIB) /* rpm-lua-extensions-based-on-rpm-lib-functionality */

#include "system.h"

#ifdef  WITH_LUA

#define _MIRE_INTERNAL
#include <rpmio_internal.h>
#include <rpmiotypes.h>
#include <rpmlog.h>
#include <rpmmacro.h>
#include <argv.h>

#include <rpmtag.h>
#include <rpmtypes.h>

#include <rpmds.h>

#define _RPMLUA_INTERNAL
#include <rpmlua.h>
#include <rpmluaext.h>

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include <rpmcli.h>

#include "debug.h"

/* RPM Lua function:
 * <result> = rpm.vercmp(
 *     <version1>  -- first  version (e.g. "N.N.N")
 *     <version2>  -- second version (e.g. "N.N.N")
 * )
 */
static int rpmluaext_vercmp(lua_State *L)
{
    const char *version1;
    const char *version2;
    int rc;

    /* fetch arguments */
    if (lua_isstring(L, 1))
        version1 = lua_tostring(L, 1);
    else {
        (void)luaL_argerror(L, 1, "first version string");
        return 0;
    }
    if (lua_isstring(L, 2))
        version2 = lua_tostring(L, 2);
    else {
        (void)luaL_argerror(L, 2, "second version string");
        return 0;
    }

    /* compare versions */
    rc = rpmvercmp(version1, version2);

    /* provide results */
    lua_pushinteger(L, rc);
    return 1;
}

/* RPM Lua function:
 * <digest> = rpm.digest(
 *     <type>,     -- digest algorithm type ("md5", "sha1", etc)
 *     <data-file> -- file to calculate digest for
 * )
 */
static int rpmluaext_digest(lua_State *L)
{
    const char *filename;
    pgpHashAlgo algo;
    FD_t fd;
    const char *digest = NULL;
    size_t digest_len = 0;
    DIGEST_CTX ctx;
    char buf[8 * BUFSIZ];
    size_t nb;

    /* fetch arguments */
    if (lua_isstring(L, 1)) {
        if ((algo = pgpHashAlgoStringToNumber(lua_tostring(L, 1), 0)) == PGPHASHALGO_ERROR) {
            (void)luaL_argerror(L, 1, "digest type");
            return 0;
        }
    }
    else {
        (void)luaL_argerror(L, 1, "digest type");
        return 0;
    }
    if (lua_isstring(L, 2))
        filename = lua_tostring(L, 2);
    else {
        (void)luaL_argerror(L, 2, "data file");
        return 0;
    }

    /* open file */
    fd = Fopen(filename, "r.fdio");
    if (fd == NULL || Ferror(fd)) {
        luaL_error(L, "failed to create transaction");
        return 0;
    }

    /* read file and calculate digest */
    ctx = rpmDigestInit(algo, RPMDIGEST_NONE);
    while ((nb = Fread(buf, sizeof(buf[0]), sizeof(buf), fd)) > 0)
        rpmDigestUpdate(ctx, buf, nb);
    rpmDigestFinal(ctx, &digest, &digest_len, 1);
    if (digest == NULL || digest[0] == '\0') {
        luaL_error(L, "failed to calculate digest");
        return 0;
    }

    /* close file */
    (void)Fclose(fd);

    /* provide results */
    lua_pushstring(L, digest);
    return 1;
}

/* RPM Lua function:
 * <ok> = rpm.signature(
 *     <data-file>,                -- file to check signature on
 *     <detached-signature-file>,  -- file to contain detached signature (or "nil" for clearsigning)
 *     <public-key-file>,          -- file containing the signing public key (or "nil" to consult RPMDB)
 *     <public-key-fingerprint>    -- fingerprint of signing public key (or "nil" to accept any signing key)
 * )
 */
static int rpmluaext_signature(lua_State *L)
{
    rpmts ts;
    rpmRC rc;
    const char *fn_data;
    const char *fn_sig;
    const char *fn_pkey;
    const char *fp;

    /* fetch arguments */
    if (lua_isstring(L, 1))
        fn_data = lua_tostring(L, 1);
    else {
        (void)luaL_argerror(L, 1, "filename of data file");
        return 0;
    }
    if (lua_isstring(L, 2))
        fn_sig = lua_tostring(L, 2);
    else
        fn_sig = NULL;
    if (lua_isstring(L, 3))
        fn_pkey = lua_tostring(L, 3);
    else
        fn_pkey = NULL;
    if (lua_isstring(L, 4))
        fp = lua_tostring(L, 4);
    else
        fp = NULL;

    /* create RPM transaction and open RPM database */
    if ((ts = rpmtsCreate()) == NULL) {
        luaL_error(L, "failed to create transaction");
        return 0;
    }
    rpmtsOpenDB(ts, O_RDONLY);

    /* check signature on integrity specification file */
    rc = rpmnsProbeSignature(ts, fn_data, fn_sig, fn_pkey, fp, 0);

    /* destroy transaction */
    (void)rpmtsFree(ts); 
    ts = NULL;

    /* provide results */
    lua_pushboolean(L, rc == RPMRC_OK);
    return 1;
}

/* callback for converting rpmlog() output into Lua result table */
static int rpmluaext_query_cb(rpmlogRec rec, rpmlogCallbackData data)
{
    lua_State *L = (lua_State *)data;
    size_t n;

    if (rpmlogRecPriority(rec) & RPMLOG_NOTICE) {
        const char *msg = rpmlogRecMessage(rec);
        if (msg != NULL) {
            luaL_checktype(L, -1, LUA_TTABLE);
            n = lua_objlen(L, -1);
            lua_pushinteger(L, n + 1);
            lua_pushstring(L, msg);
            lua_settable(L, -3);
        }
    }
    return 0;
}

/* RPM Lua function:
 * <outputs> = rpm.query(
 *     <query-format>,   -- query format   (corresponds to CLI "rpm -q --qf <query-format>")
 *     <is-all-query>    -- query all flag (corresponds to CLI "rpm -q -a")
 *     <query-filter>    -- query filter   (corresponds to CLI "rpm -q ... <query-filter>")
 * )
 */
static int rpmluaext_query(lua_State *L)
{
    rpmts ts;
    QVA_t qva;
    const char *argv[2];
    rpmlogCallback rpmlog_cb;
    rpmlogCallbackData rpmlog_cb_data;
    int ec;

    /* configure RPMDB query */
    if ((qva = (QVA_t)malloc(sizeof(*qva))) == NULL) {
        luaL_error(L, "failed to allocate query configuration");
        return 0;
    }
    memset(qva, '\0', sizeof(*qva));
    if (!lua_isstring(L, 1)) {
        (void)luaL_argerror(L, 1, "query format");
        return 0;
    }
    qva->qva_queryFormat = lua_tostring(L, 1);
    qva->qva_mode = 'q';
    qva->qva_char = ' ';
    if (!lua_isboolean(L, 2)) {
        (void)luaL_argerror(L, 1, "query all flag");
        return 0;
    }
    if (lua_toboolean(L, 2)) {
        qva->qva_source |= RPMQV_ALL;
        qva->qva_sourceCount++;
    }
    argv[0] = NULL;
    if (lua_isstring(L, 3)) {
        argv[0] = lua_tostring(L, 3);
        argv[1] = NULL;
    }

    /* create RPM transaction and open RPM database */
    if ((ts = rpmtsCreate()) == NULL) {
        luaL_error(L, "failed to create transaction");
        return 0;
    }
    rpmtsOpenDB(ts, O_RDONLY);

    /* intercept output channel */
    rpmlogGetCallback(&rpmlog_cb, &rpmlog_cb_data);
    rpmlogSetCallback(rpmluaext_query_cb, L);

    /* create result Lua table on Lua stack */
    lua_newtable(L);

    /* perform query */
    ec = rpmcliQuery(ts, qva, argv);

    /* restore output channel */
    rpmlogSetCallback(rpmlog_cb, rpmlog_cb_data);

    /* destroy transaction */
    (void)rpmtsFree(ts); 
    ts = NULL;

    return 1;
}

/* RPM Lua "rpm.*" function registry */
static const luaL_reg rpmluaext_registry[] = {
    { "vercmp",    rpmluaext_vercmp    },
    { "digest",    rpmluaext_digest    },
    { "signature", rpmluaext_signature },
    { "query",     rpmluaext_query     },
    { NULL,        NULL                }
};

/* activate our RPM Lua extensions */
void rpmluaextActivate(rpmlua lua)
{
    lua_pushvalue(lua->L, LUA_GLOBALSINDEX);
    luaL_openlib(lua->L, "rpm", rpmluaext_registry, 0);
    return;
}

#endif /* WITH_LUA */

#endif /* RPM_VENDOR_OPENPKG */

