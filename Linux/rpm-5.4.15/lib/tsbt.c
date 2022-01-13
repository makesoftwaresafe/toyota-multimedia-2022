#include <system.h>
#include <rpmio.h>
#include <rpmcli.h>
#include <argv.h>
#include <popt.h>
#include <debug.h>

static int an[256];
static int bn[256];
static int nn[256];
static int rn[256];

/* Activate nodes, count incoming edgees. */
static int count(const char * s)
{
    int ax = (int)s[0];
    int bx;
    int i;

    if (bn[ax] == 0)
	bn[ax] = 1;
    for (i = 1; (bx = (int)s[i]) != 0; i++) {
	/* Prevent dependencies on self. */
	if (bx == ax)
	    continue;
	if (bn[bx] == 0)
	    bn[bx] = 1;
	/* Count an incoming edge. */
	nn[ax]++;
    }
    return 0;
}

/* Compute paths to tree roots. */
static int parent(const char * s)
{
    int ax = (int)s[0];
    int bx;
    int i;

    for (i = 1; (bx = (int)s[i]) != 0; i++) {
	/* Prevent dependencies on self. */
	if (bx == ax)
	    continue;
	/* Save first parent. */
	if (rn[bx] == 0) {
	    rn[bx] = ax;
	    continue;
	}
	/* Multiple parents: prefer parent with fewest edges. */
	if (nn[bx] < nn[rn[bx]])
	    rn[bx] = ax;
    }
    return 0;
}

/* Find node with (a,b) label. */
static int SBTfind(int a, int b)
{
    int i;
    for (i = 0; i < 256; i++) {
	if (bn[i] == 0)
	    continue;
	if (a == an[i] && b == bn[i])
	    return i;
    }
    return 0;
}

/* Mark chains with S-B labels. */
static int SBTmark(int i)
{
    if (i >= 1) {
	int px = rn[i];
	int rx, lx;

	if (px) {
	    /* Mark all parent nodes first. */
	    rn[i] = 0;		/* Prevent loops. */
	    SBTmark(px);
	    rn[i] = px;

	    /* Does an Lnode already exist? */
	    rx = lx = SBTfind(an[px], an[px] + bn[px]);
	    while (rx > 0) {
		lx = rx;
		rx = SBTfind(an[lx] + bn[lx], bn[lx]);
	    }
	    if (lx < 1) {
		/* Add Lnode from parent */
		an[i] = an[px];
		bn[i] = an[px] + bn[px];
	    } else {
		/* Append Rnode to Lnode's end-of-chain. */
		an[i] = an[lx] + bn[lx];
		bn[i] = bn[lx];
	    }
	}
    }
    return 0;
}

static int SBTprnode(int i)
{
    int a = an[i];
    int b = bn[i];

    if (b == 1) {
	printf("Pnode['%c'] (%d, %d)\n", i, a, b);
    } else if (a > b) {
	printf("Rnode['%c'] (%d, %d) has Pnode (%d, %d)\n", i, a, b,
		(a-b), b);
    } else {
	printf("Lnode['%c'] (%d, %d)  has Pnode (%d, %d)\n", i, a, b,
		a, (b-a));
    }

    return 0;
}

static struct poptOption optionsTable[] = {
 { NULL, '\0', POPT_ARG_INCLUDE_TABLE, rpmcliAllPoptTable, 0,
        N_("Common options:"),
        NULL },

   POPT_AUTOALIAS
   POPT_AUTOHELP
   POPT_TABLEEND
};

int
main(int argc, char *argv[])
{
    poptContext optCon = rpmcliInit(argc, argv, optionsTable);
    ARGV_t av = NULL;
    int ac = 0;
    int rc = 0;
    int i;

    for (i = 0; i < 256; i++) {
	an[i] = i;
	bn[i] = 0;
	nn[i] = 0;
	rn[i] = 0;
    }

    av = poptGetArgs(optCon);
    ac = argvCount(av);

    /* Activate nodes, count edges. */
    for (i = 0; i < ac; i++)
	count(av[i]);

    /* Save parents of nodes. */
    for (i = 0; i < ac; i++)
	parent(av[i]);

    /* Compute Stern-Brocot labels. */
    for (i = 0; i < 256; i++)
	SBTmark(i);

    /* Print results. */
    for (i = 0; i < 256; i++) {
	if (bn[i] == 0)
	    continue;
	SBTprnode(i);
    }

    optCon = rpmcliFini(optCon);

    return rc;
}
