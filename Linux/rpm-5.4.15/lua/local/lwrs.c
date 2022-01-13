/*
** $Id: lwrs.c,v 1.5 2008/08/20 07:08:28 rse Exp $
** WRLinux utilities
*/


#include <stdlib.h>
#include <string.h>

#define lwrs_c

#include "lua.h"

#include "lualib.h"
#include "lauxlib.h"
#include "lwrs.h"

extern int useradd_main (int argc, char **argv);
extern int chkconfig_main (int argc, char **argv);

#if defined(USE_LUA_SHADOW) || defined(USE_LUA_CHKCONFIG)
static int lcl_makeArgv(char *in, char **argv, int max)
 {
 int i;
 char delim = ' ';
 char *start = NULL;
 char c;

 for ( i=0; i < max; ++i)
  {
   while (*in == ' ')  ++in;        /* skip spaces */
   if (!*in)  return i;             /* ignore traing spaces */
   if (*in == '"')  delim = *in++;  /* begin a quoted string */
   start = in;                      /* beginning of token */
   for ( c = *in; c != delim && c; c = *in)  ++in;  /* find end of token */
   *in++ = '\0';         /* mark end */
   *(argv+i) = start;    /* might be 0 length! */
   if (!c)  return i+1;  /* end of input string */
   delim = ' ';
  }
 return -1;
 }
#endif

static int wrs_version (lua_State *L)
	/*@globals internalState @*/
	/*@modifies L, internalState @*/
{
  printf("version 8\n");
  return 0;
}

#ifdef USE_LUA_SHADOW
static int wrs_groupadd (lua_State *L)
	/*@globals internalState @*/
	/*@modifies L, internalState @*/
{
  int argc;
  char *argv[32];  /* yeah, well, this is big enough */
  char *cmdline;   /* copy string from lua just to be safe */

  argc = lua_gettop(L);  /* number of arguments */
  if (argc != 1)  return luaL_error(L, "One argument is required!");

  cmdline = strdup(luaL_checklstring(L, 1, NULL));   /* we don't need the size */

  argc = 1 + lcl_makeArgv(cmdline, argv+1, 31);

  argv[0] = "rpm_lua";   /* make up a program name */

  useradd_main(argc, argv);

  free(cmdline);
  return 0;
}

static int wrs_useradd (lua_State *L)
	/*@globals internalState @*/
	/*@modifies L, internalState @*/
{
  int argc;
  char *argv[32];  /* yeah, well, this is big enough */
  char *cmdline;   /* copy string from lua just to be safe */

  argc = lua_gettop(L);  /* number of arguments */
  if (argc != 1)  return luaL_error(L, "One argument is required!");

  cmdline = strdup(luaL_checklstring(L, 1, NULL));   /* we don't need the size */

  argc = 1 + lcl_makeArgv(cmdline, argv+1, 31);

  argv[0] = "rpm_lua";   /* make up a program name */

  useradd_main(argc, argv);

  free(cmdline);
  return 0;
}
#endif /* USE_LUA_SHADOW */

#ifdef USE_LUA_CHKCONFIG
static int wrs_chkconfig (lua_State *L)
	/*@globals internalState @*/
	/*@modifies L, internalState @*/
{
  int argc;
  char *argv[32];  /* yeah, well, this is big enough */
  char *cmdline;   /* copy string from lua just to be safe */

  argc = lua_gettop(L);  /* number of arguments */
  if (argc != 1)  return luaL_error(L, "One argument is required!");

  cmdline = strdup(luaL_checklstring(L, 1, NULL));   /* we don't need the size */

  argc = 1 + lcl_makeArgv(cmdline, argv+1, 31);

  argv[0] = "rpm_lua";   /* make up a program name */

  chkconfig_main(argc, argv);

  free(cmdline);
  return 0;
}
#endif /* USE_LUA_CHKCONFIG */

/*@-readonlytrans@*/
/*@unchecked@*/
static const luaL_reg wrs[] = {
  {"version",   wrs_version},
#ifdef USE_LUA_SHADOW
  {"groupadd",  wrs_groupadd},
  {"useradd",  wrs_useradd},
#endif
#ifdef USE_LUA_CHKCONFIG
  {"chkconfig",  wrs_chkconfig},
#endif
  {NULL, NULL}
};
/*@=readonlytrans@*/


/*
** Open wrs library
*/
LUALIB_API int luaopen_wrs (lua_State *L) {
  luaL_openlib(L, "wrs", wrs, 0);
  return 1;
}
