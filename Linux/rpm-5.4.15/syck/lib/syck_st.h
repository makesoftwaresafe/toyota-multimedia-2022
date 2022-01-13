/* This is a public domain general purpose hash table package written by Peter Moore @ UCB. */

/* @(#) st.h 5.1 89/12/14 */

#ifndef ST_INCLUDED
#define ST_INCLUDED

typedef struct st_table st_table;

struct st_hash_type {
    int (*compare)(const void * _x, const void * _y);
/*@relnull@*/
    int (*hash)(const void * _n);
};

struct st_table {
/*@relnull@*/
    struct st_hash_type *type;
    size_t num_bins;
    size_t num_entries;
/*@relnull@*/
    struct st_table_entry **bins;
};

#define st_is_member(table,key) st_lookup(table,key,(char **)0)

enum st_retval {ST_CONTINUE, ST_STOP, ST_DELETE};

#ifdef __cplusplus
extern "C" {
#endif

/*@null@*/
st_table *st_init_table(struct st_hash_type *type)
	/*@*/;
/*@null@*/
st_table *st_init_table_with_size(struct st_hash_type *type, size_t size)
	/*@*/;
/*@null@*/
st_table *st_init_numtable(void)
	/*@*/;
/*@null@*/
st_table *st_init_numtable_with_size(int size)
	/*@*/;
/*@null@*/
st_table *st_init_strtable(void)
	/*@*/;
/*@null@*/
st_table *st_init_strtable_with_size(int size)
	/*@*/;
void st_free_table(/*@only@*/ st_table *table)
	/*@modifies table @*/;
int st_lookup(st_table *table, const char *key, /*@out@*/ const void **value)
	/*@modifies *value @*/;
int st_insert(st_table *table, const char *key, const void *value)
	/*@modifies table @*/;
void st_add_direct(st_table *table, const char *key, const void *value)
	/*@modifies table @*/;
/*@null@*/
st_table *st_copy(st_table *old_table)
	/*@modifies old_table @*/;
int st_delete(st_table *table, const void **key, /*@out@*/ const void **value)
	/*@modifies table, *key, *value @*/;
int st_delete_safe(st_table *table, const void **key, /*@out@*/ const void **value,
		char *never)
	/*@modifies table, *key, *value @*/;
void st_foreach(st_table *table,
		enum st_retval (*func) (const char *key, const void *record, void *arg),
		/*@null@*/ void *arg)
	/*@modifies table @*/;
void st_cleanup_safe(st_table *table, char *never)
	/*@modifies table @*/;

#ifdef __cplusplus
}
#endif

#define ST_NUMCMP	((int (*)()) 0)
#define ST_NUMHASH	((int (*)()) -2)

#define st_numcmp	ST_NUMCMP
#define st_numhash	ST_NUMHASH

#endif /* ST_INCLUDED */
