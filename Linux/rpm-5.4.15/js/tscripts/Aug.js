if (loglvl) print("--> Aug.js");

const AUG_NONE = 0;
const AUG_SAVE_BACKUP	= (1 << 0);	/* Keep original file as *.augsave */
const AUG_SAVE_NEWFILE	= (1 << 1);	/* Save new file as *.augnew */
const AUG_TYPE_CHECK	= (1 << 2);	/* Typecheck lenses (expensive) */
const AUG_NO_STDINC	= (1 << 3);	/* Do not use the builtin load path */
const AUG_SAVE_NOOP	= (1 << 4);	/* Record (but don't perform) save */
const AUG_NO_LOAD	= (1 << 5);	/* Do not load the tree from AUG_INIT */
const AUG_NO_MODL_AUTOLOAD = (1 << 6);

// var rpmaug = require("rpmaug");

var aug = new Aug("/", "", (AUG_NO_LOAD|AUG_SAVE_NEWFILE));
ack("typeof aug;", "object");
ack("aug instanceof Aug;", true);
ack("aug.debug = 1;", 1);
ack("aug.debug = 0;", 0);

var _defvar = "/augeas/version/defvar";
ack("aug.defvar('foo', 'bar')", "foo");
ack("aug.defvar('foo')", "foo");

ack("aug.hosts = '/files/etc/hosts/*'", undefined);
aug.localhost = "127.0.0.1";
ack('aug.localhost', '127.0.0.1');

ack("aug.load()", true);

var passwd = "/files/etc/passwd";
var rpc = passwd+"/rpc";
var rpcname = rpc+"/name";
ack("aug.print(rpc)", true);
ack("aug.get(rpcname)", "Rpcbind Daemon");
ack("aug.set(rpcname, 'Yadda Yadda')", "Yadda Yadda");
ack("aug.get(rpcname)", "Yadda Yadda");

var wild = passwd+"/rpc/*";
ack("aug.match(wild)", '/files/etc/passwd/rpc/password,/files/etc/passwd/rpc/uid,/files/etc/passwd/rpc/gid,/files/etc/passwd/rpc/name,/files/etc/passwd/rpc/home,/files/etc/passwd/rpc/shell');

var newname = "newname";
var rpcnewname = rpc+"/"+newname;
ack("aug.insert(rpcname, newname, 0)", true);
ack("aug.set(rpcnewname, 'George')", "George");
ack("aug.get(rpcnewname)", "George");

var newrpc = passwd+"/newrpc";
var newrpcnewname = newrpc+"/"+newname;
ack("aug.mv(rpc, newrpc)", true);
ack("aug.get(newrpcnewname)", "George");
ack("aug.rm(newrpc)", 8);

// XXX return depends on root in order to rewrite changed files. */
ack("aug.save()", undefined);

delete aug;

if (loglvl) print("<-- Aug.js");
