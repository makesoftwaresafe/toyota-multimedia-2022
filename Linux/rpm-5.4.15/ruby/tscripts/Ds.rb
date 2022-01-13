if ($loglvl)
  print("--> Ds.rb\n");
end

# $ds = Ds.new('rpmlib')
$ds = Ds.new();
ack("$ds.instance_of?(Ds)", true)
ack("$ds.kind_of?(Ds)", true)
ack("$ds.class.to_s", "Ds")
ack("$ds.debug = 1;", 1);
ack("$ds.debug = 0;", 0);

ack("$ds.length", 25);
# FIXME: what to call the method?
ack("$ds.Type", "Provides");
ack("$ds.buildtime = 1234;", 1234);
ack("$ds.color = 4;", 4);
ack("$ds.nopromote = 1;", 1);
ack("$ds.nopromote = 0;", 0);

ack("$ds.ix = -1;", -1);
ack("$ds.ix += 1;", 0);
ack("$ds.ix += 1;", 1);
ack("$ds.ix = 1;", 1);

ack("$ds.N", "BuiltinFiclScripts");
ack("$ds.EVR", "5.2-1");
ack("$ds.F", 16777224);
ack("$ds.DNEVR", "P rpmlib(BuiltinFiclScripts) = 5.2-1");

if ($loglvl)
  print("<-- Ds.rb\n");
end
