/*
** $Id: lshadow.c,v 1.6 2008/08/20 07:08:28 rse Exp $
** Shadow utilities
*/


#include <stdlib.h>
#include <math.h>
#include <string.h>

#define lshadow_c

#include "lua.h"

#include "lauxlib.h"
#include "lshadow.h"

extern int useradd_main (int argc, char **argv);
extern int chkconfig_main (int argc, char **argv);

#if defined(USE_LUA_SHADOW) || defined(USE_LUA_CHKCONFIG)
static int lcl_makeArgv(char *in, char **argv, int max)
 {
 int i;
 char delim = ' ';
 char *start = NULL;
 char c;

 printf("Parsing [%s]\n", in);

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

static int shadow_version (lua_State *L)
	/*@globals internalState @*/
	/*@modifies L, internalState @*/
{
  printf("version 6\n");
  return 0;
}

#ifdef USE_LUA_SHADOW
static int shadow_groupadd (lua_State *L)
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

static int shadow_useradd (lua_State *L)
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
static int shadow_chkconfig (lua_State *L)
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
static const luaL_reg shadow[] = {
  {"version",   shadow_version},
#ifdef USE_LUA_SHADOW
  {"groupadd",  shadow_groupadd},
  {"useradd",  shadow_useradd},
#endif
#ifdef USE_LUA_CHKCONFIG
  {"chkconfig",  shadow_chkconfig},
#endif
  {NULL, NULL}
};
/*@=readonlytrans@*/


/*
** Open shadow library
*/
LUALIB_API int luaopen_shadow (lua_State *L) {
  luaL_openlib(L, "shadow", shadow, 0);
  return 1;
}

