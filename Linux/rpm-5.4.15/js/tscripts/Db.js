if (loglvl) print("--> Db.js");

// ---- access methods
var DB_BTREE		= 1;
var DB_HASH		= 2;
var DB_RECNO		= 3;
var DB_QUEUE		= 4;
var DB_UNKNOWN	= 5;	/* Figure it out on open. */

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

// -------------------------
var	DB_AGGRESSIVE				= 0x00000001;
var	DB_ARCH_ABS				= 0x00000001;
var	DB_ARCH_DATA				= 0x00000002;
var	DB_ARCH_LOG				= 0x00000004;
var	DB_ARCH_REMOVE				= 0x00000008;
var	DB_AUTO_COMMIT				= 0x00000100;
var	DB_CDB_ALLDB				= 0x00000040;
var	DB_CHKSUM				= 0x00000008;
var	DB_CKP_INTERNAL				= 0x00000002;
var	DB_CREATE				= 0x00000001;
var	DB_CURSOR_BULK				= 0x00000001;
var	DB_CURSOR_TRANSIENT			= 0x00000004;
var	DB_CXX_NO_EXCEPTIONS			= 0x00000002;
var	DB_DIRECT				= 0x00000010;
var	DB_DIRECT_DB				= 0x00000080;
var	DB_DSYNC_DB				= 0x00000200;
var	DB_DUP					= 0x00000010;
var	DB_DUPSORT				= 0x00000004;
var	DB_DURABLE_UNKNOWN			= 0x00000020;
var	DB_ENCRYPT				= 0x00000001;
var	DB_ENCRYPT_AES				= 0x00000001;
var	DB_EXCL					= 0x00000040;
var	DB_EXTENT				= 0x00000040;
var	DB_FAILCHK				= 0x00000020;
var	DB_FAST_STAT				= 0x00000001;
var	DB_FCNTL_LOCKING			= 0x00000800;
var	DB_FLUSH				= 0x00000001;
var	DB_FORCE				= 0x00000001;
var	DB_FOREIGN_ABORT			= 0x00000001;
var	DB_FOREIGN_CASCADE			= 0x00000002;
var	DB_FOREIGN_NULLIFY			= 0x00000004;
var	DB_FREELIST_ONLY			= 0x00000001;
var	DB_FREE_SPACE				= 0x00000002;
var	DB_IGNORE_LEASE				= 0x00002000;
var	DB_IMMUTABLE_KEY			= 0x00000002;
var	DB_INIT_CDB				= 0x00000040;
var	DB_INIT_LOCK				= 0x00000080;
var	DB_INIT_LOG				= 0x00000100;
var	DB_INIT_MPOOL				= 0x00000200;
var	DB_INIT_REP				= 0x00000400;
var	DB_INIT_TXN				= 0x00000800;
var	DB_INORDER				= 0x00000020;
var	DB_JOIN_NOSORT				= 0x00000001;
var	DB_LOCKDOWN				= 0x00001000;
var	DB_LOCK_NOWAIT				= 0x00000001;
var	DB_LOCK_RECORD				= 0x00000002;
var	DB_LOCK_SET_TIMEOUT			= 0x00000004;
var	DB_LOCK_SWITCH				= 0x00000008;
var	DB_LOCK_UPGRADE				= 0x00000010;
var	DB_LOG_AUTO_REMOVE			= 0x00000001;
var	DB_LOG_CHKPNT				= 0x00000002;
var	DB_LOG_COMMIT				= 0x00000004;
var	DB_LOG_DIRECT				= 0x00000002;
var	DB_LOG_DSYNC				= 0x00000004;
var	DB_LOG_IN_MEMORY			= 0x00000008;
var	DB_LOG_NOCOPY				= 0x00000008;
var	DB_LOG_NOT_DURABLE			= 0x00000010;
var	DB_LOG_WRNOSYNC				= 0x00000020;
var	DB_LOG_ZERO				= 0x00000010;
var	DB_MPOOL_CREATE				= 0x00000001;
var	DB_MPOOL_DIRTY				= 0x00000002;
var	DB_MPOOL_DISCARD			= 0x00000001;
var	DB_MPOOL_EDIT				= 0x00000004;
var	DB_MPOOL_FREE				= 0x00000008;
var	DB_MPOOL_LAST				= 0x00000010;
var	DB_MPOOL_NEW				= 0x00000020;
var	DB_MPOOL_NOFILE				= 0x00000001;
var	DB_MPOOL_NOLOCK				= 0x00000002;
var	DB_MPOOL_TRY				= 0x00000040;
var	DB_MPOOL_UNLINK				= 0x00000002;
var	DB_MULTIPLE				= 0x00000800;
var	DB_MULTIPLE_KEY				= 0x00004000;
var	DB_MULTIVERSION				= 0x00000004;
var	DB_MUTEX_ALLOCATED			= 0x00000001;
var	DB_MUTEX_LOCKED				= 0x00000002;
var	DB_MUTEX_LOGICAL_LOCK			= 0x00000004;
var	DB_MUTEX_PROCESS_ONLY			= 0x00000008;
var	DB_MUTEX_SELF_BLOCK			= 0x00000010;
var	DB_MUTEX_SHARED				= 0x00000020;
var	DB_NOLOCKING				= 0x00000400;
var	DB_NOMMAP				= 0x00000008;
var	DB_NOORDERCHK				= 0x00000002;
var	DB_NOPANIC				= 0x00000800;
var	DB_NO_AUTO_COMMIT			= 0x00001000;
var	DB_ODDFILESIZE				= 0x00000080;
var	DB_ORDERCHKONLY				= 0x00000004;
var	DB_OVERWRITE				= 0x00001000;
var	DB_PANIC_ENVIRONMENT			= 0x00002000;
var	DB_PRINTABLE				= 0x00000008;
var	DB_PRIVATE				= 0x00002000;
var	DB_PR_PAGE				= 0x00000010;
var	DB_PR_RECOVERYTEST			= 0x00000020;
var	DB_RDONLY				= 0x00000400;
var	DB_RDWRMASTER				= 0x00002000;
var	DB_READ_COMMITTED			= 0x00000400;
var	DB_READ_UNCOMMITTED			= 0x00000200;
var	DB_RECNUM				= 0x00000040;
var	DB_RECOVER				= 0x00000002;
var	DB_RECOVER_FATAL			= 0x00004000;
var	DB_REGION_INIT				= 0x00004000;
var	DB_REGISTER				= 0x00008000;
var	DB_RENUMBER				= 0x00000080;
var	DB_REPMGR_CONF_2SITE_STRICT		= 0x00000001;
var	DB_REPMGR_PEER				= 0x00000001;
var	DB_REP_ANYWHERE				= 0x00000001;
var	DB_REP_CLIENT				= 0x00000001;
var	DB_REP_CONF_BULK			= 0x00000002;
var	DB_REP_CONF_DELAYCLIENT			= 0x00000004;
var	DB_REP_CONF_INMEM			= 0x00000008;
var	DB_REP_CONF_LEASE			= 0x00000010;
var	DB_REP_CONF_NOAUTOINIT			= 0x00000020;
var	DB_REP_CONF_NOWAIT			= 0x00000040;
var	DB_REP_ELECTION				= 0x00000004;
var	DB_REP_MASTER				= 0x00000002;
var	DB_REP_NOBUFFER				= 0x00000002;
var	DB_REP_PERMANENT			= 0x00000004;
var	DB_REP_REREQUEST			= 0x00000008;
var	DB_REVSPLITOFF				= 0x00000100;
var	DB_RMW					= 0x00001000;
var	DB_RPCCLIENT				= 0x00000001;
var	DB_SALVAGE				= 0x00000040;
var	DB_SA_SKIPFIRSTKEY			= 0x00000080;
var	DB_SA_UNKNOWNKEY			= 0x00000100;
var	DB_SEQ_DEC				= 0x00000001;
var	DB_SEQ_INC				= 0x00000002;
var	DB_SEQ_RANGE_SET			= 0x00000004;
var	DB_SEQ_WRAP				= 0x00000008;
var	DB_SEQ_WRAPPED				= 0x00000010;
var	DB_SET_LOCK_TIMEOUT			= 0x00000001;
var	DB_SET_REG_TIMEOUT			= 0x00000004;
var	DB_SET_TXN_NOW				= 0x00000008;
var	DB_SET_TXN_TIMEOUT			= 0x00000002;
var	DB_SHALLOW_DUP				= 0x00000100;
var	DB_SNAPSHOT				= 0x00000200;
var	DB_STAT_ALL				= 0x00000004;
var	DB_STAT_CLEAR				= 0x00000001;
var	DB_STAT_LOCK_CONF			= 0x00000008;
var	DB_STAT_LOCK_LOCKERS			= 0x00000010;
var	DB_STAT_LOCK_OBJECTS			= 0x00000020;
var	DB_STAT_LOCK_PARAMS			= 0x00000040;
var	DB_STAT_MEMP_HASH			= 0x00000008;
var	DB_STAT_MEMP_NOERROR			= 0x00000010;
var	DB_STAT_SUBSYSTEM			= 0x00000002;
var	DB_ST_DUPOK				= 0x00000200;
var	DB_ST_DUPSET				= 0x00000400;
var	DB_ST_DUPSORT				= 0x00000800;
var	DB_ST_IS_RECNO				= 0x00001000;
var	DB_ST_OVFL_LEAF				= 0x00002000;
var	DB_ST_RECNUM				= 0x00004000;
var	DB_ST_RELEN				= 0x00008000;
var	DB_ST_TOPLEVEL				= 0x00010000;
var	DB_SYSTEM_MEM				= 0x00010000;
var	DB_THREAD				= 0x00000010;
var	DB_TIME_NOTGRANTED			= 0x00008000;
var	DB_TRUNCATE				= 0x00004000;
var	DB_TXN_NOSYNC				= 0x00000001;
var	DB_TXN_NOT_DURABLE			= 0x00000002;
var	DB_TXN_NOWAIT				= 0x00000010;
var	DB_TXN_SNAPSHOT				= 0x00000002;
var	DB_TXN_SYNC				= 0x00000004;
var	DB_TXN_WAIT				= 0x00000008;
var	DB_TXN_WRITE_NOSYNC			= 0x00000020;
var	DB_UNREF				= 0x00020000;
var	DB_UPGRADE				= 0x00000001;
var	DB_USE_ENVIRON				= 0x00000004;
var	DB_USE_ENVIRON_ROOT			= 0x00000008;
var	DB_VERB_DEADLOCK			= 0x00000001;
var	DB_VERB_FILEOPS				= 0x00000002;
var	DB_VERB_FILEOPS_ALL			= 0x00000004;
var	DB_VERB_RECOVERY			= 0x00000008;
var	DB_VERB_REGISTER			= 0x00000010;
var	DB_VERB_REPLICATION			= 0x00000020;
var	DB_VERB_REPMGR_CONNFAIL			= 0x00000040;
var	DB_VERB_REPMGR_MISC			= 0x00000080;
var	DB_VERB_REP_ELECT			= 0x00000100;
var	DB_VERB_REP_LEASE			= 0x00000200;
var	DB_VERB_REP_MISC			= 0x00000400;
var	DB_VERB_REP_MSGS			= 0x00000800;
var	DB_VERB_REP_SYNC			= 0x00001000;
var	DB_VERB_REP_TEST			= 0x00002000;
var	DB_VERB_WAITSFOR			= 0x00004000;
var	DB_VERIFY				= 0x00000002;
var	DB_VERIFY_PARTITION			= 0x00040000;
var	DB_WRITECURSOR				= 0x00000008;
var	DB_WRITELOCK				= 0x00000010;
var	DB_WRITEOPEN				= 0x00008000;
var	DB_YIELDCPU				= 0x00010000;
// -------------------------

var rpmdb = require('rpmdb');
var rpmdbc = require('rpmdbc');
var rpmdbe = require('rpmdbe');
var rpmtxn = require('rpmtxn');

function BDB(dbenv, _name, _type, _re_source, _flags) {
    this.dbenv = dbenv;
    this.db = new rpmdb.Db(dbenv, 0);
    this.txn = null;
    this.dbfile = _name + ".db";
    this.dbname = null;
    this.oflags = DB_CREATE | DB_AUTO_COMMIT;
    this.dbtype = _type;
    this.dbperms = 0644;

    this.re_source = _re_source;
    this.flags = _flags;

    this.db.errfile =	this.errfile ="stderr";
    this.db.errpfx =	this.errpfx = this.dbfile;
    this.db.pagesize =	this.pagesize = 1024;
//  this.db.cachesize =	this.cachesize = 0;

    this.avg_keysize = 128;
    this.avg_datasize = 1024;

    switch (this.dbtype) {
    case DB_HASH:
	this.db.h_ffactor = this.h_ffactor =
	  Math.floor(100 * (pagesize - 32) / (avg_keysize + avg_datasize + 8));
	this.db.h_nelem = this.h_nelem = 12345;
	break;
    case DB_BTREE:
	this.bt_minkey = undefined;
	break;
    case DB_RECNO:
//	this.re_delim = 0x0a;
//	this.db.re_len = this.re_len = 40;
//	this.re_pad = 0x20;
	this.db.re_source = this.re_source;
	break;
    case DB_QUEUE:
	this.q_extentsize = 0;
	this.db.re_len = this.re_len = 40;
	break;
    }

    this.associate =
	function (secondary) { return this.db.associate(this.txn, secondary, 0); };
    this.associate_foreign =
	function (secondary, flags) { return this.db.associate_foreign(secondary, flags); };

    this.close =
	function () { return this.db.close(0); };

    this.cursor =
	function (key) {
	    var dbc = this.db.cursor();
	    if (key != undefined)
		dbc.get(key, null, (key != null ? DB_SET : DB_NEXT));
	    return dbc;
	};

    this.del =
	function (key) { return this.db.del(this.txn, key); };

    this.exists =
	function (key) { return this.db.exists(this.txn, key); };

    this.get =
	function (key) { return this.db.get(this.txn, key); };

    this.join =
	function (A, B) { return this.db.join(A, B); };

    this.key_range =
	function (key) { return this.db.key_range(this.txn, key); };

    this.put =
	function (key, val) { return this.db.put(this.txn, key, val); };

    this.sync =
	function () { return this.db.sync(); };

    this.print =
	function(msg, list) {
	    var i, w;
	    print ("-----", msg);
	    for ([i, w] in Iterator(list)) {
		var key = w;
		print('\t' + w + '\t<=>\t' + this.db.get(this.txn, key));
	    }
	    return true;
	};

    this.loop =
	function(msg, imin, imax) {
	    var i;
	    print ("-----", msg);
	    for (i = imin; i < imax; i++)
		print('\t' + i + ': ' + this.get(i));
	    return true;
	};

    if (this.flags)
	this.db.flags |= this.flags;

    return this.db.open(this.txn, this.dbfile, this.dbname, this.dbtype,
			this.oflags, this.dbperms);
}

function Fill(A) {
    var dbenv = A.dbenv;
    var db = A.db;
    var dbc = null;
    var txn = null;
    var putflags = 0;
    var imin = 1;
    var imax = 10;

    print('----> PUT ' + dbenv.home + '/' + db.dbfile);

    switch (db.dbtype) {
    case DB_HASH:
    case DB_BTREE:
	putflags = DB_NOOVERWRITE;
	break;
    case DB_RECNO:
    case DB_QUEUE:
	putflags = DB_APPEND;
	break;
    }

//    txn = dbenv.txn_begin(null, 0);
//    ack("typeof txn;", "object");
//    ack("txn instanceof rpmtxn.Txn;", true);
    for (let i = imin; i < imax; i++)
	db.put(txn, i, i.toString(2), putflags);
    db.sync();
//    txn.commit();

//  print('----> GET ' + dbenv.home + '/' + db.dbfile);
//  txn = null;
//  for (let i = imin; i < imax; i++) {
//	if (db.exists(txn, i))
//	    print('\t' + i + ': ' + db.get(txn, i));
//  }

//  db.stat_print(DB_FAST_STAT);
//  print("st_buckets:\t" + db.st_buckets);
//  print("st_cur_recno:\t" + db.st_cur_recno);
//  print("st_extentsize:\t" + db.st_extentsize);
//  print("st_ffactor:\t" + db.st_ffactor);
//  print("st_first_recno:\t" + db.st_first_recno);
//  print("st_magic:\t" + db.st_magic);
//  print("st_minkey:\t" + db.st_minkey);
//  print("st_ndata:\t" + db.st_ndata);
//  print("st_nkeys:\t" + db.st_nkeys);
//  print("st_pagcnt:\t" + db.st_pagcnt);
//  print("st_pagesize:\t" + db.st_pagesize);
//  print("st_re_len:\t" + db.st_re_len);
//  print("st_re_pad:\t" + db.st_re_pad);
//  print("st_version:\t" + db.st_version);

//  txn = dbenv.txn_begin(null, 0);
//  for (let i = 0; i < imax; i++)
//	db.del(txn, i.toString(10));
//  txn.commit();
}

// -----
var home = "./rpmdb";
var eflags = DB_CREATE | DB_INIT_LOCK | DB_INIT_MPOOL | DB_INIT_REP | DB_INIT_TXN | DB_INIT_LOG;
var emode = 0;

var dbenv = new rpmdbe.Dbe();
ack("typeof dbenv;", "object");
ack("dbenv instanceof rpmdbe.Dbe;", true);
ack('dbenv.lg_dir = "./log"', './log');

ack('dbenv.open(home, eflags, emode)', true);
ack('dbenv.home', home);
ack('dbenv.open_flags', eflags);

var errfile = "stderr";
var pagesize = 1024;
var cachesize = 1024 * 1024;
var little_endian = 1234;
var big_endian = 4321;

var dbname = null;
var oflags = DB_CREATE | DB_AUTO_COMMIT;
var dbperms = 0644;

var dbfile = "H.db";
var dbtype = DB_HASH;
var avg_keysize = 128;
var avg_datasize = 1024;
var h_ffactor = Math.floor(100 * (pagesize - 32) / (avg_keysize + avg_datasize + 8));
var h_nelem = 12345;
var H = new BDB(dbenv, "H", DB_HASH, null, 0);
ack('typeof H.db', 'object');
ack('H.db instanceof rpmdb.Db;', true);
ack('H.db.dbfile', dbfile);
ack('H.db.dbname', dbname);
ack('H.db.type', dbtype);
ack('H.db.open_flags', oflags);
ack('H.db.h_ffactor', h_ffactor);
ack('H.db.h_nelem', h_nelem);
Fill(H);
ack('H.db.sync()', true);
ack('H.db.close(0)', true);

var dbfile = "Q.db";
var dbtype = DB_QUEUE;
var q_extentsize = 0;
var Q = new BDB(dbenv, "Q", DB_QUEUE, null, 0);
ack('typeof Q.db', 'object');
ack('Q.db instanceof rpmdb.Db;', true);
ack('Q.db.dbfile', dbfile);
ack('Q.db.dbname', dbname);
ack('Q.db.type', dbtype);
ack('Q.db.open_flags', oflags);
ack('Q.db.q_extentsize', q_extentsize);
Fill(Q);
ack('Q.db.sync()', true);
ack('Q.db.close(0)', true);

var dbfile = "B.db";
var dbtype = DB_BTREE;
var bt_minkey = undefined;
var B = new BDB(dbenv, "B", DB_BTREE, null, 0);
ack('typeof B.db', 'object');
ack('B.db instanceof rpmdb.Db;', true);
ack('B.db.dbfile', dbfile);
ack('B.db.dbname', dbname);
ack('B.db.type', dbtype);
ack('B.db.open_flags', oflags);
ack('B.db.bt_minkey', bt_minkey);
Fill(B);
ack('B.db.sync()', true);
ack('B.db.close(0)', true);

var dbfile = "R.db";
var dbtype = DB_RECNO;
var re_delim = 0x0a;
var re_len = undefined;
var re_pad = 0x20;
var re_source = "Source";
var R = new BDB(dbenv, "R", DB_RECNO, null, 0);
ack('typeof R.db', 'object');
ack('R.db instanceof rpmdb.Db;', true);
ack('R.db.dbfile', dbfile);
ack('R.db.dbname', dbname);
ack('R.db.type', dbtype);
ack('R.db.open_flags', oflags);
ack('R.db.re_delim', re_delim);
ack('R.db.re_len', 0);
ack('R.db.re_pad', re_pad);
ack('R.db.re_source', null);
Fill(R);
ack('R.db.sync()', true);
ack('R.db.close(0)', true);

if (1) {
print('====> WORDS');
var W = new BDB(dbenv, "W", DB_RECNO, "words", DB_SNAPSHOT);
var X = new BDB(dbenv, "X", DB_BTREE, null, 0);

// var i = 0;
// var k = '';
// do {
//     ++i;
//     k = W.get(i);
// print('\t', i, k);
//     if (!k)
// 	break;
//      X.put(k, i);
// } while (1) ;

// var krwords = [ "boomerang", "fedora", "mugwumps", "stifle", "zebras" ];
// for (let [key,val] in Iterator(krwords)) {
//    print('    key_range(' + val + '):')
//    print('\t' + W.key_range(val));
// }

var i;
var w = "wdj";
if (X.exists(w))
    ack('X.del(w)', true);
if (W.exists(w))
    ack('W.del(w)', true);

ack('W.associate(X.db, 0)', true);
ack('W.db.put(W.txn, 0, w, DB_APPEND)', true);
// print('\t' + w + '\t<=>\t' + X.get(w));

var luwords = [ "rpm", "package", "monkey", "wdj" ];
for ([i,w] in Iterator(luwords)) {
    var got = X.get(w);
    if (got != w)
	print('\t' + w + '\t<=>\t' + got);
}

ack('X.del("wdj")', true);

ack('X.sync() && X.close(0)', true);
ack('W.sync() && W.close(0)', true);

print('<==== WORDS');
}

if (0) {
print('====> FILES');
var dbfile = "F.db";
var F = new BDB(dbenv, "F", DB_RECNO, "files", DB_SNAPSHOT);
var FN = new BDB(dbenv, "FN", DB_BTREE, null);
var BN = new BDB(dbenv, "BN", DB_BTREE, null);
var DN = new BDB(dbenv, "DN", DB_BTREE, null, DB_DUPSORT);

// var i = 0;
// var k = '';
// do {
//     ++i;
//     k = F.get(i);
// print('\t', i, k);
//     if (!k)
// 	break;
//     FN.put(k, i);
//     var ix = k.lastIndexOf("/") + 1;
//     BN.put(k.substr(ix), i);
//     DN.put(k.substr(0, ix), i);
// } while (1) ;

var dnlist = [ "/bin/", "/sbin/" ];
for (var [key, val] in Iterator(dnlist)) {
    print('    key_range(' + val + '):')
    print('\t' + DN.key_range(val) + '\t' + FN.key_range(val));
};

ack('F.db.associate(F.txn, FN.db, 0)', true);
ack('F.db.associate(F.txn, BN.db, 0)', true);
ack('F.db.associate(F.txn, DN.db, 0)', true);

var fnlist = [ "/bin/bash", "/bin/cp", "/bin/mv", "/bin/sh", "/sbin/rmt" ];

     F.loop("F", 3, 8);
     FN.print("FN", fnlist);
     BN.print("BN", [ "bash", "sh" ]);
     DN.print("DN", [ "/bin/", "/sbin/" ]);

for (var [key, val] in Iterator(fnlist)) {
    var fn = val;
    var ix = fn.lastIndexOf("/") + 1;
    var bn = fn.substr(ix);
    var dn = fn.substr(0, ix);

    var fnc = FN.cursor(fn);
    ack('fnc.count()', 1);
    ack('fnc.length', 1);
    var bnc = BN.cursor(bn);
    ack('bnc.count()', 1);
    ack('bnc.length', 1);
    var dnc = DN.cursor(dn);
    ack('dnc.count()', (dn == "/sbin/" ? 277 : 106));
    ack('dnc.length', (dn == "/sbin/" ? 277 : 106));


    var fc = F.join(fnc, bnc);
    ack('fc.get(null)', fn);
    ack('fc.close()', true);

    var fc = F.join(dnc, bnc);
    ack('fc.get(null)', fn);
    ack('fc.close()', true);

    ack('dnc.close()', true);
    ack('bnc.close()', true);
    ack('fnc.close()', true);
}

ack('DN.sync() && DN.close()', true);
ack('BN.sync() && BN.close()', true);
ack('FN.sync() && FN.close()', true);
ack('F.sync() && F.close()', true);

print('<==== FILES');
}

if (1) {
print('====> GROUPS');
var G = new BDB(dbenv, "G", DB_RECNO, "groups", DB_SNAPSHOT);
var GF = new BDB(dbenv, "GF", DB_RECNO, "groups", DB_SNAPSHOT);
var GN = new BDB(dbenv, "GN", DB_BTREE, null, DB_DUP);

var bad = "Illegal";
var glist = [ "Amusements/Games", "Documentation", "Utilities" ];

var key = 0;
var val = '';
for (key = 1; (val = G.get(key)); key++) {
print('\t', key, val);
    ack('GN.put(val, key)', true);
}

for (let [key,val] in Iterator(glist)) {
   print('    key_range(' + val + '):')
   print('\t' + GN.key_range(val));
}

ack('G.associate(GN.db, 0)', true);
ack('GF.associate_foreign(GN.db, DB_FOREIGN_ABORT)', true);

for ([i,g] in Iterator(glist)) {
    var got = GN.get(g);
    if (got != g)
	print('\t' + g + '\t<=>\t' + got);
}

ack('G.db.put(G.txn, 0, glist[0], DB_APPEND)', true);

ack('G.db.put(G.txn, 0, bad, DB_APPEND)', false);
ack('GN.del(bad)', undefined);	// XXX false instead?

ack('GN.sync() && GN.close(0)', true);
ack('GF.sync() && GF.close(0)', true);
ack('G.sync() && G.close(0)', true);

print('<==== GROUPS');
}

if (0) {
print('====> DEPS');
var Clist = [
  [ "alsa-utils",	"1.0.18",	"2" ],
  [ "anaconda-images",	"10",	"10" ],
  [ "aspell-ca",	"0.50",	"2" ],
  [ "aspell-config",	"0.27",	"2" ],
  [ "aspell-da",	"0.50",	"2" ],
  [ "aspell-de",	"0.50",	"2" ],
  [ "aspell-es",	"0.50",	"2" ],
  [ "aspell-fr",	"0.50",	"2" ],
  [ "aspell-it",	"0.50",	"2" ],
  [ "aspell-nl",	"0.50",	"2" ],
  [ "aspell-no",	"0.50",	"2" ],
  [ "aspell-pt_BR",	"2.5",	"2" ],
  [ "aspell-sv",	"0.50",	"2" ],
  [ "audispd-plugins",	"1.7.7-1",	"10" ],
  [ "automake",	"1.5",	"2" ],
  [ "automake",	"1.8",	"2" ],
  [ "bash",	"2.0.4-21",	"10" ],
  [ "binutils",	"2.17.50.0.3-4",	"2" ],
  [ "bluez-gnome",	"1.8",	"10" ],
  [ "bonobo-devel",	"1.0.8",	"2" ],
  [ "cracklib-dicts",	"2.8",	"2" ],
  [ "crontabs",	"1.5",	"10" ],
  [ "cups",	"1:1.1.20-4",	"2" ],
  [ "dbus",	"1.1.4-3.fc9",	"2" ],
  [ "dejavu-fonts",	"2.26-3",	"2" ],
  [ "dejavu-fonts-experimental",	"2.26-3",	"2" ],
  [ "desktop-backgrounds-basic",	"2.0-27",	"2" ],
  [ "desktop-backgrounds-extended",	"2.0-27",	"2" ],
  [ "dhclient",	"3.0.3-7",	"2" ],
  [ "docbook-utils",	"0.6.9-4",	"2" ],
  [ "e2fsprogs",	"1.37-4",	"2" ],
  [ "elilo",	"3.6-5",	"10" ],
  [ "evolution",	"2.4.1-5",	"10" ],
  [ "fipscheck",	"1.2.0-1",	"2" ],
  [ "firstboot",	"1.3.26",	"10" ],
  [ "fonts-hebrew",	"0.100",	"2" ],
  [ "fonts-xorg-base",	"",	"0" ],
  [ "fonts-xorg-syriac",	"",	"0" ],
  [ "gcc-c++",	"4.0.0",	"2" ],
  [ "GConf2-dbus",	"",	"0" ],
  [ "GConf2-dbus-devel",	"",	"0" ],
  [ "gdb",	"5.1-2",	"2" ],
  [ "gdb",	"6.6-9",	"2" ],
  [ "gdk-pixbuf-devel",	"0.11",	"10" ],
  [ "gdm",	"1:2.6.0.8-5",	"2" ],
  [ "ghostscript",	"7.05",	"2" ],
  [ "glib2",	"2.11.1-2",	"2" ],
  [ "glibc",	"2.2",	"2" ],
  [ "glibc-common",	"2.3.2-63",	"10" ],
  [ "gnome-libs-devel",	"1:1.4.1.2",	"2" ],
  [ "gnome-libs-devel",	"1.4.1.2",	"2" ],
  [ "gnome-power-manager",	"2.15.3",	"2" ],
  [ "gnome-themes",	"2.9.0",	"2" ],
  [ "gnome-vfs-devel",	"1.0.2",	"2" ],
  [ "groff-tools",	"1.18.1.4",	"8" ],
  [ "gtk2-engines",	"2.7.4-7",	"2" ],
  [ "gtk-nodoka-engine",	"0.7.0-2",	"2" ],
  [ "hal",	"0.5.10-3.fc11",	"2" ],
  [ "hotplug",	"3:2002_01_14-2",	"2" ],
  [ "hpijs",	"1.5",	"2" ],
  [ "hwdata",	"0.169",	"2" ],
  [ "initscripts",	"4.26",	"2" ],
  [ "initscripts",	"5.30-1",	"10" ],
  [ "initscripts",	"6.55",	"2" ],
  [ "initscripts",	"7.23",	"2" ],
  [ "initscripts",	"7.84",	"2" ],
  [ "iptables",	"1.3.2-1",	"2" ],
  [ "ipw2200-firmware",	"2.4",	"2" ],
  [ "isdn4k-utils",	"3.2-32",	"2" ],
  [ "ispell",	"3.1.21",	"2" ],
  [ "iwl4965-firmware",	"228.57.2",	"2" ],
  [ "jfsutils",	"1.1.7-2",	"2" ],
  [ "kbd",	"1.06-19",	"2" ],
  [ "kbd",	"1.12-21",	"2" ],
  [ "kdebase",	"3.1.5",	"10" ],
  [ "kdelibs",	"6:3.5.2-6",	"2" ],
  [ "kdepim",	"6:4.2.90",	"2" ],
  [ "kernel",	"0:2.6",	"2" ],
  [ "kernel",	"2.2.12-7",	"2" ],
  [ "kernel",	"2.4",	"2" ],
  [ "kernel",	"2.4.20",	"2" ],
  [ "kernel",	"2.6.12",	"2" ],
  [ "kernel",	"2.6.17",	"2" ],
  [ "kernel",	"2.6.26",	"2" ],
  [ "kudzu",	"1.2.0",	"2" ],
  [ "liberation-fonts",	"1.04.93-8",	"2" ],
  [ "libglade",	"0.17",	"2" ],
  [ "libgnomeui",	"2.15.1cvs20060505-2",	"2" ],
  [ "libpciaccess",	"0.10.3-5",	"2" ],
  [ "lm1100",	"",	"0" ],
  [ "lokkit",	"0.50-14",	"2" ],
  [ "lvm",	"",	"0" ],
  [ "man-pages",	"2.43-12",	"2" ],
  [ "man-pages-fr",	"0.9.7-14",	"2" ],
  [ "man-pages-it",	"0.3.0-17",	"2" ],
  [ "man-pages-pl",	"0.24-2",	"2" ],
  [ "mdadm",	"2.6.4-3",	"2" ],
  [ "mkinitrd",	"0:4.1.11-1",	"10" ],
  [ "mkinitrd",	"4.0",	"2" ],
  [ "mutt",	"5:1.5.16-2",	"2" ],
  [ "nautilus",	"2.0.3-1",	"10" ],
  [ "nautilus",	"2.2.0",	"2" ],
  [ "ncurses",	"5.6-13",	"2" ],
  [ "NetworkManager-openconnect",	"0:0.7.0.99-1",	"2" ],
  [ "NetworkManager-openvpn",	"1:0.7.0.99-1",	"2" ],
  [ "NetworkManager-pptp",	"1:0.7.0.99-1",	"2" ],
  [ "NetworkManager-vpnc",	"1:0.7.0.99-1",	"2" ],
  [ "nfs-utils",	"1.0.7-12",	"2" ],
  [ "nss",	"3.12.2.99.3-5",	"2" ],
  [ "nss_ldap",	"254",	"2" ],
  [ "nut",	"2.2.0",	"2" ],
  [ "oprofile",	"0.9.1-2",	"2" ],
  [ "ORBit-devel",	"1:0.5.8",	"10" ],
  [ "pam_krb5",	"1.49",	"2" ],
  [ "passivetex",	"1.21",	"2" ],
  [ "pcmcia-cs",	"",	"0" ],
  [ "pcre",	"4.0",	"2" ],
  [ "pirut",	"1.1.4",	"2" ],
  [ "plymouth",	"0.7.0-0.2009.02.26",	"2" ],
  [ "pm-utils",	"0.99.3-11",	"10" ],
  [ "ppp",	"2.4.3-3",	"2" ],
  [ "procps",	"3.2.5-6.3",	"2" ],
  [ "psacct",	"6.3.2-12",	"2" ],
  [ "python",	"2.6-9.fc11",	"2" ],
  [ "qt",	"0:2.2.2",	"2" ],
  [ "redhat-artwork",	"0.243-1",	"2" ],
  [ "redhat-artwork",	"0.35",	"2" ],
  [ "redhat-artwork",	"5.0.5",	"10" ],
  [ "reiserfs-utils",	"3.6.19-2",	"2" ],
  [ "samba-client",	"3.0",	"2" ],
  [ "samba-common",	"3.0",	"2" ],
  [ "selinux-policy",	"3.0.3-3",	"2" ],
  [ "selinux-policy-targeted",	"1.25.3-14",	"2" ],
  [ "squashfs-tools",	"4.0",	"2" ],
  [ "subversion",	"0.22.2-4",	"2" ],
  [ "subversion-devel",	"0.22.2-4",	"2" ],
  [ "sysklogd",	"1.4.1",	"2" ],
  [ "sysklogd",	"1.4.1-43",	"2" ],
  [ "system-config-date",	"1.9.35",	"2" ],
  [ "system-config-display",	"1.0.31",	"2" ],
  [ "system-config-printer",	"0.6.132",	"2" ],
  [ "system-config-printer",	"0.7.0",	"2" ],
  [ "system-config-services",	"0.99.29",	"2" ],
  [ "system-config-users",	"1.2.82",	"2" ],
  [ "tcsh",	"6.13-5",	"2" ],
  [ "tetex",	"1.0.7-66",	"2" ],
  [ "ttfonts-ja",	"1.2-23",	"2" ],
  [ "ttfonts-ko",	"1.0.11-27",	"2" ],
  [ "ttfonts-zh_CN",	"2.12-2",	"2" ],
  [ "ttfonts-zh_TW",	"2.11-20",	"2" ],
  [ "udev",	"062",	"2" ],
  [ "udev",	"063-6",	"2" ],
  [ "util-linux",	"2.11r-9",	"2" ],
  [ "util-linux",	"2.12",	"2" ],
  [ "wireless-tools",	"28-0.pre8.5",	"2" ],
  [ "wireless-tools",	"29-3",	"2" ],
  [ "Xconfigurator",	"",	"0" ],
  [ "xfsdump",	"2.0.0",	"2" ],
  [ "xfsdump",	"3.0.1",	"2" ],
  [ "xfsprogs",	"2.6.13-4",	"2" ],
  [ "xorg-x11",	"",	"0" ],
  [ "xorg-x11-drivers",	"7.3-11.fc11",	"2" ],
  [ "xorg-x11-proto-devel",	"7.2-12",	"10" ],
  [ "xorg-x11-server-Xorg",	"1.3.0.0-19.fc8",	"2" ],
  [ "xorg-x11-server-Xorg",	"1.4.99.901-14",	"2" ],
  [ "xorg-x11-server-Xorg",	"1.6.0-7",	"2" ],
  [ "xpdf",	"1:3.01-8",	"10" ],
  [ "xscreensaver",	"1:5.00-19",	"2" ],
  [ "yaboot",	"1.3.14-8",	"2" ],
  [ "ypbind",	"1.6-12",	"2" ],
  [ "yum",	"3.2.0",	"2" ],
];

var C = new BDB(dbenv, "C", DB_BTREE, null, 0);
var CN = new BDB(dbenv, "CN", DB_BTREE, null, DB_DUP);
var CEVR = new BDB(dbenv, "CEVR", DB_BTREE, null, DB_DUP);
var CF = new BDB(dbenv, "CF", DB_BTREE, null, DB_DUP);

var ops = [ "","<",">","?3","=","<=",">=","?7" ];
for (var [ ix, dep ] in Iterator(Clist)) {
    var N = dep[0];
    var EVR = dep[1];
    var F = dep[2];
    var op = ops[(F >> 1) & 0x7];
    var val = (((F & 0xf) == 0) ? N : (N + ' ' + op + ' ' + EVR));
    print('\t' + ix + ':', val);
    C.put(ix, val);
    CN.put(N, ix);
    CEVR.put(EVR, ix);
    CF.put(op, ix);
}

ack('C.db.associate(C.txn, CN.db, 0)', true);
ack('C.db.associate(C.txn, CEVR.db, 0)', true);
ack('C.db.associate(C.txn, CF.db, 0)', true);

var cnlist = [ "udev", "tcsh", "wnh" ];

     C.loop("C", 3, 8);
     CN.print("CN", cnlist);
     CEVR.print("CEVR", [ "3.2.0", "" ]);
     CF.print("CF", [ "2", "" ]);

for (var [ix, dep] in Iterator(Clist)) {
    var N = dep[0];
    var EVR = dep[1];
    var F = dep[2];
    var op = ops[(F >> 1) & 0x7];
    var val = (((F & 0xf) == 0) ? N : (N + ' ' + op + ' ' + EVR));

    var CNc = CN.cursor(N);
    nack('CNc.count()', 0);
    nack('CNc.length', 0);
    var CEVRc = CEVR.cursor(EVR);
    nack('CEVRc.count()', 0);
    nack('CEVRc.length', 0);
    var CFc = CF.cursor(op);
    nack('CFc.count()', 0);
    nack('CFc.length', 0);

    var Cc = C.join(CNc, CEVRc, CFc);
//  ack('Cc.get(null)', val);
    print('\t' + ix + ': "' + val + '"\t<=>\t"' + Cc.get(null) + '"');
    ack('Cc.close()', true);

    ack('CFc.close()', true);
    ack('CEVRc.close()', true);
    ack('CNc.close()', true);
}

ack('CF.sync() && CF.close(0)', true);
ack('CEVR.sync() && CEVR.close(0)', true);
ack('CN.sync() && CN.close(0)', true);
ack('C.sync() && C.close(0)', true);

print('<==== DEPS');
}

var db = H.db;
// ack("db.debug = 1;", 1);
// ack("db.debug = 0;", 0);

ack('db.errfile', errfile);
// ack('db.errpfx', dbfile);
ack('db.pagesize', pagesize);
// ack('db.cachesize', 265564);

ack('db.lorder', little_endian);
ack('db.byteswapped', 0);
ack('db.multiple', 0);
ack('db.create_dir', null);
// ack('db.create_dir = "."', true);

// --- not permitted with dbenv
// ack('db.encrypt', 0);
// ack('db.encrypt = "xyzzy"', true);

ack('db.flags', 0);
// ack('db.flags = DB_REGION_INIT', true);

ack('db.msgfile', null);
// ack('db.msgfile = "stdout"', true);

ack('db.priority', 0);

var txn = dbenv.txn_begin(null, 0);
ack('typeof txn', "object");
ack('txn instanceof rpmtxn.Txn', true);
ack('db.put(txn, "foo", "bar")', true);
ack('db.exists(txn, "foo")', true);
ack('db.get(txn, "foo")', 'bar');
ack('db.del(txn, "foo")', true);
ack('txn.commit()', true);

var txn = dbenv.txn_begin(null, 0);
ack('typeof txn', "object");
ack('txn instanceof rpmtxn.Txn', true);
var dbc = db.cursor(txn);
ack('typeof dbc;', "object");
ack('dbc instanceof rpmdbc.Dbc;', true);
ack('dbc.get("foo", "bar", DB_SET)', undefined);
ack('dbc.put("foo", "bar", DB_KEYLAST)', true);
ack('dbc.close()', true);
ack('txn.commit()', true);

var txn = dbenv.txn_begin(null, 0);
ack('typeof txn', "object");
ack('txn instanceof rpmtxn.Txn', true);
var dbc = db.cursor(txn);
ack('typeof dbc;', "object");
ack('dbc instanceof rpmdbc.Dbc;', true);
ack('dbc.get("foo", "bar", DB_SET)', 'bar');
ack('dbc.count()', 1);
ack('dbc.del()', true);
ack('dbc.close()', true);
ack('txn.commit()', true);

var txn = dbenv.txn_begin(null, 0);
ack('typeof txn', "object");
ack('txn instanceof rpmtxn.Txn', true);
ack('txn.name', undefined);
ack('txn.name = "john"', true);
ack('txn.name', 'john');
nack('txn.id', 0);
ack('txn.DB_SET_LOCK_TIMEOUT', undefined);
ack('txn.DB_SET_TXN_TIMEOUT', undefined);
ack('txn.abort()', true);

var txn = dbenv.txn_begin(null, 0);
ack('typeof txn', "object");
ack('txn instanceof rpmtxn.Txn', true);
ack('txn.name', undefined);
ack('txn.name = "ringo"', true);
ack('txn.name', 'ringo');
nack('txn.id', 0);
ack('txn.DB_SET_LOCK_TIMEOUT', undefined);
ack('txn.DB_SET_TXN_TIMEOUT', undefined);
ack('db.put(txn, "foo", "bar")', true);
ack('db.exists(txn, "foo")', true);
ack('db.get(txn, "foo")', 'bar');
ack('db.del(txn, "foo")', true);
ack('txn.commit()', true);

var mpf = db.mpf;
ack('mpf.clear_len', 32);
ack('mpf.flags', 0);			// todo++
ack('mpf.ftype', -1);
ack('mpf.lsn_offset', 0);
ack('mpf.maxsize', 0);
ack('mpf.pgcookie', undefined);		// todo++
ack('mpf.priority', 3);
ack('db.close(0)', true);
delete db;

var db = new rpmdb.Db(dbenv, 0);
ack('typeof db;', 'object');
ack('db instanceof rpmdb.Db;', true);
ack('db.upgrade(dbfile)', true);
ack('db.verify(dbfile, dbname,0)', true);
// ack('db.truncate()', true);
delete db;

// var db = new rpmdb.Db(dbenv, 0);
// ack('typeof db;', 'object');
// ack('db instanceof rpmdb.Db;', true);
// ack('db.remove(dbfile, dbname)', true);
// delete db;

ack('dbenv.close(0)', true);
delete dbenv;

if (loglvl) print("<-- Db.js");
