#include "system.h"

#include <rpmiotypes.h>
#include <rpmio.h>      /* for *Pool methods */
#include <rpmlog.h>
#include <yarn.h>

#define	_RPMBF_INTERNAL
#include "rpmbf.h"

#include "debug.h"

#include <iostream>

class Item {
  protected:
  public:
    yarnLock use;

    Item(long val = 0)
	{ use = yarnNewLock(val); }
    ~Item()
	{ use = yarnFreeLock(use); }

    void possess()
	{ yarnPossess(use); }
    void release()
	{ yarnRelease(use); }
    void twist(yarnTwistOP op, long val)
	{ yarnTwist(use, op, val); }
    void waitfor(yarnWaitOP op, long val)
	{ yarnWaitFor(use, op, val); }
    long peek()
	{ return yarnPeekLock(use); }
};

class Pool {
  protected:
    rpmioPool pool;
  public:
    Pool(const char * name, size_t size, int limit, int flags,
		char * (*dbg) (void *item),
		void (*init) (void *item),
		void (*fini) (void *item))
    {
	pool = rpmioNewPool(name, size, limit, flags,
		dbg, init, fini);
    }
    ~Pool()
    {
	pool = rpmioFreePool(pool);
    }
};

static rpmioPool _rpmbfPool;

class Rpmbf : public Item  {
    static void fini(void * _bf)
    {	rpmbf bf = (rpmbf) _bf;
	bf->m = 0;
	bf->n = 0;
	bf->k = 0;
	bf->bits = PBM_FREE(bf->bits);
    }
    static void * getItem()
    {   void * _item;
	if (_rpmbfPool == NULL)
	    _rpmbfPool = rpmioNewPool("bf", sizeof(*bf), -1, _rpmbf_debug,
                        NULL, NULL, fini);
	_item = (void *) rpmioGetPool(_rpmbfPool, sizeof(*bf));
	return _item;
    }
    static void linkItem(void * _item)
    { rpmioLinkPoolItem((rpmioItem)_item, __FUNCTION__, __FILE__, __LINE__); }
    static void freeItem(void * _item)
    { rpmioFreePoolItem((rpmioItem)_item, __FUNCTION__, __FILE__, __LINE__); }

  public:
    rpmbf bf;

    Rpmbf(size_t m = 0, size_t k = 0, unsigned flags = 0)
    {	bf = (rpmbf) getItem();

	static size_t ndefault = 1024;	/* XXX default estimated population. */
	static size_t kdefault = 16;

	if (k == 0) k = kdefault;
	if (m == 0) m = (3 * ndefault * k) / 2;

	bf->k = k;
	bf->m = m;
	bf->n = 0;
	bf->bits = (unsigned char *) PBM_ALLOC(bf->m-1);

	linkItem(bf);
    }
    ~Rpmbf()
    {	free(bf); }

    int add(const char * s, size_t ns)
	{ return rpmbfAdd(bf, s, ns); }
    int chk(const char * s, size_t ns)
	{ return rpmbfChk(bf, s, ns); }
    int clr(void)
	{ return rpmbfClr(bf); }
    int del(const char * s, size_t ns)
	{ return rpmbfDel(bf, s, ns); }
};

int RpmbfUnion(rpmbf a, rpmbf b)
	{ return rpmbfUnion(a, b); }
int RpmbfIntersect(rpmbf a, rpmbf b)
	{ return rpmbfIntersect(a, b); }
void RpmbfParams(size_t n, double e, size_t * mp, size_t * kp)
	{ return rpmbfParams(n, e, mp, kp); }

int main()
{
    const char * s = "Hello, world!";
    size_t ns = strlen(s);
    int xx;

    std::cout << s << "\n";

    _rpmbf_debug = -1;
    size_t n = 1024;
    double e = 1.0e-4;
    size_t m;
    size_t k;
    RpmbfParams(n, e, &m, &k);
    unsigned flags = 0;

    Rpmbf A(m, k, flags);
    yarnLock Ause = A.use;

    A.possess(); A.release();
    A.possess(); A.twist(BY, 1);
    std::cout << "twist(BY, +1) " << A.peek() << "\n";
    A.waitfor(TO_BE, 1);
    A.possess(); A.twist(BY, -1);
    std::cout << "twist(BY, -1) " << A.peek() << "\n";
    A.waitfor(TO_BE, 0);

    xx = A.add(s, ns);
    xx = A.chk(s, ns);
    xx = A.clr();
    xx = A.del(s, ns);

    Rpmbf B(m, k, flags);
    xx = B.add(s, ns);
    xx = B.chk(s, ns);
    xx = B.clr();
    xx = B.del(s, ns);

    xx = rpmbfUnion(A.bf, B.bf);
    xx = rpmbfIntersect(A.bf, B.bf);
}
