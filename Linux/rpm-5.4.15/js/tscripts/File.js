if (loglvl) print("--> File.js");

var dn = "/tmp";
var bn = "xyzzy";
var fn = dn+"/"+bn;

var f = new File(fn);
ack("typeof f", "object");
ack("f instanceof File;", true);

ack("f.path", fn);
ack("f.isOpen", false);
ack("File.currentDir", undefined);
ack("File.input", "Standard input stream");
ack("File.output", "Standard output stream");
ack("File.error", "Standard error stream");
ack("File.separator", "/");

f.open();
ack("f.isOpen", true);
ack("f.position", 0);
f.write("bing,", "bang,", "boom\n");
ack("f.position", 15);

ack("f.canAppend", true);
ack("f.canRead", true);
ack("f.canReplace", false);
ack("f.exists", true);
ack("f.hasAutoFlush", false);
ack("f.hasRandomAccess", true);
ack("f.isDirectory", false);
ack("f.isFile", true);
ack("f.isNative", false);
ack("f.isOpen", true);
ack("f.lastModified", undefined);
ack("f.length", 15);
ack("f.mode", "read,write,readWrite,append,create");
ack("f.name", bn);
ack("f.parent", dn);
ack("f.path", fn);
ack("f.position", f.length);
ack("f.type", "text");

f.close()
ack("f.isOpen", false);

if (loglvl) print("<-- File.js");
