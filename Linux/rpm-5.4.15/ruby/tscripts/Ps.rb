if ($loglvl)
  print("--> Ps.rb\n");
end

$RPMPROB_BADARCH     = 0;
$RPMPROB_BADOS       = 1;
$RPMPROB_PKG_INSTALLED = 2;
$RPMPROB_BADRELOCATE = 3;
$RPMPROB_REQUIRES    = 4;
$RPMPROB_CONFLICT    = 5;
$RPMPROB_NEW_FILE_CONFLICT = 6;
$RPMPROB_FILE_CONFLICT = 7;
$RPMPROB_OLDPACKAGE  = 8;
$RPMPROB_DISKSPACE   = 9;
$RPMPROB_DISKNODES   = 10;
$RPMPROB_RDONLY      = 11;
$RPMPROB_BADPRETRANS = 12;
$RPMPROB_BADPLATFORM = 13;
$RPMPROB_NOREPACKAGE = 14;

$ps = Ps.new
ack("$ps.instance_of?(Ps)", true)
ack("$ps.kind_of?(Ps)", true)
ack("$ps.class.to_s", "Ps")
ack("$ps.debug = 1;", 1);
ack("$ps.debug = 0;", 0);

$PKG = "PKG";
$ALT = "ALT";
$KEY = "/path/pkg";
$dn = "/root/";
$bn = "file";
$ui = 1234;
$R = "R ALT";
$C = "C ALT";
$A = "ARCH/";
$O = "OS";

ack("$ps.length", 0);
# ack("$ps.push($PKG, $ALT, $KEY, $RPMPROB_BADARCH, $dn, $bn, $ui)", nil);
# ack("$ps.push($PKG, $ALT, $KEY, $RPMPROB_BADOS, $dn, $bn, $ui)", nil);
ack("$ps.push($PKG, $ALT, $KEY, $RPMPROB_PKG_INSTALLED, $dn, $bn, $ui)", nil);
ack("$ps.push($PKG, $ALT, $KEY, $RPMPROB_BADRELOCATE, $dn, $bn, $ui)", nil);
ack("$ps.push($PKG,   $R, $KEY, $RPMPROB_REQUIRES, $dn, $bn, $ui)", nil);
ack("$ps.push($PKG,   $C, $KEY, $RPMPROB_CONFLICT, $dn, $bn, $ui)", nil);
ack("$ps.push($PKG, $ALT, $KEY, $RPMPROB_NEW_FILE_CONFLICT, $dn, $bn, $ui)", nil);
ack("$ps.push($PKG, $ALT, $KEY, $RPMPROB_FILE_CONFLICT, $dn, $bn, $ui)", nil);
ack("$ps.push($PKG, $ALT, $KEY, $RPMPROB_OLDPACKAGE, $dn, $bn, $ui)", nil);
ack("$ps.push($PKG, $ALT, $KEY, $RPMPROB_DISKSPACE, $dn, $bn, $ui)", nil);
ack("$ps.push($PKG, $ALT, $KEY, $RPMPROB_DISKNODES, $dn, $bn, $ui)", nil);
ack("$ps.push($PKG, $ALT, $KEY, $RPMPROB_RDONLY, $dn, $bn, $ui)", nil);
# ack("$ps.push($PKG, $ALT, $KEY, $RPMPROB_BADPRETRANS, $dn, $bn, $ui)", nil);
ack("$ps.push($PKG, $ALT, $KEY, $RPMPROB_BADPLATFORM, $A, $O, $ui)", nil);
ack("$ps.push($PKG, $ALT, $KEY, $RPMPROB_NOREPACKAGE, $dn, $bn, $ui)", nil);
ack("$ps.length", 12);

if ($loglvl)
  print("<-- Ps.rb\n");
end
