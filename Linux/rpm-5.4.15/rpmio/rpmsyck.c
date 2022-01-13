#include "system.h"

#if defined(WITH_SYCK)
#include <syck.h>
#include <rpmhash.h>
#include <rpmsyck.h>
#endif
#include <rpmio.h>

#include "debug.h"

#ifdef __cplusplus
GENfree(rpmsyck_node)
#endif	/* __cplusplus */

int _rpmsyck_debug = 0;

rpmioPool _rpmsyckPool = NULL;

#if defined(WITH_SYCK)

static int rpmSyckFreeNode(char *key, rpmsyck_node node, char *arg) {
    switch (node->type) {
	case T_STR:
	    node->value.key = _free(node->value.key);
	    break;

	case T_SEQ:
	    node->value.seq = _free(node->value.seq);
	    break;

	case T_MAP:
	    node->value.map = htFree(node->value.map);
	    break;

	default:
	   break;
    }
    node->tag = _free(node->tag);
    node = _free(node);

    return ST_CONTINUE;
}

static void rsFini(void * _rpmSyck)
{
    rpmSyck rs = (rpmSyck) _rpmSyck;
    if(rs->syms)
	st_foreach(rs->syms, (enum st_retval (*)(const char *, const void *, void *))rpmSyckFreeNode, 0);

    st_free_table(rs->syms);
    rs->syms = NULL;
    rs->firstNode = NULL;
}

static rpmSyck rpmSyckGetPool(/*@null@*/ rpmioPool pool)
	/*@globals _rpmsyckPool, fileSystem @*/
	/*@modifies pool, _rpmsyckPool, fileSystem @*/
{
    rpmSyck rs;

    if (_rpmsyckPool == NULL) {
	_rpmsyckPool = rpmioNewPool("syck", sizeof(*rs), -1, _rpmsyck_debug,
			NULL, NULL, rsFini);
	pool = _rpmsyckPool;
    }
    return (rpmSyck) rpmioGetPool(pool, sizeof(*rs));
}


rpmSyck rpmSyckCreate(void)
{
    rpmSyck rs = rpmSyckGetPool(_rpmsyckPool);
    rs->syms = NULL;
    rs->firstNode = NULL;

    return (rpmSyck) rpmSyckLink(rs);
}

static SYMID
rpmsyck_parse_handler(SyckParser *p, SyckNode *n)
{
    rpmsyck_node node = (rpmsyck_node) xcalloc(1, sizeof(*node));

    switch (n->kind)  {
        case syck_str_kind:
            node->type = T_STR;
	    node->value.key = syck_strndup(n->data.str->ptr, n->data.str->len);
        break;

	case syck_seq_kind: {
            rpmsyck_node val;
            rpmsyck_node seq = (rpmsyck_node) xcalloc(n->data.list->idx + 1, sizeof(*node));
	    int i;
            node->type = T_SEQ;

            for (i = 0; i < n->data.list->idx; i++) {
                SYMID oid = syck_seq_read(n, i);
                syck_lookup_sym(p, oid, (char **)&val);
                seq[i] = val[0];
            }
            seq[n->data.list->idx].type = T_END;
            node->value.seq = seq;
	}
	break;

        case syck_map_kind: {
	    hashTable ht = htCreate(n->data.pairs->idx * 2, 0, 0, NULL, NULL);
            rpmsyck_node val;
	    int i;
            node->type = T_MAP;
            for (i = 0; i < n->data.pairs->idx; i++) {
		char *key;
                SYMID oid = syck_map_read(n, map_key, i);
                syck_lookup_sym(p, oid, (char **)&val);
		key = val[0].value.key;

                oid = syck_map_read(n, map_value, i);
                syck_lookup_sym(p, oid, (char **)&val);

		htAddEntry(ht, key, val);
            }
            node->value.map = ht;
	}
        break;
    }

    node->tag = n->type_id ? syck_strndup(n->type_id, strlen(n->type_id)) : NULL;

    return syck_add_sym(p, (char *) node);
}

rpmSyck rpmSyckLoad(char *yaml) {
    SyckParser *parser;
    SYMID v;
    rpmSyck rs;

    rs = rpmSyckCreate();
    
    parser = syck_new_parser();

    syck_parser_str_auto(parser, yaml, NULL);
    syck_parser_handler(parser, rpmsyck_parse_handler);
    syck_parser_error_handler(parser, NULL);
    syck_parser_implicit_typing(parser, 1);
    syck_parser_taguri_expansion(parser, 1);

    if((v = syck_parse(parser)))
         syck_lookup_sym( parser, v, (char **)&rs->firstNode);

    rs->syms = parser->syms;
    parser->syms = NULL;
    syck_free_parser(parser);

    return rs;
}

#endif
