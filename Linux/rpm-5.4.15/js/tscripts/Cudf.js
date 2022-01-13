if (loglvl) print("--> Cudf.js");

var RPMCUDV_CUDFDOC	= 257;
var RPMCUDV_CUDF	= 258;

var GPSEE = require('rpmcudf');

var cudf = new GPSEE.Cudf();
ack("typeof cudf;", "object");
ack("cudf instanceof GPSEE.Cudf;", true);
ack("cudf.debug = 1;", 1);
ack("cudf.debug = 0;", 0);

ack('cudf.haspreamble', false);
ack('cudf.hasrequest', false);
ack('cudf.isconsistent', false);
ack('cudf.installedsize', 0);
ack('cudf.universesize', 0);
ack('cudf.preamble', undefined);
ack('cudf.request', undefined);
ack('cudf.universe', undefined);
delete cudf

var ufn = "legacy.cudf";
cudf = new GPSEE.Cudf(ufn, RPMCUDV_CUDFDOC);
// print(cudf.preamble);
// print(cudf.request);
// print(cudf.universe);
delete cudf

cudf = new GPSEE.Cudf(ufn);
ack('cudf.haspreamble', true);
ack('cudf.hasrequest', true);
ack('cudf.isconsistent', true);
ack('cudf.installedsize', 6);
ack('cudf.universesize', 20);
ack('cudf.preamble', undefined);
ack('cudf.request', undefined);
ack('cudf.universe', undefined);
delete cudf

var sfn = "legacy-sol.cudf";

cudf = new GPSEE.Cudf(sfn);
ack('cudf.haspreamble', false);
ack('cudf.hasrequest', false);
ack('cudf.isconsistent', true);
ack('cudf.installedsize', 6);
ack('cudf.universesize', 6);
ack('cudf.preamble', undefined);
ack('cudf.request', undefined);
ack('cudf.universe', undefined);
delete cudf

cudf = new GPSEE.Cudf(ufn);
ack('cudf.issolution(sfn)', true);
delete cudf

if (loglvl) print("<-- Cudf.js");
