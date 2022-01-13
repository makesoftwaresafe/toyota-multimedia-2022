if (loglvl) print("--> Sm.js");

const RPMSM_FLAGS_NONE		= 0;
const RPMSM_FLAGS_BASE		= (1 <<  1);    /* -b,--base ... */
const RPMSM_FLAGS_INSTALL	= (1 <<  2);    /* -i,--install ... */
const RPMSM_FLAGS_LIST		= (1 <<  3);    /* -l,--list-modules ... */
const RPMSM_FLAGS_REMOVE	= (1 <<  4);    /* -r,--remove ... */
const RPMSM_FLAGS_UPGRADE	= (1 <<  5);    /* -u,--upgrade ... */
const RPMSM_FLAGS_RELOAD	= (1 <<  6);    /* -R,--reload ... */
const RPMSM_FLAGS_BUILD		= (1 <<  7);    /* -B,--build ... */
const RPMSM_FLAGS_NOAUDIT	= (1 <<  8);    /* -D,--disable_dontaudit ... */
const RPMSM_FLAGS_COMMIT	= (1 <<  9);
const RPMSM_FLAGS_CREATE	= (1 << 10);
const RPMSM_FLAGS_CONNECT	= (1 << 11);
const RPMSM_FLAGS_SELECT	= (1 << 12);
const RPMSM_FLAGS_ACCESS	= (1 << 13);
const RPMSM_FLAGS_BEGIN		= (1 << 14);

const RPMSM_STATE_CLOSED	= 0;
const RPMSM_STATE_SELECTED	= 1;
const RPMSM_STATE_ACCESSED	= 2;
const RPMSM_STATE_CREATED	= 3;
const RPMSM_STATE_CONNECTED	= 4;
const RPMSM_STATE_TRANSACTION	= 5;

var rpmsm = require('rpmsm');

var dn = "/usr/share/selinux";
var store = "targeted";
var flags = RPMSM_FLAGS_SELECT;
var state = RPMSM_STATE_SELECTED;

var sm = new rpmsm.Sm(store, flags);
ack("typeof sm;", "object");
ack("sm instanceof rpmsm.Sm;", true);
ack("sm.debug = 1;", 1);
ack("sm.debug = 0;", 0);

ack("sm.store", store);
ack("sm.flags", flags);
ack("sm.state", state);

var module = 'unconfined';

// var Rcmd = 'Reload ';
// ack('sm(Rcmd);', true);

var lcmd = 'list ' + module + '-.*';
ack('sm(lcmd);', 'unconfined-3.0.1');

var rcmd = 'remove ' + module;
ack('sm(rcmd);', true);

var icmd = 'install ' + dn + '/' + store + '/' + module + '.pp.bz2';
ack('sm(icmd);', true);

var rcmd = 'remove ' + module;
ack('sm(rcmd);', true);

var ucmd = 'upgrade ' + dn + '/' + store + '/' + module + '.pp.bz2';
ack('sm(ucmd);', true);

delete sm;

if (loglvl) print("<-- Sm.js");
