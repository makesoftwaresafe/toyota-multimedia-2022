if (loglvl) print("--> Dc.js");

const PGPHASHALGO_MD5             =  1;   /*!< MD5 */
const PGPHASHALGO_SHA1            =  2;   /*!< SHA-1 */
const PGPHASHALGO_RIPEMD160       =  3;   /*!< RIPEMD-160 */
const PGPHASHALGO_MD2             =  5;   /*!< MD2 */
const PGPHASHALGO_TIGER192        =  6;   /*!< TIGER-192 */
const PGPHASHALGO_HAVAL_5_160     =  7;   /*!< HAVAL-5-160 */
const PGPHASHALGO_SHA256          =  8;   /*!< SHA-256 */
const PGPHASHALGO_SHA384          =  9;   /*!< SHA-384 */
const PGPHASHALGO_SHA512          = 10;   /*!< SHA-512 */
const PGPHASHALGO_SHA224          = 11;   /*!< SHA-224 */

const PGPHASHALGO_MD4             = 104;  /*!< (private) MD4 */
const PGPHASHALGO_RIPEMD128       = 105;  /*!< (private) RIPEMD-128 */
const PGPHASHALGO_CRC32           = 106;  /*!< (private) CRC-32 */
const PGPHASHALGO_ADLER32         = 107;  /*!< (private) ADLER-32 */
const PGPHASHALGO_CRC64           = 108;  /*!< (private) CRC-64 */
const PGPHASHALGO_JLU32           = 109;  /*!< (private) Jenkins lookup3.c */

const PGPHASHALGO_RIPEMD256       = 111;  /*!< (private) RIPEMD-256 */
const PGPHASHALGO_RIPEMD320       = 112;  /*!< (private) RIPEMD-320 */
const PGPHASHALGO_SALSA10         = 113;  /*!< (private) SALSA-10 */
const PGPHASHALGO_SALSA20         = 114;  /*!< (private) SALSA-20 */

const PGPHASHALGO_MD6_224	= 128+0;	/*!< (private) MD6-224 */
const PGPHASHALGO_MD6_256	= 128+1;	/*!< (private) MD6-256 */
const PGPHASHALGO_MD6_384	= 128+2;	/*!< (private) MD6-384 */
const PGPHASHALGO_MD6_512	= 128+3;	/*!< (private) MD6-512 */

const PGPHASHALGO_CUBEHASH_224	= 136+0;	/*!< (private) CUBEHASH-224 */
const PGPHASHALGO_CUBEHASH_256	= 136+1;	/*!< (private) CUBEHASH-256 */
const PGPHASHALGO_CUBEHASH_384	= 136+2;	/*!< (private) CUBEHASH-384 */
const PGPHASHALGO_CUBEHASH_512	= 136+3;	/*!< (private) CUBEHASH-512 */

const PGPHASHALGO_KECCAK_224	= 144+0;	/*!< (private) KECCAK-224 */
const PGPHASHALGO_KECCAK_256	= 144+1;	/*!< (private) KECCAK-256 */
const PGPHASHALGO_KECCAK_384	= 144+2;	/*!< (private) KECCAK-384 */
const PGPHASHALGO_KECCAK_512	= 144+3;	/*!< (private) KECCAK-512 */

const PGPHASHALGO_ECHO_224	= 148+0;	/*!< (private) ECHO-224 */
const PGPHASHALGO_ECHO_256	= 148+1;	/*!< (private) ECHO-256 */
const PGPHASHALGO_ECHO_384	= 148+2;	/*!< (private) ECHO-384 */
const PGPHASHALGO_ECHO_512	= 148+3;	/*!< (private) ECHO-512 */

const PGPHASHALGO_EDONR_224	= 152+0;	/*!< (private) EDONR-224 */
const PGPHASHALGO_EDONR_256	= 152+1;	/*!< (private) EDONR-256 */
const PGPHASHALGO_EDONR_384	= 152+2;	/*!< (private) EDONR-384 */
const PGPHASHALGO_EDONR_512	= 152+3;	/*!< (private) EDONR-512 */

const PGPHASHALGO_FUGUE_224	= 156+0;	/*!< (private) FUGUE-224 */
const PGPHASHALGO_FUGUE_256	= 156+1;	/*!< (private) FUGUE-256 */
const PGPHASHALGO_FUGUE_384	= 156+2;	/*!< (private) FUGUE-384 */
const PGPHASHALGO_FUGUE_512	= 156+3;	/*!< (private) FUGUE-512 */

const PGPHASHALGO_SKEIN_224	= 160+0;	/*!< (private) SKEIN-224 */
const PGPHASHALGO_SKEIN_256	= 160+1;	/*!< (private) SKEIN-256 */
const PGPHASHALGO_SKEIN_384	= 160+2;	/*!< (private) SKEIN-384 */
const PGPHASHALGO_SKEIN_512	= 160+3;	/*!< (private) SKEIN-512 */
const PGPHASHALGO_SKEIN_1024	= 160+4;	/*!< (private) SKEIN-1024 */

const PGPHASHALGO_BMW_224	= 168+0;	/*!< (private) BMW-224 */
const PGPHASHALGO_BMW_256	= 168+1;	/*!< (private) BMW-256 */
const PGPHASHALGO_BMW_384	= 168+2;	/*!< (private) BMW-384 */
const PGPHASHALGO_BMW_512	= 168+3;	/*!< (private) BMW-512 */

const PGPHASHALGO_SHABAL_224	= 176+0;	/*!< (private) SHABAL-224 */
const PGPHASHALGO_SHABAL_256	= 176+1;	/*!< (private) SHABAL-256 */
const PGPHASHALGO_SHABAL_384	= 176+2;	/*!< (private) SHABAL-384 */
const PGPHASHALGO_SHABAL_512	= 176+3;	/*!< (private) SHABAL-512 */

const PGPHASHALGO_SHAVITE3_224	= 180+0;	/*!< (private) SHAVITE3-224 */
const PGPHASHALGO_SHAVITE3_256	= 180+1;	/*!< (private) SHAVITE3-256 */
const PGPHASHALGO_SHAVITE3_384	= 180+2;	/*!< (private) SHAVITE3-384 */
const PGPHASHALGO_SHAVITE3_512	= 180+3;	/*!< (private) SHAVITE3-512 */

const PGPHASHALGO_BLAKE_224	= 184+0;	/*!< (private) BLAKE-224 */
const PGPHASHALGO_BLAKE_256	= 184+1;	/*!< (private) BLAKE-256 */
const PGPHASHALGO_BLAKE_384	= 184+2;	/*!< (private) BLAKE-384 */
const PGPHASHALGO_BLAKE_512	= 184+3;	/*!< (private) BLAKE-512 */

const PGPHASHALGO_TIB3_224	= 192+0;	/*!< (private) TIB3-224 */
const PGPHASHALGO_TIB3_256	= 192+1;	/*!< (private) TIB3-256 */
const PGPHASHALGO_TIB3_384	= 192+2;	/*!< (private) TIB3-384 */
const PGPHASHALGO_TIB3_512	= 192+3;	/*!< (private) TIB3-512 */

const PGPHASHALGO_SIMD_224	= 200+0;	/*!< (private) SIMD-224 */
const PGPHASHALGO_SIMD_256	= 200+1;	/*!< (private) SIMD-256 */
const PGPHASHALGO_SIMD_384	= 200+2;	/*!< (private) SIMD-384 */
const PGPHASHALGO_SIMD_512	= 200+3;	/*!< (private) SIMD-512 */

const PGPHASHALGO_ARIRANG_224	= 208+0;	/*!< (private) ARIRANG-224 */
const PGPHASHALGO_ARIRANG_256	= 208+1;	/*!< (private) ARIRANG-256 */
const PGPHASHALGO_ARIRANG_384	= 208+2;	/*!< (private) ARIRANG-384 */
const PGPHASHALGO_ARIRANG_512	= 208+3;	/*!< (private) ARIRANG-512 */

const PGPHASHALGO_LANE_224	= 212+0;	/*!< (private) LANE-224 */
const PGPHASHALGO_LANE_256	= 212+1;	/*!< (private) LANE-256 */
const PGPHASHALGO_LANE_384	= 212+2;	/*!< (private) LANE-384 */
const PGPHASHALGO_LANE_512	= 212+3;	/*!< (private) LANE-512 */

const PGPHASHALGO_LUFFA_224	= 216+0;	/*!< (private) LUFFA-224 */
const PGPHASHALGO_LUFFA_256	= 216+1;	/*!< (private) LUFFA-256 */
const PGPHASHALGO_LUFFA_384	= 216+2;	/*!< (private) LUFFA-384 */
const PGPHASHALGO_LUFFA_512	= 216+3;	/*!< (private) LUFFA-512 */

const PGPHASHALGO_CHI_224	= 224+0;	/*!< (private) CHI-224 */
const PGPHASHALGO_CHI_256	= 224+1;	/*!< (private) CHI-256 */
const PGPHASHALGO_CHI_384	= 224+2;	/*!< (private) CHI-384 */
const PGPHASHALGO_CHI_512	= 224+3;	/*!< (private) CHI-512 */

const PGPHASHALGO_JH_224	= 232+0;	/*!< (private) JH-224 */
const PGPHASHALGO_JH_256	= 232+1;	/*!< (private) JH-256 */
const PGPHASHALGO_JH_384	= 232+2;	/*!< (private) JH-384 */
const PGPHASHALGO_JH_512	= 232+3;	/*!< (private) JH-512 */

const PGPHASHALGO_GROESTL_224	= 240+0;	/*!< (private) GROESTL-224 */
const PGPHASHALGO_GROESTL_256	= 240+1;	/*!< (private) GROESTL-256 */
const PGPHASHALGO_GROESTL_384	= 240+2;	/*!< (private) GROESTL-384 */
const PGPHASHALGO_GROESTL_512	= 240+3;	/*!< (private) GROESTL-512 */

const PGPHASHALGO_HAMSI_224	= 248+0;	/*!< (private) HAMSI-224 */
const PGPHASHALGO_HAMSI_256	= 248+1;	/*!< (private) HAMSI-256 */
const PGPHASHALGO_HAMSI_384	= 248+2;	/*!< (private) HAMSI-384 */
const PGPHASHALGO_HAMSI_512	= 248+3;	/*!< (private) HAMSI-512 */

var GPSEE = require('rpmdc');

var dc = new GPSEE.Dc();
ack("typeof dc;", "object");
ack("dc instanceof GPSEE.Dc;", true);
ack("dc.debug = 1;", 1);
ack("dc.debug = 0;", 0);

var str = 'abc';

var md2 = new GPSEE.Dc(PGPHASHALGO_MD2);
ack("md2(str);", 'da853b0d3f88d99b30283a69e6ded6bb');
ack("md2.init(PGPHASHALGO_MD2);", true);
ack("md2.algo", PGPHASHALGO_MD2);
ack("md2.asn1", "3020300c06082a864886f70d020205000410");
ack("md2.name", "MD2");
ack("md2.update(str);", true);
ack("md2.fini();", 'da853b0d3f88d99b30283a69e6ded6bb');

var md4 = new GPSEE.Dc(PGPHASHALGO_MD4);
ack("md4(str);", 'a448017aaf21d8525fc10ae87aa6729d');
ack("md4.init(PGPHASHALGO_MD4);", true);
ack("md4.algo", PGPHASHALGO_MD4);
ack("md4.asn1", null);
ack("md4.name", "MD4");
ack("md4.update(str);", true);
ack("md4.fini();", 'a448017aaf21d8525fc10ae87aa6729d');

var md5 = new GPSEE.Dc(PGPHASHALGO_MD5);
ack("md5(str);", '900150983cd24fb0d6963f7d28e17f72');
ack("md5.init(PGPHASHALGO_MD5);", true);
ack("md5.algo", PGPHASHALGO_MD5);
ack("md5.asn1", "3020300c06082a864886f70d020505000410");
ack("md5.name", "MD5");
ack("md5.update(str);", true);
ack("md5.fini();", '900150983cd24fb0d6963f7d28e17f72');

var sha1 = new GPSEE.Dc(PGPHASHALGO_SHA1);
ack("sha1(str);", 'a9993e364706816aba3e25717850c26c9cd0d89d');
ack("sha1.init(PGPHASHALGO_SHA1);", true);
ack("sha1.algo", PGPHASHALGO_SHA1);
ack("sha1.asn1", "3021300906052b0e03021a05000414");
ack("sha1.name", "SHA1");
ack("sha1.update(str);", true);
ack("sha1.fini();", 'a9993e364706816aba3e25717850c26c9cd0d89d');

var sha224 = new GPSEE.Dc(PGPHASHALGO_SHA224);
ack("sha224(str);", '23097d223405d8228642a477bda255b32aadbce4bda0b3f7e36c9da7');
ack("sha224.init(PGPHASHALGO_SHA224);", true);
ack("sha224.algo", PGPHASHALGO_SHA224);
ack("sha224.asn1", "302d300d06096086480165030402040500041C");
ack("sha224.name", "SHA224");
ack("sha224.update(str);", true);
ack("sha224.fini();", '23097d223405d8228642a477bda255b32aadbce4bda0b3f7e36c9da7');

var sha256 = new GPSEE.Dc(PGPHASHALGO_SHA256);
ack("sha256(str);", 'ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad');
ack("sha256.init(PGPHASHALGO_SHA256);", true);
ack("sha256.algo", PGPHASHALGO_SHA256);
ack("sha256.asn1", "3031300d060960864801650304020105000420");
ack("sha256.name", "SHA256");
ack("sha256.update(str);", true);
ack("sha256.fini();", 'ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad');

var sha384 = new GPSEE.Dc(PGPHASHALGO_SHA384);
ack("sha384(str);", 'cb00753f45a35e8bb5a03d699ac65007272c32ab0eded1631a8b605a43ff5bed8086072ba1e7cc2358baeca134c825a7');
ack("sha384.init(PGPHASHALGO_SHA384);", true);
ack("sha384.algo", PGPHASHALGO_SHA384);
ack("sha384.asn1", "3041300d060960864801650304020205000430");
ack("sha384.name", "SHA384");
ack("sha384.update(str);", true);
ack("sha384.fini();", 'cb00753f45a35e8bb5a03d699ac65007272c32ab0eded1631a8b605a43ff5bed8086072ba1e7cc2358baeca134c825a7');

var sha512 = new GPSEE.Dc(PGPHASHALGO_SHA512);
ack("sha512(str);", 'ddaf35a193617abacc417349ae20413112e6fa4e89a97ea20a9eeee64b55d39a2192992a274fc1a836ba3c23a3feebbd454d4423643ce80e2a9ac94fa54ca49f');
ack("sha512.init(PGPHASHALGO_SHA512);", true);
ack("sha512.algo", PGPHASHALGO_SHA512);
ack("sha512.asn1", "3051300d060960864801650304020305000440");
ack("sha512.name", "SHA512");
ack("sha512.update(str);", true);
ack("sha512.fini();", 'ddaf35a193617abacc417349ae20413112e6fa4e89a97ea20a9eeee64b55d39a2192992a274fc1a836ba3c23a3feebbd454d4423643ce80e2a9ac94fa54ca49f');

var salsa10 = new GPSEE.Dc(PGPHASHALGO_SALSA10);
ack("salsa10(str);", '699d0db4d264ce8ba8a1bf74734a29372b689d64c985aa5a037c6e11cbdf328270d58c699523ffc2d10bcc6f9ce167fb63528ea4ef0681fe8172297f9c92d72f');
ack("salsa10.init(PGPHASHALGO_SALSA10);", true);
ack("salsa10.algo", PGPHASHALGO_SALSA10);
ack("salsa10.asn1", null);
ack("salsa10.name", "SALSA10");
ack("salsa10.update(str);", true);
ack("salsa10.fini();", '699d0db4d264ce8ba8a1bf74734a29372b689d64c985aa5a037c6e11cbdf328270d58c699523ffc2d10bcc6f9ce167fb63528ea4ef0681fe8172297f9c92d72f');

var salsa20 = new GPSEE.Dc(PGPHASHALGO_SALSA20);
ack("salsa20(str);", 'fc4f496bb7a2fbe942d170dd42ebaaf6aec7fd9d6563d0563f81ffed6e6ff5cb595237462577ab41efd8c3815f20205a905ad5fd647de1ce2d37894e6671d6a7');
ack("salsa20.init(PGPHASHALGO_SALSA20);", true);
ack("salsa20.algo", PGPHASHALGO_SALSA20);
ack("salsa20.asn1", null);
ack("salsa20.name", "SALSA20");
ack("salsa20.update(str);", true);
ack("salsa20.fini();", 'fc4f496bb7a2fbe942d170dd42ebaaf6aec7fd9d6563d0563f81ffed6e6ff5cb595237462577ab41efd8c3815f20205a905ad5fd647de1ce2d37894e6671d6a7');

var ripemd128 = new GPSEE.Dc(PGPHASHALGO_RIPEMD128);
ack("ripemd128(str);", 'c14a12199c66e4ba84636b0f69144c77');
ack("ripemd128.init(PGPHASHALGO_RIPEMD128);", true);
ack("ripemd128.algo", PGPHASHALGO_RIPEMD128);
ack("ripemd128.asn1", null);
ack("ripemd128.name", "RIPEMD128");
ack("ripemd128.update(str);", true);
ack("ripemd128.fini();", 'c14a12199c66e4ba84636b0f69144c77');

var ripemd160 = new GPSEE.Dc(PGPHASHALGO_RIPEMD160);
ack("ripemd160(str);", '8eb208f7e05d987a9b044a8e98c6b087f15a0bfc');
ack("ripemd160.init(PGPHASHALGO_RIPEMD160);", true);
ack("ripemd160.algo", PGPHASHALGO_RIPEMD160);
ack("ripemd160.asn1", "3021300906052b2403020105000414");
ack("ripemd160.name", "RIPEMD160");
ack("ripemd160.update(str);", true);
ack("ripemd160.fini();", '8eb208f7e05d987a9b044a8e98c6b087f15a0bfc');

var ripemd256 = new GPSEE.Dc(PGPHASHALGO_RIPEMD256);
ack("ripemd256(str);", 'afbd6e228b9d8cbbcef5ca2d03e6dba10ac0bc7dcbe4680e1e42d2e975459b65');
ack("ripemd256.init(PGPHASHALGO_RIPEMD256);", true);
ack("ripemd256.algo", PGPHASHALGO_RIPEMD256);
ack("ripemd256.asn1", null);
ack("ripemd256.name", "RIPEMD256");
ack("ripemd256.update(str);", true);
ack("ripemd256.fini();", 'afbd6e228b9d8cbbcef5ca2d03e6dba10ac0bc7dcbe4680e1e42d2e975459b65');

var ripemd320 = new GPSEE.Dc(PGPHASHALGO_RIPEMD320);
ack("ripemd320(str);", 'de4c01b3054f8930a79d09ae738e92301e5a17085beffdc1b8d116713e74f82fa942d64cdbc4682d');
ack("ripemd320.init(PGPHASHALGO_RIPEMD320);", true);
ack("ripemd320.algo", PGPHASHALGO_RIPEMD320);
ack("ripemd320.asn1", null);
ack("ripemd320.name", "RIPEMD320");
ack("ripemd320.update(str);", true);
ack("ripemd320.fini();", 'de4c01b3054f8930a79d09ae738e92301e5a17085beffdc1b8d116713e74f82fa942d64cdbc4682d');

var skein224 = new GPSEE.Dc(PGPHASHALGO_SKEIN_224);
ack("skein224('');", '50462a6805b74017ac85237077fb122a507911b737c9a66ff056a823');
ack("skein224.init(PGPHASHALGO_SKEIN_224);", true);
ack("skein224.algo", PGPHASHALGO_SKEIN_224);
ack("skein224.asn1", null);
ack("skein224.name", "SKEIN256");
ack("skein224.update('');", true);
ack("skein224.fini();", '50462a6805b74017ac85237077fb122a507911b737c9a66ff056a823');

var skein256 = new GPSEE.Dc(PGPHASHALGO_SKEIN_256);
ack("skein256('');", 'bc2763f707e262b80e0313791543a7ab0a4b6cd083270afb2fce4272e1bb0aa9');
ack("skein256.init(PGPHASHALGO_SKEIN_256);", true);
ack("skein256.algo", PGPHASHALGO_SKEIN_256);
ack("skein256.asn1", null);
ack("skein256.name", "SKEIN256");
ack("skein256.update('');", true);
ack("skein256.fini();", 'bc2763f707e262b80e0313791543a7ab0a4b6cd083270afb2fce4272e1bb0aa9');

var skein384 = new GPSEE.Dc(PGPHASHALGO_SKEIN_384);
ack("skein384('');", 'fa35c62fad08435e2727361228a0da5a72b14c2d7da222dc1f5142ae91f5da6c85c441f96888fccf518be2084cc495f4');
ack("skein384.init(PGPHASHALGO_SKEIN_384);", true);
ack("skein384.algo", PGPHASHALGO_SKEIN_384);
ack("skein384.asn1", null);
ack("skein384.name", "SKEIN512");
ack("skein384.update('');", true);
ack("skein384.fini();", 'fa35c62fad08435e2727361228a0da5a72b14c2d7da222dc1f5142ae91f5da6c85c441f96888fccf518be2084cc495f4');

var skein512 = new GPSEE.Dc(PGPHASHALGO_SKEIN_512);
ack("skein512('');", 'd3f7263a09837f4ce5c8ef70a5ddffac7b92d6c2ace5a12265bd5b593260a3ff20d8b4b4c5494e945448b37abb1fc526f6b46089208fde938d7f23724c4bdfb7');
ack("skein512.init(PGPHASHALGO_SKEIN_512);", true);
ack("skein512.algo", PGPHASHALGO_SKEIN_512);
ack("skein512.asn1", null);
ack("skein512.name", "SKEIN512");
ack("skein512.update('');", true);
ack("skein512.fini();", 'd3f7263a09837f4ce5c8ef70a5ddffac7b92d6c2ace5a12265bd5b593260a3ff20d8b4b4c5494e945448b37abb1fc526f6b46089208fde938d7f23724c4bdfb7');

var skein1024 = new GPSEE.Dc(PGPHASHALGO_SKEIN_1024);
ack("skein1024('');", '7bc3ce31c035df3c7a7d559bc9d9454f879f48497cc39e6d14e8190498455f396f45590405c4a15bd0ab29f5467d9132802f4354376ee864ad9f200e39a3d09b9ad06e9d0f656cf91ed9deac13a7a67a82ab983f52133129cac2a4dc13fd29c36ca6c72d6dcc6c82c66797f6b7607cb0e0f9006a27b83b60c59e4ba18de233e0');
ack("skein1024.init(PGPHASHALGO_SKEIN_1024);", true);
ack("skein1024.algo", PGPHASHALGO_SKEIN_1024);
ack("skein1024.asn1", null);
ack("skein1024.name", "SKEIN1024");
ack("skein1024.update('');", true);
ack("skein1024.fini();", '7bc3ce31c035df3c7a7d559bc9d9454f879f48497cc39e6d14e8190498455f396f45590405c4a15bd0ab29f5467d9132802f4354376ee864ad9f200e39a3d09b9ad06e9d0f656cf91ed9deac13a7a67a82ab983f52133129cac2a4dc13fd29c36ca6c72d6dcc6c82c66797f6b7607cb0e0f9006a27b83b60c59e4ba18de233e0');

var arirang_224 = new GPSEE.Dc(PGPHASHALGO_ARIRANG_224);
ack("arirang_224('');", '1937aa37e0f3262a0d539de60f1a9c5947aea2ba8cc51b7b2d468c23');
ack("arirang_224.init(PGPHASHALGO_ARIRANG_224);", true);
ack("arirang_224.algo", PGPHASHALGO_ARIRANG_224);
ack("arirang_224.asn1", null);
ack("arirang_224.name", "ARIRANG");
ack("arirang_224.update(str);", true);
ack("arirang_224.fini();", '40ce8ec7bb6adac2714d84acb9c0bd027e2d5409c630cb8a3b190a2d');

var arirang_256 = new GPSEE.Dc(PGPHASHALGO_ARIRANG_256);
ack("arirang_256('');", 'aab605111e406a8103bed1495daef867bfa9b70289b50533b9c4817d08a8def2');
ack("arirang_256.init(PGPHASHALGO_ARIRANG_256);", true);
ack("arirang_256.algo", PGPHASHALGO_ARIRANG_256);
ack("arirang_256.asn1", null);
ack("arirang_256.name", "ARIRANG");
ack("arirang_256.update(str);", true);
ack("arirang_256.fini();", '379e5431242a47052681fd06135c9a12b63fd6ff85ef9ce4272cdc9eb78e4eaf');

var arirang_384 = new GPSEE.Dc(PGPHASHALGO_ARIRANG_384);
ack("arirang_384('');", 'e413208d8dfd0ceb37cccc149e5afc16c458fdffe6ee8213bc2f69c5f617da26d84aec6123a2f6549aa6441fba165f5f');
ack("arirang_384.init(PGPHASHALGO_ARIRANG_384);", true);
ack("arirang_384.algo", PGPHASHALGO_ARIRANG_384);
ack("arirang_384.asn1", null);
ack("arirang_384.name", "ARIRANG");
ack("arirang_384.update(str);", true);
ack("arirang_384.fini();", '98b3eef2413ddd853187b2ea5b40d92e6fd1fb61f343633f6070eae71212d264ef035bea780fa22ceae3d491c986bb5b');

var arirang_512 = new GPSEE.Dc(PGPHASHALGO_ARIRANG_512);
ack("arirang_512('');", '65d6e76d7cc4065eed05d0a825ca99de85f4cf275786b37f9ff5390ffefa0ffe463405df20f3fc5285c7511ca2df60400db636a0a370f36c487c0b54df8280c3');
ack("arirang_512.init(PGPHASHALGO_ARIRANG_512);", true);
ack("arirang_512.algo", PGPHASHALGO_ARIRANG_512);
ack("arirang_512.asn1", null);
ack("arirang_512.name", "ARIRANG");
ack("arirang_512.update(str);", true);
ack("arirang_512.fini();", '84a38a38f35344d34c4a94a551816b3f9075f7ec2f6415a87c75e8472da6490482e1d489a803b41c5ad74c9d8b7cb7e5a8cd5b73ad22ed24549985daa9007e0c');

var blake224 = new GPSEE.Dc(PGPHASHALGO_BLAKE_224);
ack("blake224('');", '53f6633363b0cf3d4253963628555b5e8961339d39f057fc3782471b');
ack("blake224.init(PGPHASHALGO_BLAKE_224);", true);
ack("blake224.algo", PGPHASHALGO_BLAKE_224);
ack("blake224.asn1", null);
ack("blake224.name", "BLAKE");
ack("blake224.update(str);", true);
ack("blake224.fini();", 'a94de9b941e6ba54696e2cc6831bcb8381158bed49d3311314fbd20e');

var blake256 = new GPSEE.Dc(PGPHASHALGO_BLAKE_256);
ack("blake256('');", '73be7e1e0a7d0a2f0035edae62d4412ec43c0308145b5046849a53756bcda44b');
ack("blake256.init(PGPHASHALGO_BLAKE_256);", true);
ack("blake256.algo", PGPHASHALGO_BLAKE_256);
ack("blake256.asn1", null);
ack("blake256.name", "BLAKE");
ack("blake256.update(str);", true);
ack("blake256.fini();", 'cca0f6deb4727e1677020c8cebf8f49434002d1a8a9d86970d73eb0ee709ff43');

var blake384 = new GPSEE.Dc(PGPHASHALGO_BLAKE_384);
ack("blake384('');", 'e0820c066f522138d5cb3a5773dea16db434afa95e1c48e060de466928bb7044391b3ee77e2bbff6c0cf1e07a8295100');
ack("blake384.init(PGPHASHALGO_BLAKE_384);", true);
ack("blake384.algo", PGPHASHALGO_BLAKE_384);
ack("blake384.asn1", null);
ack("blake384.name", "BLAKE");
ack("blake384.update(str);", true);
ack("blake384.fini();", 'e22ddc662dd6c55ca5928fc954f55d5a288dcd69afd67971b90a383f0e2ef5d086dbca48186b6696dc041c78a4dd202c');

var blake512 = new GPSEE.Dc(PGPHASHALGO_BLAKE_512);
ack("blake512('');", '223d88a8c8308c15d479d1668ba97b1b2737aad82debd7d05d32f77a13f820651c36fc9eb18e2101b8e992717e671400be6a7f158cdd64afed6f81e62bf15c37');
ack("blake512.init(PGPHASHALGO_BLAKE_512);", true);
ack("blake512.algo", PGPHASHALGO_BLAKE_512);
ack("blake512.asn1", null);
ack("blake512.name", "BLAKE");
ack("blake512.update(str);", true);
ack("blake512.fini();", '73d4b67de1bedd9f4c7864d5e8b388a0c317d032c3e82df534f614dc5e91ca5b8a2e8310d92845909193f47a73bb2205a996952abe1e89978e907cd4027c35bb');

var bmw224 = new GPSEE.Dc(PGPHASHALGO_BMW_224);
ack("bmw224('');", 'bdf29a829d3f42d50604acb5ad1a851a8ec684a86ff7c4e791aad501');
ack("bmw224.init(PGPHASHALGO_BMW_224);", true);
ack("bmw224.algo", PGPHASHALGO_BMW_224);
ack("bmw224.asn1", null);
ack("bmw224.name", "BMW");
ack("bmw224.update(str);", true);
ack("bmw224.fini();", '26088053c93cbc480ad53c34fc66fff0b4f1341adb855154fa15ee54');

var bmw256 = new GPSEE.Dc(PGPHASHALGO_BMW_256);
ack("bmw256('');", '4c10f8ad695fff1fb275d175b44cbe41c40a53a166d0470b45bab2d9afb6e5c5');
ack("bmw256.init(PGPHASHALGO_BMW_256);", true);
ack("bmw256.algo", PGPHASHALGO_BMW_256);
ack("bmw256.asn1", null);
ack("bmw256.name", "BMW");
ack("bmw256.update(str);", true);
ack("bmw256.fini();", '919905e2d111e6f57cb42e5e31c9240eb0670c1919718a5c1b10e61fc5124d25');

var bmw384 = new GPSEE.Dc(PGPHASHALGO_BMW_384);
ack("bmw384('');", '62f300cced944e44fdd4e51e809c09eeefd31ee58bf977f29b0f475cb16c2f557b723553b9ab563e01d21a11e9d728e2');
ack("bmw384.init(PGPHASHALGO_BMW_384);", true);
ack("bmw384.algo", PGPHASHALGO_BMW_384);
ack("bmw384.asn1", null);
ack("bmw384.name", "BMW");
ack("bmw384.update(str);", true);
ack("bmw384.fini();", '1ef8b57640c0419e1f8c0d7ebc4196ea31fdb5fb9d2350c80db002882f59d3d1005899cd98353751840e34f619ec7abc');

var bmw512 = new GPSEE.Dc(PGPHASHALGO_BMW_512);
ack("bmw512('');", '73db7b1dc6def4ddf2f94a22e1f6d2162b3123828369ff4fd750832aede94e52d4d5c8b866860424991527175b6f62dbe0f764ac18290b92a26812d641cd5287');
ack("bmw512.init(PGPHASHALGO_BMW_512);", true);
ack("bmw512.algo", PGPHASHALGO_BMW_512);
ack("bmw512.asn1", null);
ack("bmw512.name", "BMW");
ack("bmw512.update(str);", true);
ack("bmw512.fini();", 'ded83592522d34271257b338337559204949bc4dbd9a6e66148393575673bae87334b76a3a3fc2734138f3074985ac8af8ff71ce0ca31cd7645df30849936dcf');

var chi_224 = new GPSEE.Dc(PGPHASHALGO_CHI_224);
ack("chi_224('');", '532af150697beba8ac038c53fac985a367701991a34f5c6502cfb7f0');
ack("chi_224.init(PGPHASHALGO_CHI_224);", true);
ack("chi_224.algo", PGPHASHALGO_CHI_224);
ack("chi_224.asn1", null);
ack("chi_224.name", "CHI");
ack("chi_224.update(str);", true);
ack("chi_224.fini();", '2a09fb790d6817e825e37933fb5fdc74e05cd74df7a7f2d6cc903176');

var chi_256 = new GPSEE.Dc(PGPHASHALGO_CHI_256);
ack("chi_256('');", 'fa11172a4a65e8efe6ec9c5ddafac765935fab6be89604edb17902b507302270');
ack("chi_256.init(PGPHASHALGO_CHI_256);", true);
ack("chi_256.algo", PGPHASHALGO_CHI_256);
ack("chi_256.asn1", null);
ack("chi_256.name", "CHI");
ack("chi_256.update(str);", true);
ack("chi_256.fini();", 'cf01da4d76a83ccc4003805be462cd898e243fb36ada110209661aad7d19ed37');

var chi_384 = new GPSEE.Dc(PGPHASHALGO_CHI_384);
ack("chi_384('');", '841eae69ce0d7edc0ab1b3038eb7fd62bd3b73b8e3481798243cf14e258dee93e433e5202e806a18f44f4383b3168f59');
ack("chi_384.init(PGPHASHALGO_CHI_384);", true);
ack("chi_384.algo", PGPHASHALGO_CHI_384);
ack("chi_384.asn1", null);
ack("chi_384.name", "CHI");
ack("chi_384.update(str);", true);
ack("chi_384.fini();", '924ba150d5fcf6862d4570cff69f03c343c6dea3e6db429f1883534b19d06f60157f972063e6fa162ca3337b73872790');

var chi_512 = new GPSEE.Dc(PGPHASHALGO_CHI_512);
ack("chi_512('');", '5ee66ada5094be5d8b822716bb5bdad19f58a632be85a1643f535fca6e109467cd609f16f1f46de6afd16a41a2a4ab932e997e261a4bf71931d804ea8dabcd1a');
ack("chi_512.init(PGPHASHALGO_CHI_512);", true);
ack("chi_512.algo", PGPHASHALGO_CHI_512);
ack("chi_512.asn1", null);
ack("chi_512.name", "CHI");
ack("chi_512.update(str);", true);
ack("chi_512.fini();", '0a71c7e78234e628509d7737ba22e03eea2731ace5d9e3d219641d2b5e71fc957d4bb09e9d40925ebe3e69b7077a64825188662aaccd858d3a8c6e9b6b3af3e3');

var cubehash224 = new GPSEE.Dc(PGPHASHALGO_CUBEHASH_224, 0x0801);
ack("cubehash224('');", 'b5a6f6cb6d4100dcda8f575c694f15b2f7c8c5ed145608a42a89c7ca');
ack("cubehash224.init(PGPHASHALGO_CUBEHASH_224, 0x0801);", true);
ack("cubehash224.algo", PGPHASHALGO_CUBEHASH_224);
ack("cubehash224.asn1", null);
ack("cubehash224.name", "CUBEHASH");
ack("cubehash224.update(str);", true);
ack("cubehash224.fini();", '50151e3b6b2d13a9da38aa1422ab2dacc3500bebd4215d036fdcd5ce');
ack("cubehash224.init(PGPHASHALGO_CUBEHASH_224);", true);
ack("cubehash224('');", 'f9802aa6955f4b7cf3b0f5a378fa0c9f138e0809d250966879c873ab');
ack("cubehash224(str);", '6b45504b39316bfd48dc44638a363c16b3f0263d66561b09d7d21fd7');

var cubehash256 = new GPSEE.Dc(PGPHASHALGO_CUBEHASH_256, 0x0801);
ack("cubehash256('');", '38d1e8a22d7baac6fd5262d83de89cacf784a02caa866335299987722aeabc59');
ack("cubehash256.init(PGPHASHALGO_CUBEHASH_256, 0x0801);", true);
ack("cubehash256.algo", PGPHASHALGO_CUBEHASH_256);
ack("cubehash256.asn1", null);
ack("cubehash256.name", "CUBEHASH");
ack("cubehash256.update(str);", true);
ack("cubehash256.fini();", '8de2181ab5ae4365a506cdf748f3af4b52a7b838a2c82550b8329bb6339914d7');
ack("cubehash256.init(PGPHASHALGO_CUBEHASH_256);", true);
ack("cubehash256('');", '44c6de3ac6c73c391bf0906cb7482600ec06b216c7c54a2a8688a6a42676577d');
ack("cubehash256(str);", 'a220b4bf5023e750c2a34dcd5564a8523d32e17fab6fbe0f18a0b0bf5a65632b');

var cubehash384 = new GPSEE.Dc(PGPHASHALGO_CUBEHASH_384, 0x0801);
ack("cubehash384('');", '235e819ebb93af765f7d86df6c6ff283ab24e98a07858a7d1c72604bb10c794d4721ef9ddfccaa93072eee9b53fdc69c');
ack("cubehash384.init(PGPHASHALGO_CUBEHASH_384, 0x0801);", true);
ack("cubehash384.algo", PGPHASHALGO_CUBEHASH_384);
ack("cubehash384.asn1", null);
ack("cubehash384.name", "CUBEHASH");
ack("cubehash384.update(str);", true);
ack("cubehash384.fini();", 'a6cd4e9e01f83d75a3c5b30f7c9700216a453a2c9a9399181a0ad7c52a902f12f0a3301fec05428cda66abc16c5ca7e3');
ack("cubehash384.init(PGPHASHALGO_CUBEHASH_384);", true);
ack("cubehash384('');", '98ae93ebf4e58958497f610a22c8cf60f2292319283ca6459daed1707be06e7591c5f2d84bd3339e66c770e485bfa1fb');
ack("cubehash384(str);", '287cc1738bdb9575fd716bafbb02768ce5a57ae5c08ba12f5cf74fac27ab5707e577bc93539c07af9ab92c3b1b368997');

var cubehash512 = new GPSEE.Dc(PGPHASHALGO_CUBEHASH_512, 0x0801);
ack("cubehash512('');", '90bc3f2948f7374065a811f1e47a208a53b1a2f3be1c0072759ed49c9c6c7f28f26eb30d5b0658c563077d599da23f97df0c2c0ac6cce734ffe87b2e76ff7294');
ack("cubehash512.init(PGPHASHALGO_CUBEHASH_512, 0x0801);", true);
ack("cubehash512.algo", PGPHASHALGO_CUBEHASH_512, 0x0801);
ack("cubehash512.asn1", null);
ack("cubehash512.name", "CUBEHASH");
ack("cubehash512.update(str);", true);
ack("cubehash512.fini();", 'f83d39f3f4213dbe240aa14740b214741163f37be49750cc9bf64aaa58be8f8adee7874186475cec08f7993ca7e35839291816ccc377d6173987eb95e355ee73');
ack("cubehash512.init(PGPHASHALGO_CUBEHASH_512);", true);
ack("cubehash512('');", '4a1d00bbcfcb5a9562fb981e7f7db3350fe2658639d948b9d57452c22328bb32f468b072208450bad5ee178271408be0b16e5633ac8a1e3cf9864cfbfc8e043a');
ack("cubehash512(str);", 'f63d6fa89ca9fe7ab2e171be52cf193f0c8ac9f62bad297032c1e7571046791a7e8964e5c8d91880d6f9c2a54176b05198901047438e05ac4ef38d45c0282673');

var echo_224 = new GPSEE.Dc(PGPHASHALGO_ECHO_224);
ack("echo_224('');", '17da087595166f733fff7cdb0bca6438f303d0e00c48b5e7a3075905');
ack("echo_224.init(PGPHASHALGO_ECHO_224);", true);
ack("echo_224.algo", PGPHASHALGO_ECHO_224);
ack("echo_224.asn1", null);
ack("echo_224.name", "ECHO");
ack("echo_224.update(str);", true);
ack("echo_224.fini();", 'd4f3807187a07cb8e593485e311425e68aaa00a3715789bfa66f09cd');

var echo_256 = new GPSEE.Dc(PGPHASHALGO_ECHO_256);
ack("echo_256('');", '4496cd09d425999aefa75189ee7fd3c97362aa9e4ca898328002d20a4b519788');
ack("echo_256.init(PGPHASHALGO_ECHO_256);", true);
ack("echo_256.algo", PGPHASHALGO_ECHO_256);
ack("echo_256.asn1", null);
ack("echo_256.name", "ECHO");
ack("echo_256.update(str);", true);
ack("echo_256.fini();", '871b1fad479135c37e1aad71ac9a99def41730f3e5b3e0dc3f6b7cf072fa5649');

var echo_384 = new GPSEE.Dc(PGPHASHALGO_ECHO_384);
ack("echo_384('');", '134040763f840559b84b7a1ae5d6d64fc3659821a789cc64a7f1444c09ee7f81a54d72beee8273bae5ef18ec43aa5f34');
ack("echo_384.init(PGPHASHALGO_ECHO_384);", true);
ack("echo_384.algo", PGPHASHALGO_ECHO_384);
ack("echo_384.asn1", null);
ack("echo_384.name", "ECHO");
ack("echo_384.update(str);", true);
ack("echo_384.fini();", '94cbb881848c45b7f6649b7b36901d14973248d9bfa318bd830d1c14d749e7e9bf0a69ce738ac8a1fd361411a8dc9dae');

var echo_512 = new GPSEE.Dc(PGPHASHALGO_ECHO_512);
ack("echo_512('');", '158f58cc79d300a9aa292515049275d051a28ab931726d0ec44bdd9faef4a702c36db9e7922fff077402236465833c5cc76af4efc352b4b44c7fa15aa0ef234e');
ack("echo_512.init(PGPHASHALGO_ECHO_512);", true);
ack("echo_512.algo", PGPHASHALGO_ECHO_512);
ack("echo_512.asn1", null);
ack("echo_512.name", "ECHO");
ack("echo_512.update(str);", true);
ack("echo_512.fini();", '3bf04ec89d67e0dafd1b8ab26b176abaead6b3cdc706ff7198c3c6045e77d4eaf64cd90af9c5a7674919b90ff8c9b4a7554d6cfeffb334406ec233fb0b0dd6bc');

var edonr224 = new GPSEE.Dc(PGPHASHALGO_EDONR_224);
ack("edonr224('');", 'a9c2bc54208be171cdfd054d21d97c1f4c79e822d8d9fcdbcb1d602f');
ack("edonr224.init(PGPHASHALGO_EDONR_224);", true);
ack("edonr224.algo", PGPHASHALGO_EDONR_224);
ack("edonr224.asn1", null);
ack("edonr224.name", "EDON-R");
ack("edonr224.update(str);", true);
ack("edonr224.fini();", '5663c4939520faf6123165a466f25601952ea9e424ddc96befd04094');

var edonr256 = new GPSEE.Dc(PGPHASHALGO_EDONR_256);
ack("edonr256('');", '3b71fd43dade942c07842b181f4d987e78d2ac7e3a7e8bb06fec99a60b60eaba');
ack("edonr256.init(PGPHASHALGO_EDONR_256);", true);
ack("edonr256.algo", PGPHASHALGO_EDONR_256);
ack("edonr256.asn1", null);
ack("edonr256.name", "EDON-R");
ack("edonr256.update(str);", true);
ack("edonr256.fini();", '54d78b13c74eda5aedc271cc881fb22f8399afd3040b6a392d739405508dd851');

var edonr384 = new GPSEE.Dc(PGPHASHALGO_EDONR_384);
ack("edonr384('');", 'ccd8e612b93a3f8c24867eb204c1dc3f2de24bd54d92908c5f5a73c6a4da14c8742a5fc3a6b5428658e85f0175de95a7');
ack("edonr384.init(PGPHASHALGO_EDONR_384);", true);
ack("edonr384.algo", PGPHASHALGO_EDONR_384);
ack("edonr384.asn1", null);
ack("edonr384.name", "EDON-R");
ack("edonr384.update(str);", true);
ack("edonr384.fini();", '0e7cd7857877e0895b1cdf49f41d209c727d2e579b9b9adc60279782b99072ec7eced3165f477548fa60727e01c77cc6');

var edonr512 = new GPSEE.Dc(PGPHASHALGO_EDONR_512);
ack("edonr512('');", 'c57f7e17fdc1ce5074cc748c9bd38f9f51ebe88fbe6eda3190c4314cafb1abb2980fac582d6e4ba8c641947d944bc56b74fb15de5546ad4f77934fca00052719');
ack("edonr512.init(PGPHASHALGO_EDONR_512);", true);
ack("edonr512.algo", PGPHASHALGO_EDONR_512);
ack("edonr512.asn1", null);
ack("edonr512.name", "EDON-R");
ack("edonr512.update(str);", true);
ack("edonr512.fini();", '1b14db155f1d406594b8cef70a4362ec6b5de6a5daf50ec999e987c19d3049e2de5977bb05b1bb220050a1ea5b46a9f1740acafbf6b45032adc90c628372c22b');

var fugue_224 = new GPSEE.Dc(PGPHASHALGO_FUGUE_224);
ack("fugue_224('');", 'e2cd30d51a913c4ed2388a141f90caa4914de43010849e7b8a7a9ccd');
ack("fugue_224.init(PGPHASHALGO_FUGUE_224);", true);
ack("fugue_224.algo", PGPHASHALGO_FUGUE_224);
ack("fugue_224.asn1", null);
ack("fugue_224.name", "FUGUE");
ack("fugue_224.update(str);", true);
ack("fugue_224.fini();", '5e12f2cfb0b72d27c81a50d591741625138500a3151edea3d72813f8');

var fugue_256 = new GPSEE.Dc(PGPHASHALGO_FUGUE_256);
ack("fugue_256('');", 'd6ec528980c130aad1d1acd28b9dd8dbdeae0d79eded1fca72c2af9f37c2246f');
ack("fugue_256.init(PGPHASHALGO_FUGUE_256);", true);
ack("fugue_256.algo", PGPHASHALGO_FUGUE_256);
ack("fugue_256.asn1", null);
ack("fugue_256.name", "FUGUE");
ack("fugue_256.update(str);", true);
ack("fugue_256.fini();", '2b6cca67a397ecd4f27a0bed242b04f328b89ff5c51c61da1e69cc5a7419084c');

var fugue_384 = new GPSEE.Dc(PGPHASHALGO_FUGUE_384);
ack("fugue_384('');", '466d05f6812b58b8628e53816b2a99d173b804a964de971829159c3791ac8b524eebbf5fc73ba40ea8eea446d5424a30');
ack("fugue_384.init(PGPHASHALGO_FUGUE_384);", true);
ack("fugue_384.algo", PGPHASHALGO_FUGUE_384);
ack("fugue_384.asn1", null);
ack("fugue_384.name", "FUGUE");
ack("fugue_384.update(str);", true);
ack("fugue_384.fini();", '5e5e10e8b19c2895d406a998ca860cb5698ec950fb4c65bbc41b9f4f53737750e38ff896998df9968066899c3ad5431f');

var fugue_512 = new GPSEE.Dc(PGPHASHALGO_FUGUE_512);
ack("fugue_512('');", '3124f0cbb5a1c2fb3ce747ada63ed2ab3bcd74795cef2b0e805d5319fcc360b4617b6a7eb631d66f6d106ed0724b56fa8c1110f9b8df1c6898e7ca3c2dfccf79');
ack("fugue_512.init(PGPHASHALGO_FUGUE_512);", true);
ack("fugue_512.algo", PGPHASHALGO_FUGUE_512);
ack("fugue_512.asn1", null);
ack("fugue_512.name", "FUGUE");
ack("fugue_512.update(str);", true);
ack("fugue_512.fini();", '6d3c8217e4ddcb6ad1e651ff24be6a50b177c11e0df51ddcf7e90787274927bf1d77bf870d74551c00818fd1d049f384f8c56a8c2c040de2febef2637dc4b20f');

var groestl_224 = new GPSEE.Dc(PGPHASHALGO_GROESTL_224);
ack("groestl_224('');", '07f3750f4831d8ad1763cd46ab6b40c6c49f1cdcc78bb64f8d40c7df');
ack("groestl_224.init(PGPHASHALGO_GROESTL_224);", true);
ack("groestl_224.algo", PGPHASHALGO_GROESTL_224);
ack("groestl_224.asn1", null);
ack("groestl_224.name", "GROESTL");
ack("groestl_224.update(str);", true);
ack("groestl_224.fini();", '321c42cdba3fa677c3cb5dcf4dc3e2556060c8878eec84290b6b4155');

var groestl_256 = new GPSEE.Dc(PGPHASHALGO_GROESTL_256);
ack("groestl_256('');", '6c462dc6861c19dbc6fa818f3da84bccf6eab703db10a9781eec6633ea9e8b7b');
ack("groestl_256.init(PGPHASHALGO_GROESTL_256);", true);
ack("groestl_256.algo", PGPHASHALGO_GROESTL_256);
ack("groestl_256.asn1", null);
ack("groestl_256.name", "GROESTL");
ack("groestl_256.update(str);", true);
ack("groestl_256.fini();", '90f0b04123a30b8b1cd805624e1f64096e0462547f6eaa3145abcda822ec26ea');

var groestl_384 = new GPSEE.Dc(PGPHASHALGO_GROESTL_384);
ack("groestl_384('');", '7dbc0745fc81f89cf3ae0148c42fc5f0106af016d23de296364fa0b03befdebb284e87ac093132419db98d7e1d73fbfa');
ack("groestl_384.init(PGPHASHALGO_GROESTL_384);", true);
ack("groestl_384.algo", PGPHASHALGO_GROESTL_384);
ack("groestl_384.asn1", null);
ack("groestl_384.name", "GROESTL");
ack("groestl_384.update(str);", true);
ack("groestl_384.fini();", 'fe8dd63c8c51253e3a9d271e1fc496c6317ab2c868bc73456d68a560a8d1f758b864ec39f1be185cac7d8924392feeb0');

var groestl_512 = new GPSEE.Dc(PGPHASHALGO_GROESTL_512);
ack("groestl_512('');", 'a94b5251dc711c2813d70d58dd4f84648f90d2700d9417b4c58070069cf68fa86a720e7ee409d64d06adce285bfd60e09d5bcff5bbe7ca2922b96869ae6489be');
ack("groestl_512.init(PGPHASHALGO_GROESTL_512);", true);
ack("groestl_512.algo", PGPHASHALGO_GROESTL_512);
ack("groestl_512.asn1", null);
ack("groestl_512.name", "GROESTL");
ack("groestl_512.update(str);", true);
ack("groestl_512.fini();", '1f2dcaa986d01d212fb633886fcb11f8aaeea18ba7ddd5a251cfc490f9b2850b78ab5ca7870014d21f880dac4cce07e66ba125071a6a30a3b3e35f19cbd15a20');

var hamsi_224 = new GPSEE.Dc(PGPHASHALGO_HAMSI_224);
ack("hamsi_224('');", '6f5708887722a92764a4d55527feaeba32f297f05d35a8276301d508');
ack("hamsi_224.init(PGPHASHALGO_HAMSI_224);", true);
ack("hamsi_224.algo", PGPHASHALGO_HAMSI_224);
ack("hamsi_224.asn1", null);
ack("hamsi_224.name", "HAMSI");
ack("hamsi_224.update(str);", true);
ack("hamsi_224.fini();", 'f5942eef7b5f487e8bb94407e0cebcec853214440af688455786e806');

var hamsi_256 = new GPSEE.Dc(PGPHASHALGO_HAMSI_256);
ack("hamsi_256('');", '750e9ec469f4db626bee7e0c10ddaa1bd01fe194b94efbabebd24764dc2b13e9');
ack("hamsi_256.init(PGPHASHALGO_HAMSI_256);", true);
ack("hamsi_256.algo", PGPHASHALGO_HAMSI_256);
ack("hamsi_256.asn1", null);
ack("hamsi_256.name", "HAMSI");
ack("hamsi_256.update(str);", true);
ack("hamsi_256.fini();", '6b017b90971fdb646700dea0e50e7ac1f6a75a849b2809a55eedcde4c65daf1f');

var hamsi_384 = new GPSEE.Dc(PGPHASHALGO_HAMSI_384);
ack("hamsi_384('');", 'fb38fae25a490c57839c01b858a1f641a2d2c027cec26827d07c0685f7756721b952d9cf4ab1e196663be21b0bbc1b60');
ack("hamsi_384.init(PGPHASHALGO_HAMSI_384);", true);
ack("hamsi_384.algo", PGPHASHALGO_HAMSI_384);
ack("hamsi_384.asn1", null);
ack("hamsi_384.name", "HAMSI");
ack("hamsi_384.update(str);", true);
ack("hamsi_384.fini();", 'edc3513afd752245c93e26aa83ec95086b5f25cce61a0aa7a8522f5f263340011a631e8b7e4903ad926bdff7bcb19cfe');

var hamsi_512 = new GPSEE.Dc(PGPHASHALGO_HAMSI_512);
ack("hamsi_512('');", 'd9bd390844eab0e08ebc8e687fc09e2d22fbf023ebed5c667b6f760fd2e9164e03846ed3df52bd323ab0b748b57575c2d85fece5fc0e99a90f786d14f6d443b3');
ack("hamsi_512.init(PGPHASHALGO_HAMSI_512);", true);
ack("hamsi_512.algo", PGPHASHALGO_HAMSI_512);
ack("hamsi_512.asn1", null);
ack("hamsi_512.name", "HAMSI");
ack("hamsi_512.update(str);", true);
ack("hamsi_512.fini();", '74a3b36a4cdf908973db6ace5b680ea27f5a70bbf57ae5fb5683135a5b1c70a3929835b68af9aec61ffd0a964c552c2bf8cd62c90ea397411a50a3683b98ffc6');

var jh224 = new GPSEE.Dc(PGPHASHALGO_JH_224);
ack("jh224('');", '12c53596fb61ad2865c0a39b7efe88166f9eb1f5fc5b434b9c45057e');
ack("jh224.init(PGPHASHALGO_JH_224);", true);
ack("jh224.algo", PGPHASHALGO_JH_224);
ack("jh224.asn1", null);
ack("jh224.name", "JH");
ack("jh224.update(str);", true);
ack("jh224.fini();", '31fcf15a8e9bc7c4feae367e1bbd08c5861b389cbe369024f6024666');

var jh256 = new GPSEE.Dc(PGPHASHALGO_JH_256);
ack("jh256('');", '020cdd61951fdcadcc544096176332b213b4604ac42dfa82026b7cd750a0a90b');
ack("jh256.init(PGPHASHALGO_JH_256);", true);
ack("jh256.algo", PGPHASHALGO_JH_256);
ack("jh256.asn1", null);
ack("jh256.name", "JH");
ack("jh256.update(str);", true);
ack("jh256.fini();", '76804776ead1964566b1918f96deed2936d7648666660128820d73531e29aae6');

var jh384 = new GPSEE.Dc(PGPHASHALGO_JH_384);
ack("jh384('');", '604cb69a44994b89d8d25ade362d7e3304532862ecde225dfbff8c76dc9a236754e6c662463b19b7e8d034f3da0b4b41');
ack("jh384.init(PGPHASHALGO_JH_384);", true);
ack("jh384.algo", PGPHASHALGO_JH_384);
ack("jh384.asn1", null);
ack("jh384.name", "JH");
ack("jh384.update(str);", true);
ack("jh384.fini();", 'e936ff1633661dd71de4e59f46980893cdd13cce315cc633d6b9dd3b2411b6063662cf5c9cb52b37d385e3f6f5489b9c');

var jh512 = new GPSEE.Dc(PGPHASHALGO_JH_512);
ack("jh512('');", '96d728dd0c96091d228c962b5013a9e4248af4a6eee112d71ee02930a62c8a9a0adcd4f710e297c8f6c24342106ef276f8e4cf45d220e0cc39aed85bd071c31f');
ack("jh512.init(PGPHASHALGO_JH_512);", true);
ack("jh512.algo", PGPHASHALGO_JH_512);
ack("jh512.asn1", null);
ack("jh512.name", "JH");
ack("jh512.update(str);", true);
ack("jh512.fini();", '4d604cbc28a6c0e457baeec929a0fbfe750fd446d66c6353aba51940c49f98aae3ca95d08cf1acd96d348917f2fc37ab6a561e272e9566251873e293bbce2578');

var keccak224 = new GPSEE.Dc(PGPHASHALGO_KECCAK_224);
ack("keccak224('');", '6c60c1d4dc10aee01988c45a33b38bc3045971724ce7e83cdda61635');
ack("keccak224.init(PGPHASHALGO_KECCAK_224);", true);
ack("keccak224.algo", PGPHASHALGO_KECCAK_224);
ack("keccak224.asn1", null);
ack("keccak224.name", "KECCAK");
ack("keccak224.update(str);", true);
ack("keccak224.fini();", '78e64e68f6be46a65dfc1a255c3c5ad6e1ab24be22260ddf059d8dd7');

var keccak256 = new GPSEE.Dc(PGPHASHALGO_KECCAK_256);
ack("keccak256('');", 'bcde039a63f98b125e7fe5cb8999c05dab163f857bcae719fb09b8d5e1da6f0c');
ack("keccak256.init(PGPHASHALGO_KECCAK_256);", true);
ack("keccak256.algo", PGPHASHALGO_KECCAK_256);
ack("keccak256.asn1", null);
ack("keccak256.name", "KECCAK");
ack("keccak256.update(str);", true);
ack("keccak256.fini();", '4e51e1369fe550ba6daeba7068701a653c1216762f41427d976789345687f056');

var keccak384 = new GPSEE.Dc(PGPHASHALGO_KECCAK_384);
ack("keccak384('');", '3f65c0fbe79d43f11d844a448a61b8316db1b681c252f9f5f3fd4da255a655187cff6c0ef96c8c9e7df899a36aa783a9');
ack("keccak384.init(PGPHASHALGO_KECCAK_384);", true);
ack("keccak384.algo", PGPHASHALGO_KECCAK_384);
ack("keccak384.asn1", null);
ack("keccak384.name", "KECCAK");
ack("keccak384.update(str);", true);
ack("keccak384.fini();", 'bf1c5ae872a458211fbe9fd263da97341697003d36f315148d17f7d407caad126443581a2c7c86aa91f5de4ee6375963');

var keccak512 = new GPSEE.Dc(PGPHASHALGO_KECCAK_512);
ack("keccak512('');", '8596f8df2e856ec888823da8ccc914139f31baee6aa5c37dbe30bddbfd75c63cdc205f15f30faa348e27b5f90495b339a606e3c84bfcdcd55e88b0e178b56feb');
ack("keccak512.init(PGPHASHALGO_KECCAK_512);", true);
ack("keccak512.algo", PGPHASHALGO_KECCAK_512);
ack("keccak512.asn1", null);
ack("keccak512.name", "KECCAK");
ack("keccak512.update(str);", true);
ack("keccak512.fini();", '4a2e21878d2785dffb751bb0c635e1f5780152922ffe7ef5342f7442d877754a3f866cd5b2d9f2711b02b24f64e437e4484a8d24b7878d288e9c550729ff954e');

var lane_224 = new GPSEE.Dc(PGPHASHALGO_LANE_224);
ack("lane_224('');", '059b1d054b857bb991c68f42122b6871b3b36c4a3af2d50899a73cda');
ack("lane_224.init(PGPHASHALGO_LANE_224);", true);
ack("lane_224.algo", PGPHASHALGO_LANE_224);
ack("lane_224.asn1", null);
ack("lane_224.name", "LANE");
ack("lane_224.update(str);", true);
ack("lane_224.fini();", '2056437f23356c417f68e0b6827839361d052ed02250386bf2b1623f');

var lane_256 = new GPSEE.Dc(PGPHASHALGO_LANE_256);
ack("lane_256('');", '39d0a057848d3b41a1539a9d1fb843d95c7cac409bdd2597655542584eda637b');
ack("lane_256.init(PGPHASHALGO_LANE_256);", true);
ack("lane_256.algo", PGPHASHALGO_LANE_256);
ack("lane_256.asn1", null);
ack("lane_256.name", "LANE");
ack("lane_256.update(str);", true);
ack("lane_256.fini();", '7cc93b0901d29b0fdf354af65184bc7bc4af179b9270ddf3727cac33e398d0ec');

var lane_384 = new GPSEE.Dc(PGPHASHALGO_LANE_384);
ack("lane_384('');", 'a77d6bd42e74f21b9ec470ae0525c53f5b35d6b6c3241f8007f4cacc6aa496df663a90a35eef8d45703452742e33110c');
ack("lane_384.init(PGPHASHALGO_LANE_384);", true);
ack("lane_384.algo", PGPHASHALGO_LANE_384);
ack("lane_384.asn1", null);
ack("lane_384.name", "LANE");
ack("lane_384.update(str);", true);
ack("lane_384.fini();", '826d911054abe9b781ad60a6e9332fc816b377a3c4f63aa699cc2b4fb78a42fe8b06d9ad89b8e297ec7b6be6c9ac8d40');

var lane_512 = new GPSEE.Dc(PGPHASHALGO_LANE_512);
ack("lane_512('');", 'bdee2ca1f13ab522a3a9e045dc6f236deab315dc8c322ee20333837762a422ca43bcd6f79964cded6531011f3207b76a6859097eaa5fc6e865bedfa80d73ee91');
ack("lane_512.init(PGPHASHALGO_LANE_512);", true);
ack("lane_512.algo", PGPHASHALGO_LANE_512);
ack("lane_512.asn1", null);
ack("lane_512.name", "LANE");
ack("lane_512.update(str);", true);
ack("lane_512.fini();", 'f149df86c9a94c2fd100f68dee46bac886686ba512ec9e7aac3c997be204ce7b6fd583429fa0d281d80d4acd73751b2fd19fde98db07922b077dbe8b1f1dc932');

var luffa_224 = new GPSEE.Dc(PGPHASHALGO_LUFFA_224);
ack("luffa_224('');", 'd69dcfd468dc331d5159c5c40cd1877e9b8ea2a50e6a7245630286ed');
ack("luffa_224.init(PGPHASHALGO_LUFFA_224);", true);
ack("luffa_224.algo", PGPHASHALGO_LUFFA_224);
ack("luffa_224.asn1", null);
ack("luffa_224.name", "LUFFA");
ack("luffa_224.update(str);", true);
ack("luffa_224.fini();", 'f1d566a4b469a38ea31717dbb35d1bb9ac184ec2c08ee58c31bfcbc6');

var luffa_256 = new GPSEE.Dc(PGPHASHALGO_LUFFA_256);
ack("luffa_256('');", 'd69dcfd468dc331d5159c5c40cd1877e9b8ea2a50e6a7245630286edb5924b2e');
ack("luffa_256.init(PGPHASHALGO_LUFFA_256);", true);
ack("luffa_256.algo", PGPHASHALGO_LUFFA_256);
ack("luffa_256.asn1", null);
ack("luffa_256.name", "LUFFA");
ack("luffa_256.update(str);", true);
ack("luffa_256.fini();", 'f1d566a4b469a38ea31717dbb35d1bb9ac184ec2c08ee58c31bfcbc641645526');

var luffa_384 = new GPSEE.Dc(PGPHASHALGO_LUFFA_384);
ack("luffa_384('');", '7404fa448793341a9e0ef5361b7388136d44bbb65f7925d2a6600e4e2f2aca5ab5a7d6fead4f4762b2c60fea5f2d3779');
ack("luffa_384.init(PGPHASHALGO_LUFFA_384);", true);
ack("luffa_384.algo", PGPHASHALGO_LUFFA_384);
ack("luffa_384.asn1", null);
ack("luffa_384.name", "LUFFA");
ack("luffa_384.update(str);", true);
ack("luffa_384.fini();", 'b13b97f6739ad0d575972c1c81a242f747ac1029f19a87f35e1ce16568b4e73054a962fade288e43452395cf05737ff9');

var luffa_512 = new GPSEE.Dc(PGPHASHALGO_LUFFA_512);
ack("luffa_512('');", '2a490f6bb9c236cee38717e6d8655f78aaecf4ffe2fe29d06383d8d6151c4c81eed9064831825ffc0c5da6b6adb1ebdcfdd8ab7434cd2f27df5df58b8b958a7f');
ack("luffa_512.init(PGPHASHALGO_LUFFA_512);", true);
ack("luffa_512.algo", PGPHASHALGO_LUFFA_512);
ack("luffa_512.asn1", null);
ack("luffa_512.name", "LUFFA");
ack("luffa_512.update(str);", true);
ack("luffa_512.fini();", '4c1faae4bda064ee9c50b6952eb95c3e1026c6840b9e498c2514eb9378377fe9ef2d6d1e17bc395346982d1cbb8ce6855f4602c8bf2ed11bfcd3e453314b1feb');

// XXX md6sum, not NIST, values used as test vectors.
var md6_224 = new GPSEE.Dc(PGPHASHALGO_MD6_224);
ack("md6_224('');", 'd2091aa2ad17f38c51ade2697f24cafc3894c617c77ffe10fdc7abcb');
ack("md6_224.init(PGPHASHALGO_MD6_224);", true);
ack("md6_224.algo", PGPHASHALGO_MD6_224);
ack("md6_224.asn1", null);
ack("md6_224.name", "MD6");
ack("md6_224.update(str);", true);
ack("md6_224.fini();", '510c30e4202a5cdd8a4f2ae9beebb6f5988128897937615d52e6d228');

var md6_256 = new GPSEE.Dc(PGPHASHALGO_MD6_256);
ack("md6_256('');", 'bca38b24a804aa37d821d31af00f5598230122c5bbfc4c4ad5ed40e4258f04ca');
ack("md6_256.init(PGPHASHALGO_MD6_256);", true);
ack("md6_256.algo", PGPHASHALGO_MD6_256);
ack("md6_256.asn1", null);
ack("md6_256.name", "MD6");
ack("md6_256.update(str);", true);
ack("md6_256.fini();", '230637d4e6845cf0d092b558e87625f03881dd53a7439da34cf3b94ed0d8b2c5');

var md6_384 = new GPSEE.Dc(PGPHASHALGO_MD6_384);
ack("md6_384('');", 'b0bafffceebe856c1eff7e1ba2f539693f828b532ebf60ae9c16cbc3499020401b942ac25b310b2227b2954ccacc2f1f');
ack("md6_384.init(PGPHASHALGO_MD6_384);", true);
ack("md6_384.algo", PGPHASHALGO_MD6_384);
ack("md6_384.asn1", null);
ack("md6_384.name", "MD6");
ack("md6_384.update(str);", true);
ack("md6_384.fini();", 'e2c6d31dd8872cbd5a1207481cdac581054d13a4d4fe6854331cd8cf3e7cbafbaddd6e2517972b8ff57cdc4806d09190');

var md6_512 = new GPSEE.Dc(PGPHASHALGO_MD6_512);
ack("md6_512('');", '6b7f33821a2c060ecdd81aefddea2fd3c4720270e18654f4cb08ece49ccb469f8beeee7c831206bd577f9f2630d9177979203a9489e47e04df4e6deaa0f8e0c0');
ack("md6_512.init(PGPHASHALGO_MD6_512);", true);
ack("md6_512.algo", PGPHASHALGO_MD6_512);
ack("md6_512.asn1", null);
ack("md6_512.name", "MD6");
ack("md6_512.update(str);", true);
ack("md6_512.fini();", '00918245271e377a7ffb202b90f3bda5477d8feab12d8a3a8994ebc55fe6e74ca8341520032eeea3fdef892f2882378f636212af4b2683ccf80bf025b7d9b457');

var shabal224 = new GPSEE.Dc(PGPHASHALGO_SHABAL_224);
ack("shabal224('');", '562b4fdbe1706247552927f814b66a3d74b465a090af23e277bf8029');
ack("shabal224.init(PGPHASHALGO_SHABAL_224);", true);
ack("shabal224.algo", PGPHASHALGO_SHABAL_224);
ack("shabal224.asn1", null);
ack("shabal224.name", "SHABAL");
ack("shabal224.update(str);", true);
ack("shabal224.fini();", 'f47578239607af492d5f7df9241818adf6fba4180ddcbef6e39ac1e9');

var shabal256 = new GPSEE.Dc(PGPHASHALGO_SHABAL_256);
ack("shabal256('');", 'aec750d11feee9f16271922fbaf5a9be142f62019ef8d720f858940070889014');
ack("shabal256.init(PGPHASHALGO_SHABAL_256);", true);
ack("shabal256.algo", PGPHASHALGO_SHABAL_256);
ack("shabal256.asn1", null);
ack("shabal256.name", "SHABAL");
ack("shabal256.update(str);", true);
ack("shabal256.fini();", '07225fab83ca48fb480d22219410d5ca008359efbfd315829029afe2cb3f0404');

var shabal384 = new GPSEE.Dc(PGPHASHALGO_SHABAL_384);
ack("shabal384('');", 'ff093d67d22b06a674b5f384719150d617e0ff9c8923569a2ab60cda886df63c91a25f33cd71cc22c9eebc5cd6aee52a');
ack("shabal384.init(PGPHASHALGO_SHABAL_384);", true);
ack("shabal384.algo", PGPHASHALGO_SHABAL_384);
ack("shabal384.asn1", null);
ack("shabal384.name", "SHABAL");
ack("shabal384.update(str);", true);
ack("shabal384.fini();", '66613058865271722c0295774aa77258a5082bebbb5a02f9d6aee9ad303fc71cbf19e2f599ddfde88cf0bf30a028e530');

var shabal512 = new GPSEE.Dc(PGPHASHALGO_SHABAL_512);
ack("shabal512('');", 'fc2d5dff5d70b7f6b1f8c2fcc8c1f9fe9934e54257eded0cf2b539a2ef0a19ccffa84f8d9fa135e4bd3c09f590f3a927ebd603ac29eb729e6f2a9af031ad8dc6');
ack("shabal512.init(PGPHASHALGO_SHABAL_512);", true);
ack("shabal512.algo", PGPHASHALGO_SHABAL_512);
ack("shabal512.asn1", null);
ack("shabal512.name", "SHABAL");
ack("shabal512.update(str);", true);
ack("shabal512.fini();", '4a7f0f707c1b0c1d12ddcfa8aa0f9d2410dd9bab57c2d56705fc1acb02066f99678738cedb20a2aba94842a441e77bc02656fe5690f98b421d029bfc4df09f91');

var shavite3_224 = new GPSEE.Dc(PGPHASHALGO_SHAVITE3_224);
ack("shavite3_224('');", '401d9f7dda63265b7b0c1cfebc196c1ce0cbe994dc6595bd0bb07fb4');
ack("shavite3_224.init(PGPHASHALGO_SHAVITE3_224);", true);
ack("shavite3_224.algo", PGPHASHALGO_SHAVITE3_224);
ack("shavite3_224.asn1", null);
ack("shavite3_224.name", "SHAVITE3");
ack("shavite3_224.update(str);", true);
ack("shavite3_224.fini();", 'ab74fc01e9fe46826bafb4383b3374174aeac9dbd2bdd0b2d18cf90f');

var shavite3_256 = new GPSEE.Dc(PGPHASHALGO_SHAVITE3_256);
ack("shavite3_256('');", '6646a740fd2bf0d79752627538777f32be04e22c3849d6c79fbbbcc2e68f4500');
ack("shavite3_256.init(PGPHASHALGO_SHAVITE3_256);", true);
ack("shavite3_256.algo", PGPHASHALGO_SHAVITE3_256);
ack("shavite3_256.asn1", null);
ack("shavite3_256.name", "SHAVITE3");
ack("shavite3_256.update(str);", true);
ack("shavite3_256.fini();", 'b89f6ce5ef30aa6a2da80ef1767ed90ef2e169f9fdd7b2d07b05157e9a59b04f');

var shavite3_384 = new GPSEE.Dc(PGPHASHALGO_SHAVITE3_384);
ack("shavite3_384('');", 'ac40319f6ebdb57a820d2dbc29218f9c58a319df0651d2550cda83a6778a57620aeb53762a677d8df6fda419f1d9a349');
ack("shavite3_384.init(PGPHASHALGO_SHAVITE3_384);", true);
ack("shavite3_384.algo", PGPHASHALGO_SHAVITE3_384);
ack("shavite3_384.asn1", null);
ack("shavite3_384.name", "SHAVITE3");
ack("shavite3_384.update(str);", true);
ack("shavite3_384.fini();", '7951ee74b2c791b96ea23678f9e1202f04fa4a328c65199e896aba4b33e739957eeeef81b19f9d67b82b0846af40fe0c');

var shavite3_512 = new GPSEE.Dc(PGPHASHALGO_SHAVITE3_512);
ack("shavite3_512('');", 'c25c67d74d00d9c69468c6b9e23b2d8732f953bf097fd79de15a828907bc3cbd640b8ee7ed3a456e6a840887ede15df6f4ba8c9771f5422267df667e26156e55');
ack("shavite3_512.init(PGPHASHALGO_SHAVITE3_512);", true);
ack("shavite3_512.algo", PGPHASHALGO_SHAVITE3_512);
ack("shavite3_512.asn1", null);
ack("shavite3_512.name", "SHAVITE3");
ack("shavite3_512.update(str);", true);
ack("shavite3_512.fini();", '6c7d3d688ecccbd285d6e7bb3cf00b10d292a45ec90f2b85426ece62eb068748e3a293c787196a920550ea0ac4b8d00e77a6066fb1c5952610d356dc8d0f589d');

var simd_224 = new GPSEE.Dc(PGPHASHALGO_SIMD_224);
ack("simd_224('');", '3a6b867e2fb0c448370e2855f3794b557124c81077373311103d0c64');
ack("simd_224.init(PGPHASHALGO_SIMD_224);", true);
ack("simd_224.algo", PGPHASHALGO_SIMD_224);
ack("simd_224.asn1", null);
ack("simd_224.name", "SIMD");
ack("simd_224.update(str);", true);
ack("simd_224.fini();", '1b7ef70809f56f20d5584491ffbf422ec4526f283664a9cb58bcc687');

var simd_256 = new GPSEE.Dc(PGPHASHALGO_SIMD_256);
ack("simd_256('');", '1a53c82220377d3e9a783b106210995a0f3931b6d002f99c243accd15dac587d');
ack("simd_256.init(PGPHASHALGO_SIMD_256);", true);
ack("simd_256.algo", PGPHASHALGO_SIMD_256);
ack("simd_256.asn1", null);
ack("simd_256.name", "SIMD");
ack("simd_256.update(str);", true);
ack("simd_256.fini();", 'a9569ffe053db1533641b631ea2a589114992564ad106fbe67edb7abfbadd91b');

var simd_384 = new GPSEE.Dc(PGPHASHALGO_SIMD_384);
ack("simd_384('');", 'c5f08c18d50448edf6924ec71616a3626687db426a99c1606f36918913a83b59411b58a6033447f005dc5153d7af0482');
ack("simd_384.init(PGPHASHALGO_SIMD_384);", true);
ack("simd_384.algo", PGPHASHALGO_SIMD_384);
ack("simd_384.asn1", null);
ack("simd_384.name", "SIMD");
ack("simd_384.update(str);", true);
ack("simd_384.fini();", 'd725c6408e188f482209982ae6129df43a7c562e2f9b84fb0281730e9de6db4d83cf0606f1cdd5c066522e15d8253d4b');

var simd_512 = new GPSEE.Dc(PGPHASHALGO_SIMD_512);
ack("simd_512('');", '426ab39fe63816339e65d100e34ddd593038852edc60e5eb166f3173b35a5124587c1d8bcc29b0cbb0930cf6eccac44a40f21895bb1bb7dd89c67e1f77010243');
ack("simd_512.init(PGPHASHALGO_SIMD_512);", true);
ack("simd_512.algo", PGPHASHALGO_SIMD_512);
ack("simd_512.asn1", null);
ack("simd_512.name", "SIMD");
ack("simd_512.update(str);", true);
ack("simd_512.fini();", '1c42e151ab96e0c9e378b7e8c8140aedcc553a3744a081eab8fd9c16bf4557f242d4a5fd7bc26ca71d38339387cfc57fd4cc79411d5258d0427b7ef9cf355429');

var tib3_224 = new GPSEE.Dc(PGPHASHALGO_TIB3_224);
ack("tib3_224('');", '0bb081f80b8d738cea5fde2ca9975ba4a5ad83196731b2432d8b3a18');
ack("tib3_224.init(PGPHASHALGO_TIB3_224);", true);
ack("tib3_224.algo", PGPHASHALGO_TIB3_224);
ack("tib3_224.asn1", null);
ack("tib3_224.name", "TIB3");
ack("tib3_224.update(str);", true);
ack("tib3_224.fini();", '096169965ba26b60e075c40cb5666d289d142ba6b5eb5bfeaee07186');

var tib3_256 = new GPSEE.Dc(PGPHASHALGO_TIB3_256);
ack("tib3_256('');", '67fcdaea6c8af476fa5f46fbc6f74bb7c47d1989170855385114b88c7be73677');
ack("tib3_256.init(PGPHASHALGO_TIB3_256);", true);
ack("tib3_256.algo", PGPHASHALGO_TIB3_256);
ack("tib3_256.asn1", null);
ack("tib3_256.name", "TIB3");
ack("tib3_256.update(str);", true);
ack("tib3_256.fini();", 'd14c723a5e6676afc53a92ffdf9ae66fdcfaca1ecef72a60dfafbec316f266e4');

var tib3_384 = new GPSEE.Dc(PGPHASHALGO_TIB3_384);
ack("tib3_384('');", '6968b5191ba3ccad2dab7a7cc4cc8ab4ea27c71a5a9868ec3c987909c8385345813fa5ad4097d0df7decfbff178b6671');
ack("tib3_384.init(PGPHASHALGO_TIB3_384);", true);
ack("tib3_384.algo", PGPHASHALGO_TIB3_384);
ack("tib3_384.asn1", null);
ack("tib3_384.name", "TIB3");
ack("tib3_384.update(str);", true);
ack("tib3_384.fini();", 'c59874e7a57724266238e9841ddbec63d67af12399eed11fc598f485b5ba0e779f41d0b10aa9eb0a5bd91647d618675b');

var tib3_512 = new GPSEE.Dc(PGPHASHALGO_TIB3_512);
ack("tib3_512('');", 'ca9b25aee65a2effb5fc6946a0bbae32055dd8e2f108f8411b4d4ea5f891a58766d10d5ebfa7bfcafb16502a5fc557120e8130d32bd9d80b67c45bdaeea8de58');
ack("tib3_512.init(PGPHASHALGO_TIB3_512);", true);
ack("tib3_512.algo", PGPHASHALGO_TIB3_512);
ack("tib3_512.asn1", null);
ack("tib3_512.name", "TIB3");
ack("tib3_512.update(str);", true);
ack("tib3_512.fini();", '98b174141294cfa25463777e2f37299aa4584f6d1156c631146ac75a84d9c05a06b33cec49cd2900dec9ae961bc8f27dae566f2c9d6ebf3702d0ce8f173524e7');

var tiger192 = new GPSEE.Dc(PGPHASHALGO_TIGER192);
ack("tiger192(str);", '2aab1484e8c158f2bfb8c5ff41b57a525129131c957b5f93');
ack("tiger192.init(PGPHASHALGO_TIGER192);", true);
ack("tiger192.algo", PGPHASHALGO_TIGER192);
ack("tiger192.asn1", "3029300d06092b06010401da470c0205000418");
ack("tiger192.name", "TIGER192");
ack("tiger192.update(str);", true);
ack("tiger192.fini();", '2aab1484e8c158f2bfb8c5ff41b57a525129131c957b5f93');

var crc32 = new GPSEE.Dc(PGPHASHALGO_CRC32);
ack("crc32(str);", '352441c2');
ack("crc32.init(PGPHASHALGO_CRC32);", true);
ack("crc32.algo", PGPHASHALGO_CRC32);
ack("crc32.asn1", null);
ack("crc32.name", "CRC32");
ack("crc32.update(str);", true);
ack("crc32.fini();", '352441c2');

var crc64 = new GPSEE.Dc(PGPHASHALGO_CRC64);
ack("crc64(str);", '2cd8094a1a277627');
ack("crc64.init(PGPHASHALGO_CRC64);", true);
ack("crc64.algo", PGPHASHALGO_CRC64);
ack("crc64.asn1", null);
ack("crc64.name", "CRC64");
ack("crc64.update(str);", true);
ack("crc64.fini();", '2cd8094a1a277627');

var adler32 = new GPSEE.Dc(PGPHASHALGO_ADLER32);
ack("adler32(str);", '024d0127');
ack("adler32.init(PGPHASHALGO_ADLER32);", true);
ack("adler32.algo", PGPHASHALGO_ADLER32);
ack("adler32.asn1", null);
ack("adler32.name", "ADLER32");
ack("adler32.update(str);", true);
ack("adler32.fini();", '024d0127');

var jlu32 = new GPSEE.Dc(PGPHASHALGO_JLU32);
ack("jlu32(str);", '110255fd');
ack("jlu32.init(PGPHASHALGO_JLU32);", true);
ack("jlu32.algo", PGPHASHALGO_JLU32);
ack("jlu32.asn1", null);
ack("jlu32.name", "JLU32");
ack("jlu32.update(str);", true);
ack("jlu32.fini();", '110255fd');

if (loglvl) print("<-- Dc.js");
