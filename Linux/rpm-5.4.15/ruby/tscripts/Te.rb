if ($loglvl)
  print("--> Te.rb\n");
end

$RPMTAG_NAME = 1000;
$N = "popt";

$te = Te.new
ack("$te.instance_of?(Te)", true)
ack("$te.kind_of?(Te)", true)
ack("$te.class.to_s", "Te")
ack("$te.debug = 1;", 1);
ack("$te.debug = 0;", 0);

if ($loglvl)
  print("<-- Te.rb\n");
end
