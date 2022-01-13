if (loglvl) print("--> Mpw.js");

var rpmmpw = require('rpmmpw');

var mpw = new rpmmpw.Mpw();
ack("typeof mpw;", "object");
ack("mpw instanceof Mpw;", true);
// ack("mpw.debug = 1;", 1);
// ack("mpw.debug = 0;", 0);

// ===== Basic tests
var wa = mpw('0000000987654321');
ack('wa.toString(16)', '0x987654321');
ack('wa.toString(10)', '40926266145')
ack('wa.toString(8)', '0460731241441')
ack('wa.toString(2)', '2#100110000111011001010100001100100001')

var wb = mpw(0x10);
ack('wb.toString(16)', '0x10');
var wc = mpw('fedcba000000000');
ack('wc.toString(16)', '0xfedcba000000000');

var za = 0x0000000007654321;
var wa = mpw(za);
var zb = 0x0000000000000010;
var wb = mpw(zb);
var zc = 0;

ack('mpw(wa, wb, "+").toString(10)', (za + zb).toString(10));
ack('mpw(wa, wb, "-").toString(10)', (za - zb).toString(10));
ack('mpw(wa, wb, "*").toString(10)', (za * zb).toString(10));
ack('mpw(wa, wb, "/").toString(10)', Math.floor(za / zb).toString(10));
ack('mpw(wa, wb, "%").toString(10)', (za % zb).toString(10));
ack('mpw(wa, wb, "**").toString(10)', Math.pow(za, zb).toString(10));
ack('mpw(wa, wb, "<<").toString(10)', (za << zb).toString(10));
ack('mpw(wa, wb, ">>").toString(10)', (za >> zb).toString(10));
ack('mpw(wa, wb, "&").toString(10)', (za & zb).toString(10));
ack('mpw(wa, wb, "^").toString(10)', (za ^ zb).toString(10));
ack('mpw(wa, wb, "|").toString(10)', (za | zb).toString(10));

// XXX FIXME divide-by-zero exception handling
// var foo = mpw(1, 0, "/");
// var foo = mpw(1, 0, "%");

var m = mpw(15);
ack('m.toString(10)', '15');
ack('m.isPrime()', false);
ack('m.toString(10)', m.valueOf());
var x = mpw(7);
ack('x.toString(10)', '7');
ack('x.toString(10)', x.valueOf());
var y = mpw(13);
ack('y.toString(10)', '13');
ack('y.toString(10)', y.valueOf());

ack('mpw(x, m, "invm").toString(10)', '13');
ack('mpw(x, m, "sqrm").toString(10)', '4');

ack('mpw(x, y, m, "addm").toString(10)', '5');
ack('mpw(x, y, m, "subm").toString(10)', '9');
ack('mpw(x, y, m, "mulm").toString(10)', '1');
ack('mpw(x, y, m, "powm").toString(10)', '7');
 
var zx = 2*3*5*19;
// zx *= zx;
// zx *= zx;
// FIXME segfault, check gcd scaling for zy
// zx *= zx;
var zy = 7*11*13*19;
ack('mpw(zx, zy, "gcd").toString(10)', '19');

ack('mpw(zx, "neg").toString(10)', (-zx).toString(10));
ack('mpw(zx, "abs").toString(10)', Math.abs(zx).toString(10));
ack('mpw(zx, "~").toString(10)', (~zx).toString(10));

zx = -1.2345678901234567890123456789012345678901234567890123456789e10;
ack('mpw(zx*zx, "sqrt").toString(10)', Math.floor(Math.sqrt(zx*zx)).toString(10));
zx = -1.2345678901234e10;
ack('mpw(zx*zx, "sqrt").toString(10)', Math.floor(Math.sqrt(zx*zx)).toString(10));
// for (zx = 0; zx < 1025; zx++) {
//      ack('mpw(zx*zx, "sqrt").toString(10)', Math.sqrt(zx*zx).toString(10));
// }
// for (var i = 0; i < 128; i++) {
// zx = (1 << i) - 1;
// ack('mpw(zx*zx, "sqrt").toString(10)', Math.sqrt(zx*zx).toString(10));
// }

// ===== Carry/borrow tests
var a = 0x7fffffff;
wa = mpw(-a, -a, "+");
var za = (-a) + (-a);
wb = mpw(-1);
var zb = -1;
wc = mpw(1);
var zc = 1;
wd = mpw(a, a, "+");
var zd = a + a;

ack('mpw(wa, wa, "+").toString(10)', (za + za).toString(10));
ack('mpw(wb, wd, "+").toString(10)', (zb + zd).toString(10));
ack('mpw(wc, wa, "+").toString(10)', (zc + za).toString(10));
ack('mpw(wd, wd, "+").toString(10)', (zd + zd).toString(10));
 
ack('mpw(wb, wa, "-").toString(10)', (zb - za).toString(10));
ack('mpw(wc, wd, "-").toString(10)', (zc - zd).toString(10));

// ===== Signs
var wpa = mpw(13);
var wma = mpw(-13);
var wpb = mpw(wpa, 3, "-");
var wmb = mpw(-10);
var zpa = 13;
var zma = -zpa;
var zpb = zpa - 3;
var zmb = -zpb;

ack('mpw(wma, wmb, "+").toString(10)', (zma + zmb).toString(10));
ack('mpw(wma, wpb, "+").toString(10)', (zma + zpb).toString(10));
ack('mpw(wpa, wmb, "+").toString(10)', (zpa + zmb).toString(10));
ack('mpw(wpa, wpb, "+").toString(10)', (zpa + zpb).toString(10));

ack('mpw(wma, wmb, "-").toString(10)', (zma - zmb).toString(10));
ack('mpw(wma, wpb, "-").toString(10)', (zma - zpb).toString(10));
ack('mpw(wpa, wmb, "-").toString(10)', (zpa - zmb).toString(10));
ack('mpw(wpa, wpb, "-").toString(10)', (zpa - zpb).toString(10));
ack('mpw(wmb, wma, "-").toString(10)', (zmb - zma).toString(10));
ack('mpw(wmb, wpa, "-").toString(10)', (zmb - zpa).toString(10));
ack('mpw(wpb, wma, "-").toString(10)', (zpb - zma).toString(10));
ack('mpw(wpb, wpa, "-").toString(10)', (zpb - zpa).toString(10));
 
ack('mpw(wma, wmb, "*").toString(10)', (zma * zmb).toString(10));
ack('mpw(wma, wpb, "*").toString(10)', (zma * zpb).toString(10));
ack('mpw(wpa, wmb, "*").toString(10)', (zpa * zmb).toString(10));
ack('mpw(wpa, wpb, "*").toString(10)', (zpa * zpb).toString(10));
 
ack('mpw(wma, wmb, "/").toString(10)', Math.floor(zma/zmb).toString(10));
ack('mpw(wma, wpb, "/").toString(10)', Math.floor(zma/zpb).toString(10));
ack('mpw(wpa, wmb, "/").toString(10)', Math.floor(zpa/zmb).toString(10));
ack('mpw(wpa, wpb, "/").toString(10)', Math.floor(zpa/zpb).toString(10));
ack('mpw(wmb, wma, "/").toString(10)', Math.floor(zmb/zma).toString(10));
ack('mpw(wmb, wpa, "/").toString(10)', Math.floor(zmb/zpa).toString(10));
ack('mpw(wpb, wma, "/").toString(10)', Math.floor(zpb/zma).toString(10));
ack('mpw(wpb, wpa, "/").toString(10)', Math.floor(zpb/zpa).toString(10));
 
ack('mpw(wma, wmb, "**").toString(10)', Math.floor(Math.pow(zma, zmb)).toString(10));
ack('mpw(wma, wpb, "**").toString(10)', Math.pow(zma, zpb).toString(10));
ack('mpw(wpa, wmb, "**").toString(10)', Math.floor(Math.pow(zpa, zmb)).toString(10));
ack('mpw(wpa, wpb, "**").toString(10)', Math.pow(zpa, zpb).toString(10));
ack('mpw(wmb, wma, "**").toString(10)', Math.floor(Math.abs(Math.pow(zmb, zma))).toString(10));
ack('mpw(wmb, wpa, "**").toString(10)', Math.pow(zmb, zpa).toString(10));
ack('mpw(wpb, wma, "**").toString(10)', Math.floor(Math.pow(zpb, zma)).toString(10));
ack('mpw(wpb, wpa, "**").toString(10)', Math.pow(zpb, zpa).toString(10));
 
ack('mpw(wma, wmb, "%").toString(10)', (zma % zmb).toString(10));
ack('mpw(wma, wpb, "%").toString(10)', (zma % zpb).toString(10));
ack('mpw(wpa, wmb, "%").toString(10)', (zpa % zmb).toString(10));
ack('mpw(wpa, wpb, "%").toString(10)', (zpa % zpb).toString(10));
ack('mpw(wmb, wma, "%").toString(10)', (zmb % zma).toString(10));
ack('mpw(wmb, wpa, "%").toString(10)', (zmb % zpa).toString(10));
ack('mpw(wpb, wma, "%").toString(10)', (zpb % zma).toString(10));
ack('mpw(wpb, wpa, "%").toString(10)', (zpb % zpa).toString(10));

ack('mpw.eq(wma, wma)', zma == zma);
ack('mpw.ne(wma, wma)', zma != zma);
ack('mpw.eq(wma, wpa)', zma == zpa);
ack('mpw.ne(wma, wpa)', zma != zpa);
ack('mpw.eq(wpa, wma)', zpa == zma);
ack('mpw.ne(wpa, wma)', zpa != zma);
ack('mpw.eq(wpa, wpa)', zpa == zpa);
ack('mpw.ne(wpa, wpa)', zpa != zpa);

ack('mpw.lt(wma, wma)', zma < zma);
ack('mpw.le(wma, wma)', zma <= zma);
ack('mpw.gt(wma, wma)', zma > zma);
ack('mpw.ge(wma, wma)', zma >= zma);
ack('mpw.lt(wma, wpa)', zma < zpa);
ack('mpw.le(wma, wpa)', zma <= zpa);
ack('mpw.gt(wma, wpa)', zma > zpa);
ack('mpw.ge(wma, wpa)', zma >= zpa);
ack('mpw.lt(wpa, wma)', zpa < zma);
ack('mpw.le(wpa, wma)', zpa <= zma);
ack('mpw.gt(wpa, wma)', zpa > zma);
ack('mpw.ge(wpa, wma)', zpa >= zma);
ack('mpw.lt(wpa, wpa)', zpa < zpa);
ack('mpw.le(wpa, wpa)', zpa <= zpa);
ack('mpw.gt(wpa, wpa)', zpa > zpa);
ack('mpw.ge(wpa, wpa)', zpa >= zpa);

ack('mpw.min(zma, zmb)', Math.min(zma, zmb));
ack('mpw.min(zma, zpb)', Math.min(zma, zpb));
ack('mpw.min(zpa, zmb)', Math.min(zpa, zmb));
ack('mpw.min(zpa, zpb)', Math.min(zpa, zpb));
ack('mpw.max(zma, zmb)', Math.max(zma, zmb));
ack('mpw.max(zma, zpb)', Math.max(zma, zpb));
ack('mpw.max(zpa, zmb)', Math.max(zpa, zmb));
ack('mpw.max(zpa, zpb)', Math.max(zpa, zpb));

var za = 5;
var zb = 3;
// FIXME
// ack('mpw(+za, 0, "%").toString(10)', za.toString(10));
ack('mpw(+5, +3, "%").toString(10)', '2');
ack('mpw(+5, -3, "%").toString(10)', '-1');
ack('mpw(-5, +3, "%").toString(10)', '1');
ack('mpw(-5, -3, "%").toString(10)', '-2');
function mathmod(x, y) { return (x - y * Math.floor(x/y)); }
ack('mpw(+za, +zb, "%").toString(10)', mathmod(+za, +zb).toString(10));
ack('mpw(+za, -zb, "%").toString(10)', mathmod(+za, -zb).toString(10));
ack('mpw(-za, +zb, "%").toString(10)', mathmod(-za, +zb).toString(10));
ack('mpw(-za, -zb, "%").toString(10)', mathmod(-za, -zb).toString(10));

ack('mpw(+za, +zb, "%").toString(10)', (+za % +zb).toString(10));
ack('mpw(+za, -zb, "%").toString(10)', (+za % -zb).toString(10));
ack('mpw(-za, +zb, "%").toString(10)', (-za % +zb).toString(10));
ack('mpw(-za, -zb, "%").toString(10)', (-za % -zb).toString(10));

// ===== Knuth poly
var lo = 2;
var hi = 10;
// var bases = [8, 10, 16];
var bases = [13];

for (var i in bases) {
    var t = bases[i];
    var tm1 = (t - 1);
    var tm2 = (t - 2);
    print("=====\t("+t+"**m - 1) * ("+t+"**n - 1), m,n in ["+lo+","+hi+")");
    for (var m = lo; m < hi; m++) {
	wb = mpw(t, m, "**", 1, "-");
	for (var n = m+1; n < hi+1; n++) {
	    wc = mpw(t, n, "**", 1, "-");
	    wa = mpw(wb, wc, "*");
//	    print(wa.toString(t));
	    switch (t) {
	    case 8:	zs = '0';	break;
	    case 10:	zs = '';	break;
	    case 16:	zs = '0x';	break;
	    default:	zs = t.toString(10) + "#";	break;
	    }
	    for (var j = 0; j < (m - 1); j++) zs += (t - 1).toString(t);
	    zs += (t - 2).toString(t);
	    for (var j = 0; j < (n - m); j++) zs += (t - 1).toString(t);
	    for (var j = 0; j < (m - 1); j++) zs += "0";
	    zs += "1";
//	    print(zs);
	    ack('wa.toString(t)', zs);
	}
    }
}

// =======================================================
function RSA(p, q, e) {
  this.p = mpw(p);
  ack('mpw(p).isPrime()', true);
  this.pm1 = mpw(this.p, 1, "-");
  this.q = mpw(q);
  ack('mpw(q).isPrime()', true);
  this.qm1 = mpw(this.q, 1, "-");
  this.e = mpw(e);

  this.n = mpw(this.p, this.q, "*");
  this.phi = mpw(this.pm1, this.qm1, "*");
// ack('mpw(this.e, this.phi, "gcd").toString(10)', '1');
  this.d = mpw(this.e, this.phi, "invm");
// ack('mpw(this.e, this.d, "*", this.phi, "%").toString(10)', '1');

  this.sign =
    function (hm) {
	this.hm = mpw(hm);
	this.s = mpw(this.hm, this.d, this.n, "powm");
	return this.s;
    };
  this.verify =
    function (hm, s) {
	return (mpw(s, this.e, this.n, "powm").toString(10) == hm.toString(10)
		? true : false);
    }
  return true;
}

function RSAv21(p, q, d, e, dP, dQ, qInv) {
  this.p = mpw(p);
  ack('mpw(p).isPrime()', true);
  this.pm1 = mpw(this.p, 1, "-");
  this.q = mpw(q);
  ack('mpw(q).isPrime()', true);
  this.qm1 = mpw(this.q, 1, "-");
  this.d = mpw(d);
  this.e = mpw(e);

  this.n = mpw(this.p, this.q, "*");
  this.lambda = mpw(this.p, 1, "-", this.q, 1, "-", "*");
//  ack('mpw(this.n, this.lambda, "gcd").toString(10)', '1');
//  ack('mpw(this.e, this.lambda, "gcd").toString(10)', '1');

//  this.d = mpw(this.e, this.lambda, "invm");
//  ack('mpw(this.e, this.lambda, "invm").toString(16)', d.toString(16));

//  ack('mpw(this.e, this.d, this.lambda, "mulm").toString(10)', '1');

    this.dP = mpw(this.e, this.p, 1, "-", "invm");
    ack('mpw(this.e, this.p, 1, "-", "invm").toString(16)', dP.toString(16));
    ack('mpw(this.e, this.dP, this.p, 1, "-", "mulm").toString(10)', '1');
    ack('mpw(this.e, this.dP, "*", this.p, 1, "-", "%").toString(10)', '1');

    this.dQ = mpw(this.e, this.q, 1, "-", "invm");
    ack('mpw(this.e, this.q, 1, "-", "invm").toString(16)', dQ.toString(16));
    ack('mpw(this.e, this.dQ, this.q, 1, "-", "mulm").toString(10)', '1');
    ack('mpw(this.e, this.dQ, "*", this.q, 1, "-", "%").toString(10)', '1');

    this.qInv = mpw(this.q, this.p, "invm");
    ack('this.qInv.toString(16)', qInv.toString(16));
    ack('mpw(this.q, this.qInv, this.p, "mulm").toString(10)', '1');
  
  this.sign =
    function (hm) {
	this.hm = mpw(hm);
	this.s = mpw(this.hm, this.d, this.n, "powm");
	return this.s;
    };
  this.verify =
    function (hm, s) {
	return (mpw(s, this.e, this.n, "powm").toString(10) == hm.toString(10)
		? true : false);
    }
  return this;
}

// ===== RSA example (from "Handbook of Applied Cryptography" 11.20 p434).
var p   = 7927;
var q   = 6997;
var n   = 55465219;
ack('p * q', n);
var e   = 5;
var phi = 55450296;
ack('(p - 1) * (q - 1)', phi);
var d   = 44360237;
var hm  = 31229978;
var s   = 30729435;

print("===== RSA");
var rsa = new RSA(p, q, e);

ack('rsa.verify(hm, rsa.sign(hm))', true);

ack('rsa.p.toString(10)', p.toString(10));
ack('rsa.q.toString(10)', q.toString(10));
ack('rsa.n.toString(10)', n.toString(10));
ack('rsa.e.toString(10)', e.toString(10));
ack('rsa.phi.toString(10)', phi.toString(10));
ack('rsa.d.toString(10)', d.toString(10));
ack('rsa.hm.toString(10)', hm.toString(10));
ack('rsa.s.toString(10)', s.toString(10));

// ====== RSA using PKCS #1 v1.5 examples from
// 	http://www.wigiwigi.com/index.php?title=RSA_in_32bits

// ===== Encryption
rsa.n = mpw(
	'a9e167983f39d55ff2a093415ea6798985c8355d9a915bfb1d01da197026170f'+
	'bda522d035856d7a986614415ccfb7b7083b09c991b81969376df9651e7bd9a9'+
	'3324a37f3bbbaf460186363432cb07035952fc858b3104b8cc18081448e64f1c'+
	'fb5d60c4e05c1f53d37f53d86901f105f87a70d1be83c65f38cf1c2caa6aa7eb');
rsa.e = 0x10001;
rsa.d = mpw(
	'67cd484c9a0d8f98c21b65ff22839c6df0a6061dbceda7038894f21c6b0f8b35'+
	'de0e827830cbe7ba6a56ad77c6eb517970790aa0f4fe45e0a9b2f419da8798d6'+
	'308474e4fc596cc1c677dca991d07c30a0a2c5085e217143fc0d073df0fa6d14'+
	'9e4e63f01758791c4b981c3d3db01bdffa253ba3c02c9805f61009d887db0319');

var PS =    '257f48fd1f1793b7e5e02306f2d3228f5c95adf5f31566729f132aa12009'+
	'e3fc9b2b475cd6944ef191e3f59545e671e474b555799fe3756099f044964038'+
	'b16b2148e9a2f9c6f44bb5c52e3c6c8061cf694145fafdb24402ad1819eacedf'+
	'4a36c6e4d2cd8fc1d62e5a1268f496';
var D = '4e636af98e40f3adcfccb698f4e80b9f';

//	em = 00 || 02 || PS || 00 || D
var em = mpw('00' + '02' + PS + '00' + D);
var c = mpw(
	'3d2ab25b1eb667a40f504cc4d778ec399a899c8790edecef062cd739492c9ce5'+
	'8b92b9ecf32af4aac7a61eaec346449891f49a722378e008eff0b0a8dbc6e621'+
	'edc90cec64cf34c640f5b36c48ee9322808af8f4a0212b28715c76f3cb99ac7e'+
	'609787adce055839829e0142c44b676d218111ffe69f9d41424e177cba3a435b');

ack('mpw(em, rsa.e, rsa.n, "powm").toString(16)',  c.toString(16));
ack('mpw( c, rsa.d, rsa.n, "powm").toString(16)', em.toString(16));

// ===== Signing
rsa.n = mpw(
	'E08973398DD8F5F5E88776397F4EB005BB5383DE0FB7ABDC7DC775290D052E6D'+
	'12DFA68626D4D26FAA5829FC97ECFA82510F3080BEB1509E4644F12CBBD832CF'+
	'C6686F07D9B060ACBEEE34096A13F5F7050593DF5EBA3556D961FF197FC981E6'+
	'F86CEA874070EFAC6D2C749F2DFA553AB9997702A648528C4EF357385774575F');
rsa.e = 0x10001;
rsa.d = mpw(
	'00A403C327477634346CA686B57949014B2E8AD2C862B2C7D748096A8B91F736'+
	'F275D6E8CD15906027314735644D95CD6763CEB49F56AC2F376E1CEE0EBF282D'+
	'F439906F34D86E085BD5656AD841F313D72D395EFE33CBFF29E4030B3D05A28F'+
	'B7F18EA27637B07957D32F2BDE8706227D04665EC91BAF8B1AC3EC9144AB7F21');

var M = 'abc';
var PS =    'FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF'+
	'FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF'+
	'FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF';
var asn1 = '3021300906052B0E03021A05000414';
var hm = 'A9993E364706816ABA3E25717850C26C9CD0D89D';
var T = asn1 + hm;

//	eb = 00 || 01 || PS || 00 || T
var eb = mpw('00' + '01' + PS + '00' + T);
var c = mpw(
	'60AD5A78FB4A4030EC542C8974CD15F55384E836554CEDD9A322D5F4135C6267'+
	'A9D20970C54E6651070B0144D43844C899320DD8FA7819F7EBC6A7715287332E'+
	'C8675C136183B3F8A1F81EF969418267130A756FDBB2C71D9A667446E34E0EAD'+
	'9CF31BFB66F816F319D0B7E430A5F2891553986E003720261C7E9022C0D9F11F');

ack('mpw(eb, rsa.d, rsa.n, "powm").toString(16)',  c.toString(16));
ack('mpw( c, rsa.e, rsa.n, "powm").toString(16)', eb.toString(16));

delete rsa;

// ===== RSAES-OAEP examples from
//	ftp://ftp.rsasecurity.com/pub/pkcs/pkcs-1/pkcs-1v2-1-vec.zip
// var n   = mpw(
// 	'a8b3b284af8eb50b387034a860f146c4919f318763cd6c5598c8ae4811a1e0ab'+
// 	'c4c7e0b082d693a5e7fced675cf4668512772c0cbc64a742c6c630f533c8cc72'+
// 	'f62ae833c40bf25842e984bb78bdbf97c0107d55bdb662f5c4e0fab9845cb514'+
// 	'8ef7392dd3aaff93ae1e6b667bb3d4247616d4f5ba10d4cfd226de88d39f16fb');
// var e   = 0x10001;
// var d   = mpw(
// 	'53339cfdb79fc8466a655c7316aca85c55fd8f6dd898fdaf119517ef4f52e8fd'+
// 	'8e258df93fee180fa0e4ab29693cd83b152a553d4ac4d1812b8b9fa5af0e7f55'+
// 	'fe7304df41570926f3311f15c4d65a732c483116ee3d3d2d0af3549ad9bf7cbf'+
// 	'b78ad884f84d5beb04724dc7369b31def37d0cf539e9cfcdd3de653729ead5d1');
// var p   = mpw(
// 	'd32737e7267ffe1341b2d5c0d150a81b586fb3132bed2f8d5262864a9cb9f30a'+
// 	'f38be448598d413a172efb802c21acf1c11c520c2f26a471dcad212eac7ca39d');
// var q   = mpw(
// 	'cc8853d1d54da630fac004f471f281c7b8982d8224a490edbeb33d3e3d5cc93c'+
// 	'4765703d1dd791642f1f116a0dd852be2419b2af72bfe9a030e860b0288b5d77');
// var dP   = mpw(
// 	'0e12bf1718e9cef5599ba1c3882fe8046a90874eefce8f2ccc20e4f2741fb0a3'+
// 	'3a3848aec9c9305fbecbd2d76819967d4671acc6431e4037968db37878e695c1');
// var dQ   = mpw(
// 	'95297b0f95a2fa67d00707d609dfd4fc05c89dafc2ef6d6ea55bec771ea33373'+
// 	'4d9251e79082ecda866efef13c459e1a631386b7e354c899f5f112ca85d71583');
// var qInv = mpw(
// 	'4f456c502493bdc0ed2ab756a3a6ed4d67352a697d4216e93212b127a63d5411'+
// 	'ce6fa98d5dbefd73263e3728142743818166ed7dd63687dd2a8ca1d2f4fbd8e1');
// var m    = mpw('6628194e12073db03ba94cda9ef9532397d50dba79b987004afefe34');
// var h    = mpw('8176326046e571e18464d875262420772782ac3a');
// var seed = mpw('18b776ea21069d69776a33e96bad48e1dda0a5ef');
// var s    = mpw(
// 	'354fe67b4a126d5d35fe36c777791a3f7ba13def484e2d3908aff722fad468fb'+
// 	'21696de95d0be911c2d3174f8afcc201035f7b6d8e69402de5451618c21a535f'+
// 	'a9d7bfc5b8dd9fc243f8cf927db31322d6e881eaa91a996170e657a05a266426'+
// 	'd98c88003f8477c1227094a0d9fa1e8c4024309ce1ecccb5210035d47ac72e8a');

//=================
var n   = mpw(
	'bbf82f090682ce9c2338ac2b9da871f7368d07eed41043a440d6b6f07454f51f'+
	'b8dfbaaf035c02ab61ea48ceeb6fcd4876ed520d60e1ec4619719d8a5b8b807f'+
	'afb8e0a3dfc737723ee6b4b7d93a2584ee6a649d060953748834b2454598394e'+
	'e0aab12d7b61a51f527a9a41f6c1687fe2537298ca2a8f5946f8e5fd091dbdcb');
var e   = 0x11;
var d   = mpw(
	'a5dafc5341faf289c4b988db30c1cdf83f31251e0668b42784813801579641b2'+
	'9410b3c7998d6bc465745e5c392669d6870da2c082a939e37fdcb82ec93edac9'+
	'7ff3ad5950accfbc111c76f1a9529444e56aaf68c56c092cd38dc3bef5d20a93'+
	'9926ed4f74a13eddfbe1a1cecc4894af9428c2b7b8883fe4463a4bc85b1cb3c1');
var p   = mpw(
	'eecfae81b1b9b3c908810b10a1b5600199eb9f44aef4fda493b81a9e3d84f632'+
	'124ef0236e5d1e3b7e28fae7aa040a2d5b252176459d1f397541ba2a58fb6599');
var q   = mpw(
	'c97fb1f027f453f6341233eaaad1d9353f6c42d08866b1d05a0f2035028b9d86'+
	'9840b41666b42e92ea0da3b43204b5cfce3352524d0416a5a441e700af461503');
var dP   = mpw(
	'54494ca63eba0337e4e24023fcd69a5aeb07dddc0183a4d0ac9b54b051f2b13e'+
	'd9490975eab77414ff59c1f7692e9a2e202b38fc910a474174adc93c1f67c981');
var dQ   = mpw(
	'471e0290ff0af0750351b7f878864ca961adbd3a8a7e991c5c0556a94c3146a7'+
	'f9803f8f6f8ae342e931fd8ae47a220d1b99a495849807fe39f9245a9836da3d');
var qInv = mpw(
	'b06c4fdabb6301198d265bdbae9423b380f271f73453885093077fcd39e2119f'+
	'c98632154f5883b167a967bf402b4e9e2e0f9656e698ea3666edfb25798039f7');
var msg   = mpw('d436e99569fd32a7c8a05bbc90d32c49');
var lhash = mpw('da39a3ee5e6b4b0d3255bfef95601890afd80709');
var db = mpw(
	'da39a3ee5e6b4b0d3255bfef95601890afd80709000000000000000000000000'+
	'0000000000000000000000000000000000000000000000000000000000000000'+
	'000000000000000000000000000000000000000000000000000001d436e99569'+
	'fd32a7c8a05bbc90d32c49');
var seed = mpw('aafd12f659cae63489b479e5076ddec2f06cb58f');

var em = mpw(
	'00eb7a19ace9e3006350e329504b45e2ca82310b26dcd87d5c68f1eea8f55267'+
	'c31b2e8bb4251f84d7e0b2c04626f5aff93edcfb25c9c2b3ff8ae10e839a2ddb'+
	'4cdcfe4ff47728b4a1b7c1362baad29ab48d2869d5024121435811591be392f9'+
	'82fb3e87d095aeb40448db972f3ac14f7bc275195281ce32d2f1b76d4d353e2d');
var c    = mpw(
	'1253e04dc0a5397bb44a7ab87e9bf2a039a33d1e996fc82a94ccd30074c95df7'+
	'63722017069e5268da5d1c0b4f872cf653c11df82314a67968dfeae28def04bb'+
	'6d84b1c31d654a1970e5783bd6eb96a024c2ca2f4a90fe9f2ef5c9c140e5bb48'+
	'da9536ad8700c84fc9130adea74e558d51a74ddf85d8b50de96838d6063e0955');
var cmodp = mpw(
	'de63d4723566faa759bfe408821dd52572ec92854ddf87a2b664d44daa37ca34'+
	'6a05203d82ff2de8e36cec1d34f98eb605e2a7d26de7af369ce4ecae14e35633');
var cmodq = mpw(
	'a2d924ded9c36d623ed9a65b5d862cfbec8b199c64279c5414e641196ef1c93c'+
	'507a9b5213881aad05b4ccfa028ac1ec61420974bf1625836b0b7d05fbb75336');
var m1 = mpw(
	'896ca26cd7e4871c7fc968a8edea11e271824f0e0365521794f1e9e943b4a44b'+
	'57c9e395a1467478f526496b4bb91f1cbaea900ffc602cf0c6636eba84fc9ff7');
var m2 = mpw(
	'4ebb227585f0c1312dca19e0b541db1499fbf14e270e698e239a8c27a96cda9a'+
	'740974de937b5c9c93ead9462c6575021a23d46499dc9f6b35897559608f19be');
var h = mpw(
	'012b2b24150e76e159bd8ddb4276e07bfac188e08d6047cf0efb8ae2aebdf251'+
	'c40ebc23dcfd4a34424394ada92cfcbe1b2effbb60fdfb03359a95368d980925');
var m = mpw(
	'00eb7a19ace9e3006350e329504b45e2ca82310b26dcd87d5c68f1eea8f55267'+
	'c31b2e8bb4251f84d7e0b2c04626f5aff93edcfb25c9c2b3ff8ae10e839a2ddb'+
	'4cdcfe4ff47728b4a1b7c1362baad29ab48d2869d5024121435811591be392f9'+
	'82fb3e87d095aeb40448db972f3ac14f7bc275195281ce32d2f1b76d4d353e2d');

print("===== RSAES-OAEP");
var rsa = new RSA(p, q, e);
ack('rsa.n.toString(16)', n.toString(16));
ack('rsa.e.toString(10)', e.toString(10));
ack('rsa.d.toString(16)', d.toString(16));
ack('rsa.p.toString(16)', p.toString(16));
ack('rsa.q.toString(16)', q.toString(16));

ack('mpw(em, rsa.e, rsa.n, "powm").toString(16)',  c.toString(16));
ack('mpw( c, rsa.d, rsa.n, "powm").toString(16)', em.toString(16));

ack('mpw(c, rsa.p, "%").toString(16)', cmodp.toString(16));
ack('mpw(c, rsa.q, "%").toString(16)', cmodq.toString(16));

// ack('mpw(c, rsa.dP, rsa.p, "powm").toString(16)', m1.toString(16));
// ack('mpw(cmodp, rsa.dP, rsa.p, "powm").toString(16)', m1.toString(16));
// ack('mpw(c, rsa.dQ, rsa.q, "powm").toString(16)', m2.toString(16));
// ack('mpw(cmodq, rsa.dQ, rsa.q, "powm").toString(16)', m2.toString(16));

// ack('mpw(m1, m2, "-", rsa.qInv, "*", rsa.p, "%").toString(16)', h.toString(16));

ack('mpw(m2, rsa.q, h, "*", "+").toString(16)', em.toString(16));

delete rsa;

print("===== RSAES-OAEP 2.1");
var rsa = new RSAv21(p, q, d, e, dP, dQ, qInv);

ack('rsa.n.toString(16)', n.toString(16));
ack('rsa.e.toString(10)', e.toString(10));
ack('rsa.d.toString(16)', d.toString(16));
ack('rsa.p.toString(16)', p.toString(16));
ack('rsa.q.toString(16)', q.toString(16));

ack('rsa.dP.toString(16)', dP.toString(16));
ack('rsa.dQ.toString(16)', dQ.toString(16));
ack('rsa.qInv.toString(16)', qInv.toString(16));

// ack('rsa.verify(hm, rsa.sign(hm))', true);
ack('mpw(em, rsa.e, rsa.n, "powm").toString(16)',  c.toString(16));
ack('mpw( c, rsa.d, rsa.n, "powm").toString(16)', em.toString(16));

// ack('rsa.phi.toString(10)', phi.toString(10));
// ack('rsa.hm.toString(10)', hm.toString(10));
// ack('rsa.s.toString(10)', s.toString(10));

delete rsa;

// ===== RSASSA-PSS example from
//	ftp://ftp.rsasecurity.com/pub/pkcs/pkcs-1/pkcs-1v2-1-vec.zip
// var n    = mpw(
// 	'a56e4a0e701017589a5187dc7ea841d156f2ec0e36ad52a44dfeb1e61f7ad991'+
// 	'd8c51056ffedb162b4c0f283a12a88a394dff526ab7291cbb307ceabfce0b1df'+
// 	'd5cd9508096d5b2b8b6df5d671ef6377c0921cb23c270a70e2598e6ff89d19f1'+
// 	'05acc2d3f0cb35f29280e1386b6f64c4ef22e1e1f20d0ce8cffb2249bd9a2137');
// var e   = 0x10001;
// var d    = mpw(
// 	'33a5042a90b27d4f5451ca9bbbd0b44771a101af884340aef9885f2a4bbe92e8'+
// 	'94a724ac3c568c8f97853ad07c0266c8c6a3ca0929f1e8f11231884429fc4d9a'+
// 	'e55fee896a10ce707c3ed7e734e44727a39574501a532683109c2abacaba283c'+
// 	'31b4bd2f53c3ee37e352cee34f9e503bd80c0622ad79c6dcee883547c6a3b325');
// var p    = mpw(
// 	'e7e8942720a877517273a356053ea2a1bc0c94aa72d55c6e86296b2dfc967948'+
// 	'c0a72cbccca7eacb35706e09a1df55a1535bd9b3cc34160b3b6dcd3eda8e6443');
// var q    = mpw(
// 	'b69dca1cf7d4d7ec81e75b90fcca874abcde123fd2700180aa90479b6e48de8d'+
// 	'67ed24f9f19d85ba275874f542cd20dc723e6963364a1f9425452b269a6799fd');
// var dP   = mpw(
// 	'28fa13938655be1f8a159cbaca5a72ea190c30089e19cd274a556f36c4f6e19f'+
// 	'554b34c077790427bbdd8dd3ede2448328f385d81b30e8e43b2fffa027861979');
// var dQ   = mpw(
// 	'1a8b38f398fa712049898d7fb79ee0a77668791299cdfa09efc0e507acb21ed7'+
// 	'4301ef5bfd48be455eaeb6e1678255827580a8e4e8e14151d1510a82a3f2e729');
// var qInv = mpw(
// 	'27156aba4126d24a81f3a528cbfb27f56886f840a9f6e86e17a44b94fe931958'+
// 	'4b8e22fdde1e5a2e3bd8aa5ba8d8584194eb2190acf832b847f13a3d24a79f4d');
// var hm  = 31229978;
// var s   = mpw(
// 	'9074308fb598e9701b2294388e52f971faac2b60a5145af185df5287b5ed2887'+
// 	'e57ce7fd44dc8634e407c8e0e4360bc226f3ec227f9d9e54638e8d31f5051215'+
// 	'df6ebb9c2f9579aa77598a38f914b5b9c1bd83c4e2f9f382a0d0aa3542ffee65'+
// 	'984a601bc69eb28deb27dca12c82c2d4c3f66cd500f1ff2b994d8a4e30cbb33c');

//=================
var n    = mpw(
	'a2ba40ee07e3b2bd2f02ce227f36a195024486e49c19cb41bbbdfbba98b22b0e'+
	'577c2eeaffa20d883a76e65e394c69d4b3c05a1e8fadda27edb2a42bc000fe88'+
	'8b9b32c22d15add0cd76b3e7936e19955b220dd17d4ea904b1ec102b2e4de775'+
	'1222aa99151024c7cb41cc5ea21d00eeb41f7c800834d2c6e06bce3bce7ea9a5');
var e   = 0x10001;
var d    = mpw(
	'50e2c3e38d886110288dfc68a9533e7e12e27d2aa56d2cdb3fb6efa990bcff29'+
	'e1d2987fb711962860e7391b1ce01ebadb9e812d2fbdfaf25df4ae26110a6d7a'+
	'26f0b810f54875e17dd5c9fb6d641761245b81e79f8c88f0e55a6dcd5f133abd'+
	'35f8f4ec80adf1bf86277a582894cb6ebcd2162f1c7534f1f4947b129151b71');
var p    = mpw(
	'd17f655bf27c8b16d35462c905cc04a26f37e2a67fa9c0ce0dced472394a0df7'+
	'43fe7f929e378efdb368eddff453cf007af6d948e0ade757371f8a711e278f6b');
var q    = mpw(
	'c6d92b6fee7414d1358ce1546fb62987530b90bd15e0f14963a5e2635adb6934'+
	'7ec0c01b2ab1763fd8ac1a592fb22757463a982425bb97a3a437c5bf86d03f2f');
var dP   = mpw(
	'9d0dbf83e5ce9e4b1754dcd5cd05bcb7b55f1508330ea49f14d4e889550f8256'+
	'cb5f806dff34b17ada44208853577d08e4262890acf752461cea05547601bc4f');
var dQ   = mpw(
	'1291a524c6b7c059e90e46dc83b2171eb3fa98818fd179b6c8bf6cecaa476303'+
	'abf283fe05769cfc495788fe5b1ddfde9e884a3cd5e936b7e955ebf97eb563b1');
var qInv = mpw(
	'a63f1da38b950c9ad1c67ce0d677ec2914cd7d40062df42a67eb198a176f9742'+
	'aac7c5fea14f2297662b84812c4defc49a8025ab4382286be4c03788dd01d69f');
var msg  = mpw(
	'859eef2fd78aca00308bdc471193bf55bf9d78db8f8a672b484634f3c9c26e64'+
	'78ae10260fe0dd8c082e53a5293af2173cd50c6d5d354febf78b26021c25c027'+
	'12e78cd4694c9f469777e451e7f8e9e04cd3739c6bbfedae487fb55644e9ca74'+
	'ff77a53cb729802f6ed4a5ffa8ba159890fc');
var mhash = mpw('37b66ae0445843353d47ecb0b4fd14c110e62d6a');
var salt = mpw('e3b5d5d002c1bce50c2b65ef88a188d83bce7e61');
var em = mpw(
	'66e4672e836ad121ba244bed6576b867d9a447c28a6e66a5b87dee7fbc7e65af'+
	'5057f86fae8984d9ba7f969ad6fe02a4d75f7445fefdd85b6d3a477c28d24ba1'+
	'e3756f792dd1dce8ca94440ecb5279ecd3183a311fc896da1cb39311af37ea4a'+
	'75e24bdbfd5c1da0de7cecdf1a896f9d8bc816d97cd7a2c43bad546fbe8cfebc');
var c = mpw(
	'8daa627d3de7595d63056c7ec659e54406f10610128baae821c8b2a0f3936d54'+
	'dc3bdce46689f6b7951bb18e840542769718d5715d210d85efbb596192032c42'+
	'be4c29972c856275eb6d5a45f05f51876fc6743deddd28caec9bb30ea99e02c3'+
	'488269604fe497f74ccd7c7fca1671897123cbd30def5d54a2b5536ad90a747e');
var cmodp = mpw(
	'3e4f9f1d6075fe401607694fee634b4115d42d06d6d627987204610da92a8e1a'+
	'2056fb78fe627fea87e6622eb6fa0461e398153ed978e56add8df1f5a6c31c19');
var cmodq = mpw(
	'25710bf7a5d3c479bc36e783b2be445281d0c7ac5c3a9fcacd94f45e7a183c55'+
	'2d931040c6f7b3bcd1a0e43ae22322dfe71e1b3af7cab021f6915178f9744199');

print("===== RSASSA-PSS");
var rsa = new RSA(p, q, e);
ack('rsa.n.toString(16)', n.toString(16));
ack('rsa.e.toString(10)', e.toString(10));
ack('rsa.d.toString(16)', d.toString(16));
ack('rsa.p.toString(16)', p.toString(16));
ack('rsa.q.toString(16)', q.toString(16));

// ack('rsa.verify(hm, rsa.sign(hm))', true);
ack('mpw(em, rsa.d, rsa.n, "powm").toString(16)',  c.toString(16));
ack('mpw( c, rsa.e, rsa.n, "powm").toString(16)', em.toString(16));

ack('mpw(c, rsa.p, "%").toString(16)', cmodp.toString(16));
ack('mpw(c, rsa.q, "%").toString(16)', cmodq.toString(16));

delete rsa;

print("===== RSASSA-PSS 2.1");
var rsa = new RSAv21(p, q, d, e, dP, dQ, qInv);

ack('rsa.n.toString(16)', n.toString(16));
ack('rsa.e.toString(10)', e.toString(10));
ack('rsa.d.toString(16)', d.toString(16));
ack('rsa.p.toString(16)', p.toString(16));
ack('rsa.q.toString(16)', q.toString(16));
ack('rsa.dP.toString(16)', dP.toString(16));
ack('rsa.dQ.toString(16)', dQ.toString(16));
ack('rsa.qInv.toString(16)', qInv.toString(16));

// ack('rsa.verify(hm, rsa.sign(hm))', true);
ack('mpw(em, rsa.d, rsa.n, "powm").toString(16)', c.toString(16));
ack('mpw( c, rsa.e, rsa.n, "powm").toString(16)', em.toString(16));

ack('mpw(c, rsa.p, "%").toString(16)', cmodp.toString(16));
ack('mpw(c, rsa.q, "%").toString(16)', cmodq.toString(16));

delete rsa;

// =======================================================
function ElGamal(p, alpha, a, k) {
  this.p = mpw(p);
  ack('mpw(p).isPrime()', true);
  this.pm1 = mpw(p, 1, "-");
  this.alpha = mpw(alpha);
  this.a = mpw(a);
  this.k = mpw(k);

  this.y = mpw(this.alpha, this.a, this.p, "powm");

  this.sign =
    function (hm) {
	this.hm = mpw(hm);
	this.r = mpw(this.alpha, this.k, this.p, "powm");
	this.kinv = mpw(this.k, this.pm1, "invm");
//	ack('mpw(this.k, this.kinv, this.pm1, "mulm").toString(10)', '1');
	this.s = mpw(this.hm, this.a, this.r, this.pm1, "mulm", this.pm1, "subm", this.kinv, this.pm1, "mulm");
	return true;
    };

  this.verify =
    function (hm) {
	this.hm = mpw(hm);
	this.v1 = mpw(this.y, this.r, this.p, "powm", this.r, this.s, this.p, "powm", this.p, "mulm");
	this.v2 = mpw(this.alpha, this.hm, this.p, "powm");
	return (elgamal.v2.toString(10) == elgamal.v1.toString(10)
		? true : false);
    }

  return this;
}

// ===== ElGamal example (from "Handbook of Applied Cryptography" 11.65 p455).
// Keygen ElGamal
print("===== ElGamal");
var p     = 2357;
var alpha = 2;
var a     = 1751;
var y     = 1185;
var hm    = 1463;
var k     = 1529;
var r     = 1490;
var s     = 1777;
var v1    = 1072;

var pm1 = p - 1;

var elgamal = new ElGamal(p, alpha, a, k);
ack('elgamal.p.toString(10)', p.toString(10));
ack('elgamal.alpha.toString(10)', alpha.toString(10));
ack('elgamal.a.toString(10)', a.toString(10));
ack('elgamal.k.toString(10)', k.toString(10));
ack('elgamal.y.toString(10)', y.toString(10));

ack('elgamal.sign(hm)', true);
ack('elgamal.r.toString(10)', r.toString(10));
var kinv = mpw(k, pm1, "invm");
ack('elgamal.kinv.toString(10)', kinv.toString(10));
ack('elgamal.s.toString(10)', s.toString(10));

ack('elgamal.verify(hm)', true);
ack('elgamal.v1.toString(10)', v1.toString(10));
ack('elgamal.v2.toString(10)', v1.toString(10));

delete elgamal;

// =======================================================
function DSA(p, q, g) {
  this.p = mpw(p);
  ack('mpw(p).isPrime()', true);
  this.q = mpw(q);
  ack('mpw(q).isPrime()', true);
  this.g = mpw(g);

//  ack('mpw(this.p, 1, "-", this.q, "%").toString(10)', '0');
//  this.pdivq = mpw(this.p, 1, "-", this.q, "/");
//  this.alpha = mpw(this.g, this.pdivq, this.p, "powm");

//  this.x = mpw(this.c, this.q, 1, "-", "%", 1, "+");
//  ack('mpw(this.c, this.q, 1, "-", "%", 1, "+").toString(16)', this.x.toString(16));
//  this.y = mpw(this.alpha, this.a, this.p, "powm");
//  ack('mpw(this.g, this.x, this.p, "powm").toString(16)', this.y.toString(16));

  this.sign =
    function (hm) {
	this.hm = mpw(hm);
//	this.k = this.q.randomK();
//	this.kinv = mpw(this.k, this.q, "invm");
	ack('mpw(this.k, this.kinv, this.q, "mulm").toString(10)', '1');
	this.r = mpw(this.g, this.k, this.p, "powm", this.q, "%");
	this.s = mpw(this.x, this.r, this.q, "mulm", this.hm, this.q, "addm", this.kinv, this.q, "mulm");
	return true;
    };

  this.verify =
    function (hm) {
	this.hm = mpw(hm);
	this.w = mpw(this.s, this.q, "invm");
	this.u1 = mpw(this.w, this.hm, this.q, "mulm");
	this.u2 = mpw(this.r, this.w, this.q, "mulm");
	this.v1 = mpw(this.g, this.u1, this.p, "powm");
	this.v2 = mpw(this.y, this.u2, this.p, "powm");
	this.v = mpw(this.v1, this.v2, this.p, "mulm", this.q, "%");

	return (this.v.toString(10) == this.r.toString(10)
		? true : false);
    }
  return this;
}

// ===== DSA example (from "Handbook of Applied Cryptography" 11.57 p453).
// print("===== DSA");
// var p     = 124540019;
// var q     = 17389;
// var g     = 110217528;
// var a     = 12496;
// var pdivq = 7162;
// var alpha = 10083255;
// var y     = 119946265;
 
// var hm    = 5246;
// var k     = 9557;
// var r     = 34;
// var kinv  = 7631;
// var s     = 13049;
 
// var w     = 1799;
// var u1    = 12716;
// var u2    = 8999;
// var v     = 27039929;
 
// var dsa = new DSA(p, q, g, a, k);
 
// ack('dsa.p.toString(10)', p.toString(10));
// ack('dsa.q.toString(10)', q.toString(10));
// ack('dsa.g.toString(10)', g.toString(10));
// ack('dsa.a.toString(10)', a.toString(10));
// ack('dsa.k.toString(10)', k.toString(10));
// ack('dsa.pdivq.toString(10)', pdivq.toString(10));
// ack('dsa.alpha.toString(10)', alpha.toString(10));
// ack('dsa.y.toString(10)', y.toString(10));
 
// ack('dsa.sign(hm)', true);
// ack('dsa.hm.toString(10)', hm.toString(10));
// ack('dsa.r.toString(10)', r.toString(10));
// ack('dsa.kinv.toString(10)', kinv.toString(10));
// ack('dsa.s.toString(10)', s.toString(10));
 
// ack('dsa.verify(hm)', true);
// ack('dsa.w.toString(10)', w.toString(10));
// ack('dsa.u1.toString(10)', u1.toString(10));
// ack('dsa.u2.toString(10)', u2.toString(10));
// ack('dsa.v.toString(10)', v.toString(10));
 
// delete dsa;

// ===== DSA example from 
//     http://csrc.nist.gov/groups/ST/toolkit/documents/Examples/DSA2_All.pdf
print("===== DSA");
var p = mpw('e0a67598cd1b763b'+
	'c98c8abb333e5dda0cd3aa0e5e1fb5ba8a7b4eabc10ba338'+
	'fae06dd4b90fda70d7cf0cb0c638be3341bec0af8a7330a3'+
	'307ded2299a0ee606df035177a239c34a912c202aa5f83b9'+
	'c4a7cf0235b5316bfc6efb9a248411258b30b839af172440'+
	'f32563056cb67a861158ddd90e6a894c72a5bbef9e286c6b');
var q = mpw('e950511eab424b9a19a2aeb4e159b7844c589c4f');
var g = mpw('d29d5121b0423c27'+
	'69ab21843e5a3240ff19cacc792264e3bb6be4f78edd1b15'+
	'c4dff7f1d905431f0ab16790e1f773b5ce01c804e509066a'+
	'9919f5195f4abc58189fd9ff987389cb5bedf21b4dab4f8b'+
	'76a055ffe2770988fe2ec2de11ad92219f0b351869ac24da'+
	'3d7ba87011a701ce8ee7bfe49486ed4527b7186ca4610a75');
var c = mpw('f3b4c192'+
	'385620feec46cb7f5d55fe0c231b0404a61539729ea1291c');

var dsa = new DSA(p, q, g);

var x = mpw('d0ec4e50bb290a42e9e355c73d8809345de2e139');
var y = mpw('25282217f5730501'+
	'dd8dba3edfcf349aaffec20921128d70fac44110332201bb'+
	'a3f10986140cbb97c726938060473c8ec97b4731db004293'+
	'b5e730363609df9780f8d883d8c4d41ded6a2f1e1bbbdc97'+
	'9e1b9d6d3c940301f4e978d65b19041fcf1e8b518f5c0576'+
	'c770fe5a7a485d8329ee2914a2de1b5da4a6128ceab70f79');
dsa.c = mpw(c);
dsa.x = mpw(dsa.c, dsa.q, 1, "-", "%", 1, "+");
dsa.y = mpw(dsa.g, dsa.x, dsa.p, "powm");
ack('dsa.x.toString(16)', x.toString(16));
ack('dsa.y.toString(16)', y.toString(16));

var hm   = mpw('a9993e364706816aba3e25717850c26c9cd0d89d');
var k    = mpw('349c55648dcf992f3f33e8026cfac87c1d2ba075');
var kinv = mpw('d557a1b4e7346c4a55427a28d47191381c269bde');
var r    = mpw('636155ac9a4633b4665d179f9e4117df68601f34');
var s    = mpw('6c540b02d9d4852f89df8cfc99963204f4347704');

dsa.k = mpw(k);
dsa.kinv = mpw(kinv);
ack('dsa.sign(hm)', true);
ack('dsa.r.toString(16)', r.toString(16));
ack('dsa.s.toString(16)', s.toString(16));

var w  = mpw('7e4f353b71d1a3dc2946f6bae3b9285ef736ce34');
var u1 = mpw('00abfd278373edf8ce08347d49cd81308880103e');
var u2 = mpw('0da3ab84ae3ff412d703a63b41d1ec61d64b061c');
var v1 = mpw('9c26b54969e24166'+
	'd89b06f5bec3b0df8179e4f9cf7606f67162edd150f73a1f'+
	'9e09e49d21ab0d04b4a02e2446c47bc9311e38c80effd862'+
	'fb69fe39eab9fc270b494a575e0d0862bef45df1a826a448'+
	'8bac5b3757e9c3513dd4d965e6b0ee18811bc711013cf4ea'+
	'beb578878c133783f80342cafa147b0c1cc6e51e937c8d11');
var v2 = mpw('841232c0aa641d81'+
	'74aa4619826357f66fdf66e9a9b2a7c832e901d04b07b0e3'+
	'7c35596fe9e873e7888af879aa4795f558d9be9aa6cc302c'+
	'b29b9ccb03d72c5ebcfda30e03f3ab574d3c31c161b2fa41'+
	'd31f49f9d2df72a5a91378455b0a852614b6e40b3c4d1cfe'+
	'81e28ea6ce9d563b7506413cd29bfb7bde64e2f14828a631');
var v3 = mpw(v1, v2, p, "mulm");
var v  = mpw('636155ac9a4633b4665d179f9e4117df68601f34');

ack('dsa.verify(hm)', true);
ack('dsa.w.toString(16)', w.toString(16));
ack('dsa.u1.toString(16)', u1.toString(16));
ack('dsa.u2.toString(16)', u2.toString(16));
ack('dsa.v1.toString(16)', v1.toString(16));
ack('dsa.v2.toString(16)', v2.toString(16));
ack('dsa.v.toString(16)', v.toString(16));

dsa.p = mpw(
	'8ca8d2c0c04101b8bd0e079dd8bae6890c60ecfee4be2600c43de5401906f20a'+
	'9c25870a3c2544c33ba6080eaa1e442f0c148f20c3cc52b2e068fcb7ffaa7203');
dsa.q = mpw('f37f99c87899a57e9af91beb78e47bde64028309');
dsa.g = mpw(
	'1d667d15baae47abd369b8fcff5a80ec874b10ce204cc636843c74c5b9fac935'+
	'd4f279b79bb05f2abdc75df9eeb884c585041b4dc8d883ae0e7e45843b7e36ce');

var hm = mpw('f49a5dac15284e7fc72d66f85dbf3d1a5b97b575');
dsa.x = mpw('ac19b8c465a05b2bbca71e8e36ee70d31ad5e946');
dsa.y = mpw(
	'1bfdc2e1627153415eaf74aeb89a938e9352b50a601d5fbdfaffe3d8d1f5ef99'+
	'7e93675d10a10b9a882475d7bf9756f8d75b5dd9cd0185a28fdaaeabdf28946e');
dsa.r = mpw('11f0065438ec658438caa16178250c93d4bb750f');
dsa.s = mpw('47693a4c58b1564590afe893272a4c6672af2656');
ack('dsa.verify(hm)', true);

var hm = mpw('82153920a1378c06ba9a34b99b4704ab622064a1');
dsa.x = mpw('83f10c8644a9390d6b6c05539ef0731159973870');
dsa.y = mpw(
	'0a79f8fbc27416188f3996ee1e68749859337cc0330b1e598e75301e4f1bb02c'+
	'd5bd9467d96f28d7aab0847641ff3b32111bc52ca6fdd62c7f1b2995cc4c2887');
dsa.r = mpw('593aea363bbe8479cd3ac77018d5f799215fd784');
dsa.s = mpw('369642bf2850334eeeb045db96f77a7733aefb7e');
ack('dsa.verify(hm)', true);

delete dsa

// =======================================================
function CurveFp(p, a, b) {
    this.p = mpw(p);
    this.a = mpw(a);
    this.b = mpw(b);

  this.contains =
    function (x, y) {
        if (x == undefined && y == undefined)
            return true;
	var t1 = mpw(y, y, "*");
	var t2 = mpw(x, x, "*", this.a, "+", x, "*", this.b, "+");
	// XXX FIXME negative 0
	t1 = mpw(t1, t2, "-", this.p, "%", "abs");
	return (mpw.eq(t1, 0));
    }

    return this;
}

// =======================================================
function PointFp(C, x, y, n) {
    this.C = C;
    this.x = (x != undefined ? mpw(x) : undefined);
    this.y = (y != undefined ? mpw(y) : undefined);
    this.z = mpw(1);
    this.n = (n != undefined ? mpw(n) : undefined);
    if (!C.contains(this.x, this.y))
	print("NACK: ", this.x, this.y);

  this.cmp =
    function (p1, p2) {
	if (p1.x == undefined || p1.y == undefined || p2.x == undefined || p2.y == undefined)
	    return (p1.x == undefined && p1.y == undefined && p2.x == undefined && p2.y == undefined);
	return (mpw.eq(p1.x, p2.x) && mpw.eq(p1.y, p2.y));
    }

  this.dbl =
    function (P) {
	var C = P.C;
	if (P.x == undefined || P.y == undefined)
	    return new PointFp(C, undefined, undefined, P.n);
	var l = mpw(P.x, P.x, "*", 3, "*", C.a, "+", C.p, "%", 2, P.y, "abs", "*", C.p, "%", C.p, "invm", "*", C.p, "%", "abs");
	if (mpw.lt(P.y, 0))
	    l = mpw(l, "neg");
	var x = mpw(l, l, "*", 2, P.x, "*", "-", C.p, "%");
	var y = mpw(P.x, x, "-", l, "*", P.y, "-", C.p, "%");
//	y = mpw(y, "abs");	// XXX force positive for now
	return new PointFp(C, x, y, P.n);
    }

  this.add =
    function (P, Q) {
	var C = P.C;
	if (Q.x == undefined || Q.y == undefined)
	    return new PointFp(C, P.x, P.y, P.n);
	if (P.x == undefined || P.y == undefined)
	    return new PointFp(C, Q.x, Q.y, Q.n);
	if (mpw.eq(P.x, Q.x)) {
	    if (mpw.eq(mpw(P.y, Q.y, "+", C.p, "%", "abs"), 0))
		return new PointFp(C, undefined, undefined, P.n);
	    return P.dbl(P);
	}
	if (mpw.gt(P.x, Q.x)) {
	    var l = mpw(P.y, Q.y, "-", C.p, "%", P.x, Q.x, "-", C.p, "invm", "*", C.p, "%");
	} else {
	    var l = mpw(Q.y, P.y, "-", C.p, "%", Q.x, P.x, "-", C.p, "invm", "*", C.p, "%");
	}
	var x = mpw(l, l, "*", P.x, Q.x, "+", "-", C.p, "%");
	var y = mpw(l, P.x, x, "-", "*", P.y, "-", C.p, "%");
	return new PointFp(C, x, y, P.n);
    }

  this.sub =
    function (P, Q) {
	if (Q.x == undefined || Q.y == undefined)
	    return new PointFp(P.C, P.x, P.y, P.n);
	N = new PointFp(Q.C, Q.x, mpw(Q.y, "neg"), Q.n);
	return this.add(P, N);
    }

  this.mul =
    function (P, m) {
	var C = P.C;
	var e = mpw(m);
	if (P.n != undefined && !mpw.eq(P.n, 0))
	    e = mpw(e, P.n, "%", "abs");
	if (mpw.eq(e, 0) || P.x == undefined || P.y == undefined)
	    return new PointFp(C, undefined, undefined, P.n);
	var N = new PointFp(C, P.x, mpw(P.y, "neg"), P.n);
	var e3 = mpw(3, e, "*");
	var i = mpw(e3, e3, "^", "not");
	i = mpw(i, i, 1, ">>", "^");
	while (mpw.eq(mpw(e3, i, "&"), 0))
	    i = mpw(i, 1, ">>");
	i = mpw(i, 1, ">>");
	var r = new PointFp(C, P.x, P.y, P.n);
	while (mpw.gt(i, 1)) {
	    r = r.dbl(r);
	    if (!mpw.eq(mpw(e3, i, "&"), 0) &&  mpw.eq(mpw( e, i, "&"), 0))
		r = r.add(r, P);
	    if ( mpw.eq(mpw(e3, i, "&"), 0) && !mpw.eq(mpw( e, i, "&"), 0))
		r = r.add(r, N);
	    i = mpw(i, 1, ">>");
	}
	return r;
    }

    return this;
}

function walkDbl(P, imax) {
    var PP = P.dbl(P);
    for (let i = 1; i < imax; i++)
	PP = P.dbl(PP);
    return PP;
}

function walkAdd(P, Q, imax) {
    var PQ = P.add(P, Q);
    for (let i = 1; i < imax; i++)
	PQ = P.add(PQ, Q);
    return PQ;
}

function walkMul(P, imax) {
    PM = new PointFp(P.C, undefined, undefined, P.n);
    for (let i = 1; i < imax; i++)
	PM = P.mul(P, i);
    return PM;
}

function walkDblMul(P, imax) {
  PP = P.dbl(P);
  for (let i = 1; i < imax; i++) {
    var j = Math.pow(2, i);
    PM = P.mul(P, j);
ack('PP.cmp(PP,PM)', true);
    PP = PP.dbl(PP);
  }
  return true;
}

function walkDblAdd(P, imax) {
  var j = 2;
  PQ = P.add(P, P);
  PP = P.dbl(P);
  for (let i = 1; i < imax; i++) {
     while (j < Math.pow(2, i)) {
 	PQ = PQ.add(PQ, P);
	j++;
    }
ack('PP.cmp(PP,PQ)', true);
    PP = PP.dbl(PP);
  }
  return true;
}

function checkP(C, x, y, n) {
    var P = new PointFp(C, x, y, n);
//    var PP = P.sub(P, P);
//    var PP = P.dbl(P);
//    var P1 = P.mul(P, 1);
//    var P2 = P.mul(P, 2);

    walkDbl(P, 10);
    walkDblMul(P, 10);

//    walkAdd(P, P, 10);
//    walkDblAdd(P, 2);

    var N = new PointFp(C, x, mpw(y, "neg"), n);
    var NN = N.sub(N, N);
    var NN = N.dbl(N);
    var N1 = N.mul(N, 1);
    var N2 = N.mul(N, 2);

    walkDbl(N, 10);
    walkDblMul(N, 10);

//    walkAdd(N, N, 1);
//    walkDblAdd(N, 1);

    var NP = N.add(N, P);

    return P;
}

// ===== secp 112r1
print("===== SECP 112r1");
var p    = mpw('db7c2abf62e35e668076bead208b');
ack('mpw(mpw(mpw(2, 128, "**"), 3, "-"), 76439, "/").toString(16)', p.toString(16));
var b    = mpw('659ef8ba043916eede8911702b22');
var n    = mpw('db7c2abf62e35e7628dfac6561c5');
var gx   = mpw('09487239995a5ee76b55f9c2f098');
var gy   = mpw('a89ce5af8724c0a23e0e0ff77500');

C = new CurveFp(p, -3, b);
G = checkP(C, gx, gy, n);
// walkMul(G, 1024);

delete C;
delete G;

// ===== SECP 128r1
print("===== SECP 128r1");
var p    = mpw('fffffffdffffffffffffffffffffffff');
ack('mpw(mpw(mpw(2, 128, "**"), mpw(2, 97, "**"), "-"), 1, "-").toString(16)', p.toString(16));
var b    = mpw('e87579c11079f43dd824993c2cee5ed3');
var n    = mpw('fffffffe0000000075a30d1b9038a115');
var gx   = mpw('161ff7528b899b2d0c28607ca52c5b86');
var gy   = mpw('cf5ac8395bafeb13c02da292dded7a83');

C = new CurveFp(p, -3, b);
G = checkP(C, gx, gy, n);
walkMul(G, 5);

delete C;
delete G;

// ===== SECP 160r1
print("===== SECP 160r1");
var p    = mpw('ffffffffffffffffffffffffffffffff7fffffff');
ack('mpw(mpw(mpw(2, 160, "**"), mpw(2, 31, "**"), "-"), 1, "-").toString(16)', p.toString(16));
var b    = mpw('1c97befc54bd7a8b65acf89f81d4d4adc565fa45');
var n    = mpw('0100000000000000000001f4c8f927aed3ca752257');
var gx   = mpw('4a96b5688ef573284664698968c38bb913cbfc82');
var gy   = mpw('23a628553168947d59dcc912042351377ac5fb32');

C = new CurveFp(p, -3, b);
G = checkP(C, gx, gy, n);
walkMul(G, 53);

delete C;
delete G;

// ===== ECDSA P-192 example from 
//	http://csrc.nist.gov/groups/ST/toolkit/documents/Examples/ECDSA_Prime.pdf
print("===== P-192");
var p    = mpw(mpw(mpw(2, 192, "**"), mpw(2, 64, "**"), "-"), 1, "-");
ack('p.toString(10)', '6277101735386680763835789423207666416083908700390324961279');
var n    = mpw('ffffffffffffffffffffffff99def836146bc9b1b4d22831');
ack('n.toString(10)', '6277101735386680763835789423176059013767194773182842284081');

var c    = mpw('3099d2bbbfcb2538542dcd5fb078b6ef5f3d6fe2c745de65');
var b    = mpw('64210519e59c80e70fa7e9ab72243049feb8deecc146b9b1');
ack('mpw(mpw(b, p, "sqrm"), c, p, "mulm").toString(10)', mpw(p, 27, "-").toString(10));

var gx   = mpw('188da80eb03090f67cbf20eb43a18800f4ff0afd82ff1012');
var gy   = mpw('07192b95ffc8da78631011ed6b24cdd573f977a11e794811');

C = new CurveFp(p, -3, b);
G = checkP(C, gx, gy, n);
walkMul(G, 15);

//===================== FIPS 186-3

var d    = mpw('7891686032fd8057f636b44b1f47cce564d2509923a7465b');
var qx   = mpw('fba2aac647884b504eb8cd5a0a1287babcc62163f606a9a2');
var qy   = mpw('dae6d4cc05ef4f27d79ee38b71c9c8ef4865d98850d84aa5');
Q = checkP(C, qx, qy, n);
GQ = walkAdd(G, Q, 1);
QG = walkAdd(Q, G, 82);
walkMul(Q, 5);

// dG = G.mul(G, d);
// ack('Q.cmp(Q,dG)', true);

var k    = mpw('d06cb0a0ef2f708b0744f08aa06b6deedea9c0f80a69d847');
var msg  = "Example of ECDSA with P-192";
var e    = mpw('1b376f0b735c615ceeeb31baee654b0a374825db');
var kinv = mpw('5a277943c5a4d34b3c7dd97bdfe9b82c042586701088c00b');
ack('mpw(k, n, "invm").toString(16)', kinv.toString(16));
var r    = mpw('f0ecba72b88cde399cc5a18e2a8b7da54d81d04fb9802821');
var s    = mpw('1e6d3d4ae2b1fab2bd2040f5dabf00f854fa140b6d21e8ed');
ack('mpw(kinv, mpw(e, mpw(r, d, n, "mulm"), n, "addm"), n, "mulm").toString(16)', s.toString(16));

// kG = G.mul(G, k);
// ack('mpw(kG.x, kG.n, "%").toString(16)', r.toString(16));

var w    = mpw(s, n, "invm");
var u1   = mpw('785760fd37767d546003fa66933b7d202642352331b15b84');
ack('mpw(e, w, n, "mulm").toString(16)', u1.toString(16));
var u2   = mpw('de0747072e426e307ba1e19bd5c1b57f9e29220ae97cc9bc');
ack('mpw(r, w, n, "mulm").toString(16)', u2.toString(16));
var v    = mpw('f0ecba72b88cde399cc5a18e2a8b7da54d81d04fb9802821');

// uG = G.mul(G, u1);
// uQ = Q.mul(Q, u2);
// V = G.add(uG, uQ);
// ack('mpw(V.x, V.n, "%").toString(16)', v.toString(16));

//===================== X9.66-1998 J.3.1

var d    = mpw('1A8D598FC15BF0FD89030B5CB1111AEB92AE8BAF5EA475FB');
// Q compressed = 02 62B12D60 690CDCF3 30BABAB6 E69763B4 71F994DD 702D16A5
var qx   = mpw('62B12D60690CDCF330BABAB6E69763B471F994DD702D16A5');
var qy   = mpw(qx, qx, "*", -3, "+", qx, "*", b, "+", p, "%", p, 1, "+", 2, ">>", p, "powm");
Q = checkP(C, qx, qy, n);
// GQ = walkAdd(G, Q, 1);
// QG = walkAdd(Q, G, 82);
// walkMul(Q, 5);

// dG = G.mul(G, d);
// ack('Q.cmp(Q,dG)', true);

var k    = mpw('FA6DE29746BBEB7F8BB1E761F85F7DFB2983169D82FA2F4E');
var msg  = "abc";
var e    = mpw('A9993E364706816ABA3E25717850C26C9CD0D89D');
ack('e.toString(10)', '968236873715988614170569073515315707566766479517');
var kinv = mpw(k, n, "invm");
// ack('mpw(k, n, "invm").toString(16)', kinv.toString(16));
var r    = mpw('885052380FF147B734C330C43D39B2C4A89F29B0F749FEAD');
ack('r.toString(10)', '3342403536405981729393488334694600415596881826869351677613');
var s    = mpw('E9ECC78106DEF82BF1070CF1D4D804C3CB390046951DF686');
ack('s.toString(10)', '5735822328888155254683894997897571951568553642892029982342');
ack('mpw(kinv, mpw(e, mpw(r, d, n, "mulm"), n, "addm"), n, "mulm").toString(16)', s.toString(16));

// kG = G.mul(G, k);
// ack('mpw(kG.x, kG.n, "%").toString(16)', r.toString(16));

var w    = mpw(s, n, "invm");
ack('w.toString(10)', '3250964404472526825130516490452346217749189704049629042861');
var u1   = mpw('688E3FF66F55524374317D0AA03FA2B6E7DB44964CF9D457');
ack('u1.toString(10)', '2563697409189434185194736134579731015366492496392189760599');
ack('mpw(e, w, n, "mulm").toString(16)', u1.toString(16));
var u2   = mpw('9FBBC2459B10ADC858EE18BE81541041229A839EF04268518');
ack('u2.toString(10)', '62666438133486179671864777102357858491364063233387822220568');
// XXX FIXME wrong answer
// ack('mpw(r, w, n, "mulm").toString(16)', u2.toString(16));
var v    = mpw('885052380FF147B734C330C43D39B2C4A89F29B0F749FEAD');
ack('v.toString(10)', '3342403536405981729393488334694600415596881826869351677613');

// uG = G.mul(G, u1);
// uQ = Q.mul(Q, u2);
// V = G.add(uG, uQ);
// ack('mpw(V.x, V.n, "%").toString(16)', v.toString(16));

delete C;
delete G;
delete Q;

// ===== ECDSA P-224 example from 
//	http://csrc.nist.gov/groups/ST/toolkit/documents/Examples/ECDSA_Prime.pdf
print("===== P-224");
var p    = mpw(mpw(mpw(2, 224, "**"), mpw(2, 96, "**"), "-"), 1, "+");
ack('p.toString(10)', '26959946667150639794667015087019630673557916260026308143510066298881');
var n    = mpw('ffffffffffffffffffffffffffff16a2e0b8f03e13dd29455c5c2a3d');
ack('n.toString(10)', '26959946667150639794667015087019625940457807714424391721682722368061');

var c    = mpw('5b056c7e11dd68f40469ee7f3c7a7d74f7d121116506d031218291fb');
var b    = mpw('b4050a850c04b3abf54132565044b0b7d7bfd8ba270b39432355ffb4');
// FIXME mulm?
ack('mpw(mpw(b, b, "*", p, "%"), c, "*", p, "%").toString(10)', mpw(p, 27, "-").toString(10));

var gx   = mpw('b70e0cbd6bb4bf7f321390b94a03c1d356c21122343280d6115c1d21');
var gy   = mpw('bd376388b5f723fb4c22dfe6cd4375a05a07476444d5819985007e34');

C = new CurveFp(p, -3, b);
G = checkP(C, gx, gy, n);
walkMul(G, 5);

var d    = mpw('3f0c488e987c80be0fee521f8d90be6034ec69ae11ca72aa777481e8');
var qx   = mpw('e84fb0b8e7000cb657d7973cf6b42ed78b301674276df744af130b3e');
var qy   = mpw('4376675c6fc5612c21a0ff2d2a89d2987df7a2bc52183b5982298555');
Q = checkP(C, qx, qy, n);
GQ = walkAdd(G, Q, 5);
QG = walkAdd(Q, G, 1);
walkMul(Q, 39);

// dG = G.mul(G, d);
// ack('Q.cmp(Q,dG)', true);

var k    = mpw('a548803b79df17c40cde3ff0e36d025143bcbba146ec32908eb84937');
var msg  = "Example of ECDSA with P-224";
var e    = mpw('1f1e1cf892926cfccfc5a28feef3d807d23f778008dba4b35f04b2fd');
var kinv = mpw('b4d9d81feff7b325e09e770c40bace8b008d6074371967326f39130c');
ack('mpw(k, n, "invm").toString(16)', kinv.toString(16));
var r    = mpw('c3a3f5b82712532004c6f6d1db672f55d931c3409ea1216d0be77380');
var s    = mpw('c5aa1eae6095dea34c9bd84da3852cca41a8bd9d5548f36dabdf6617');
ack('mpw(kinv, mpw(e, mpw(r, d, n, "mulm"), n, "addm"), n, "mulm").toString(16)', s.toString(16));

// kG = G.mul(G, k);
// ack('mpw(kG.x, kG.n, "%").toString(16)', r.toString(16));

var w    = mpw(s, n, "invm");
var u1   = mpw('69df611df949498ebe20c1e453cf231cdd2f30adeecba9335481295d');
ack('mpw(e, w, n, "mulm").toString(16)', u1.toString(16));
var u2   = mpw('86daaf97dc9bb13a66ec7b735e69bccd60f395efb2cdfded8a3ccbcf');
ack('mpw(r, w, n, "mulm").toString(16)', u2.toString(16));
var v    = mpw('c3a3f5b82712532004c6f6d1db672f55d931c3409ea1216d0be77380');

// uG = G.mul(G, u1);
// uQ = Q.mul(Q, u2);
// V = G.add(uG, uQ);
// ack('mpw(V.x, V.n, "%").toString(16)', v.toString(16));

delete C;
delete G;
delete Q;

// ===== ECDSA P-239 example from X9.62-1998 J.3.2
print("===== P-239");
var p    = mpw('7FFFFFFFFFFFFFFFFFFFFFFF7FFFFFFFFFFF8000000000007FFFFFFFFFFF');
ack('p.toString(10)', '883423532389192164791648750360308885314476597252960362792450860609699839');
var n    = mpw('7FFFFFFFFFFFFFFFFFFFFFFF7FFFFF9E5E9A9F5D9071FBD1522688909D0B');
ack('n.toString(10)', '883423532389192164791648750360308884807550341691627752275345424702807307');

var b    = mpw('6B016C3BDCF18941D0D654921475CA71A9DB2FB27D1D37796185C2942C0A');
var a    = mpw('7FFFFFFFFFFFFFFFFFFFFFFF7FFFFFFFFFFF8000000000007FFFFFFFFFFC');

// G compressed = 020FFA 963CDCA8 816CCC33 B8642BED F905C3D3 58573D3F 27FBBD3B 3CB9AAAF
var gx   = mpw('0FFA963CDCA8816CCC33B8642BEDF905C3D358573D3F27FBBD3B3CB9AAAF');
var gy   = mpw(gx, gx, "*", -3, "+", gx, "*", b, "+", p, "%", p, 1, "+", 2, ">>", p, "powm");

C = new CurveFp(p, -3, b);
G = checkP(C, gx, gy, n);
// walkMul(G, 128);

var d    = mpw('7EF7C6FABEFFFDEA864206E80B0B08A9331ED93E698561B64CA0F7777F3D');
ack('d.toString(10)', '876300101507107567501066130761671078357010671067781776716671676178726717');
// Q compressed = 025B6D C53BC61A 2548FFB0 F671472D E6C9521A 9D2D2534 E65ABFCB D5FE0C70
var qx   = mpw('5B6DC53BC61A2548FFB0F671472DE6C9521A9D2D2534E65ABFCBD5FE0C70');
var qy   = mpw(qx, qx, "*", -3, "+", qx, "*", b, "+", p, "%", p, 1, "+", 2, ">>", p, "powm");
Q = checkP(C, qx, qy, n);
// GQ = walkAdd(G, Q, 5);
// QG = walkAdd(Q, G, 1);
// walkMul(Q, 128);

dG = G.mul(G, d);
// ack('Q.cmp(Q,dG)', true);

var k    = mpw('656C7196BF87DCC5D1F1020906DF2782360D36B2DE7A17ECE37D503784AF');
ack('k.toString(10)', '700000017569056646655505781757157107570501575775705779575555657156756655');
var msg  = "abc";
var e    = mpw('A9993E364706816ABA3E25717850C26C9CD0D89D');
ack('e.toString(10)', '968236873715988614170569073515315707566766479517');
var kinv = mpw(k, n, "invm");
var ry   = mpw('20C08272B9E6C92B518A5AC5EB2835BE0102809D77E69304A6F7C522B47B');
var r    = mpw('2CB7F36803EBB9C427C58D8265F11FC5084747133078FC279DE874FBECB0');
ack('r.toString(10)', '308636143175167811492622547300668018854959378758531778147462058306432176');
var s    = mpw('2EEAE988104E9C2234A3C2BEB1F53BFA5DC11FF36A875D1E3CCB1F7E45CF');
ack('s.toString(10)', '323813553209797357708078776831250505931891051755007842781978505179448783');
ack('mpw(kinv, mpw(e, mpw(r, d, n, "mulm"), n, "addm"), n, "mulm").toString(16)', s.toString(16));

kG = G.mul(G, k);
ack('kG.x.toString(10)', '308636143175167811492622547300668018854959378758531778147462058306432176');
ack('mpw(kG.x, kG.n, "%").toString(16)', r.toString(16));

var w    = mpw(s, n, "invm");
ack('w.toString(16)', '0x7886c9885b6fa74077186378969e35af072f6ba36b7364104e7a94447124');
ack('w.toString(10)', '831843418332978390463010021843350581892480848636408104706147767766249764');
var u1   = mpw('11F9D347D0996B633D8D83734C6F7E280011DEDC142CF5B3359A82CF15D9');
ack('u1.toString(10)', '124064965052014194622159338097387562954788117638383503089995672152118745');
ack('mpw(e, w, n, "mulm").toString(16)', u1.toString(16));
var u2   = mpw('1975746D082DE951CA9C5129DF423065B870E28FE8E7A9A27D465787BC5CB');
ack('u2.toString(10)', '2811363736140754465407544341268382421438687214093897850239246340491822539');
// XXX FIXME wrong answer
ack('mpw(r, w, "*", n, "%").toString(16)', u2.toString(16));
var v    = mpw('2CB7F36803EBB9C427C58D8265F11FC5084747133078FC279DE874FBECB0');
ack('v.toString(10)', '308636143175167811492622547300668018854959378758531778147462058306432176');

uG = G.mul(G, u1);
ack('uG.x.toString(16)', mpw('64C429FAF03DC1707700D2011D439836B4C712DCFFD8E4B7ED9937F62D1F').toString(16));
ack('uG.y.toString(16)', mpw('6580DE1A6ECEDFD783538C7C9D8398BAE8B5A697EEFD004AA59660800F48').toString(16));
uQ = Q.mul(Q, u2);
ack('uQ.x.toString(16)', mpw('3DCA0CAFD86C59DDD9FC251A20739F69845168F5922E523B7994AFC92D9D').toString(16));
ack('uQ.y.toString(16)', mpw('5532B0A717E945EED3D8AD1C26AB37907E942833CD22AFFE63AC1F5BC8FE').toString(16));

uQ.x   = mpw('3DCA0CAFD86C59DDD9FC251A20739F69845168F5922E523B7994AFC92D9D');
uQ.y   = mpw(uQ.x, uQ.x, "*", -3, "+", uQ.x, "*", b, "+", p, "%", p, 1, "+", 2, ">>", p, "powm");
V = G.add(uG, uQ);
ack('V.x.toString(16)', mpw('2CB7F36803EBB9C427C58D8265F11FC5084747133078FC279DE874FBECB0').toString(16));
ack('V.y.toString(16)', mpw('20C08272B9E6C92B518A5AC5EB2835BE0102809D77E69304A6F7C522B47B').toString(16));
ack('mpw(V.x, V.n, "%").toString(16)', v.toString(16));

delete C;
delete G;
delete Q;

// ===== ECDSA P-256 example from 
//	http://csrc.nist.gov/groups/ST/toolkit/documents/Examples/ECDSA_Prime.pdf
print("===== P-256");
var p    = mpw(2, 256, "**");
p = mpw(p, mpw(2, 224, "**"), "-");
p = mpw(p, mpw(2, 192, "**"), "+");
p = mpw(p, mpw(2,  96, "**"), "+");
p = mpw(p, 1, "-");
ack('p.toString(10)', '115792089210356248762697446949407573530086143415290314195533631308867097853951');
var n    = mpw('ffffffff00000000ffffffffffffffffbce6faada7179e84f3b9cac2fc632551');
ack('n.toString(10)', '115792089210356248762697446949407573529996955224135760342422259061068512044369');

var c    = mpw('7efba1662985be9403cb055c75d4f7e0ce8d84a9c5114abcaf3177680104fa0d');
var b    = mpw('5ac635d8aa3a93e7b3ebbd55769886bc651d06b0cc53b0f63bce3c3e27d2604b');
ack('mpw(mpw(b, p, "sqrm"), c, p, "mulm").toString(10)', mpw(p, 27, "-").toString(10));

var gx   = mpw('6b17d1f2e12c4247f8bce6e563a440f277037d812deb33a0f4a13945d898c296');
var gy   = mpw('4fe342e2fe1a7f9b8ee7eb4a7c0f9e162bce33576b315ececbb6406837bf51f5');

C = new CurveFp(p, -3, b);
G = checkP(C, gx, gy, n);
walkMul(G, 3);

var d    = mpw('c477f9f65c22cce20657faa5b2d1d8122336f851a508a1ed04e479c34985bf96');
var qx   = mpw('b7e08afdfe94bad3f1dc8c734798ba1c62b3a0ad1e9ea2a38201cd0889bc7a19');
var qy   = mpw('3603f747959dbf7a4bb226e41928729063adc7ae43529e61b563bbc606cc5e09');
Q = checkP(C, qx, qy, n);
GQ = walkAdd(G, Q, 1);
QG = walkAdd(Q, G, 1);
walkMul(Q, 9);

// dG = G.mul(G, d);
// ack('Q.cmp(Q,dG)', true);

var k    = mpw('7a1a7e52797fc8caaa435d2a4dace39158504bf204fbe19f14dbb427faee50ae');
var msg  = "Example of ECDSA with P-256";
var e    = mpw('a41a41a12a799548211c410c65d8133afde34d28bdd542e4b680cf2899c8a8c4');
var kinv = mpw('62159e5ba9e712fb098cce8fe20f1bed8346554e98ef3c7c1fc3332ba67d87ef');
ack('mpw(k, n, "invm").toString(16)', kinv.toString(16));
var r    = mpw('2b42f576d07f4165ff65d1f3b1500f81e44c316f1f0b3ef57325b69aca46104f');
var s    = mpw('dc42c2122d6392cd3e3a993a89502a8198c1886fe69d262c4b329bdb6b63faf1');
// FIXME mulm?
ack('mpw(mpw(mpw(r, d, "*", n, "%"), e, "+", n, "%"), kinv, "*", n, "%").toString(16)', s.toString(16));

// kG = G.mul(G, k);
// ack('mpw(kG.x, kG.n, "%").toString(16)', r.toString(16));

var w    = mpw(s, n, "invm");
var u1   = mpw('b807bf3281dd13849958f444fd9aea808d074c2c48ee8382f6c47a435389a17e');
// FIXME mulm?
ack('mpw(e, w, "*", n, "%").toString(16)', u1.toString(16));
var u2   = mpw('1777f73443a4d68c23d1fc4cb5f8b7f2554578ee87f04c253df44efd181c184c');
// FIXME mulm?
ack('mpw(r, w, "*", n, "%").toString(16)', u2.toString(16));
var v    = mpw('2b42f576d07f4165ff65d1f3b1500f81e44c316f1f0b3ef57325b69aca46104f');

// uG = G.mul(G, u1);
// uQ = Q.mul(Q, u2);
// V = G.add(uG, uQ);
// ack('mpw(V.x, V.n, "%").toString(16)', v.toString(16));

delete C;
delete G;
delete Q;

// ===== ECDSA P-384 example from 
//	http://csrc.nist.gov/groups/ST/toolkit/documents/Examples/ECDSA_Prime.pdf
print("===== P-384");
var p    = mpw(2, 384, "**");
p = mpw(p, mpw(2, 128, "**"), "-");
p = mpw(p, mpw(2,  96, "**"), "-");
p = mpw(p, mpw(2,  32, "**"), "+");
p = mpw(p, 1, "-");
ack('p.toString(10)', '39402006196394479212279040100143613805079739270465446667948293404245721771496870329047266088258938001861606973112319');
var n    = mpw('ffffffffffffffffffffffffffffffffffffffffffffffffc7634d81f4372ddf581a0db248b0a77aecec196accc52973');
ack('n.toString(10)', '39402006196394479212279040100143613805079739270465446667946905279627659399113263569398956308152294913554433653942643');

var c    = mpw('79d1e655f868f02fff48dcdee14151ddb80643c1406d0ca10dfe6fc52009540a495e8042ea5f744f6e184667cc722483');
var b    = mpw('b3312fa7e23ee7e4988e056be3f82d19181d9c6efe8141120314088f5013875ac656398d8a2ed19d2a85c8edd3ec2aef');
// FIXME mulm?
ack('mpw(mpw(b, b, "*", p, "%"), c, "*", p, "%").toString(10)', mpw(p, 27, "-").toString(10));

var gx   = mpw('aa87ca22be8b05378eb1c71ef320ad746e1d3b628ba79b9859f741e082542a385502f25dbf55296c3a545e3872760ab7');
var gy   = mpw('3617de4a96262c6f5d9e98bf9292dc29f8f41dbd289a147ce9da3113b5f0b8c00a60b1ce1d7e819d7a431d7c90ea0e5f');

C = new CurveFp(p, -3, b);
G = checkP(C, gx, gy, n);
walkMul(G, 7);

var d    = mpw('f92c02ed629e4b48c0584b1c6ce3a3e3b4faae4afc6acb0455e73dfc392e6a0ae393a8565e6b9714d1224b57d83f8a08');
var qx   = mpw('3bf701bc9e9d36b4d5f1455343f09126f2564390f2b487365071243c61e6471fb9d2ab74657b82f9086489d9ef0f5cb5');
var qy   = mpw('d1a358eafbf952e68d533855ccbdaa6ff75b137a5101443199325583552a6295ffe5382d00cfcda30344a9b5b68db855');
Q = checkP(C, qx, qy, n);
GQ = walkAdd(G, Q, 2);
QG = walkAdd(Q, G, 10);
walkMul(Q, 5);

// dG = G.mul(G, d);
// ack('Q.cmp(Q,dG)', true);

var k    = mpw('2e44ef1f8c0bea8394e3dda81ec6a7842a459b534701749e2ed95f054f0137680878e0749fc43f85edcae06cc2f43fef');
var msg  = "Example of ECDSA with P-384";
var e    = mpw('5aea187d1c4f6e1b35057d20126d836c6adbbc7049ee0299c9529f5e0b3f8b5a7411149d6c30d6cb2b8af70e0a781e89');
var kinv = mpw('ac227da51929533dfc2e9eefb4e0f7bd22392ca73289ed1c6c00b214e8874d8007c8ac46b25d677dfe9b1c6c10a47e4a');
ack('mpw(k, n, "invm").toString(16)', kinv.toString(16));
var r    = mpw('30ea514fc0d38d8208756f068113c7cada9f66a3b40ea3b313d040d9b57dd41a332795d02cc7d507fcef9faf01a27088');
var s    = mpw('cc808e504be414f46c9027bcbf78adf067a43922d6fcaa66c4476875fbb7b94efd1f7d5dbe620bfb821c46d549683ad8');
ack('mpw(kinv, mpw(e, mpw(r, d, n, "mulm"), n, "addm"), n, "mulm").toString(16)', s.toString(16));

// kG = G.mul(G, k);
// ack('mpw(kG.x, kG.n, "%").toString(16)', r.toString(16));

var w    = mpw(s, n, "invm");
var u1   = mpw('9c0590ee8000b79832dc4c6776f7e5fd2a74be161741c7c2d2f038d439831696a1b8ece4199d225b12b76dd9e637b250');
ack('mpw(e, w, n, "mulm").toString(16)', u1.toString(16));
var u2   = mpw('8f77be5b0eb32a1a3b9274cfda53518a01aad4afc4bd46a392b7c7de4eed3fe6dee54f3064234fe7fde57ae45532c24d');
ack('mpw(r, w, n, "mulm").toString(16)', u2.toString(16));
var v    = mpw('30ea514fc0d38d8208756f068113c7cada9f66a3b40ea3b313d040d9b57dd41a332795d02cc7d507fcef9faf01a27088');

// uG = G.mul(G, u1);
// uQ = Q.mul(Q, u2);
// V = G.add(uG, uQ);
// ack('mpw(V.x, V.n, "%").toString(16)', v.toString(16));

delete C;
delete G;
delete Q;

// ===== ECDSA P-521 example from 
//	http://csrc.nist.gov/groups/ST/toolkit/documents/Examples/ECDSA_Prime.pdf
print("===== P-521");
var p    = mpw(mpw(2, 521, "**"), 1, "-");
ack('p.toString(10)', '6864797660130609714981900799081393217269435300143305409394463459185543183397656052122559640661454554977296311391480858037121987999716643812574028291115057151');
var n    = mpw('01fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffa51868783bf2f966b7fcc0148f709a5d03bb5c9b8899c47aebb6fb71e91386409');
ack('n.toString(10)', '6864797660130609714981900799081393217269435300143305409394463459185543183397655394245057746333217197532963996371363321113864768612440380340372808892707005449');

var c    = mpw('79d1e655f868f02fff48dcdee14151ddb80643c1406d0ca10dfe6fc52009540a495e8042ea5f744f6e184667cc722483');
var b    = mpw('051953eb9618e1c9a1f929a21a0b68540eea2da725b99b315f3b8b489918ef109e156193951ec7e937b1652c0bd3bb1bf073573df883d2c34f1ef451fd46b503f00');
// FIXME: wrong answer
// ack('mpw(mpw(b, p, "sqrm"), c, p, "mulm").toString(10)', mpw(p, 27, "-").toString(10));

var gx   = mpw('c6858e06b70404e9cd9e3ecb662395b4429c648139053fb521f828af606b4d3dbaa14b5e77efe75928fe1dc127a2ffa8de3348b3c1856a429bf97e7e31c2e5bd66');
var gy   = mpw('11839296a789a3bc0045c8a5fb42c7d1bd998f54449579b446817afbd17273e662c97ee72995ef42640c550b9013fad0761353c7086a272c24088be94769fd16650');

C = new CurveFp(p, -3, b);
G = checkP(C, gx, gy, n);
// walkMul(G, 1024);

var d    = mpw('0100085f47b8e1b8b11b7eb33028c0b2888e304bfc98501955b45bba1478dc184eeedf09b86a5f7c21994406072787205e69a63709fe35aa93ba333514b24f961722');
var qx   = mpw('0098e91eef9a68452822309c52fab453f5f117c1da8ed796b255e9ab8f6410cca16e59df403a6bdc6ca467a37056b1e54b3005d8ac030decfeb68df18b171885d5c4');
var qy   = mpw('0164350c321aecfc1cca1ba4364c9b15656150b4b78d6a48d7d28e7f31985ef17be8554376b72900712c4b83ad668327231526e313f5f092999a4632fd50d946bc2e');
Q = checkP(C, qx, qy, n);
// GQ = walkAdd(G, Q, 1024);
// QG = walkAdd(Q, G, 1024);
// walkMul(Q, 1024);
// walkDblAdd(Q, 3);
// walkDblMul(Q, 31);

dG = G.mul(G, d);
ack('Q.cmp(Q,dG)', true);

var k    = mpw('c91e2349ef6ca22d2de39dd51819b6aad922d3aecdeab452ba172f7d63e370cecd70575f597c09a174ba76bed05a48e562be0625336d16b8703147a6a231d6bf');
var msg  = "Example of ECDSA with P-512";
var e    = mpw('9bf0e1deeda31e00f925b77f7cb6b1ced7368de1dc75bb9f94582c1ca709205d32af90025b02fa132fbebd6cddcd9172c0d66d8e581767a8b6f71de60be1f932');
var kinv = mpw('01eab94335a7ed337bce83c95de95447925edb0ee27f8e8378713e767d6da570fccfb4f13dcf57f898e77ddb540a9453e0c3d5c97ae8d2ec843590bcb1d349044c09');
ack('mpw(k, n, "invm").toString(16)', kinv.toString(16));
var r    = mpw('0140c8edca57108ce3f7e7a240ddd3ad74d81e2de62451fc1d558fdc79269adacd1c2526eeeef32f8c0432a9d56e2b4a8a732891c37c9b96641a9254ccfe5dc3e2ba');
var s    = mpw('00d72f15229d0096376da6651d9985bfd7c07f8d49583b545db3eab20e0a2c1e8615bd9e298455bdeb6b61378e77af1c54eee2ce37b2c61f5c9a8232951cb988b5b1');
ack('mpw(kinv, mpw(e, mpw(r, d, n, "mulm"), n, "addm"), n, "mulm").toString(16)', s.toString(16));

kG = G.mul(G, k);
ack('mpw(kG.x, kG.n, "%").toString(16)', r.toString(16));

var w    = mpw(s, n, "invm");
var u1   = mpw('01697eefb6bd3a6db024254fe69fd19c80eb04b71cdd16af72f322106093f971cb08c29f6f8950f0f61e45bf65bac39a590dcb043758c6606907f216a759b4ea4be4');
ack('mpw(e, w, n, "mulm").toString(16)', u1.toString(16));
var u2   = mpw('014e7fc3ee94b91e092f660253dcaf92a70306bdfa317a0ab7efb2c8286944bee5f146114f2c61950f8c8699cce1a22fe632ea89967d33fefcb0e7607b4b66d157b6');
ack('mpw(r, w, n, "mulm").toString(16)', u2.toString(16));
var v    = mpw('0140c8edca57108ce3f7e7a240ddd3ad74d81e2de62451fc1d558fdc79269adacd1c2526eeeef32f8c0432a9d56e2b4a8a732891c37c9b96641a9254ccfe5dc3e2ba');

uG = G.mul(G, u1);
uQ = Q.mul(Q, u2);
V = G.add(uG, uQ);
ack('mpw(V.x, V.n, "%").toString(16)', v.toString(16));

delete C;
delete G;
delete Q;

// ##############################################################
// Curve K-163
// ##############################################################
print("===== K-163");
var n    = mpw('0004000000000000000000020108a2e0cc0d99f8a5ef');
ack('n.toString(10)', '5846006549323611672814741753598448348329118574063');

var d    = mpw('7ff9959e88692406041273ab70e00b88ea18c9e6');
var qx   = mpw('2cc266f6d83736500418d2e00ce2d07c2ff8f734');
var qy   = mpw('00028553ea0676c40d577dcfe5259968b341a1d33af2');

var k    = mpw('1e7ec8c98ab49fc14e1bc841ebf61a0ad0961472');
var msg  = "Example of ECDSA with K-163"

var rx   = mpw('000718ef5ef376b8f5301b5fd2a761989c417807a275');

var e    = mpw('590d17df8bf17246eaf975b41864abcc7a747021');
var kinv = mpw('39252c5d41a936736cae1413da88924935f61b58');

var r    = mpw('000318ef5ef376b8f5301b5dd19ebeb7d033de0efc86');
ack('r.toString(10)', '4526858839996231728501073084029310952934625705094');
var s    = mpw('0002fd6b089af933c881d4b9feb3a550cbded50cd7bd');
ack('s.toString(10)', '4369764869049498617479414634111521353329149794237');

var w    = mpw(s, n, "invm");
var u1   = mpw('000332dd232ef2d65c09309d52c7be1646d41fc9de20');
var u2   = mpw('0003d355dad4b1418780a63a87dafc06d38bfc287312');
var rx   = mpw('000718ef5ef376b8f5301b5fd2a761989c417807a275');
var v    = mpw('000318ef5ef376b8f5301b5dd19ebeb7d033de0efc86');

var F = [];
function Fuv(p, u, v)
{
    var sum = 0;
print("Fuv("+p+", "+ u.toString(2) +", "+ v.toString(2) +")");
    for (var k = 1; k < p - 1; k++) {
	var ubit = (u >> F[k+1]) & 1;
	var vbit = (v >> F[p-k]) & 1;
	if (ubit && vbit)
	    sum ^= 1;
    }
print("F(" + u.toString(2) + " , " + v.toString(2) + ") = ", sum);
    return sum;
}

function Fgenerate(T, m, g)
{
    var p = T * m + 1;
    var w = 1;
    for (var j = 0; j < T; j++) {
	var n = w;
	for (var i = 0; i < m; i++) {
	    F[n] = i;
	    n  = (2 * n) % p;
	}
	w = (g * w) % p;
    }
    print(JSON.stringify(F));
}

function Fmul()
{
    var T = 4;
    var m = 7;
    var g = 12;
    var p = T * m + 1;

    Fgenerate(T, m, g);

    Fuv(p, 0x57, 0x61);
    Fuv(p, 0x2f, 0x43);
    Fuv(p, 0x5e, 0x07);
    Fuv(p, 0x3d, 0x0e);
    Fuv(p, 0x7a, 0x1c);
    Fuv(p, 0x75, 0x38);
    Fuv(p, 0x6b, 0x70);

}

// =======================================================
// Return y^2 + xy - x^3 - x^2 (modulo p)
function vfyBinaryCurve(p, x, y) {
    var t1 = mpw(y, y, p, "mulp");
    var t2 = mpw(x, y, p, "mulp");
    t1 = mpw(t1, t2, p, "addp");
    t2 = mpw(x, x, p, "mulp");
    t1 = mpw(t1, t2, p, "subp");
    t2 = mpw(t2, x, p, "mulp");
    t1 = mpw(t1, t2, p, "subp");
    return t1;
}

// =======================================================
// Return y^2 + xy - x^3 - 1 (modulo p)
function vfyKoblitzCurve0(p, x, y) {
    var t1 = mpw(mpw(mpw(x, y, "+"), y, "*"), p, "%");
    var t2 = mpw(mpw(mpw(x, x, "*"), p, "%"), x, "*");
    t1 = mpw(t1, t2, "-");
    t1 = mpw(t1, 1, "-");
    t1 = mpw(t1, p, "%");
    return t1;
}

// =======================================================
// Return y^2 + xy - x^3 - x^2 - 1 (modulo p)
function vfyKoblitzCurve1(p, x, y) {
    var t1 = vfyBinaryCurve(p, x, y);
    t1 = mpw(t1, 1, p, "subp");
    return t1;
}

// ##############################################################
// Curve B-163
// ##############################################################
print("===== B-163");
// Fmul();
var p    = mpw(2, 163, "**");
p = mpw(p, mpw(2,   7, "**"), "+");
p = mpw(p, mpw(2,   6, "**"), "+");
p = mpw(p, mpw(2,   3, "**"), "+");
p = mpw(p, 1, "+");
ack('p.toString(16)', '0x800000000000000000000000000000000000000c9');
var n    = mpw('00040000000000000000000292fe77e70c12a4234c33');
ack('n.toString(10)', '5846006549323611672814742442876390689256843201587');

// polynomial basis
var b  = mpw('20a601907b8c953ca1481eb10512f78744a3205fd');
var gx = mpw('3f0eba16286a2d57ea0991168d4994637e8343e36');
var gy = mpw('0d51fbc6c71a0094fa2cdd545b11c5c0c797324f1');
// normal basis
// var b  = mpw('6645f3cacf1638e139c6cd13ef61734fbc9e3d9fb');
// var gx = mpw('0311103c17167564ace77ccb09c681f886ba54ee8');
// var gy = mpw('333ac13c6447f2e67613bf7009daf98c87bb50c7f');

// var t1 = vfyBinaryCurve(p, gx, gy);
// ack('t1.toString(16)', b.toString(16));

var d    = mpw('000348d138c2de9447bd288feed177222ee377fb7bea');
var qx   = mpw('00066b015c0b72b0f81b1ecba6f58e7545d94744644c');
var qy   = mpw('ba6d4d62419155b186a29784f4aa4b8e8e1e7f76');

var k    = mpw('8ed0f93f7d492bb3991847d0e96f9cc3947259aa');
var msg  = "Example of ECDSA with B-163"

var rx   = mpw('000760938a97d88b30fdfb2cce1a4c59783ad0ed8fde');
var e    = mpw('728d59bbe028509dd5d2ce480f458e2925232ac3');
var kinv = mpw('00036dc66491684211373aaa8bd16024dd0a12a8ff11');

var r    = mpw('000360938a97d88b30fdfb2a3b1bd4726c282cca43ab');
ack('r.toString(10)', '4935858308701913308545983330530451819807197774763');
var s    = mpw('00019a7b5043d93a13d714b4717fc0698e6791cf7f7c');
ack('s.toString(10)', '2343436199767730626813973682401889576912314924924');

var e    = mpw('728d59bbe028509dd5d2ce480f458e2925232ac3');

var w    = mpw(s, n, "invm");
var u1   = mpw('0003c4099db241a80c807e02d81be10955b2eb2e5d8c');
var u2   = mpw('000160ccedabb7e3e767a604d8b042a65751708cc262');
var rx   = mpw('000760938a97d88b30fdfb2cce1a4c59783ad0ed8fde');
var v    = mpw('000360938a97d88b30fdfb2a3b1bd4726c282cca43ab');

// ##############################################################
// Curve K-233
// ##############################################################
print("===== K-233");
var n    = mpw('008000000000000000000000000000069d5bb915bcd46efb1ad5f173abdf');
ack('n.toString(10)', '3450873173395281893717377931138512760570940988862252126328087024741343');

var d    = mpw('8434613f4b799b4c26e4d7ab8e9481b04b09e648c94affd14b611a20');
var qx   = mpw('017c9dd766aefbe4de4b15f46db0671dc4ca0767ed51ecea94757d9c662e');
var qy   = mpw('01cdd726084837ae73c11c27d605c6eb2d5e31482358780305c2522b151b');

var k    = mpw('000190da60fe3b179b96611db7c7e5217c9aff0aee435782ebfb2dfff27f');
var msg  = "Example of ECDSA with K-233"

var rx   = mpw('01bea7231662e6516f11e37d59d500eae71d116e9b7bbce5964b88d4cc4d');
var e    = mpw('cb2314b21e913f4d345af272ed611f3eba00388056921da8a450f731');
var kinv = mpw('00759a37371c026e261add7278b81ee15d18e2b0d9bd1a077aebf96aa067');

var r    = mpw('003ea7231662e6516f11e37d59d500d70f09e62d64fe6ff445c9b479c8b0');
ack('r.toString(10)', '1689118280210305605171693773892079201866275012021226933371217775282352');
var s    = mpw('007503ec8e630386ce36a6276f221bc8ee4592cca5960a1eabe3bd2a282a');
ack('s.toString(10)', '3154727010507234306862258506085323724325131083359956345879910870034474');

var w    = mpw(s, n, "invm");
var u1   = mpw('0043a8ea51489b412b0a1ca97865a12491e8e144159e56cdc8bbe201d0d5');
var u2   = mpw('0036d6ac76bcac51707f796e6fa1cb649f7fd2c69aed93d01ab7c8ae35aa');
var rx   = mpw('01bea7231662e6516f11e37d59d500eae71d116e9b7bbce5964b88d4cc4d');
var v    = mpw('003ea7231662e6516f11e37d59d500d70f09e62d64fe6ff445c9b479c8b0');

// ##############################################################
// Curve B-233
// ##############################################################
print("===== B-233");
var n    = mpw('01000000000000000000000000000013e974e72f8a6922031d2603cfe0d7');
ack('n.toString(10)', '6901746346790563787434755862277025555839812737345013555379383634485463');

var d    = mpw('8af6d5a8e875977c7d4ba1f611cf7b6d70b26140bf84a1cc281f1b7b');
var qx   = mpw('01d3ad52d68f8383f582e2ba00f89ce1632211edc24440c31798e0c8ed40');
var qy   = mpw('006c3b96cc0e6bc59355a1294e22dbf1d4b9071c28da1389b6debe0e7f43');

var k    = mpw('8a65482917ba18f1e8b266a3795b0a3a09c439fa6b611e37123baf72');
var msg  = "Example of ECDSA with B-233"

var rx   = mpw('0186806715d9620f0a3e62c1ba593d9817b6dcb23de85bf504c326629e63');
var e    = mpw('6a8389d3644c7fa90d7f57e6049d0dd41b8e473cd296c71d0ff232b3');
var kinv = mpw('0097589e4b9b7c0f0c1e58d2ac61e8297d83fe7d00172e64b7d0df207dd9');

var r    = mpw('0086806715d9620f0a3e62c1ba593d842e41f582b37f39f1e79d2292bd8c');
ack('r.toString(10)', '3626155233584346473011272901749115607875304189783836849535359105809804');
var s    = mpw('0040159539861be6673c0a3b2e49f5f6c9532b60130c6fc78826c9e900ef');
ack('s.toString(10)', '1727709532304725124330354137202758438690663000173119669782159003549935');

var w    = mpw(s, n, "invm");
var u1   = mpw('0009621acb96ba987e2354a4ca3273c142bc963cce4cb84e17f4a6a8a3e1');
var u2   = mpw('006f64d0192b2965749d0ac29c7840245cc70bdb296f7e71cfc59c069515');
var rx   = mpw('0186806715d9620f0a3e62c1ba593d9817b6dcb23de85bf504c326629e63');
var v    = mpw('0086806715d9620f0a3e62c1ba593d842e41f582b37f39f1e79d2292bd8c');

// ##############################################################
// Curve K-283
// ##############################################################
print("===== K-283");
var n    = mpw('01ffffffffffffffffffffffffffffffffffe9ae2ed07577265dff7f94451e061e163c61');
ack('n.toString(10)', '3885337784451458141838923813647037813284811733793061324295874997529815829704422603873');

var d    = mpw('00069e6d19f7e454a83664ff49208f6038eaf842e164df42d0f64948ff9c94b014988329');
var qx   = mpw('01b64a60d4a365409635aaa27e1708d90b839afa2d9820e12b79c3af1094b6010aaef5be');
var qy   = mpw('0334b5f30ca21756bde6d47738f2458f56fbf6bdc76fcfb8f3e591455f041a952ee87a8e');

var k    = mpw('e3084442d66fa9a02c42890163e57ee33ca1f4583c65bcbde92781c7a3c83e89b773');
var msg  = "Example of ECDSA with K-283"

var rx   = mpw('07c973d58fd17a06aa8f39d5ec42e0a6b992f6cc61f157565dd7036c147d9005400c1328');
var e    = mpw('184f9aea741e7668b8b5c72c81617fa4068929628f77bd2f7a713a0a09916b81');
var kinv = mpw('0045d85f04239846deb60444da59f95ca0ca13fb9c30b6972e852e332e223067143d174d');

var r    = mpw('01c973d58fd17a06aa8f39d5ec42e0a6b99339c1d57ff6f0eabd04ed57ae35f2e5c95e05');
ack('r.toString(10)', '3471401162510341375623239443838810951682842412872079937478668905492984462483269443077');
var s    = mpw('014a4ed02cbe4d76ed5ddaa34a9f2d7390af2de327edbc3335119d3e43cbb7fe0384d841');
ack('s.toString(10)', '2506557860315181892752630984589168964256966490874765067882847886248065073786590124097');

var w    = mpw(s, n, "invm");
var u1   = mpw('001950717a3ba3fe8cc4677c5c6a1f9ec4c92a7d405e39e133250abc8038a945b86fbbb8');
var u2   = mpw('00b8d9b42e8b8c1c7b610132b88a9d3ddc8beae7bd8a7e5fc94fc937c46779bf8e33ca2a');
var rx   = mpw('07c973d58fd17a06aa8f39d5ec42e0a6b992f6cc61f157565dd7036c147d9005400c1328');
var v    = mpw('01c973d58fd17a06aa8f39d5ec42e0a6b99339c1d57ff6f0eabd04ed57ae35f2e5c95e05');

// ##############################################################
// Curve B-283
// ##############################################################
print("===== B-283");
var n    = mpw('03ffffffffffffffffffffffffffffffffffef90399660fc938a90165b042a7cefadb307');
ack('n.toString(10)', '7770675568902916283677847627294075626569625924376904889109196526770044277787378692871');

var d    = mpw('010652d37b0a9db64d4033ac6549cd1df37e1eede2612c2363257c6aff6c8cb5dcb63648');
var qx   = mpw('0390858e9327a714c74af0c3adedf4e6c75cafdcc46507a49e415b138a094b6f43e882ac');
var qy   = mpw('00d4a65d973cd150a5221bedf872a4ba207ff4427dfffd4827c5bf169e719162504d0631');

var k    = mpw('0100ec321393e6dd6c4d47be5ae189e5e35408579d0862178f94ccbba3c4049a4d88e297');
var msg  = "Example of ECDSA with B-283"

var rx   = mpw('077cb284ac41e72eda2a93eb8d6dff58620f6c69d528dfe90d909aa5cabc03a34e5d5a76');
var e    = mpw('f0bf4aef3f694ebdde0a79445c897adb2430b91877c772da9b7362cb03aea87f');
var kinv = mpw('00ab6d18af222d8fde7d93894d4faeeb36accd4fb68ec95d9e9bff4c08aff3c631a67be4');

var r    = mpw('037cb284ac41e72eda2a93eb8d6dff58620f7cd99b927eec7a060a8f6fb7d9265eafa76f');
ack('r.toString(10)', '6774278697741420663687521446478692454362079599099886695356533960517936672443416422255');
var s    = mpw('00a37ac10aebfc22fc6e6ee22e8f235e3eeb0555a0f0f9da92d9ffa734ad767956d27f23');
ack('s.toString(10)', '1240572480066211322672976019324920068198845924194402393411512731505068634362155794211');

var w    = mpw(s, n, "invm");
var u1   = mpw('01a9af084b9e04574c178a2d00c97cfe36f25d5f834fcd7044235a92e89ac3346c8c9af3');
var u2   = mpw('009949042a40d7613f3880cfcc3b3e9d22ee9c130651cee1f263efc881cf9a53a564fbce');
var rx   = mpw('077cb284ac41e72eda2a93eb8d6dff58620f6c69d528dfe90d909aa5cabc03a34e5d5a76');
var v    = mpw('037cb284ac41e72eda2a93eb8d6dff58620f7cd99b927eec7a060a8f6fb7d9265eafa76f');

// ##############################################################
// Curve K-409
// ##############################################################
print("===== K-409");
var n    = mpw('007ffffffffffffffffffffffffffffffffffffffffffffffffffe5f83b2d4ea20400ec4557d5ed3e3e7ca5b4b5c83b8e01e5fcf');
ack('n.toString(10)', '330527984395124299475957654016385519914202341482140609642324395022880711289249191050673258457777458014096366590617731358671');

var d    = mpw('00019f5789fe26e0e700c69e253e9f74d76eafb4c979d0b1584d4fe98715d45b7baaa851e02a1ecaed8b96602cf611d8a504bbd5');
var qx   = mpw('0127065590df9265fdfba4ed6edf76a9bc8ce880b58b6f571a1ab62ba3401269441f3b95ecd0909465022240ae45c7b36a91de58');
var qy   = mpw('003c85268d9267302090425bbc14c3d9ae1c1cfc78e0bfcccfc1fb5dca5b195c6f8cfbe2d85e4071b71317aa2b0b65c391f82502');

var k    = mpw('0001592048516ccd793c7b863b00985fdba71c3d1edf449f667ac0d05ef37d15a94ad3282f29f7e9fd9491872f931354a1ccfa39');
var msg  = "Example of ECDSA with K-409"

var rx   = mpw('00ef421a230aa8b471939a77bf4f2c64fc0b4caa39edcd06337a9115af89df725e3c748ae018320e89d77abcd3aa13a6ccf10c34');
var e    = mpw('b18623fe7d6b79c3947651cf64a066400f89dc989d07bfd8c1aaf75e3c9b3d48fc457204168de4ed4eca8e240e009b95');
var kinv = mpw('0064d066b74c9771a843f341a1853f0520696aa57338c21c4a839507b5ee65cb98a7f87c8e53037c02980cf5185300b709901d59');

var r    = mpw('006f421a230aa8b471939a77bf4f2c64fc0b4caa39edcd06337a92b62bd70a883dfc65c68a9ad33aa5efb061884d8fedecd2ac65');
ack('r.toString(10)', '287296502609903755047276062317671877380342987214246771450934811188142631858192434973293671444180485746280616241641144101989');
var s    = mpw('00425f2ff9cbbf1b9e3fc17c4b66303622d7749047373cc9f919758cd88420c4cd0fdf14b819a4ada9961c3e6095000467c2f823');
ack('s.toString(10)', '171388639085828986597824337788600168526139265051294280998358619335390078299394527527443716590351871100863295063908700846115');

var w    = mpw(s, n, "invm");
var u1   = mpw('007fc6bdb66a9d2a23bf69d4f96bb3e7e24e365b45d111c666747a42b39275ed0ba4f2866d3723dd13851a45208c864525d5f530');
var u2   = mpw('0005e16d63457088b1f3012e844c852e23c9f1225aa569a883de8a5828dde30917d23c4d807a371aa1fc5db25149abbcd53f7558');
var rx   = mpw('00ef421a230aa8b471939a77bf4f2c64fc0b4caa39edcd06337a9115af89df725e3c748ae018320e89d77abcd3aa13a6ccf10c34');
var v    = mpw('006f421a230aa8b471939a77bf4f2c64fc0b4caa39edcd06337a92b62bd70a883dfc65c68a9ad33aa5efb061884d8fedecd2ac65');

// ##############################################################
// Curve B-409
// ##############################################################
print("===== B-409");
var n    = mpw('010000000000000000000000000000000000000000000000000001e2aad6a612f33307be5fa47c3c9e052f838164cd37d9a21173');
ack('n.toString(10)', '661055968790248598951915308032771039828404682964281219284648798304157774827374805208143723762179110965979867288366567526771');

var d    = mpw('4af896db379abdf70c8fade9ebd28cd530f2ecb336b4de84bd6e065ef56c8c548c532d00fa55ca8acf3e98adbca9f78d241b');
var qx   = mpw('01951c5e41607e9317f247d49a389d0e120f479d47737543098ae5e1bb62bd59de70e1c584ae655c702d39dd4f7883e1876c4a9b');
var qy   = mpw('016b16b98a3353d75beb4d3576c64568ba381463cf77d4aeb85218d2d546e7a1ee3ab9316d8c7df00d155b7891b2c0bf4b5e942e');

var k    = mpw('6a0b81d9320b5c305d730b1c1e74b03fafb88a7ec355990b75f9b70e8532433296a32492cba06f8583d5b19c5b8c5d6d07ec');
var msg  = "Example of ECDSA with B-409"

var rx   = mpw('01f3e4da3101c64239d76831995c0ec1e56ce4690c42ddd53dbf3ef725d819df090b8632f327499b5b99c280d7f410cd7105c8db');
var e    = mpw('4bbf1bc0ddf9d3b7bfe21fc68642b3e5508ca6ba4d365c1d00abbfabdb0f3ec2b0be995ae803de47d0880bf192649edc');
var kinv = mpw('00a202ea455d0e1a5ef09054b39259c768db76ffd1a77b6281fc7056a4a23a1012cdd604e4d7993e0d9edd422defd782c1225a1a');

var r    = mpw('00f3e4da3101c64239d76831995c0ec1e56ce4690c42ddd53dbf3d147b0173cc15d87e749382cd5ebd9492fd568f43959763b768');
ack('r.toString(10)', '629795133852997848663164416330632008108488131029367711198400899015295569161990628452056613359116571349413663774547185481576');
var s    = mpw('00292fa994dc6ea367236ad73956dbc1eb62b8779df438165407141587e3feed883741cdf5542f255bebc57b9d0c87ad403b8eab');
ack('s.toString(10)', '106353011790980050510402230388010919168837770971007460637281001297135413850335587267331709367101624723953375840680931987115');

var w    = mpw(s, n, "invm");
var u1   = mpw('0033d4f3ce2b7d4f2548f54c92420a8c30e6b02d768cbd6ca4ab020e88be82eefe4d58a9b4db2854561ead8975e1a58fb45c2117');
var u2   = mpw('00eb0671192b38cbc911cade604d9e048c7e660af3c7085d54839c1b7549691e5f7fffdab4003ade05208775d4a0287e448740af');
var rx   = mpw('01f3e4da3101c64239d76831995c0ec1e56ce4690c42ddd53dbf3ef725d819df090b8632f327499b5b99c280d7f410cd7105c8db');
var v    = mpw('00f3e4da3101c64239d76831995c0ec1e56ce4690c42ddd53dbf3d147b0173cc15d87e749382cd5ebd9492fd568f43959763b768');

// ##############################################################
// Curve K-571
// ##############################################################
print("===== K-571");
var n    = mpw('020000000000000000000000000000000000000000000000000000000000000000000000131850e1f19a63e4b391a8db917f4138b630d84be5d639381e91deb45cfe778f637c1001');
ack('n.toString(10)', '1932268761508629172347675945465993672149463664853217499328617625725759571144780212268133978522706711834706712800825351461273674974066617311929682421617092503555733685276673');

var d    = mpw('01042fde4d66e76725e7957e208a85cf23bc0d5b8d001b36aeafb34ad1104004ccf99afdfabca11585a4eb5263c87052cb05ef7fb39d9e5f6cf495e9dce5840b83fbc5ff3ad8b2f3');
var qx   = mpw('04d9cfe0a7338fea703e007f5d10babd2df3f319b47df1e23c4f7e5abf5014c1390b78f117e6af8258a48f56acb9faac788530b5ccdb1ab7e9390ec5dd7a39d5eeaf6c41bf50ac76');
var qy   = mpw('064732c504f81dc5f9b0e882b6da46e124e8241358f077896d25ecf028ad0e6011993c85e68741a07d7817c400cf94b1a3f524f48668b5b970972618616db4362a769d16cac34bf0');

var k    = mpw('0104063c918de62000a3fd87775d8d71398722bd153b8ea33060349c5fe6cf6cb4677957e6ba50d3c8a8b5182b9cf962954a6bbb5f7868b88e5778aa62a0cf8002bf19da3049ff51');
var msg  = "Example of ECDSA with K-571"

var rx   = mpw('0668758c313fbf1945f775d95b25866dbc8d001d9c7ef4ffa53774e8c68927a52707f034cebb3a712be6d164f2ffe1897b069f8fbaec4650b5372ddfa31cdeccfa78569197cf50f1');
var e    = mpw('46e30abdd459269cf19af76900af7131b4f639227414719ebebe548ccd4026b75c1f52618547aa3821f29fcf685e33640bd9e29f7fe46817627d4139eee411c6');
var kinv = mpw('0075a02beaea51660a1d05053b173c9c6dccaeba80f72ca08dec3c32e2a47cbc5674998ad19fc77b6615bfbed482451af7fd9b416b64b0e8f8429449fa9685f2b9c8dc2108544d98');

var r    = mpw('0068758c313fbf1945f775d95b25866dbc8d001d9c7ef4ffa53774e8c68927a52707f034957247cb5717a5b6d84ae6f6c688dbe59859bd6d03b48237476742afe37cefe36d5b20ee');
ack('r.toString(10)', '394224984077781794110496797090243013264310697694235133152929357615210591117311039789635553626060843213156329295090794526694846023847676866900261239702547152597222099198190');
var s    = mpw('00f5c8e975a1da26b2e0acd4f486c4a4231c1e29ee8ecfa03a697761498f5d53ff898afc16945975d328d34a8dcdc6d0613c73fe4f516f5685e23716fe105ed3472c3358b5b4ad41');
ack('s.toString(10)', '927582646247034536177419741157642737320661633591180227914401573622207818585635957265237189528319320264021523961000220604458498430712577999892194455415491822501994555551041');

var w    = mpw(s, n, "invm");
var u1   = mpw('0121b1f8919c4ed41d4676bc6f417dbf7f93c2314c74796642d1b17df4fd04c114ae293707913e2ebb65e173d52c3678c8366eb3c94e7b7ecee53f6b0b625ac62b924e3d6b975ff3');
var u2   = mpw('01afbf8d1d31511413eb6846b32ced90d38e515ff26cb8334ef619677794cc3b92da014e9b0e39bf49d8933d3e998145594128612aeccbfc7004d08777fd90bc197d13dc6238bf0b');
var rx   = mpw('0668758c313fbf1945f775d95b25866dbc8d001d9c7ef4ffa53774e8c68927a52707f034cebb3a712be6d164f2ffe1897b069f8fbaec4650b5372ddfa31cdeccfa78569197cf50f1');
var v    = mpw('0068758c313fbf1945f775d95b25866dbc8d001d9c7ef4ffa53774e8c68927a52707f034957247cb5717a5b6d84ae6f6c688dbe59859bd6d03b48237476742afe37cefe36d5b20ee');

// ##############################################################
// Curve B-571
// ##############################################################
print("===== B-571");
var n    = mpw('03ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffe661ce18ff55987308059b186823851ec7dd9ca1161de93d5174d66e8382e9bb2fe84e47');
ack('n.toString(10)', '3864537523017258344695351890931987344298927329706434998657235251451519142289560424536143999389415773083133881121926944486246872462816813070234528288303332411393191105285703');

var d    = mpw('0003cce32ba00dc3a7eefc9eb6f6cbfb9c5f0e57f532b7ee6826d4a75d0e756fd533900f2cea8cccc50ee22ce079398d371ec4a2ec45cc24b887606678e9c67453d0f5e768e9d752');
var qx   = mpw('00310ead2bef3ddb84f9fc1777a7ee179ffcb77aab497bdc00e290597a5fce306fe419d2f1f208e54850516526db8e03b0519bef60e3a3cc8198fbca8c469acfe46ab70d5c31874f');
var qy   = mpw('0373ce6ea68f55d1501d5203aca03c5ab709a337a8e03b03838f47c06762065fbdd08a102a08c42ff1760145be54d8606d326ea22a54df034fac30988049820beba2b0af9f6404b3');

var k    = mpw('01062ff6d95c49ac610cb9af9900d59c288669c3626306db7eb7f119499ba1d54cb6be888758caada69952675cc0cd4999176879bc302a7e2a5118dfc7d538da114ccac2baf9ad08');
var msg  = "Example of ECDSA with B-571"

var rx   = mpw('00e17447e422a2c0085190354ac149210c1137a92c10f9b14d225e6510da76b19ef44d39390dd9d808c9dfbae67d9cf0e7be79a9e72fa8fa1dfe89f43fb6d093a6ceb30e136eaba3');
var e    = mpw('60edef7da1d9d35a77d1da441ebb63454501f2bb1af8a4c49d281298e5f4d4e6b7e9bce4b66b2512bf590288b57915bfd3aed2c2604a5c574107df674faf9779');
var kinv = mpw('028c3ae12bd7922b837fe05066136bb45eda0337d39e31c3d4b9164c93f17fd7549471eb0385fccea8768dd6e5925adf1d1888826ff6aecc48f3db39905d46a644eb2f0c3a3dcbbd');

var r    = mpw('00e17447e422a2c0085190354ac149210c1137a92c10f9b14d225e6510da76b19ef44d39390dd9d808c9dfbae67d9cf0e7be79a9e72fa8fa1dfe89f43fb6d093a6ceb30e136eaba3');
ack('r.toString(10)', '850855762239502026591058023510563289273348857487181767834743944602362175979470988825265490247148142789747022832891719663177918797433599028444576028042439884754109942639523');
var s    = mpw('00624e852c7b6a061b4b39a907b518200fed380fd692c9ae147c5250f4852434afa24a1ca5062c48e5fc217fb689ab3a7266b5522f176f32a5cea22d6bf2820d349e4193bcee75c8');
ack('s.toString(10)', '371005865765721334746413249075321341103084481747665324048935832930246035334494733931256416306196179464892337407155249561524652370280994639556574699020735912775043796334024');

var w    = mpw(s, n, "invm");
var u1   = mpw('01a73be1676983c3b1583a8504c46e290ac2ffc3fd7866e6242ae2a3dc40f362e2b799173698175f7f094e9fa06ee09b3aa7d284549b8fcf4671051b829aa38ef59d066b3554de19');
var u2   = mpw('010df449e4fb576079a7d63a59f000da5e24da7800fa29bf321084ed549834b1ed95e5baf48fd65830f2bd0957b6f709dac14af90be29993fe428bd7df93206cc9ed9296230b8657');
var rx   = mpw('00e17447e422a2c0085190354ac149210c1137a92c10f9b14d225e6510da76b19ef44d39390dd9d808c9dfbae67d9cf0e7be79a9e72fa8fa1dfe89f43fb6d093a6ceb30e136eaba3');
var v    = mpw('00e17447e422a2c0085190354ac149210c1137a92c10f9b14d225e6510da76b19ef44d39390dd9d808c9dfbae67d9cf0e7be79a9e72fa8fa1dfe89f43fb6d093a6ceb30e136eaba3');

if (loglvl) print("<-- Mpw.js");
