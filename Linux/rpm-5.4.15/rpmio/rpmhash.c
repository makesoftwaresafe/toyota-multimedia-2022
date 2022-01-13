/**
 * \file rpmio/rpmhash.c
 * Hash table implemenation
 */

#include "system.h"
#include <rpmiotypes.h>
#include <rpmio.h>
#include <rpmhash.h>
#include "debug.h"

/*@unchecked@*/
int _ht_debug = 0;

typedef /*@owned@*/ const void * voidptr;

typedef	struct hashBucket_s * hashBucket;

/**
 */
struct hashBucket_s {
    voidptr key;			/*!< hash key */
/*@owned@*/ voidptr * data;		/*!< pointer to hashed data */
    int dataCount;			/*!< length of data (0 if unknown) */
/*@dependent@*/hashBucket next;		/*!< pointer to next item in bucket */
};

/**
 */
struct hashTable_s {
    struct rpmioItem_s _item;	/*!< usage mutex and pool identifier. */
    int numBuckets;			/*!< number of hash buckets */
    size_t keySize;			/*!< size of key (0 if unknown) */
    int freeData;	/*!< should data be freed when table is destroyed? */
    hashBucket * buckets;		/*!< hash bucket array */
/*@relnull@*/
    hashFunctionType fn;		/*!< generate hash value for key */
/*@relnull@*/
    hashEqualityType eq;		/*!< compare hash keys for equality */
#if defined(__LCLINT__)
/*@refs@*/
    int nrefs;				/*!< (unused) keep splint happy */
#endif
};

#ifdef __cplusplus
GENfree(hashBucket)
GENfree(hashBucket *)
GENfree(voidptr *)
#endif	/* __cplusplus */

/**
 * Find entry in hash table.
 * @param ht            pointer to hash table
 * @param key           pointer to key value
 * @return pointer to hash bucket of key (or NULL)
 */
static /*@shared@*/ /*@null@*/
hashBucket findEntry(hashTable ht, const void * key)
	/*@*/
{
    rpmuint32_t hash = 0;
    hashBucket b;

    /*@-modunconnomods@*/
    hash = ht->fn(hash, key, 0) % ht->numBuckets;
    b = ht->buckets[hash];

    while (b && b->key && ht->eq(b->key, key))
	b = b->next;
    /*@=modunconnomods@*/

    return b;
}

int hashEqualityString(const void * key1, const void * key2)
{
    const char *k1 = (const char *)key1;
    const char *k2 = (const char *)key2;
    return strcmp(k1, k2);
}

rpmuint32_t hashFunctionString(rpmuint32_t h, const void * data, size_t size)
{
    const char *key = (const char *) data;

    if (size == 0)
	size = strlen(key);

    /*
     * DJBX33A (Daniel J. Bernstein, Times 33 with Addition)
     *
     * This is Daniel J. Bernstein's popular `times 33' hash function as
     * posted by him years ago on comp.lang.c. It basically uses a  function
     * like ``hash(i) = hash(i-1) * 33 + str[i]''. This is one of the best
     * known hash functions for strings. Because it is both computed very
     * fast and distributes very well.
     *
     * The magic of number 33, i.e. why it works better than many other
     * constants, prime or not, has never been adequately explained by
     * anyone. So I try an explanation: if one experimentally tests all
     * multipliers between 1 and 256 (as RSE did now) one detects that even
     * numbers are not useable at all. The remaining 128 odd numbers
     * (except for the number 1) work more or less all equally well. They
     * all distribute in an acceptable way and this way fill a hash table
     * with an average percent of approx. 86%.
     *
     * If one compares the Chi^2 values of the variants, the number 33 not
     * even has the best value. But the number 33 and a few other equally
     * good numbers like 17, 31, 63, 127 and 129 have nevertheless a great
     * advantage to the remaining numbers in the large set of possible
     * multipliers: their multiply operation can be replaced by a faster
     * operation based on just one shift plus either a single addition
     * or subtraction operation. And because a hash function has to both
     * distribute good _and_ has to be very fast to compute, those few
     * numbers should be preferred and seems to be the reason why Daniel J.
     * Bernstein also preferred it.
     *
     * Below you can find the variant implemented with the
     * multiplication optimized via bit shifts and hash unrolled eight
     * times for speed.
     *                     -- Ralf S. Engelschall <rse@engelschall.com>
     */
    if (h == 0)
	h = 5381;
    for (; size >= 8; size -= 8) {
	h = ((h << 5) + h) + (rpmuint32_t)*key++;
	h = ((h << 5) + h) + (rpmuint32_t)*key++;
	h = ((h << 5) + h) + (rpmuint32_t)*key++;
	h = ((h << 5) + h) + (rpmuint32_t)*key++;
	h = ((h << 5) + h) + (rpmuint32_t)*key++;
	h = ((h << 5) + h) + (rpmuint32_t)*key++;
	h = ((h << 5) + h) + (rpmuint32_t)*key++;
	h = ((h << 5) + h) + (rpmuint32_t)*key++;
    }
    switch (size) {
	case 7: h = ((h << 5) + h) + (rpmuint32_t)*key++; /*@fallthrough@*/
	case 6: h = ((h << 5) + h) + (rpmuint32_t)*key++; /*@fallthrough@*/
	case 5: h = ((h << 5) + h) + (rpmuint32_t)*key++; /*@fallthrough@*/
	case 4: h = ((h << 5) + h) + (rpmuint32_t)*key++; /*@fallthrough@*/
	case 3: h = ((h << 5) + h) + (rpmuint32_t)*key++; /*@fallthrough@*/
	case 2: h = ((h << 5) + h) + (rpmuint32_t)*key++; /*@fallthrough@*/
	case 1: h = ((h << 5) + h) + (rpmuint32_t)*key++; break;
	default: /* case 0: */ break;
    }

    return h;
}

void htAddEntry(hashTable ht, const void * key, const void * data)
{
    rpmuint32_t hash = 0;
    hashBucket b;

    hash = ht->fn(hash, key, 0) % ht->numBuckets;
    b = ht->buckets[hash];

    while (b && b->key && ht->eq(b->key, key))
	b = b->next;

    if (b == NULL) {
	b = (hashBucket) xmalloc(sizeof(*b));
	if (ht->keySize) {
	    char *k = (char *) xmalloc(ht->keySize);
	    memcpy(k, key, ht->keySize);
	    b->key = k;
	} else {
	    b->key = key;
	}
	b->dataCount = 0;
	b->next = ht->buckets[hash];
	b->data = NULL;
	ht->buckets[hash] = b;
    }

    b->data = (voidptr *)
		xrealloc(b->data, sizeof(*b->data) * (b->dataCount + 1));
    b->data[b->dataCount++] = data;
}

int htHasEntry(hashTable ht, const void * key)
{
    hashBucket b;

    if (!(b = findEntry(ht, key))) return 0; else return 1;
}

int htGetEntry(hashTable ht, const void * key, const void * data,
	       int * dataCount, const void * tableKey)
{
    hashBucket b;

    if ((b = findEntry(ht, key)) == NULL)
	return 1;

    if (data)
	*(const void ***)data = (const void **) b->data;
    if (dataCount)
	*dataCount = b->dataCount;
    if (tableKey)
	*(const void **)tableKey = b->key;

    return 0;
}

const void ** htGetKeys(hashTable ht)
{
    const void ** keys = (const void **)
		xcalloc(ht->numBuckets+1, sizeof(const void*));
    const void ** keypointer = keys;
    hashBucket b, n;
    int i;

    for (i = 0; i < ht->numBuckets; i++) {
	b = ht->buckets[i];
	if (b == NULL)
	    continue;
	if (b->data)
	    *(keys++) = b->key;
	do {
	    n = b->next;
	    if(n != NULL)
    		*(keys++) = n->key;
	} while ((b = n) != NULL);
    }

    return keypointer;
}

/*@-mustmod@*/	/* XXX splint on crack */
static void htFini(void * _ht)
	/*@modifies _ht @*/
{
    hashTable ht = (hashTable) _ht;
    hashBucket b, n;
    int i;

    for (i = 0; i < ht->numBuckets; i++) {
	b = ht->buckets[i];
	if (b == NULL)
	    continue;
	ht->buckets[i] = NULL;
	if (ht->keySize > 0)
	    b->key = _free(b->key);
	do {
	    n = b->next;
	    if (b->data) {
		if (ht->freeData)
		    *b->data = _free(*b->data);
		b->data = _free(b->data);
	    }
	    b = _free(b);
	} while ((b = n) != NULL);
    }

    ht->buckets = _free(ht->buckets);
}
/*@=mustmod@*/

/*@unchecked@*/ /*@only@*/ /*@null@*/
rpmioPool _htPool;

static hashTable htGetPool(/*@null@*/ rpmioPool pool)
	/*@globals _htPool, fileSystem @*/
	/*@modifies pool, _htPool, fileSystem @*/
{
    hashTable ht;

    if (_htPool == NULL) {
	_htPool = rpmioNewPool("ht", sizeof(*ht), -1, _ht_debug,
			NULL, NULL, htFini);
	pool = _htPool;
    }
    return (hashTable) rpmioGetPool(pool, sizeof(*ht));
}

hashTable htCreate(int numBuckets, size_t keySize, int freeData,
		hashFunctionType fn, hashEqualityType eq)
{
    hashTable ht = htGetPool(_htPool);

    ht->numBuckets = numBuckets;
    ht->buckets = (hashBucket *) xcalloc(numBuckets, sizeof(*ht->buckets));
    ht->keySize = keySize;
    ht->freeData = freeData;
    /*@-assignexpose@*/
    ht->fn = (fn != NULL ? fn : hashFunctionString);
    ht->eq = (eq != NULL ? eq : hashEqualityString);
    /*@=assignexpose@*/

    return htLink(ht);
}
