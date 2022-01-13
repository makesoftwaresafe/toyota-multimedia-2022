if ($loglvl)
  print("--> Mi.rb\n");
end

$RPMTAG_NAME = 1000;
$N = "popt";

# $mi = $ts.mi();
# $mi.each { |h| puts h['name'] }

$mi = $ts.mi($RPMTAG_NAME, $N);
# $mi = Mi.new($ts, $RPMTAG_NAME, $N);
ack("$mi.instance_of?(Mi)", true)
ack("$mi.kind_of?(Mi)", true)
ack("$mi.class.to_s", "Mi")
ack("$mi.debug = 1;", 1);
ack("$mi.debug = 0;", 0);

ack("$mi.length", 1);
ack("$mi.count", 1);
ack("$mi.instance", 0);	# zero before iterating
ack("$mi.pattern($RPMTAG_NAME, $N)", true);

$h = $mi.next()
ack("$mi.instance", 0);	# non-zero when iterating

ack("$h['name']", $N);

if ($loglvl)
  print("<-- Mi.rb\n");
end
