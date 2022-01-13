if (loglvl) print("--> Dbc.js");

// ----- access flags
var	DB_AFTER		=  1;	/* Dbc.put */
var	DB_APPEND		=  2;	/* Db.put */
var	DB_BEFORE		=  3;	/* Dbc.put */
var	DB_CONSUME		=  4;	/* Db.get */
var	DB_CONSUME_WAIT		=  5;	/* Db.get */
var	DB_CURRENT		=  6;	/* Dbc.get, Dbc.put, DbLogc.get */
var	DB_FIRST		=  7;	/* Dbc.get, DbLogc->get */
var	DB_GET_BOTH		=  8;	/* Db.get, Dbc.get */
var	DB_GET_BOTHC		=  9;	/* Dbc.get (internal) */
var	DB_GET_BOTH_RANGE	= 10;	/* Db.get, Dbc.get */
var	DB_GET_RECNO		= 11;	/* Dbc.get */
var	DB_JOIN_ITEM		= 12;	/* Dbc.get; don't do primary lookup */
var	DB_KEYFIRST		= 13;	/* Dbc.put */
var	DB_KEYLAST		= 14;	/* Dbc.put */
var	DB_LAST			= 15;	/* Dbc.get, DbLogc->get */
var	DB_NEXT			= 16;	/* Dbc.get, DbLogc->get */
var	DB_NEXT_DUP		= 17;	/* Dbc.get */
var	DB_NEXT_NODUP		= 18;	/* Dbc.get */
var	DB_NODUPDATA		= 19;	/* Db.put, Dbc.put */
var	DB_NOOVERWRITE		= 20;	/* Db.put */
var	DB_NOSYNC		= 21;	/* Db.close */
var	DB_OVERWRITE_DUP	= 22;	/* Dbc.put, Db.put; no DB_KEYEXIST */
var	DB_POSITION		= 23;	/* Dbc.dup */
var	DB_PREV			= 24;	/* Dbc.get, DbLogc->get */
var	DB_PREV_DUP		= 25;	/* Dbc.get */
var	DB_PREV_NODUP		= 26;	/* Dbc.get */
var	DB_SET			= 27;	/* Dbc.get, DbLogc->get */
var	DB_SET_RANGE		= 28;	/* Dbc.get */
var	DB_SET_RECNO		= 29;	/* Db.get, Dbc.get */
var	DB_UPDATE_SECONDARY	= 30;	/* Dbc.get, Dbc.del (internal) */
var	DB_SET_LTE		= 31;	/* Dbc.get (internal) */
var	DB_GET_BOTH_LTE		= 32;	/* Dbc.get (internal) */

var rpmdbc = require('rpmdbc');

var dbc = new rpmdbc.Dbc();
ack("typeof dbc;", "object");
ack("dbc instanceof rpmdbc.Dbc;", true);
ack("dbc.debug = 1;", 1);
ack("dbc.debug = 0;", 0);

dbc.close();

if (loglvl) print("<-- Dbc.js");
