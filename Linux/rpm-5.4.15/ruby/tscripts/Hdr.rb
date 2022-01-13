if ($loglvl)
  print("--> Hdr.rb\n");
end

$RPMTAG_NAME = 1000;
$RPMTAG_REQUIRENAME = 1049;
$RPMTAG_BASENAMES = 1117;

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
ack("$mi.instance", 0); # non-zero when iterating
ack("$h.instance_of?(Hdr)", true)
ack("$h.kind_of?(Hdr)", true)
ack("$h.class.to_s", "Hdr")
ack("$ds.debug = 1;", 1);
ack("$ds.debug = 0;", 0);

$ds = $h.ds();
ack("$ds.instance_of?(Ds)", true)
ack("$ds.kind_of?(Ds)", true)
ack("$ds.class.to_s", "Ds")

$ds = $h.ds($RPMTAG_NAME);
ack("$ds.instance_of?(Ds)", true)
ack("$ds.kind_of?(Ds)", true)
ack("$ds.class.to_s", "Ds")

$ds = $h.ds($RPMTAG_REQUIRENAME);
ack("$ds.instance_of?(Ds)", true)
ack("$ds.kind_of?(Ds)", true)
ack("$ds.class.to_s", "Ds")

$fi = $h.fi();
ack("$fi.instance_of?(Fi)", true)
ack("$fi.kind_of?(Fi)", true)
ack("$fi.class.to_s", "Fi")

$fi = $h.fi($RPMTAG_BASENAMES);
ack("$fi.instance_of?(Fi)", true)
ack("$fi.kind_of?(Fi)", true)
ack("$fi.class.to_s", "Fi")

ack("$h['name']", $N);
ack("$h['epoch']", nil);
ack("$h['version']", nil);
ack("$h['release']", nil);
ack("$h['summary']", nil);
ack("$h['description']", nil);
ack("$h['buildtime']", nil);
ack("$h['buildhost']", nil);
ack("$h['installtime']", nil);
ack("$h['size']", nil);
ack("$h['distribution']", nil);
ack("$h['vendor']", nil);
ack("$h['license']", nil);
ack("$h['packager']", nil);
ack("$h['group']", nil);

ack("$h['changelogtime']", nil);
ack("$h['changelogname']", nil);

ack("$h['url']", nil);
ack("$h['os']", nil);
ack("$h['arch']", nil);

ack("$h['prein']", nil);
ack("$h['preinprog']", nil);
ack("$h['postin']", nil);
ack("$h['postinprog']", nil);
ack("$h['preun']", nil);
ack("$h['preunprog']", nil);
ack("$h['postun']", nil);
ack("$h['postunprog']", nil);

# ack("$h['filenames']", nil);
ack("$h['dirnames']", nil);
ack("$h['basenames']", nil);

ack("$h['filesizes']", nil);
ack("$h['filestates']", nil);
ack("$h['filemodes']", nil);
ack("$h['filerdevs']", nil);

ack("$h['sourcerpm']", nil);
ack("$h['archivesize']", nil);

ack("$h['providename']", nil);
ack("$h['requirename']", nil);
ack("$h['conflictname']", nil);
ack("$h['obsoletesname']", nil);

ack("$h.keys()", nil);
$legacyHeader = true;
ack("$h.unload($legacyHeader)", nil);
$origin = "http://rpm5.org/files/popt/popt-1.14-1.i386.rpm";
ack("$h.setorigin($origin)", $origin);
ack("$h.getorigin()", $origin);
$qfmt = "%{buildtime:date}";
ack("$h.sprintf($qfmt)", nil);

if ($loglvl)
  print("<-- Hdr.rb\n");
end
