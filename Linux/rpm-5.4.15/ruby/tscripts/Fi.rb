if ($loglvl)
  print("--> Fi.rb\n");
end

$RPMTAG_NAME = 1000;
$N = "popt";

$ts = Ts.new();
$mi = $ts.mi($RPMTAG_NAME, $N);
# $mi = Mi.new($ts, $RPMTAG_NAME, $NN);
ack("$mi.instance_of?(Mi)", true)
ack("$mi.kind_of?(Mi)", true)
ack("$mi.class.to_s", "Mi")

ack("$mi.length", 1);
ack("$mi.count", 1);
ack("$mi.instance", 0);  # zero before iterating

# $h = Hdr.new();
$h = $mi.next()
ack("$h.instance_of?(Hdr)", true)
ack("$h.kind_of?(Hdr)", true)
ack("$h.class.to_s", "Hdr")
ack("$mi.instance", 0); # non-zero when iterating

$fi = Fi.new($ts, $h);
ack("$fi.instance_of?(Fi)", true)
ack("$fi.kind_of?(Fi)", true)
ack("$fi.class.to_s", "Fi")
ack("$fi.debug = 1;", 1);
ack("$fi.debug = 0;", 0);

ack("$fi.fc", nil);
ack("$fi.fx", nil);
ack("$fi.dc", nil);
ack("$fi.dx", nil);

ack("$fi.dx = 0;", 0);
ack("$fi.dx += 1;", 1);
ack("$fi.dx += 1;", 2);

ack("$fi.fx = -1;", -1);
ack("$fi.fx += 1;", 0);
ack("$fi.fx += 1;", 1);

ack("$fi.bn", nil);
ack("$fi.dn", nil);
ack("$fi.fn", nil);
ack("$fi.vflags", nil);
ack("$fi.fflags", nil);
ack("$fi.fmode", nil);
ack("$fi.fstate", nil);
ack("$fi.fdigest", 'FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF');
ack("$fi.flink", nil);
ack("$fi.fsize", nil);
ack("$fi.frdev", nil);
ack("$fi.fmtime", nil);
ack("$fi.fuser", nil);
ack("$fi.fgroup", nil);
ack("$fi.fcolor", nil);
ack("$fi.fclass", "symbolic link");

if ($loglvl)
  print("<-- Fi.rb\n");
end
