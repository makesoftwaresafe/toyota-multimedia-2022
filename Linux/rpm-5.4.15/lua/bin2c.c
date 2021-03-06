/*
* bin2c.c
* convert files to byte arrays for automatic loading
* Luiz Henrique de Figueiredo (lhf@tecgraf.puc-rio.br)
* Fixed to Lua 5.1. Antonio Scuri (scuri@tecgraf.puc-rio.br)
* Generated files will work also for Lua 5.0
* 08 Dec 2005
*/

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

static void dump(FILE* f, int n)
{
  printf("static const unsigned char B%d[]={\n",n);
  for (n=1;;n++)
  {
    int c=getc(f); 
    if (c==EOF) break;
    printf("%3u,",c);
    if (n==20) { putchar('\n'); n=0; }
  }
  printf("\n};\n\n");
}

static void fdump(const char* fn, int n)
{
  FILE* f= fopen(fn,"rb");		/* must open in binary mode */
  if (f==NULL)
  {
    fprintf(stderr,"bin2c: cannot open ");
    perror(fn);
    exit(1);
  }
  else
  {
    printf("/* %s */\n",fn);
    dump(f,n);
    fclose(f);
  }
}

static void emit(const char* fn, int n)
{
  static int oneshot = 0;
  if (!oneshot) {
    printf(" int error;\n");
    oneshot++;
  }
  printf(" error = luaL_loadbuffer(L,(const char*)B%d,sizeof(B%d),\"%s\") || lua_pcall(L, 0, 0, 0);\n", n, n, fn);
  printf(" if (error) { fprintf(stderr, \"error: Lua script failed: %%s\", lua_tostring(L, -1)); lua_pop(L, 1); }\n");
}

int main(int argc, char* argv[])
{
  printf("/* code automatically generated by bin2c -- DO NOT EDIT */\n");
  printf("{\n");
  if (argc<2)
  {
    dump(stdin,0);
    emit("=stdin",0);
  }
  else
  {
    int i;
    printf("/* #include'ing this file in a C program is equivalent to calling\n");
    for (i=1; i<argc; i++) printf("  if (luaL_loadfile(L,\"%s\")==0) lua_pcall(L, 0, 0, 0); \n",argv[i]);
    printf("*/\n");
    for (i=1; i<argc; i++) fdump(argv[i],i);
    for (i=1; i<argc; i++) emit(argv[i],i);
  }
  printf("}\n");
  return 0;
}
