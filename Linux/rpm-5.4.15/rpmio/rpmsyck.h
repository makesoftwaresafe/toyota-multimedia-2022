#ifndef H_RPMSYCK
#define H_RPMSYCK

#if defined(WITH_SYCK)

#include <syck.h>
#include <rpmhash.h>

typedef enum {
    T_END,
    T_STR,
    T_SEQ,
    T_MAP
} syck_type_t; 

typedef /*@abstract@*/ struct rpmsyck_node_s * rpmsyck_node;

struct rpmsyck_node_s {
    syck_type_t type;
    char *tag;
    union {
	char *key;
    	rpmsyck_node seq;
	hashTable map;
    } value;
};

typedef /*@abstract@*/ /*@refcounted@*/ struct rpmSyck_s * rpmSyck;

struct rpmSyck_s {
    struct rpmioItem_s _item;	/*!< usage mutex and pool identifier. */
    rpmsyck_node firstNode;
    st_table *syms;
#if defined(__LCLINT__)
/*@refs@*/
    int nrefs;			/*!< (unused) keep splint happy */
#endif
};

/**
 * Create rpmSyck YAML document.
 * @return		pointer to initialized YAML document
 */
/*@newref@*/ /*@null@*/
rpmSyck rpmSyckCreate(void);

/**
 * Load rpmSyck YAML document from text.
 * @param yaml		string variable holding the YAML document as text.
 * @return		pointer to initialized YAML document
 */
/*@newref@*/ /*@null@*/
rpmSyck rpmSyckLoad(char *yaml);

/**
 * Unreference a rpmSyck YAML document instance.
 * @param rs		YAML document
 * @return		NULL on last dereference
 */
/*@unused@*/ /*@null@*/
rpmSyck rpmSyckUnlink (/*@killref@*/ /*@null@*/ rpmSyck rs)
	/*@modifies rs @*/;
#define	rpmSyckUnlink(_rs)	\
    ((rpmSyck)rpmioUnlinkPoolItem((rpmioItem)(_rs), __FUNCTION__, __FILE__, __LINE__))

/**
 * Reference a rpmSyck YAML document instance.
 * @param rs		YAML document
 * @return		new YAML document reference
 */
/*@unused@*/ /*@newref@*/ /*@null@*/
rpmSyck rpmSyckLink (/*@null@*/ rpmSyck rs)
	/*@modifies rs @*/;
#define	rpmSyckLink(_rs)	\
    ((rpmSyck)rpmioLinkPoolItem((rpmioItem)(_rs), __FUNCTION__, __FILE__, __LINE__))

/**
 * Destroy rpmSyck YAML document.
 * @param rs            pointer to YAML document
 * @return		NULL on last dereference
 */
/*@null@*/
rpmSyck rpmSyckFree( /*@only@*/ rpmSyck rs)
	/*@modifies rs @*/;
#define	rpmSyckFree(_rs)	\
    ((rpmSyck)rpmioFreePoolItem((rpmioItem)(_rs), __FUNCTION__, __FILE__, __LINE__))

#endif

#endif /* H_RPMSYCK */
