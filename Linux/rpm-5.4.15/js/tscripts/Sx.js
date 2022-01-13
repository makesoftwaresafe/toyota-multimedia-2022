if (loglvl) print("--> Sx.js");

var rpmsx = require('rpmsx');

var sx = new rpmsx.Sx();
ack("typeof sx;", "object");
ack("sx instanceof rpmsx.Sx;", true);
ack("sx.debug = 1;", 1);
ack("sx.debug = 0;", 0);

print("    enabled: "+sx.enabled);
print(" mlsenabled: "+sx.mlsenabled);
print("    current: "+sx.current);
print("        pid: "+sx.pid);
print("       ppid: "+sx.ppid);
print("       prev: "+sx.prev);
print("       exec: "+sx.exec);
print("   fscreate: "+sx.fscreate);
print("  keycreate: "+sx.keycreate);
print(" sockcreate: "+sx.sockcreate);
print("    enforce: "+sx.enforce);
print("       deny: "+sx.deny);
print(" policyvers: "+sx.policyvers);

print("       path: "+sx.path);
print("       root: "+sx.root);
print("   booleans: "+sx.booleans);
print("   contexts: "+sx.contexts);
print("customtypes: "+sx.customtypes);
print("    default: "+sx.default);
print("   failsafe: "+sx.failsafe);
print("       fcon: "+sx.fcon);
print("   fconhome: "+sx.fconhome);
print("  fconlocal: "+sx.fconlocal);
print("   fconsubs: "+sx.fconsubs);
print("    homedir: "+sx.homedir);
print("      media: "+sx.media);
print("  netfilter: "+sx.netfilter);
print("  removable: "+sx.removable);
print("  securetty: "+sx.securetty);
print("       user: "+sx.user);
print(" virtdomain: "+sx.virtdomain);
print("  virtimage: "+sx.virtimage);
print("          X: "+sx.X);
print("     binary: "+sx.binary);
print("  usersconf: "+sx.usersconf);
print("   xlations: "+sx.xlations);
print("     colors: "+sx.colors);
print("      users: "+sx.users);

ack("sx.current = sx.current;", sx.current);
ack("sx.exec = sx.current;", sx.current);
ack("sx.fscreate = sx.current;", sx.current);
ack("sx.keycreate = sx.current;", sx.current);
ack("sx.sockcreate = sx.current;", sx.current);
ack("sx.enforce = sx.enforce;", sx.enforce);

var fn_bash = '/bin/bash';
var fn_sh = '/bin/sh';
ack("sx(fn_bash);", 'system_u:object_r:shell_exec_t:s0');
ack("sx(fn_sh);", 'system_u:object_r:bin_t:s0');

delete sx;

if (loglvl) print("<-- Sx.js");
