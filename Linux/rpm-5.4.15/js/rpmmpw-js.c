/**
 * \file js/rpmmpw-js.c
 */

#include "system.h"
#include <math.h>

#include "rpmmpw-js.h"
#include "rpmjs-debug.h"

#include <rpmbc.h>
#include "beecrypt/mpprime.h"	/* mpptrials() */

#include "debug.h"

/*@unchecked@*/
static int _debug = 0;

/* Required JSClass vectors */
#define	rpmmpw_addprop		JS_PropertyStub
#define	rpmmpw_delprop		JS_PropertyStub
#define	rpmmpw_convert		JS_ConvertStub

/* Optional JSClass vectors */
#define	rpmmpw_getobjectops	NULL
#define	rpmmpw_checkaccess	NULL
#define	rpmmpw_call		rpmmpw_call
#define	rpmmpw_construct	rpmmpw_ctor
#define	rpmmpw_xdrobject	NULL
#define	rpmmpw_hasinstance	NULL
#define	rpmmpw_mark		NULL
#define	rpmmpw_reserveslots	NULL

/* Extended JSClass vectors */
#define rpmmpw_equality		NULL
#define rpmmpw_outerobject	NULL
#define rpmmpw_innerobject	NULL
#define rpmmpw_iteratorobject	NULL
#define rpmmpw_wrappedobject	NULL

#define	OBJ_IS_MPW(cx,obj)	((obj) && JS_GET_CLASS((cx), (obj)) == &rpmmpwClass)
#define	JSVAL_IS_MPW(cx, v)	(JSVAL_IS_OBJECT(v) && OBJ_IS_MPW(cx, JSVAL_TO_OBJECT(v)))

/* --- helpers */
typedef struct mpwObject_s {
    int ob_size;
    mpw data[1];
} mpwObject;

#define mpw_Check(_o)		(1)	/* XXX FIXME */
#define mpw_CheckExact(_o)	(1)	/* XXX FIXME */

#define MP_ROUND_B2W(_b)	MP_BITS_TO_WORDS((_b) + MP_WBITS - 1)

#define MPW_SIZE(_a)	(size_t)((_a)->ob_size < 0 ? -(_a)->ob_size : (_a)->ob_size)
#define MPW_DATA(_a)	((_a)->data)

#define ABS(_x)		((_x) < 0 ? -(_x) : (_x))
#if !defined(MAX)
#define MAX(x, y) ((x) < (y) ? (y) : (x))
#endif
#if !defined(MIN)
#define MIN(x, y) ((x) > (y) ? (y) : (x))
#endif

#define	MPBITCNT(_s, _d) (MP_WORDS_TO_BITS(_s) - mpmszcnt((_s), (_d)))

#define	BITS_TO_DIGITS(_b)	(((_b) + SHIFT - 1)/SHIFT)
#define	DIGITS_TO_BITS(_d)	((_d) * SHIFT)

/*@unchecked@*/
static int _ie = 0x44332211;
/*@unchecked@*/
static union _dendian {
/*@unused@*/
    int i;
    char b[4];
} *_endian = (union _dendian *)&_ie;
#define        _RPMBC_IS_BIG_ENDIAN()         (_endian->b[0] == '\x44')
#define        _RPMBC_IS_LITTLE_ENDIAN()      (_endian->b[0] == '\x11')

/*@unchecked@*/ /*@observer@*/
static const char *initialiser_name = "";

/*@unchecked@*/ /*@observer@*/
static const struct {
  /* Number of digits in the conversion base that always fits in an mp_limb_t.
     For example, for base 10 on a machine where a mp_limb_t has 32 bits this
     is 9, since 10**9 is the largest number that fits into a mp_limb_t.  */
  int chars_per_limb;

  /* log(2)/log(conversion_base) */
  double chars_per_bit_exactly;

  /* base**chars_per_limb, i.e. the biggest number that fits a word, built by
     factors of base.  Exception: For 2, 4, 8, etc, big_base is log2(base),
     i.e. the number of bits used to represent each digit in the base.  */
  unsigned int big_base;

  /* A BITS_PER_MP_LIMB bit approximation to 1/big_base, represented as a
     fixed-point number.  Instead of dividing by big_base an application can
     choose to multiply by big_base_inverted.  */
  unsigned int big_base_inverted;
} mp_bases[257] = {
  /*  0 */ {0, 0.0, 0, 0},
  /*  1 */ {0, 1e37, 0, 0},
  /*  2 */ {32, 1.0000000000000000, 0x1, 0x0},
  /*  3 */ {20, 0.6309297535714574, 0xcfd41b91, 0x3b563c24},
  /*  4 */ {16, 0.5000000000000000, 0x2, 0x0},
  /*  5 */ {13, 0.4306765580733931, 0x48c27395, 0xc25c2684},
  /*  6 */ {12, 0.3868528072345416, 0x81bf1000, 0xf91bd1b6},
  /*  7 */ {11, 0.3562071871080222, 0x75db9c97, 0x1607a2cb},
  /*  8 */ {10, 0.3333333333333333, 0x3, 0x0},
  /*  9 */ {10, 0.3154648767857287, 0xcfd41b91, 0x3b563c24},
  /* 10 */ {9, 0.3010299956639812, 0x3b9aca00, 0x12e0be82},
  /* 11 */ {9, 0.2890648263178878, 0x8c8b6d2b, 0xd24cde04},
  /* 12 */ {8, 0.2789429456511298, 0x19a10000, 0x3fa39ab5},
  /* 13 */ {8, 0.2702381544273197, 0x309f1021, 0x50f8ac5f},
  /* 14 */ {8, 0.2626495350371935, 0x57f6c100, 0x74843b1e},
  /* 15 */ {8, 0.2559580248098155, 0x98c29b81, 0xad0326c2},
  /* 16 */ {8, 0.2500000000000000, 0x4, 0x0},
  /* 17 */ {7, 0.2446505421182260, 0x18754571, 0x4ef0b6bd},
  /* 18 */ {7, 0.2398124665681314, 0x247dbc80, 0xc0fc48a1},
  /* 19 */ {7, 0.2354089133666382, 0x3547667b, 0x33838942},
  /* 20 */ {7, 0.2313782131597592, 0x4c4b4000, 0xad7f29ab},
  /* 21 */ {7, 0.2276702486969530, 0x6b5a6e1d, 0x313c3d15},
  /* 22 */ {7, 0.2242438242175754, 0x94ace180, 0xb8cca9e0},
  /* 23 */ {7, 0.2210647294575037, 0xcaf18367, 0x42ed6de9},
  /* 24 */ {6, 0.2181042919855316, 0xb640000, 0x67980e0b},
  /* 25 */ {6, 0.2153382790366965, 0xe8d4a51, 0x19799812},
  /* 26 */ {6, 0.2127460535533632, 0x1269ae40, 0xbce85396},
  /* 27 */ {6, 0.2103099178571525, 0x17179149, 0x62c103a9},
  /* 28 */ {6, 0.2080145976765095, 0x1cb91000, 0x1d353d43},
  /* 29 */ {6, 0.2058468324604344, 0x23744899, 0xce1decea},
  /* 30 */ {6, 0.2037950470905062, 0x2b73a840, 0x790fc511},
  /* 31 */ {6, 0.2018490865820999, 0x34e63b41, 0x35b865a0},
  /* 32 */ {6, 0.2000000000000000, 0x5, 0x0},
  /* 33 */ {6, 0.1982398631705605, 0x4cfa3cc1, 0xa9aed1b3},
  /* 34 */ {6, 0.1965616322328226, 0x5c13d840, 0x63dfc229},
  /* 35 */ {6, 0.1949590218937863, 0x6d91b519, 0x2b0fee30},
  /* 36 */ {6, 0.1934264036172708, 0x81bf1000, 0xf91bd1b6},
  /* 37 */ {6, 0.1919587200065601, 0x98ede0c9, 0xac89c3a9},
  /* 38 */ {6, 0.1905514124267734, 0xb3773e40, 0x6d2c32fe},
  /* 39 */ {6, 0.1892003595168700, 0xd1bbc4d1, 0x387907c9},
  /* 40 */ {6, 0.1879018247091076, 0xf4240000, 0xc6f7a0b},
  /* 41 */ {5, 0.1866524112389434, 0x6e7d349, 0x28928154},
  /* 42 */ {5, 0.1854490234153689, 0x7ca30a0, 0x6e8629d},
  /* 43 */ {5, 0.1842888331487062, 0x8c32bbb, 0xd373dca0},
  /* 44 */ {5, 0.1831692509136336, 0x9d46c00, 0xa0b17895},
  /* 45 */ {5, 0.1820879004699383, 0xaffacfd, 0x746811a5},
  /* 46 */ {5, 0.1810425967800402, 0xc46bee0, 0x4da6500f},
  /* 47 */ {5, 0.1800313266566926, 0xdab86ef, 0x2ba23582},
  /* 48 */ {5, 0.1790522317510414, 0xf300000, 0xdb20a88},
  /* 49 */ {5, 0.1781035935540111, 0x10d63af1, 0xe68d5ce4},
  /* 50 */ {5, 0.1771838201355579, 0x12a05f20, 0xb7cdfd9d},
  /* 51 */ {5, 0.1762914343888821, 0x1490aae3, 0x8e583933},
  /* 52 */ {5, 0.1754250635819545, 0x16a97400, 0x697cc3ea},
  /* 53 */ {5, 0.1745834300480449, 0x18ed2825, 0x48a5ca6c},
  /* 54 */ {5, 0.1737653428714400, 0x1b5e4d60, 0x2b52db16},
  /* 55 */ {5, 0.1729696904450771, 0x1dff8297, 0x111586a6},
  /* 56 */ {5, 0.1721954337940981, 0x20d38000, 0xf31d2b36},
  /* 57 */ {5, 0.1714416005739134, 0x23dd1799, 0xc8d76d19},
  /* 58 */ {5, 0.1707072796637201, 0x271f35a0, 0xa2cb1eb4},
  /* 59 */ {5, 0.1699916162869140, 0x2a9ce10b, 0x807c3ec3},
  /* 60 */ {5, 0.1692938075987814, 0x2e593c00, 0x617ec8bf},
  /* 61 */ {5, 0.1686130986895011, 0x3257844d, 0x45746cbe},
  /* 62 */ {5, 0.1679487789570419, 0x369b13e0, 0x2c0aa273},
  /* 63 */ {5, 0.1673001788101741, 0x3b27613f, 0x14f90805},
  /* 64 */ {5, 0.1666666666666667, 0x6, 0x0},
  /* 65 */ {5, 0.1660476462159378, 0x4528a141, 0xd9cf0829},
  /* 66 */ {5, 0.1654425539190583, 0x4aa51420, 0xb6fc4841},
  /* 67 */ {5, 0.1648508567221603, 0x50794633, 0x973054cb},
  /* 68 */ {5, 0.1642720499620502, 0x56a94400, 0x7a1dbe4b},
  /* 69 */ {5, 0.1637056554452156, 0x5d393975, 0x5f7fcd7f},
  /* 70 */ {5, 0.1631512196835108, 0x642d7260, 0x47196c84},
  /* 71 */ {5, 0.1626083122716342, 0x6b8a5ae7, 0x30b43635},
  /* 72 */ {5, 0.1620765243931223, 0x73548000, 0x1c1fa5f6},
  /* 73 */ {5, 0.1615554674429964, 0x7b908fe9, 0x930634a},
  /* 74 */ {5, 0.1610447717564444, 0x84435aa0, 0xef7f4a3c},
  /* 75 */ {5, 0.1605440854340214, 0x8d71d25b, 0xcf5552d2},
  /* 76 */ {5, 0.1600530732548213, 0x97210c00, 0xb1a47c8e},
  /* 77 */ {5, 0.1595714156699382, 0xa1563f9d, 0x9634b43e},
  /* 78 */ {5, 0.1590988078692941, 0xac16c8e0, 0x7cd3817d},
  /* 79 */ {5, 0.1586349589155960, 0xb768278f, 0x65536761},
  /* 80 */ {5, 0.1581795909397823, 0xc3500000, 0x4f8b588e},
  /* 81 */ {5, 0.1577324383928644, 0xcfd41b91, 0x3b563c24},
  /* 82 */ {5, 0.1572932473495469, 0xdcfa6920, 0x28928154},
  /* 83 */ {5, 0.1568617748594410, 0xeac8fd83, 0x1721bfb0},
  /* 84 */ {5, 0.1564377883420715, 0xf9461400, 0x6e8629d},
  /* 85 */ {4, 0.1560210650222250, 0x31c84b1, 0x491cc17c},
  /* 86 */ {4, 0.1556113914024939, 0x342ab10, 0x3a11d83b},
  /* 87 */ {4, 0.1552085627701551, 0x36a2c21, 0x2be074cd},
  /* 88 */ {4, 0.1548123827357682, 0x3931000, 0x1e7a02e7},
  /* 89 */ {4, 0.1544226628011101, 0x3bd5ee1, 0x11d10edd},
  /* 90 */ {4, 0.1540392219542636, 0x3e92110, 0x5d92c68},
  /* 91 */ {4, 0.1536618862898642, 0x4165ef1, 0xf50dbfb2},
  /* 92 */ {4, 0.1532904886526781, 0x4452100, 0xdf9f1316},
  /* 93 */ {4, 0.1529248683028321, 0x4756fd1, 0xcb52a684},
  /* 94 */ {4, 0.1525648706011593, 0x4a75410, 0xb8163e97},
  /* 95 */ {4, 0.1522103467132434, 0x4dad681, 0xa5d8f269},
  /* 96 */ {4, 0.1518611533308632, 0x5100000, 0x948b0fcd},
  /* 97 */ {4, 0.1515171524096389, 0x546d981, 0x841e0215},
  /* 98 */ {4, 0.1511782109217764, 0x57f6c10, 0x74843b1e},
  /* 99 */ {4, 0.1508442006228941, 0x5b9c0d1, 0x65b11e6e},
  /* 100 */ {4, 0.1505149978319906, 0x5f5e100, 0x5798ee23},
  /* 101 */ {4, 0.1501904832236880, 0x633d5f1, 0x4a30b99b},
  /* 102 */ {4, 0.1498705416319474, 0x673a910, 0x3d6e4d94},
  /* 103 */ {4, 0.1495550618645152, 0x6b563e1, 0x314825b0},
  /* 104 */ {4, 0.1492439365274121, 0x6f91000, 0x25b55f2e},
  /* 105 */ {4, 0.1489370618588283, 0x73eb721, 0x1aadaccb},
  /* 106 */ {4, 0.1486343375718350, 0x7866310, 0x10294ba2},
  /* 107 */ {4, 0.1483356667053617, 0x7d01db1, 0x620f8f6},
  /* 108 */ {4, 0.1480409554829326, 0x81bf100, 0xf91bd1b6},
  /* 109 */ {4, 0.1477501131786861, 0x869e711, 0xe6d37b2a},
  /* 110 */ {4, 0.1474630519902391, 0x8ba0a10, 0xd55cff6e},
  /* 111 */ {4, 0.1471796869179852, 0x90c6441, 0xc4ad2db2},
  /* 112 */ {4, 0.1468999356504447, 0x9610000, 0xb4b985cf},
  /* 113 */ {4, 0.1466237184553111, 0x9b7e7c1, 0xa5782bef},
  /* 114 */ {4, 0.1463509580758620, 0xa112610, 0x96dfdd2a},
  /* 115 */ {4, 0.1460815796324244, 0xa6cc591, 0x88e7e509},
  /* 116 */ {4, 0.1458155105286054, 0xacad100, 0x7b8813d3},
  /* 117 */ {4, 0.1455526803620167, 0xb2b5331, 0x6eb8b595},
  /* 118 */ {4, 0.1452930208392429, 0xb8e5710, 0x627289db},
  /* 119 */ {4, 0.1450364656948130, 0xbf3e7a1, 0x56aebc07},
  /* 120 */ {4, 0.1447829506139581, 0xc5c1000, 0x4b66dc33},
  /* 121 */ {4, 0.1445324131589439, 0xcc6db61, 0x4094d8a3},
  /* 122 */ {4, 0.1442847926987864, 0xd345510, 0x3632f7a5},
  /* 123 */ {4, 0.1440400303421672, 0xda48871, 0x2c3bd1f0},
  /* 124 */ {4, 0.1437980688733776, 0xe178100, 0x22aa4d5f},
  /* 125 */ {4, 0.1435588526911310, 0xe8d4a51, 0x19799812},
  /* 126 */ {4, 0.1433223277500932, 0xf05f010, 0x10a523e5},
  /* 127 */ {4, 0.1430884415049874, 0xf817e01, 0x828a237},
  /* 128 */ {4, 0.1428571428571428, 0x7, 0x0},
  /* 129 */ {4, 0.1426283821033600, 0x10818201, 0xf04ec452},
  /* 130 */ {4, 0.1424021108869747, 0x11061010, 0xe136444a},
  /* 131 */ {4, 0.1421782821510107, 0x118db651, 0xd2af9589},
  /* 132 */ {4, 0.1419568500933153, 0x12188100, 0xc4b42a83},
  /* 133 */ {4, 0.1417377701235801, 0x12a67c71, 0xb73dccf5},
  /* 134 */ {4, 0.1415209988221527, 0x1337b510, 0xaa4698c5},
  /* 135 */ {4, 0.1413064939005528, 0x13cc3761, 0x9dc8f729},
  /* 136 */ {4, 0.1410942141636095, 0x14641000, 0x91bf9a30},
  /* 137 */ {4, 0.1408841194731412, 0x14ff4ba1, 0x86257887},
  /* 138 */ {4, 0.1406761707131039, 0x159df710, 0x7af5c98c},
  /* 139 */ {4, 0.1404703297561400, 0x16401f31, 0x702c01a0},
  /* 140 */ {4, 0.1402665594314587, 0x16e5d100, 0x65c3ceb1},
  /* 141 */ {4, 0.1400648234939879, 0x178f1991, 0x5bb91502},
  /* 142 */ {4, 0.1398650865947379, 0x183c0610, 0x5207ec23},
  /* 143 */ {4, 0.1396673142523192, 0x18eca3c1, 0x48ac9c19},
  /* 144 */ {4, 0.1394714728255649, 0x19a10000, 0x3fa39ab5},
  /* 145 */ {4, 0.1392775294872041, 0x1a592841, 0x36e98912},
  /* 146 */ {4, 0.1390854521985406, 0x1b152a10, 0x2e7b3140},
  /* 147 */ {4, 0.1388952096850913, 0x1bd51311, 0x2655840b},
  /* 148 */ {4, 0.1387067714131417, 0x1c98f100, 0x1e7596ea},
  /* 149 */ {4, 0.1385201075671774, 0x1d60d1b1, 0x16d8a20d},
  /* 150 */ {4, 0.1383351890281539, 0x1e2cc310, 0xf7bfe87},
  /* 151 */ {4, 0.1381519873525671, 0x1efcd321, 0x85d2492},
  /* 152 */ {4, 0.1379704747522905, 0x1fd11000, 0x179a9f4},
  /* 153 */ {4, 0.1377906240751463, 0x20a987e1, 0xf59e80eb},
  /* 154 */ {4, 0.1376124087861776, 0x21864910, 0xe8b768db},
  /* 155 */ {4, 0.1374358029495937, 0x226761f1, 0xdc39d6d5},
  /* 156 */ {4, 0.1372607812113589, 0x234ce100, 0xd021c5d1},
  /* 157 */ {4, 0.1370873187823978, 0x2436d4d1, 0xc46b5e37},
  /* 158 */ {4, 0.1369153914223921, 0x25254c10, 0xb912f39c},
  /* 159 */ {4, 0.1367449754241439, 0x26185581, 0xae150294},
  /* 160 */ {4, 0.1365760475984821, 0x27100000, 0xa36e2eb1},
  /* 161 */ {4, 0.1364085852596902, 0x280c5a81, 0x991b4094},
  /* 162 */ {4, 0.1362425662114337, 0x290d7410, 0x8f19241e},
  /* 163 */ {4, 0.1360779687331669, 0x2a135bd1, 0x8564e6b7},
  /* 164 */ {4, 0.1359147715670014, 0x2b1e2100, 0x7bfbb5b4},
  /* 165 */ {4, 0.1357529539050150, 0x2c2dd2f1, 0x72dadcc8},
  /* 166 */ {4, 0.1355924953769864, 0x2d428110, 0x69ffc498},
  /* 167 */ {4, 0.1354333760385373, 0x2e5c3ae1, 0x6167f154},
  /* 168 */ {4, 0.1352755763596663, 0x2f7b1000, 0x5911016e},
  /* 169 */ {4, 0.1351190772136599, 0x309f1021, 0x50f8ac5f},
  /* 170 */ {4, 0.1349638598663645, 0x31c84b10, 0x491cc17c},
  /* 171 */ {4, 0.1348099059658080, 0x32f6d0b1, 0x417b26d8},
  /* 172 */ {4, 0.1346571975321549, 0x342ab100, 0x3a11d83b},
  /* 173 */ {4, 0.1345057169479844, 0x3563fc11, 0x32dee622},
  /* 174 */ {4, 0.1343554469488779, 0x36a2c210, 0x2be074cd},
  /* 175 */ {4, 0.1342063706143054, 0x37e71341, 0x2514bb58},
  /* 176 */ {4, 0.1340584713587979, 0x39310000, 0x1e7a02e7},
  /* 177 */ {4, 0.1339117329233981, 0x3a8098c1, 0x180ea5d0},
  /* 178 */ {4, 0.1337661393673756, 0x3bd5ee10, 0x11d10edd},
  /* 179 */ {4, 0.1336216750601996, 0x3d311091, 0xbbfb88e},
  /* 180 */ {4, 0.1334783246737591, 0x3e921100, 0x5d92c68},
  /* 181 */ {4, 0.1333360731748201, 0x3ff90031, 0x1c024c},
  /* 182 */ {4, 0.1331949058177136, 0x4165ef10, 0xf50dbfb2},
  /* 183 */ {4, 0.1330548081372441, 0x42d8eea1, 0xea30efa3},
  /* 184 */ {4, 0.1329157659418126, 0x44521000, 0xdf9f1316},
  /* 185 */ {4, 0.1327777653067443, 0x45d16461, 0xd555c0c9},
  /* 186 */ {4, 0.1326407925678156, 0x4756fd10, 0xcb52a684},
  /* 187 */ {4, 0.1325048343149731, 0x48e2eb71, 0xc193881f},
  /* 188 */ {4, 0.1323698773862368, 0x4a754100, 0xb8163e97},
  /* 189 */ {4, 0.1322359088617821, 0x4c0e0f51, 0xaed8b724},
  /* 190 */ {4, 0.1321029160581950, 0x4dad6810, 0xa5d8f269},
  /* 191 */ {4, 0.1319708865228925, 0x4f535d01, 0x9d15039d},
  /* 192 */ {4, 0.1318398080287045, 0x51000000, 0x948b0fcd},
  /* 193 */ {4, 0.1317096685686114, 0x52b36301, 0x8c394d1d},
  /* 194 */ {4, 0.1315804563506306, 0x546d9810, 0x841e0215},
  /* 195 */ {4, 0.1314521597928493, 0x562eb151, 0x7c3784f8},
  /* 196 */ {4, 0.1313247675185968, 0x57f6c100, 0x74843b1e},
  /* 197 */ {4, 0.1311982683517524, 0x59c5d971, 0x6d02985d},
  /* 198 */ {4, 0.1310726513121843, 0x5b9c0d10, 0x65b11e6e},
  /* 199 */ {4, 0.1309479056113158, 0x5d796e61, 0x5e8e5c64},
  /* 200 */ {4, 0.1308240206478128, 0x5f5e1000, 0x5798ee23},
  /* 201 */ {4, 0.1307009860033912, 0x614a04a1, 0x50cf7bde},
  /* 202 */ {4, 0.1305787914387386, 0x633d5f10, 0x4a30b99b},
  /* 203 */ {4, 0.1304574268895465, 0x65383231, 0x43bb66bd},
  /* 204 */ {4, 0.1303368824626505, 0x673a9100, 0x3d6e4d94},
  /* 205 */ {4, 0.1302171484322746, 0x69448e91, 0x374842ee},
  /* 206 */ {4, 0.1300982152363760, 0x6b563e10, 0x314825b0},
  /* 207 */ {4, 0.1299800734730872, 0x6d6fb2c1, 0x2b6cde75},
  /* 208 */ {4, 0.1298627138972530, 0x6f910000, 0x25b55f2e},
  /* 209 */ {4, 0.1297461274170591, 0x71ba3941, 0x2020a2c5},
  /* 210 */ {4, 0.1296303050907487, 0x73eb7210, 0x1aadaccb},
  /* 211 */ {4, 0.1295152381234257, 0x7624be11, 0x155b891f},
  /* 212 */ {4, 0.1294009178639407, 0x78663100, 0x10294ba2},
  /* 213 */ {4, 0.1292873358018581, 0x7aafdeb1, 0xb160fe9},
  /* 214 */ {4, 0.1291744835645007, 0x7d01db10, 0x620f8f6},
  /* 215 */ {4, 0.1290623529140715, 0x7f5c3a21, 0x14930ef},
  /* 216 */ {4, 0.1289509357448472, 0x81bf1000, 0xf91bd1b6},
  /* 217 */ {4, 0.1288402240804449, 0x842a70e1, 0xefdcb0c7},
  /* 218 */ {4, 0.1287302100711566, 0x869e7110, 0xe6d37b2a},
  /* 219 */ {4, 0.1286208859913518, 0x891b24f1, 0xddfeb94a},
  /* 220 */ {4, 0.1285122442369443, 0x8ba0a100, 0xd55cff6e},
  /* 221 */ {4, 0.1284042773229231, 0x8e2ef9d1, 0xcceced50},
  /* 222 */ {4, 0.1282969778809442, 0x90c64410, 0xc4ad2db2},
  /* 223 */ {4, 0.1281903386569819, 0x93669481, 0xbc9c75f9},
  /* 224 */ {4, 0.1280843525090381, 0x96100000, 0xb4b985cf},
  /* 225 */ {4, 0.1279790124049077, 0x98c29b81, 0xad0326c2},
  /* 226 */ {4, 0.1278743114199984, 0x9b7e7c10, 0xa5782bef},
  /* 227 */ {4, 0.1277702427352035, 0x9e43b6d1, 0x9e1771a9},
  /* 228 */ {4, 0.1276667996348261, 0xa1126100, 0x96dfdd2a},
  /* 229 */ {4, 0.1275639755045533, 0xa3ea8ff1, 0x8fd05c41},
  /* 230 */ {4, 0.1274617638294791, 0xa6cc5910, 0x88e7e509},
  /* 231 */ {4, 0.1273601581921740, 0xa9b7d1e1, 0x8225759d},
  /* 232 */ {4, 0.1272591522708010, 0xacad1000, 0x7b8813d3},
  /* 233 */ {4, 0.1271587398372755, 0xafac2921, 0x750eccf9},
  /* 234 */ {4, 0.1270589147554692, 0xb2b53310, 0x6eb8b595},
  /* 235 */ {4, 0.1269596709794558, 0xb5c843b1, 0x6884e923},
  /* 236 */ {4, 0.1268610025517973, 0xb8e57100, 0x627289db},
  /* 237 */ {4, 0.1267629036018709, 0xbc0cd111, 0x5c80c07b},
  /* 238 */ {4, 0.1266653683442337, 0xbf3e7a10, 0x56aebc07},
  /* 239 */ {4, 0.1265683910770258, 0xc27a8241, 0x50fbb19b},
  /* 240 */ {4, 0.1264719661804097, 0xc5c10000, 0x4b66dc33},
  /* 241 */ {4, 0.1263760881150453, 0xc91209c1, 0x45ef7c7c},
  /* 242 */ {4, 0.1262807514205999, 0xcc6db610, 0x4094d8a3},
  /* 243 */ {4, 0.1261859507142915, 0xcfd41b91, 0x3b563c24},
  /* 244 */ {4, 0.1260916806894653, 0xd3455100, 0x3632f7a5},
  /* 245 */ {4, 0.1259979361142023, 0xd6c16d31, 0x312a60c3},
  /* 246 */ {4, 0.1259047118299582, 0xda488710, 0x2c3bd1f0},
  /* 247 */ {4, 0.1258120027502338, 0xdddab5a1, 0x2766aa45},
  /* 248 */ {4, 0.1257198038592741, 0xe1781000, 0x22aa4d5f},
  /* 249 */ {4, 0.1256281102107963, 0xe520ad61, 0x1e06233c},
  /* 250 */ {4, 0.1255369169267456, 0xe8d4a510, 0x19799812},
  /* 251 */ {4, 0.1254462191960791, 0xec940e71, 0x15041c33},
  /* 252 */ {4, 0.1253560122735751, 0xf05f0100, 0x10a523e5},
  /* 253 */ {4, 0.1252662914786691, 0xf4359451, 0xc5c2749},
  /* 254 */ {4, 0.1251770521943144, 0xf817e010, 0x828a237},
  /* 255 */ {4, 0.1250882898658681, 0xfc05fc01, 0x40a1423},
  /* 256 */ {4, 0.1250000000000000, 0x8, 0x0},
};

static void prtmpw(const char * msg, mpwObject * x)
	/*@global stderr, fileSystem @*/
	/*@modifies stderr, fileSystem @*/
{
fprintf(stderr, "%5.5s %p[%u]:\t", msg, MPW_DATA(x), (unsigned)MPW_SIZE(x)), mpfprintln(stderr, MPW_SIZE(x), MPW_DATA(x));
}

static size_t
mpsizeinbase(size_t xsize, mpw* xdata, size_t base)
	/*@*/
{
    size_t nbits;
    size_t res;

    if (xsize == 0)
	return 1;

    /* XXX assumes positive integer. */
    nbits = MP_WORDS_TO_BITS(xsize) - mpmszcnt(xsize, xdata);
    if ((base & (base-1)) == 0) {	/* exact power of 2 */
	size_t lbits = mp_bases[base].big_base;
	res = (nbits + (lbits - 1)) / lbits;
    } else {
	res = (nbits * mp_bases[base].chars_per_bit_exactly) + 1;
    }
if (_debug < -1)
fprintf(stderr, "<== mpsizeinbase(%p[%u], %u) res %u\n", xdata, (unsigned)xsize, (unsigned)base, (unsigned)res);
    return res;
}

#ifdef	DYING
static void myndivmod(mpw* result, size_t xsize, const mpw* xdata, size_t ysize, const mpw* ydata, register mpw* workspace)
{
	/* result must be xsize+1 in length */
	/* workspace must be ysize+1 in length */
	/* expect ydata to be normalized */
	mpw q;
	mpw msw = *ydata;
	size_t qsize = xsize-ysize;

	*result = (mpge(ysize, xdata, ydata) ? 1 : 0);
	mpcopy(xsize, result+1, xdata);
	if (*result)
		(void) mpsub(ysize, result+1, ydata);
	result++;

	while (qsize--)
	{
		q = mppndiv(result[0], result[1], msw);

/*@-evalorder@*/
		*workspace = mpsetmul(ysize, workspace+1, ydata, q);
/*@=evalorder@*/

		while (mplt(ysize+1, result, workspace))
		{
			(void) mpsubx(ysize+1, workspace, ysize, ydata);
			q--;
		}
		(void) mpsub(ysize+1, result, workspace);
		*(result++) = q;
	}
}
#endif

static char *
mpstr(char * t, size_t nt, size_t size, mpw* data, mpw base)
	/*@modifies t @*/
{
    static char bchars[] = "0123456789abcdefghijklmnopqrstuvwxyz";
    size_t asize = size + 1;
    mpw* adata = alloca(asize * sizeof(*adata));
    size_t anorm;
    mpw* zdata = alloca((asize+1) * sizeof(*zdata));
    mpw* wksp = alloca((1+1) * sizeof(*wksp));
    size_t result;

if (_debug < -1)
fprintf(stderr, "*** mpstr(%p[%u], %p[%u], %u):\t", t, (unsigned)nt, data, (unsigned)size, (unsigned)base), mpfprintln(stderr, size, data);

    mpsetx(asize, adata, size, data);

    t[nt] = '\0';
    while (nt--) {

	mpndivmod(zdata, asize, adata, 1, &base, wksp);

if (_debug < -1) {
fprintf(stderr, "    a %p[%u]:\t", adata, (unsigned)asize), mpfprintln(stderr, asize, adata);
fprintf(stderr, "    z %p[%u]:\t", zdata, (unsigned)asize+1), mpfprintln(stderr, asize+1, zdata);
}
	result = zdata[asize];
	t[nt] = bchars[result];

	if (mpz(asize, zdata))
	    break;

	anorm = asize - mpsize(asize, zdata);
	if (anorm < asize)
	    asize -= anorm;
	mpsetx(asize+1, adata, asize, zdata+anorm);
	asize++;
    }
    /* XXX Fill leading zeroes (if any). */
    while (nt--)
	t[nt] = '0';
    return t;
}

static jsval
mpw_format(JSContext *cx, mpwObject * z, size_t base, int addL)
	/*@modifies t @*/
{
    size_t zsize = MPW_SIZE(z);
    mpw* zdata = MPW_DATA(z);
    jsval v;
    char * s;
    size_t i;
    size_t nt;
    size_t size;
    mpw* data;
    char * t, * te;
    char prefix[5];
    char * tcp = prefix;
    int sign;

#ifdef	FIXME
    if (z == NULL || !mpw_Check(z)) {
	PyErr_BadInternalCall();
	return NULL;
    }
#endif

if (_debug < -1)
fprintf(stderr, "==> %s(%p,%u,%d):\t", __FUNCTION__, z, (unsigned)base, addL), mpfprintln(stderr, zsize, zdata);

    assert(base >= 2 && base <= 36);

    i = 0;
    if (addL && initialiser_name != NULL)
	i = strlen(initialiser_name) + 2; /* e.g. 'mpw(' + ')' */

    sign = z->ob_size;
    nt = MPBITCNT(zsize, zdata);
    if (nt == 0) {
	base = 10;	/* '0' in every base, right */
	nt = 1;
	size = 1;
	data = alloca(size * sizeof(*data));
	*data = 0;
    } else if (sign < 0) {
	*tcp++ = '-';
	i += 1;		/* space to hold '-' */
	size = MP_ROUND_B2W(nt);
	data = zdata + (zsize - size);
    } else {
	size = MP_ROUND_B2W(nt);
	data = zdata + (zsize - size);
    }

    if (addL && size > 1)
	i++;	/* space for 'L' suffix */

    nt = mpsizeinbase(size, data, base);
    i += nt;

    if (base == 16) {
	*tcp++ = '0';
	*tcp++ = 'x';
	i += 2;		/* space to hold '0x' */
    } else if (base == 8) {
	*tcp++ = '0';
	i += 1;		/* space to hold the extra '0' */
    } else if (base > 10) {
	*tcp++ = '0' + base / 10;
	*tcp++ = '0' + base % 10;
	*tcp++ = '#';
	i += 3;		/* space to hold e.g. '12#' */
    } else if (base < 10) {
	*tcp++ = '0' + base;
	*tcp++ = '#';
	i += 2;		/* space to hold e.g. '6#' */
    }

    s = xcalloc(1, i+1);

    /* get the beginning of the string memory and start copying things */
    te = s;
    if (addL && initialiser_name != NULL && *initialiser_name != '\0') {
	te = stpcpy(te, initialiser_name);
	*te++ = '('; /*')'*/
    }

    /* copy the already prepared prefix; e.g. sign and base indicator */
    *tcp = '\0';
    t = te = stpcpy(te, prefix);

    (void) mpstr(te, nt, size, data, base);

    /* Nuke leading zeroes. */
    nt = 0;
    while (t[nt] == '0')
	nt++;
    if (t[nt] == '\0')	/* all zeroes special case. */
	nt--;
    if (nt > 0)
    do {
	*t = t[nt];
    } while (*t++ != '\0');

    te += strlen(te);

    if (addL) {
	if (size > 1)
	    *te++ = 'L';
	if (initialiser_name != NULL && *initialiser_name != '\0')
	    *te++ = /*'('*/ ')';
    }
    *te = '\0';

assert((te - s) <= (int)i);

    v = STRING_TO_JSVAL(JS_NewStringCopyZ(cx, s));
    s = _free(s);
    return v;
}

/**
 *  Precomputes the sliding window table for computing powers of x.
 *
 * Sliding Window Exponentiation, Algorithm 14.85 in "Handbook of Applied Cryptography".
 *
 * First of all, the table with the powers of g can be reduced by
 * about half; the even powers don't need to be accessed or stored.
 *
 * Get up to K bits starting with a one, if we have that many still available.
 *
 * Do the number of squarings of A in the first column, then multiply by
 * the value in column two, and finally do the number of squarings in
 * column three.
 *
 * This table can be used for K=2,3,4 and can be extended.
 *
 *
\verbatim
	   0 : - | -       | -
	   1 : 1 |  g1 @ 0 | 0
	  10 : 1 |  g1 @ 0 | 1
	  11 : 2 |  g3 @ 1 | 0
	 100 : 1 |  g1 @ 0 | 2
	 101 : 3 |  g5 @ 2 | 0
	 110 : 2 |  g3 @ 1 | 1
	 111 : 3 |  g7 @ 3 | 0
	1000 : 1 |  g1 @ 0 | 3
	1001 : 4 |  g9 @ 4 | 0
	1010 : 3 |  g5 @ 2 | 1
	1011 : 4 | g11 @ 5 | 0
	1100 : 2 |  g3 @ 1 | 2
	1101 : 4 | g13 @ 6 | 0
	1110 : 3 |  g7 @ 3 | 1
	1111 : 4 | g15 @ 7 | 0
\endverbatim
 *
 */
static void mpslide(size_t xsize, const mpw* xdata,
		size_t size, /*@out@*/ mpw* slide)
	/*@modifies slide @*/
{
    size_t rsize = (xsize > size ? xsize : size);
    mpw* result = alloca(2 * rsize * sizeof(*result));

    mpsqr(result, xsize, xdata);			/* x^2 temp */
    mpsetx(size, slide, xsize+xsize, result);
if (_debug < 0)
fprintf(stderr, "\t  x^2:\t"), mpfprintln(stderr, size, slide);
    mpmul(result,   xsize, xdata, size, slide);	/* x^3 */
    mpsetx(size, slide+size, xsize+size, result);
if (_debug < 0)
fprintf(stderr, "\t  x^3:\t"), mpfprintln(stderr, size, slide+size);
    mpmul(result,  size, slide, size, slide+size);	/* x^5 */
    mpsetx(size, slide+2*size, size+size, result);
if (_debug < 0)
fprintf(stderr, "\t  x^5:\t"), mpfprintln(stderr, size, slide+2*size);
    mpmul(result,  size, slide, size, slide+2*size);	/* x^7 */
    mpsetx(size, slide+3*size, size+size, result);
if (_debug < 0)
fprintf(stderr, "\t  x^7:\t"), mpfprintln(stderr, size, slide+3*size);
    mpmul(result,  size, slide, size, slide+3*size);	/* x^9 */
    mpsetx(size, slide+4*size, size+size, result);
if (_debug < 0)
fprintf(stderr, "\t  x^9:\t"), mpfprintln(stderr, size, slide+4*size);
    mpmul(result,  size, slide, size, slide+4*size);	/* x^11 */
    mpsetx(size, slide+5*size, size+size, result);
if (_debug < 0)
fprintf(stderr, "\t x^11:\t"), mpfprintln(stderr, size, slide+5*size);
    mpmul(result,  size, slide, size, slide+5*size);	/* x^13 */
    mpsetx(size, slide+6*size, size+size, result);
if (_debug < 0)
fprintf(stderr, "\t x^13:\t"), mpfprintln(stderr, size, slide+6*size);
    mpmul(result,  size, slide, size, slide+6*size);	/* x^15 */
    mpsetx(size, slide+7*size, size+size, result);
if (_debug < 0)
fprintf(stderr, "\t x^15:\t"), mpfprintln(stderr, size, slide+7*size);
    mpsetx(size, slide, xsize, xdata);		/* x^1 */
if (_debug < 0)
fprintf(stderr, "\t  x^1:\t"), mpfprintln(stderr, size, slide);
}

/*@observer@*/ /*@unchecked@*/
static byte mpslide_presq[16] =
{ 0, 1, 1, 2, 1, 3, 2, 3, 1, 4, 3, 4, 2, 4, 3, 4 };

/*@observer@*/ /*@unchecked@*/
static byte mpslide_mulg[16] =
{ 0, 0, 0, 1, 0, 2, 1, 3, 0, 4, 2, 5, 1, 6, 3, 7 };

/*@observer@*/ /*@unchecked@*/
static byte mpslide_postsq[16] =
{ 0, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0 };

/**
 * Exponentiation with precomputed sliding window table.
 */
static void mpnpowsld_w(mpnumber* n, size_t size, const mpw* slide,
		size_t psize, const mpw* pdata)
	/*@modifies n @*/
{
    size_t rsize = (n->size > size ? n->size : size);
    mpw* rdata = alloca(2 * rsize * sizeof(*rdata));
    short lbits = 0;
    short kbits = 0;
    byte s;
    mpw temp;
    short count;

if (_debug < 0)
fprintf(stderr, "npowsld: p\t"), mpfprintln(stderr, psize, pdata);
    /* 2. A = 1, i = t. */
    mpzero(n->size, n->data);
    n->data[n->size-1] = 1;

    /* Find first bit set in exponent. */
    temp = *pdata;
    count = 8 * sizeof(temp);
    while (count != 0) {
	if (temp & MP_MSBMASK)
	    break;
	temp <<= 1;
	count--;
    }

    while (psize) {
	while (count != 0) {

	    /* Shift next bit of exponent into sliding window. */
	    kbits <<= 1;
	    if (temp & MP_MSBMASK)
		kbits++;

	    /* On 1st non-zero in window, try to collect K bits. */
	    if (kbits != 0) {
		if (lbits != 0)
		    lbits++;
		else if (temp & MP_MSBMASK)
		    lbits = 1;
		else
		    {};

		/* If window is full, then compute and clear window. */
		if (lbits == 4) {
if (_debug < 0)
fprintf(stderr, "*** #1 lbits %d kbits %d\n", lbits, kbits);
		    for (s = mpslide_presq[kbits]; s > 0; s--) {
			mpsqr(rdata, n->size, n->data);
			mpsetx(n->size, n->data, 2*n->size, rdata);
if (_debug < 0)
fprintf(stderr, "\t pre1:\t"), mpfprintln(stderr, n->size, n->data);
		    }

		    mpmul(rdata, n->size, n->data,
				size, slide+mpslide_mulg[kbits]*size);
		    mpsetx(n->size, n->data, n->size+size, rdata);
if (_debug < 0)
fprintf(stderr, "\t mul1:\t"), mpfprintln(stderr, n->size, n->data);

		    for (s = mpslide_postsq[kbits]; s > 0; s--) {
			mpsqr(rdata, n->size, n->data);
			mpsetx(n->size, n->data, 2*n->size, rdata);
if (_debug < 0)
fprintf(stderr, "\tpost1:\t"), mpfprintln(stderr, n->size, n->data);
		    }

		    lbits = kbits = 0;
		}
	    } else {
		mpsqr(rdata, n->size, n->data);
		mpsetx(n->size, n->data, 2*n->size, rdata);
if (_debug < 0)
fprintf(stderr, "\t  sqr:\t"), mpfprintln(stderr, n->size, n->data);
	    }

	    temp <<= 1;
	    count--;
	}
	if (--psize) {
	    count = 8 * sizeof(temp);
	    temp = *(pdata++);
	}
    }

    if (kbits != 0) {
if (_debug < 0)
fprintf(stderr, "*** #1 lbits %d kbits %d\n", lbits, kbits);
	for (s = mpslide_presq[kbits]; s > 0; s--) {
	    mpsqr(rdata, n->size, n->data);
	    mpsetx(n->size, n->data, 2*n->size, rdata);
if (_debug < 0)
fprintf(stderr, "\t pre2:\t"), mpfprintln(stderr, n->size, n->data);
	}

	mpmul(rdata, n->size, n->data,
			size, slide+mpslide_mulg[kbits]*size);
	mpsetx(n->size, n->data, n->size+size, rdata);
if (_debug < 0)
fprintf(stderr, "\t mul2:\t"), mpfprintln(stderr, n->size, n->data);

	for (s = mpslide_postsq[kbits]; s > 0; s--) {
	    mpsqr(rdata, n->size, n->data);
	    mpsetx(n->size, n->data, 2*n->size, rdata);
if (_debug < 0)
fprintf(stderr, "\tpost2:\t"), mpfprintln(stderr, n->size, n->data);
	}
    }
}

/**
 * mpnpow_w
 *
 * Uses sliding window exponentiation; needs extra storage:
 *	if K=3, needs 4*size, if K=4, needs 8*size
 */
static void mpnpow_w(mpnumber* n, size_t xsize, const mpw* xdata,
		size_t psize, const mpw* pdata)
	/*@modifies n @*/
{
    size_t xbits = MPBITCNT(xsize, xdata);
    size_t pbits = MPBITCNT(psize, pdata);
    size_t nbits;
    mpw *slide;
    size_t nsize;
    size_t size;

    /* Special case: 0**P and X**(-P) */
    if (xbits == 0 || (psize > 0 && mpmsbset(psize, pdata))) {
	mpnsetw(n, 0);
	return;
    }
    /* Special case: X**0 and 1**P */
    if (pbits == 0 || mpisone(xsize, xdata)) {
	mpnsetw(n, 1);
	return;
    }

    /* Normalize (to mpw boundary) exponent. */
    pdata += psize - MP_ROUND_B2W(pbits);
    psize -= MP_BITS_TO_WORDS(pbits);

    /* Calculate size of result. */
    if (xbits == 0) xbits = 1;
    nbits = (*pdata) * xbits;
    nsize = MP_ROUND_B2W(nbits);

    /* XXX Add 1 word to carry sign bit */
    if (!mpmsbset(xsize, xdata) && (nbits & (MP_WBITS - 1)) == 0)
	nsize++;

    size = MP_ROUND_B2W(15 * xbits);

if (_debug < 0)
fprintf(stderr, "*** pbits %u xbits %u nsize %u size %u\n", (unsigned)pbits, (unsigned)xbits, (unsigned)nsize, (unsigned)size);
    mpnsize(n, nsize);

    /* 1. Precompute odd powers of x (up to 2**K). */
    slide = (mpw*) alloca( (8*size) * sizeof(mpw));

    mpslide(xsize, xdata, size, slide);

    /*@-internalglobs -mods@*/ /* noisy */
    mpnpowsld_w(n, size, slide, psize, pdata);
    /*@=internalglobs =mods@*/

}

/** z = sqrt(x). */
static void mpsqrt(mpw* zdata, size_t xsize, const mpw* xdata)
{
	size_t size = MP_BITS_TO_WORDS(mpbits(xsize, xdata) + MP_WBITS);
	mpw * ydata = alloca(size * sizeof(*ydata));
	mpw * wdata = alloca(size * sizeof(*wdata));
	mpw * vdata = alloca(size * sizeof(*vdata));

	/* XXX check x < 0 and raise exception */
	if (mpleone(xsize, xdata)) {
		mpsetx(xsize, zdata, xsize, xdata);
		return;
	}

//	short op = num;
	mpsetx(size, ydata, xsize, xdata);
//	short res = 0;
	mpzero(xsize, zdata);
//	short one = 1 << 14; // The second-to-top bit is set: 1L<<30 for long
	mpzero(size, wdata);
	mpsetmsb(size, wdata);
	mpdivtwo(size, wdata);

//	while (one > op)
//	    one >>= 2;
	while (mpgt(size, wdata, ydata))
		mprshift(size, wdata, 2);

//	while (one != 0)
	while (!mpz(size, wdata)) {
//	    if (op >= res + one)
	    mpsetx(size, vdata, xsize, zdata);
	    mpadd(size, vdata, wdata);
	    if (mpge(size, ydata, vdata)) {
//		op -= res + one;
		mpsubx(size, ydata, xsize, zdata);
		mpsub(size, ydata, wdata);
//		res = (res >> 1) + one;
		mprshift(xsize, zdata, 1);
		mpaddx(xsize, zdata, size, wdata);
            } else {
//		res >>= 1;
		mprshift(xsize, zdata, 1);
	    }
//	    one >>= 2;
	    mprshift(size, wdata, 2);
	}
}

/* ---------- */

static mpwObject *
mpw_New(int ob_size)
	/*@*/
{
    size_t size = ABS(ob_size);
    size_t nb;
    mpwObject * z;

    /* XXX Make sure that 0 has allocated space. */
    if (size == 0)
	size++;
    nb = size * sizeof(*z->data);
    z = xmalloc(sizeof(*z)+nb);	/* XXX FIXME: JS class wrapping? */

    z->ob_size = ob_size;

    if (size > 0)
	memset(&z->data, 0, nb);

if (_debug < -1)
fprintf(stderr, "<== %s(%d) z %p\n", __FUNCTION__, ob_size, z);
    return z;
}

static mpwObject *
mpw_FromMPW(size_t size, mpw* data, int normalize)
{
    mpwObject * z;

    if (normalize) {
	size_t norm = size - MP_ROUND_B2W(MPBITCNT(size, data));
	if (norm > 0 && norm < size) {
	    size -= norm;
	    data += norm;
	}
    }

    z = mpw_New(size);
    if (z == NULL)
	return NULL;

    if (size > 0)
	memcpy(&z->data, data, size * sizeof(*z->data));

    return z;
}

static mpwObject *
mpw_FromLong(long ival)
	/*@*/
{
    mpwObject * z = mpw_New(1);

    if (z == NULL)
	return NULL;

    if (ival < 0) {
	z->ob_size = -z->ob_size;
	ival = -ival;
    }
    z->data[0] = (mpw) ival;

    return z;
}

static mpwObject *
mpw_FromDouble(double dval)
{
    static int mbits = 52;
    int exp = 0;
    double frac = frexp(dval, &exp);
    uint64_t flval;
    uint64_t llval;
#if (MP_WBITS == 64)
    size_t xsize = 1;
    mpw xdata[1];
#else
    size_t xsize = 2;
    mpw xdata[2];
#endif
    int bits = exp - MP_WBITS;
    size_t zsize = 1;
    mpwObject * z;

    if (bits > 0)
	zsize += MP_BITS_TO_WORDS(bits + MP_WBITS-1);

    if (frac < 0.0) {
	frac = -frac;
	llval = -dval;
    } else
	llval = dval;
    flval = ldexp(frac, mbits);

#if (MP_WBITS == 64)
    xdata[0] = (mpw)flval;
#else
    xdata[0] = (mpw) ((flval >> 32) & 0xffffffff);
    xdata[1] = (mpw) ((flval      ) & 0xffffffff);
#endif
    if (exp == mbits)
	exp = 0;	/* nothing to do */
    else if (exp < mbits) {
	mprshift(xsize, xdata, (mbits - exp));
	exp = 0;
    } else if (exp < 64) {
	mplshift(xsize, xdata, (exp - mbits));
	exp = 0;
    } else
	exp -= mbits;

    z = mpw_New(zsize);
    mpsetx(MPW_SIZE(z), MPW_DATA(z), xsize, xdata);
    if (exp > 0) {
	mplshift(MPW_SIZE(z), MPW_DATA(z), exp);
    }
    if (dval < 0.0)
	z->ob_size = -z->ob_size;

    return z;
}

#define	_CHARMASK(_a)	(_a)
static mpwObject *
mpw_FromString(const char * str, const char ** sep, int base)
	/*@*/
{
    const char * s = str, * se;
    mpwObject * z = NULL;
    mpw zbase, zval;
    int sign = 1;
    int ndigits;

    if ((base != 0 && base < 2) || base > 36) {
#ifdef	FIXME
	PyErr_SetString(PyExc_ValueError, "mpw() arg 2 must be >= 2 and <= 36");
#else
assert(0);
#endif
	return NULL;
    }
    while (*s != '\0' && isspace(_CHARMASK(*s)))
	s++;
    if (*s == '+')
	++s;
    else if (*s == '-') {
	++s;
	sign = -1;
    }
    while (*s != '\0' && isspace(_CHARMASK(*s)))
	s++;
    if (base == 0) {
	if (s[0] != '0')
	    base = 10;
	else if (s[1] == 'x' || s[1] == 'X')
	    base = 16;
	else
	    base = 8;
    }
    if (base == 16 && s[0] == '0' && (s[1] == 'x' || s[1] == 'X'))
	s += 2;

    /* Validate characters as digits of base. */
    for (se = s; *se != '\0'; se++) {
	int k;

	if (*se <= '9')
	    k = *se - '0';
	else if (*se >= 'a')
	    k = *se - 'a' + 10;
	else if (*se >= 'A')
	    k = *se - 'A' + 10;
	else
	    k = -1;
	if (k < 0 || k >= base)
	    break;
    }
    if (se == s)
	goto onError;

    ndigits = (se - s);

    if (*se == 'L' || *se == 'l')
	se++; 
    while (*se && isspace(_CHARMASK(*se)))
	se++;
    if (sep)
	*sep = se;
    if (*se != '\0')
	goto onError;

    /* Allocate mpw. */

    /* Convert digit string. */
    zbase = base;
    for (se = s; *se != '\0'; se++) {
	if (*se <= '9')
	    zval = *se - '0';
	else if (*se >= 'a')
	    zval = *se - 'a' + 10;
	else if (*se >= 'A')
	    zval = *se - 'A' + 10;
    }

    if (sign < 0 && z != NULL && z->ob_size != 0)
	z->ob_size = -(z->ob_size);

    return z;

onError:
#ifdef	FIXME
    PyErr_Format(PyExc_ValueError, "invalid literal for mpw(): %.200s", str);
    Py_XDECREF(z);
#endif
    return NULL;
}
#undef	_CHARMASK

static mpwObject *
mpw_FromHEX(const char * hex)
	/*@*/
{
    size_t len = strlen(hex);
    size_t size = MP_NIBBLES_TO_WORDS(len + MP_WNIBBLES - 1);
    mpwObject * z = mpw_New(size);

    if (z != NULL && size > 0)
	hs2ip(MPW_DATA(z), size, hex, len);

if (_debug < 0)
fprintf(stderr, "<== %s(%s) z %p\n\t", __FUNCTION__, hex, z), mpfprintln(stderr, MPW_SIZE(z), MPW_DATA(z));
    return z;
}

#ifdef	NOTYET
static mpwObject *
mpw_FromLongObject(PyLongObject *lo)
	/*@*/
{
    mpwObject * z;
    int lsize = ABS(lo->ob_size);
    int lbits = DIGITS_TO_BITS(lsize);
    size_t zsize = MP_BITS_TO_WORDS(lbits) + 1;
    mpw* zdata;
    unsigned char * zb;
    size_t nzb;
    int is_littleendian = 0;
    int is_signed = 0;

    lsize = zsize;
    if (lo->ob_size < 0)
	lsize = -lsize;
    z = mpw_New(lsize);
    if (z == NULL)
	return NULL;

    zdata = MPW_DATA(z);
    zb = (unsigned char *) zdata;
    nzb = MP_WORDS_TO_BYTES(zsize);

    /* Grab long as big-endian unsigned octets. */
    if (_PyLong_AsByteArray(lo, zb, nzb, is_littleendian, is_signed)) {
	Py_XDECREF(z);
	return NULL;
    }

    /* Endian swap zdata's mpw elements. */
    if (IS_LITTLE_ENDIAN()) {
	mpw w = 0;
	int zx = 0;
	while (nzb) {
	    w <<= 8;
	    w |= *zb++;
	    nzb--;
	    if ((nzb % MP_WBYTES) == 0) {
		zdata[zx++] = w;
		w = 0;
	    }
	}
    }

    return z;
}
#endif	/* NOTYET */

/* ---------- */
#define	_c2i(_a, _b)	((int)(((_a)<<8)+(_b)))
#define	ADDM	_c2i('M', '+')
#define	SUBM	_c2i('M', '-')
#define	MULM	_c2i('M', '*')
#define	DIVM	_c2i('M', '/')	/* XXX unimplemented */
#define	MODM	_c2i('M', '%')	/* XXX unimplemented */
#define	POWM	_c2i('M', '^')

#define	ADDP	_c2i('P', '+')
#define	SUBP	_c2i('P', '-')
#define	MULP	_c2i('P', '*')
#define	DIVP	_c2i('P', '/')	/* XXX unimplemented */
#define	MODP	_c2i('P', '%')	/* XXX unimplemented */
#define	POWP	_c2i('P', '^')

/** Compute 1 argument operations. */
static mpwObject *
mpw_ops1(int op, mpwObject *x)
        /*@*/
{
    mpwObject * z = NULL;

assert(x);
    if (x == NULL)
	goto exit;

if (_debug < 0) {
fprintf(stderr, "==> %s(%c)\n", __FUNCTION__, (op & 0xff));
prtmpw("a", x);
}

    z = mpw_FromMPW(MPW_SIZE(x), MPW_DATA(x), 0);
    z->ob_size = x->ob_size;

    switch (op) {
    default:
	goto exit;
	/*@notreached@*/ break;
    case '_':
	z->ob_size = -z->ob_size;
	break;
    case 's':	/* sqrt(x) */
	mpsqrt(MPW_DATA(z), MPW_SIZE(x), MPW_DATA(x));
	break;
    case '!':
	if (z->ob_size < 0)
	    z->ob_size = -z->ob_size;
	break;
    case '~': 
#ifdef	DYING
	/* Implement ~z as -(z+1) */
	(void) mpaddw(MPW_SIZE(z), MPW_DATA(z), 1);
	if (x->ob_size > 0)
	    z->ob_size = -z->ob_size;
#else
	(void) mpnot(MPW_SIZE(z), MPW_DATA(z));
#endif
	break;
    }

if (_debug < 0)
fprintf(stderr, "<== %s(%c) %p[%u]\t", __FUNCTION__, (op & 0xff), MPW_DATA(z), (unsigned)MPW_SIZE(z)), mpfprintln(stderr, MPW_SIZE(z), MPW_DATA(z));

exit:
assert(z);
    return z;
}

/** Compute 2 argument operations. */
static mpwObject *
mpw_ops2(int op, mpwObject *x, mpwObject *m)
        /*@*/
{
    mpwObject * z = NULL;
    size_t xsize;
    mpw* xdata;
    size_t msize;
    mpw* mdata;
    size_t mnorm;
    size_t asize;
    mpw* adata;
    size_t bsize;
    mpw* bdata;
    size_t shift;
    size_t zsize;
    mpw* zdata;
    mpw* wksp;
    mpbarrett b;
    int carry;
    int zsign = 0;

    mpbzero(&b);
assert(x);
assert(m);
    if (x == NULL || m == NULL)
	goto exit;

    xsize = MPW_SIZE(x);
    xdata = MPW_DATA(x);
    msize = MPW_SIZE(m);
    mdata = MPW_DATA(m);
    mnorm = msize - mpsize(msize, mdata);
    if (mnorm > 0 && mnorm < msize) {
	msize -= mnorm;
	mdata += mnorm;
    }

if (_debug < 0) {
fprintf(stderr, "==> %s(%c)\n", __FUNCTION__, (op & 0xff));
prtmpw("a", x);
prtmpw("b", m);
}

    switch (op) {
    default:
	goto exit;
	/*@notreached@*/ break;
    case '+':
	zsize = MAX(xsize, msize) + 1;
	zdata = alloca(zsize * sizeof(*zdata));
	mpsetx(zsize, zdata, xsize, xdata);
	if (x->ob_size < 0) {
	    zsign = 1;
	    if (m->ob_size < 0) {
		carry = mpaddx(zsize-1, zdata+1, msize, mdata);
		if (carry) {
if (_debug)
fprintf(stderr, "add --: carry\n");
		    *zdata = 1;
		}
	    } else {
		carry = mpsubx(zsize-1, zdata+1, msize, mdata);
		if (carry) {
if (_debug)
fprintf(stderr, "add -+: borrow\n");
		    *zdata = MP_ALLMASK;
		    mpneg(zsize, zdata);
		    zsign = 0;
		}
	    }
	} else {
	    zsign = 0;
	    if (m->ob_size < 0) {
		carry = mpsubx(zsize-1, zdata+1, msize, mdata);
		if (carry) {
if (_debug)
fprintf(stderr, "add +-: borrow\n");
		    *zdata = MP_ALLMASK;
		    mpneg(zsize, zdata);
		    zsign = 1;
		}
	    } else {
		carry = mpaddx(zsize-1, zdata+1, msize, mdata);
		if (carry) {
if (_debug)
fprintf(stderr, "add ++: carry\n");
		    *zdata = 1;
		}
	    }
	}
	z = mpw_FromMPW(zsize, zdata, 1);
	if (zsign)
	    z->ob_size = -z->ob_size;
	break;
    case '-':
	zsize = MAX(xsize, msize) + 1;
	zdata = alloca(zsize * sizeof(*zdata));
	mpsetx(zsize, zdata, xsize, xdata);
	if (x->ob_size < 0) {
	    zsign = 1;
	    if (m->ob_size < 0) {
		carry = mpsubx(zsize-1, zdata+1, msize, mdata);
		if (carry) {
if (_debug)
fprintf(stderr, "sub --: borrow\n");
		    *zdata = MP_ALLMASK;
		    mpneg(zsize, zdata);
		    zsign = 0;
		}
	    } else {
		carry = mpaddx(zsize-1, zdata+1, msize, mdata);
		if (carry) {
if (_debug)
fprintf(stderr, "sub -+: carry\n");
		    *zdata = 1;
		}
	    }
	} else {
	    zsign = 0;
	    if (m->ob_size < 0) {
		carry = mpaddx(zsize-1, zdata+1, msize, mdata);
		if (carry) {
if (_debug)
fprintf(stderr, "sub +-: carry\n");
		    *zdata = 1;
		}
	    } else {
		carry = mpsubx(zsize-1, zdata+1, msize, mdata);
		if (carry) {
if (_debug)
fprintf(stderr, "sub ++: borrow\n");
		    *zdata = MP_ALLMASK;
		    mpneg(zsize, zdata);
		    zsign = 1;
		}
	    }
	}
	z = mpw_FromMPW(zsize-1, zdata+1, 1);
	if (zsign)
	    z->ob_size = -z->ob_size;
	break;
    case '*':
	zsize = xsize + msize;
	zdata = alloca(zsize * sizeof(*zdata));
	zsign = x->ob_size * m->ob_size;
	mpmul(zdata, xsize, xdata, msize, mdata);
	z = mpw_FromMPW(zsize, zdata, 1);
	if (zsign < 0)
	    z->ob_size = -z->ob_size;
	break;
    case '/':
	asize = xsize+1;
	adata = alloca(asize * sizeof(*adata));
	mpsetx(asize, adata, xsize, xdata);
	bsize = msize;
	bdata = alloca(bsize * sizeof(*bdata));
	mpsetx(bsize, bdata, msize, mdata);

	zsize = asize + 1;
	zdata = alloca(zsize * sizeof(*zdata));
	zsign = x->ob_size * m->ob_size;
	wksp = alloca((bsize+1) * sizeof(*wksp));

	shift = mpnorm(bsize, bdata);
	mplshift(asize, adata, shift);
	mpndivmod(zdata, asize, adata, bsize, bdata, wksp);

	zsize -= bsize;

	if (zsign < 0)
	    (void) mpaddw(zsize, zdata, (mpw)1);

	z = mpw_FromMPW(zsize, zdata, 1);
	if (zsign < 0)
	    z->ob_size = -z->ob_size;
	break;
    case '%':
	if (mpz(msize, mdata)) {
	    z = mpw_FromMPW(xsize, xdata, 1);
	    break;
	}
	asize = MAX(xsize, msize) + 1;
	adata = alloca(asize * sizeof(*adata));
	mpsetx(asize, adata, xsize, xdata);
	bsize = msize;
	bdata = mdata;

	zsize = asize;
	zdata = alloca(zsize * sizeof(*zdata));
	zsign = x->ob_size * m->ob_size;
	wksp = alloca((2*bsize+1) * sizeof(*wksp));

	mpmod(zdata, asize, adata, bsize, bdata, wksp);

#ifdef	NOTYET
	if (zsign < 0) {
	    if (m->ob_size < 0) {
		(void) mpsubx(zsize, zdata, bsize, bdata);
		mpneg(zsize, zdata);
	    } else if (x->ob_size < 0) {
		mpneg(zsize, zdata);
		(void) mpaddx(zsize, zdata, bsize, bdata);
	    }
	}
#endif

	z = mpw_FromMPW(zsize, zdata, 1);

#ifdef	NOTYET
	if (zsign < 0)
#endif
	{
#ifdef	PYTHON_DIVISOR
	    if (m->ob_size < 0)		/* divisor determines sign. */
#else
	    if (x->ob_size < 0)		/* dividend determines sign. */
#endif
		z->ob_size = -z->ob_size;
	}
	break;
    case '<':
	/* XXX FIXME: negative count? sign?. */
	shift = (size_t) (msize == 1 ? (mdata[0] & 0x00ffffff) : 0);
	msize = MP_ROUND_B2W(shift - mpmszcnt(xsize, xdata));
	asize = MAX(xsize, msize);
	adata = alloca(asize * sizeof(*adata));
	mpsetx(asize, adata, xsize, xdata);
	if (shift > 0)
	    mplshift(asize, adata, shift);
	z = mpw_FromMPW(asize, adata, 0);
	break;
    case '>':
	/* XXX FIXME: negative count? sign?. */
	shift = (size_t) (msize == 1 ? mdata[0] : 0);
	z = mpw_FromMPW(xsize, xdata, 0);
	if (shift > 0)
	    mprshift(MPW_SIZE(z), MPW_DATA(z), shift);
	break;
    case '&':
	/* XXX reset to original size for now. */
	msize = MPW_SIZE(m);
	mdata = MPW_DATA(m);
	if (xsize <= msize) {
	    z = mpw_FromMPW(xsize, xdata, 0);
	    mpand(MPW_SIZE(z), MPW_DATA(z), mdata + (msize - xsize));
	} else {
	    z = mpw_FromMPW(msize, mdata, 0);
	    mpand(MPW_SIZE(z), MPW_DATA(z), xdata + (xsize - msize));
	}
	break;
    case '|':
	/* XXX reset to original size for now. */
	msize = MPW_SIZE(m);
	mdata = MPW_DATA(m);
	if (xsize <= msize) {
	    z = mpw_FromMPW(xsize, xdata, 0);
	    mpor(MPW_SIZE(z), MPW_DATA(z), mdata + (msize - xsize));
	} else {
	    z = mpw_FromMPW(msize, mdata, 0);
	    mpor(MPW_SIZE(z), MPW_DATA(z), xdata + (xsize - msize));
	}
	break;
    case '^':
	/* XXX reset to original size for now. */
	msize = MPW_SIZE(m);
	mdata = MPW_DATA(m);
	if (xsize <= msize) {
	    z = mpw_FromMPW(xsize, xdata, 0);
	    mpxor(MPW_SIZE(z), MPW_DATA(z), mdata + (msize - xsize));
	} else {
	    z = mpw_FromMPW(msize, mdata, 0);
	    mpxor(MPW_SIZE(z), MPW_DATA(z), xdata + (xsize - msize));
	}
	break;
    case 'P':
    {	mpnumber zn;

	mpnzero(&zn);
	if (msize == 0 || (msize == 1 && *mdata == 0))
	    mpnsetw(&zn, 1);
	else if (mpz(xsize, xdata) || m->ob_size < 0)
	    mpnsetw(&zn, 0);
	else {
	    zsign = (x->ob_size > 0 || mpeven(msize, mdata)) ? 1 : -1;
	    mpnpow_w(&zn, xsize, xdata, msize, mdata);
	}
	z = mpw_FromMPW(zn.size, zn.data, 1);
	mpnfree(&zn);
	if (zsign < 0)
	    z->ob_size = -z->ob_size;
    }	break;
    case 'G':
	/* XXX this scaling is not correct. */
	wksp = alloca((xsize) * sizeof(*wksp));
	z = mpw_New(msize);
	mpgcd_w(xsize, xdata, mdata, MPW_DATA(z), wksp);
	break;
    case 'I':
	wksp = alloca(((1+1+6)*msize+6)*sizeof(*wksp));
	mpsetx(msize, wksp+msize, xsize, xdata);
	(void) mpextgcd_w(msize, mdata, wksp+msize, wksp, wksp+2*msize);
	z = mpw_FromMPW(msize, wksp, 0);
	break;
#ifdef	DYING
    case 'R':
    {	rngObject * r = ((rngObject *)x);

	wksp = alloca(msize*sizeof(*wksp));
	mpbset(&b, msize, mdata);
	z = mpw_New(msize);
	mpbrnd_w(&b, &r->rngc, MPW_DATA(z), wksp);
    }	break;
#endif
    case 'S':
	/* XXX is scaling correct if xsize > msize? */
	wksp = alloca((4*msize+2)*sizeof(*wksp));
	mpbset(&b, msize, mdata);
	z = mpw_New(msize);
	mpbsqrmod_w(&b, xsize, xdata, MPW_DATA(z), wksp);
	break;
    }

if (_debug)
fprintf(stderr, "<== %s(%c) %p[%u]\t", __FUNCTION__, (op & 0xff), MPW_DATA(z), (unsigned)MPW_SIZE(z)), mpfprintln(stderr, MPW_SIZE(z), MPW_DATA(z));

exit:
    mpbfree(&b);
assert(z);
    return z;
}

/** Compute 3 argument operations. */
static mpwObject *
mpw_ops3(int op, mpwObject *x, mpwObject *y, mpwObject *m)
        /*@*/
{
    mpwObject * z = NULL;
    size_t xsize;
    mpw* xdata;
    size_t ysize;
    mpw* ydata;
    size_t msize;
    mpw* mdata;
    size_t zsize;
    mpw* zdata;
    mpbarrett b;
    mpw* wksp;

    mpbzero(&b);
assert(x);
assert(y);
assert(m);
    if (x == NULL || y == NULL || m == NULL)
	goto exit;

if (_debug < 0) {
fprintf(stderr, "==> %s(%c)\n", __FUNCTION__, (op & 0xff));
prtmpw("a", x);
prtmpw("b", y);
prtmpw("c", m);
}

    xsize = MPW_SIZE(x);
    xdata = MPW_DATA(x);
    ysize = MPW_SIZE(y);
    ydata = MPW_DATA(y);
    msize = MPW_SIZE(m);
    mdata = MPW_DATA(m);

    mpbset(&b, msize, mdata);

    zsize = b.size;
    zdata = alloca(zsize * sizeof(*zdata));

#ifdef	NOTYET	/* XXX no known scaling problems (yet) */
    wksp = alloca(zsize * sizeof(*zdata));
    mpsetx(zsize, wksp, xsize, xdata);
    xsize = zsize;
    xdata = wksp;
    wksp = alloca(zsize * sizeof(*zdata));
    mpsetx(zsize, wksp, ysize, ydata);
    ysize = zsize;
    ydata = wksp;
#endif

    wksp = alloca((4*zsize+2)*sizeof(*wksp));

    switch (op) {
    case DIVM:	/* XXX unimplemented */
    case MODM:	/* XXX unimplemented */
    default:
	goto exit;
	/*@notreached@*/ break;
    case ADDM:
	mpbaddmod_w(&b, xsize, xdata, ysize, ydata, zdata, wksp);
	break;
    case SUBM:
	mpbsubmod_w(&b, xsize, xdata, ysize, ydata, zdata, wksp);
	break;
    case MULM:
	mpbmulmod_w(&b, xsize, xdata, ysize, ydata, zdata, wksp);
	break;
    case POWM:
	mpbpowmod_w(&b, xsize, xdata, ysize, ydata, zdata, wksp);
	break;
    }

    z = mpw_FromMPW(zsize, zdata, 1);

exit:
if (_debug < 0)
fprintf(stderr, "<== %s(%c) %p[%u]\t", __FUNCTION__, (op & 0xff), MPW_DATA(z), (unsigned)MPW_SIZE(z)), mpfprintln(stderr, MPW_SIZE(z), MPW_DATA(z));

    mpbfree(&b);
assert(z);
    return z;
}

#ifdef	NOTYET
static uint32_t ceillog2(uint32_t v)
{
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    v++;
    return v;
}
#endif

/** Compute 3 argument polynomial operations. */
static mpwObject *
mpw_pops3(int op, mpwObject *x, mpwObject *y, mpwObject *m)
        /*@*/
{
    mpwObject * z = NULL;

assert(x);
assert(y);
assert(m);
    if (x == NULL || y == NULL || m == NULL)
	goto exit;

    /* Handle GF(2^m) extension field polynomial arithmetic. */
    switch (op) {
    case DIVP:	/* XXX unimplemented */
    case MODP:	/* XXX unimplemented */
    default:
assert(0);
	/*@notreached@*/ break;
    case ADDP:
	z = mpw_ops2('^', x, y);
	/* XXX reduce modulo p */
	break;
    case SUBP:
	z = mpw_ops2('^', x, y);
	/* XXX reduce modulo p */
	break;
    case MULP:
	z = mpw_ops3(MULM, x, y, m);	/* FIXME */
	/* XXX reduce modulo p */
	break;
    case POWP:
	z = mpw_ops3(POWM, x, y, m);	/* FIXME */
	/* XXX reduce modulo p */
	break;
    }

exit:
if (_debug < 0)
fprintf(stderr, "<== %s(%c) %p[%u]\t", __FUNCTION__, (op & 0xff), MPW_DATA(z), (unsigned)MPW_SIZE(z)), mpfprintln(stderr, MPW_SIZE(z), MPW_DATA(z));

assert(z);
    return z;
}

static JSBool
mpw_wrap(JSContext * cx, jsval * vp, mpwObject * z)
{
    JSObject *o;
    JSBool ok = JS_FALSE;

assert(z != NULL);
    if ((o = JS_NewObject(cx, &rpmmpwClass, NULL, NULL)) != NULL
     && JS_SetPrivate(cx, o, (void *)z))
    {
	*vp = OBJECT_TO_JSVAL(o);
	ok = JS_TRUE;
    } else
	*vp = JSVAL_VOID;
if (_debug < 0)
fprintf(stderr, "<== %s(%p,%p,0x%x[%s])\t", __FUNCTION__, cx, z, (unsigned)*vp, v2s(cx, *vp)), mpfprintln(stderr, MPW_SIZE(z), MPW_DATA(z));
    return ok;
}

static mpwObject *
mpw_j2mpw(JSContext *cx, jsval v)
{
    mpwObject * z = NULL;
    if (JSVAL_IS_INT(v))
	z = mpw_FromLong(JSVAL_TO_INT(v));
    else if (JSVAL_IS_DOUBLE(v))
	z = mpw_FromDouble((double)JSVAL_TO_DOUBLE(v));
    else if (JSVAL_IS_STRING(v)) {
	const char * s = JS_EncodeString(cx, JSVAL_TO_STRING(v));
	z = mpw_FromHEX(s);
	s = _free(s);
    }
    else if (JSVAL_IS_OBJECT(v)) {
	JSObject *o = JSVAL_TO_OBJECT(v);
	if (OBJ_IS_MPW(cx, o))
	    z = JS_GetInstancePrivate(cx, o, &rpmmpwClass, NULL);
    }
if (z == NULL) {
fprintf(stderr, "*** %s: 0x%x[%s]\n", __FUNCTION__, (unsigned)v, v2s(cx, v));
assert(z != NULL);
}
    return z;
}

/* --- Object methods */

/** Convert to string. */
static JSBool
mpw_toString(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmmpwClass, NULL);
    jsuint _base = 10;
    JSBool ok;

_METHOD_DEBUG_ENTRY(_debug);
    if ((ok = JS_ConvertArguments(cx, argc, argv, "/u", &_base))) {
	/* XXX FIXME check _base is 0 or [2,36] */
	*vp = mpw_format(cx, ptr, _base, 0);
    } else
	*vp = JSVAL_VOID;
    return ok;
}

#ifdef	NOTYET
/** Convert to string in base 10. */
static JSBool
mpw_valueOf(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmmpwClass, NULL);
    jsuint _base = 10;
    JSBool ok = JS_TRUE;

_METHOD_DEBUG_ENTRY(_debug);
    *vp = mpw_format(cx, ptr, _base, 0);
    return ok;
}
#endif

/** Miller-Rabin prime test. */
static JSBool
mpw_isPrime(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmmpwClass, NULL);
    mpwObject * z = ptr;
    size_t zsize = (z ? MPW_SIZE(z) : 0);
    mpw * zdata = (z ? MPW_DATA(z) : NULL);
    jsuint _trials = mpptrials(MP_WORDS_TO_BITS(zsize));
    JSBool ok = JS_TRUE;

_METHOD_DEBUG_ENTRY(_debug);
    if (z == NULL || zsize == 0 || zdata == NULL
     || (ok = JS_ConvertArguments(cx, argc, argv, "/u", &_trials)))
    {
	/* XXX select "FIPS 186" or "Mersenne Twister" */
	const randomGenerator* rng = randomGeneratorDefault();
	randomGeneratorContext rngc;

	if (!randomGeneratorContextInit(&rngc, rng)) {
	    mpw * wksp = alloca((8*zsize+2) * sizeof(*wksp));
	    mpbarrett b;

	    mpbzero(&b);
	    mpbset(&b, zsize, zdata);
	    *vp = mpbpprime_w(&b, &rngc, _trials, wksp)
		? JSVAL_TRUE : JSVAL_FALSE;
	    mpbfree(&b);
	} else
	    *vp = JSVAL_FALSE;
	randomGeneratorContextFree(&rngc);
    } else
	*vp = JSVAL_FALSE;
    return ok;
}

/** Return random k invertible modulo q. */
static JSBool
mpw_randomK(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmmpwClass, NULL);
    mpwObject * q = ptr;
    JSBool ok = JS_TRUE;

_METHOD_DEBUG_ENTRY(_debug);
    if (q) {
	/* XXX select "FIPS 186" or "Mersenne Twister" */
	const randomGenerator* rng = randomGeneratorDefault();
	randomGeneratorContext rngc;

	if (!randomGeneratorContextInit(&rngc, rng)) {
	    size_t qsize = MPW_SIZE(q);
	    mpw *qdata = MPW_DATA(q);
	    mpw *wksp = alloca(((1+1+6)*qsize+6) * sizeof(*wksp));
	    mpbarrett b;

	    mpbzero(&b);
	    mpbset(&b, qsize, qdata);
	    mpbrndinv_w(&b, &rngc, wksp, wksp+qsize, wksp+2*qsize);
	    ok = mpw_wrap(cx, vp, mpw_FromMPW(qsize, wksp, 0));
	    mpbfree(&b);
	} else
	    *vp = JSVAL_NULL;
	randomGeneratorContextFree(&rngc);
    } else
	*vp = JSVAL_NULL;
    return ok;
}

/** Return eq(x, y). */
static JSBool
mpw_eq(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmmpwClass, NULL);
    JSBool ok = JS_TRUE;

_METHOD_DEBUG_ENTRY(_debug);
    if (argc == 2) {
	mpwObject * x = mpw_j2mpw(cx, argv[0]);
	mpwObject * y = mpw_j2mpw(cx, argv[1]);

	*vp = ((x->ob_size * y->ob_size) > 0 && mpeqx(MPW_SIZE(x), MPW_DATA(x), MPW_SIZE(y), MPW_DATA(y)))
		? JSVAL_TRUE : JSVAL_FALSE;

	if (!JSVAL_IS_MPW(cx, argv[0])) x = _free(x);
	if (!JSVAL_IS_MPW(cx, argv[1])) y = _free(y);
    } else
	ok = JS_FALSE;
    return ok;
}

/** Return ne(x, y). */
static JSBool
mpw_ne(JSContext *cx, uintN argc, jsval *vp)
{
    JSBool ok = JS_TRUE;
    if ((ok = mpw_eq(cx, argc, vp)))
	*vp = (*vp == JSVAL_TRUE ? JSVAL_FALSE : JSVAL_TRUE);
    return ok;
}

/** Return lt(x, y). */
static JSBool
mpw_lt(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmmpwClass, NULL);
    JSBool ok = JS_TRUE;

_METHOD_DEBUG_ENTRY(_debug);
    if (argc == 2) {
	mpwObject * x = mpw_j2mpw(cx, argv[0]);
	mpwObject * y = mpw_j2mpw(cx, argv[1]);

	if (x->ob_size < 0 && y->ob_size > 0)
	    *vp = JSVAL_TRUE;
	else
	if (x->ob_size > 0 && y->ob_size < 0)
	    *vp = JSVAL_FALSE;
	else
	if (x->ob_size < 0 && y->ob_size < 0)
	    *vp = mpgtx(MPW_SIZE(x), MPW_DATA(x), MPW_SIZE(y), MPW_DATA(y))
		? JSVAL_TRUE : JSVAL_FALSE;
	if (x->ob_size > 0 && y->ob_size > 0)
	    *vp = mpltx(MPW_SIZE(x), MPW_DATA(x), MPW_SIZE(y), MPW_DATA(y))
		? JSVAL_TRUE : JSVAL_FALSE;

	if (!JSVAL_IS_MPW(cx, argv[0])) x = _free(x);
	if (!JSVAL_IS_MPW(cx, argv[1])) y = _free(y);
    } else
	ok = JS_FALSE;
    return ok;
}

/** Return ge(x, y). */
static JSBool
mpw_ge(JSContext *cx, uintN argc, jsval *vp)
{
    JSBool ok = JS_TRUE;
    if ((ok = mpw_lt(cx, argc, vp)))
	*vp = (*vp == JSVAL_TRUE ? JSVAL_FALSE : JSVAL_TRUE);
    return ok;
}

/** Return gt(x, y). */
static JSBool
mpw_gt(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmmpwClass, NULL);
    JSBool ok = JS_TRUE;

_METHOD_DEBUG_ENTRY(_debug);
    if (argc == 2) {
	mpwObject * x = mpw_j2mpw(cx, argv[0]);
	mpwObject * y = mpw_j2mpw(cx, argv[1]);

	if (x->ob_size < 0 && y->ob_size > 0)
	    *vp = JSVAL_FALSE;
	else
	if (x->ob_size > 0 && y->ob_size < 0)
	    *vp = JSVAL_TRUE;
	else
	if (x->ob_size < 0 && y->ob_size < 0)
	    *vp = mpltx(MPW_SIZE(x), MPW_DATA(x), MPW_SIZE(y), MPW_DATA(y))
		? JSVAL_TRUE : JSVAL_FALSE;
	if (x->ob_size > 0 && y->ob_size > 0)
	    *vp = mpgtx(MPW_SIZE(x), MPW_DATA(x), MPW_SIZE(y), MPW_DATA(y))
		? JSVAL_TRUE : JSVAL_FALSE;

	if (!JSVAL_IS_MPW(cx, argv[0])) x = _free(x);
	if (!JSVAL_IS_MPW(cx, argv[1])) y = _free(y);
    } else
	ok = JS_FALSE;
    return ok;
}

/** Return le(x, y). */
static JSBool
mpw_le(JSContext *cx, uintN argc, jsval *vp)
{
    JSBool ok = JS_TRUE;
    if ((ok = mpw_gt(cx, argc, vp)))
	*vp = (*vp == JSVAL_TRUE ? JSVAL_FALSE : JSVAL_TRUE);
    return ok;
}

/** Return min(x, y). */
static JSBool
mpw_min(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmmpwClass, NULL);
    JSBool ok = JS_TRUE;

_METHOD_DEBUG_ENTRY(_debug);
    /* XXX generalize to N values? */
    if (argc == 2) {
	mpwObject * x = mpw_j2mpw(cx, argv[0]);
	mpwObject * y = mpw_j2mpw(cx, argv[1]);

	if (x->ob_size < 0 && y->ob_size > 0)
	    *vp = argv[0];
	else
	if (x->ob_size > 0 && y->ob_size < 0)
	    *vp = argv[1];
	else
	if (x->ob_size < 0 && y->ob_size < 0)
	    *vp = mpgex(MPW_SIZE(x), MPW_DATA(x), MPW_SIZE(y), MPW_DATA(y))
		? argv[0] : argv[1];
	else
	if (x->ob_size > 0 && y->ob_size > 0)
	    *vp = mplex(MPW_SIZE(x), MPW_DATA(x), MPW_SIZE(y), MPW_DATA(y))
		? argv[0] : argv[1];

	if (!JSVAL_IS_MPW(cx, argv[0])) x = _free(x);
	if (!JSVAL_IS_MPW(cx, argv[1])) y = _free(y);
    } else
	ok = JS_FALSE;
    return ok;
}

/** Return max(x, y). */
static JSBool
mpw_max(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmmpwClass, NULL);
    JSBool ok = JS_TRUE;

_METHOD_DEBUG_ENTRY(_debug);
    /* XXX generalize to N values? */
    if (argc == 2) {
	mpwObject * x = mpw_j2mpw(cx, argv[0]);
	mpwObject * y = mpw_j2mpw(cx, argv[1]);

	if (x->ob_size < 0 && y->ob_size > 0)
	    *vp = argv[1];
	else
	if (x->ob_size > 0 && y->ob_size < 0)
	    *vp = argv[0];
	else
	if (x->ob_size < 0 && y->ob_size < 0)
	    *vp = mpgex(MPW_SIZE(x), MPW_DATA(x), MPW_SIZE(y), MPW_DATA(y))
		? argv[1] : argv[0];
	else
	if (x->ob_size > 0 && y->ob_size > 0)
	    *vp = mplex(MPW_SIZE(x), MPW_DATA(x), MPW_SIZE(y), MPW_DATA(y))
		? argv[1] : argv[0];

	if (!JSVAL_IS_MPW(cx, argv[0])) x = _free(x);
	if (!JSVAL_IS_MPW(cx, argv[1])) y = _free(y);
    } else
	ok = JS_FALSE;
    return ok;
}

#ifdef DYING
/** Return random number 1 < r < b-1. */
static JSBool
mpw_Rndm(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmmpwClass, NULL);
    JSBool ok;

_METHOD_DEBUG_ENTRY(_debug);
assert(argc == 2);
    ok = mpw_wrap(cx, vp,
		mpw_ops2('R', mpw_j2mpw(cx, argv[0]), mpw_j2mpw(cx, argv[1])));
    return ok;
}
#endif

static JSFunctionSpec rpmmpw_funcs[] = {
#ifdef	NOTYET
    JS_FS("toExponential", mpw_toExponential,	0,0),
    JS_FS("toFixed",	mpw_toFixed,		0,0),
    JS_FS("toLocaleString", mpw_toLocaleString,	0,0),
    JS_FS("toSource",	mpw_toSource,		0,0),
    JS_FS("toJSON",	mpw_toJSON,		0,0),
    JS_FS("valueOf",	mpw_valueOf,		0,0),
#endif
    JS_FS("toString",	mpw_toString,		0,0),
    JS_FS("isPrime",	mpw_isPrime,		0,0),
    JS_FS("randomK",	mpw_randomK,		0,0),
    JS_FS("eq",		mpw_eq,			0,0),
    JS_FS("ne",		mpw_ne,			0,0),
    JS_FS("lt",		mpw_lt,			0,0),
    JS_FS("ge",		mpw_ge,			0,0),
    JS_FS("gt",		mpw_gt,			0,0),
    JS_FS("le",		mpw_le,			0,0),
    JS_FS("min",	mpw_min,		0,0),
    JS_FS("max",	mpw_max,		0,0),
    JS_FS_END
};

/* --- Object properties */
enum rpmmpw_tinyid {
    _DEBUG	= -2,
};

static JSPropertySpec rpmmpw_props[] = {
    {"debug",	_DEBUG,		JSPROP_ENUMERATE,	NULL,	NULL},
    {NULL, 0, 0, NULL, NULL}
};

static JSBool
rpmmpw_getprop(JSContext *cx, JSObject *obj, jsid id, jsval *vp)
{
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmmpwClass, NULL);
    jsint tiny = JSVAL_TO_INT(id);

    /* XXX the class has ptr == NULL, instances have ptr != NULL. */
    if (ptr == NULL)
	return JS_TRUE;

    switch (tiny) {
    case _DEBUG:
	*vp = INT_TO_JSVAL(_debug);
	break;
    default:
	break;
    }

    return JS_TRUE;
}

static JSBool
rpmmpw_setprop(JSContext *cx, JSObject *obj, jsid id, JSBool strict, jsval *vp)
{
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmmpwClass, NULL);
    jsint tiny = JSVAL_TO_INT(id);

    /* XXX the class has ptr == NULL, instances have ptr != NULL. */
    if (ptr == NULL)
	return JS_TRUE;

    switch (tiny) {
    case _DEBUG:
	if (!JS_ValueToInt32(cx, *vp, &_debug))
	    break;
	break;
    default:
	break;
    }

    return JS_TRUE;
}

static JSBool
rpmmpw_resolve(JSContext *cx, JSObject *obj, jsid id, uintN flags,
	JSObject **objp)
{
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmmpwClass, NULL);

_RESOLVE_DEBUG_ENTRY(_debug < 0);

    if ((flags & JSRESOLVE_ASSIGNING)
     || (ptr == NULL)) { /* don't resolve to parent prototypes objects. */
	*objp = NULL;
	goto exit;
    }

    *objp = obj;	/* XXX always resolve in this object. */

exit:
    return JS_TRUE;
}

static JSBool
rpmmpw_enumerate(JSContext *cx, JSObject *obj, JSIterateOp op,
		  jsval *statep, jsid *idp)
{

_ENUMERATE_DEBUG_ENTRY(_debug < 0);

    switch (op) {
    case JSENUMERATE_INIT:
    case JSENUMERATE_INIT_ALL:
	*statep = JSVAL_VOID;
        if (idp)
            *idp = JSVAL_ZERO;
        break;
    case JSENUMERATE_NEXT:
	*statep = JSVAL_VOID;
	if (!JSID_IS_VOID(*idp))
            break;
        /*@fallthrough@*/
    case JSENUMERATE_DESTROY:
	*statep = JSVAL_NULL;
        break;
    }
    return JS_TRUE;
}

/* --- Object ctors/dtors */
static mpwObject *
rpmmpw_init(JSContext *cx, JSObject *obj, jsval v)
{
    mpwObject * z = NULL;

if (_debug)
fprintf(stderr, "==> %s(%p,%p,0x%x[%s])\n", __FUNCTION__, cx, obj, (unsigned)v, v2s(cx, v));

    if (JSVAL_IS_INT(v)) {
	z = mpw_FromLong((long)JSVAL_TO_INT(v));
    } else
    if (JSVAL_IS_DOUBLE(v)) {
	z = mpw_FromDouble((double)JSVAL_TO_DOUBLE(v));
    } else
    if (JSVAL_IS_STRING(v)) {
	const char * s = JS_EncodeString(cx, JSVAL_TO_STRING(v));
	z = mpw_FromHEX(s);
	s = _free(s);
    } else {
	z = mpw_New(1);
    }

    if (!JS_SetPrivate(cx, obj, (void *)z))
	z = _free(z);
if (_debug)
fprintf(stderr, "<== %s(%p,%p,0x%x[%s]) z %p\n", __FUNCTION__, cx, obj, (unsigned)v, v2s(cx, v), z);
    return z;
}

static void
rpmmpw_dtor(JSContext *cx, JSObject *obj)
{
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmmpwClass, NULL);
    mpwObject * z = ptr;

_DTOR_DEBUG_ENTRY(_debug);

    (void) _free(z);
}

static JSBool
rpmmpw_ctor(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_NewObjectForConstructor(cx, vp);
    JSBool ok = JS_FALSE;
    jsval v = JSVAL_NULL;

_CTOR_DEBUG_ENTRY(_debug);

    if (!(ok = JS_ConvertArguments(cx, argc, argv, "/v", &v)))
	goto exit;

    if (JS_IsConstructing(cx, vp)) {
	(void) rpmmpw_init(cx, obj, v);
    } else {
	if ((obj = JS_NewObject(cx, &rpmmpwClass, NULL, NULL)) == NULL)
	    goto exit;
	*vp = OBJECT_TO_JSVAL(obj);
    }
    ok = JS_TRUE;

exit:
    return ok;
}

static JSBool
rpmmpw_call(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    /* XXX obj is the global object so lookup "this" object. */
    JSObject * o = NULL;
    void * ptr = NULL;
    mpwObject * z = ptr;
    JSBool ok = JS_TRUE;
    jsval v = JSVAL_NULL;
    void ** stack = memset(alloca(argc*sizeof(*stack)), 0, (argc*sizeof(*stack)));
    char * freeme = memset(alloca(argc*sizeof(*freeme)), 0, (argc*sizeof(*freeme)));
    int ix = -1;
    uintN i;
    
    switch (argc) {
    case 0:
	/* Return a new Mpw object. */
	if ((o = rpmjs_NewMpwObject(cx, v)) != NULL)
	    ptr = z = JS_GetInstancePrivate(cx, o, &rpmmpwClass, NULL);
	*vp = OBJECT_TO_JSVAL(o);
	ok = JS_TRUE;
	break;
    default:
	*vp = JSVAL_NULL;
	for (i = 0; i < argc; i++) {
	    v = argv[i];

	    /* Check for operations. */
	    if (JSVAL_IS_STRING(v)) {
		const char * s = JS_EncodeString(cx, JSVAL_TO_STRING(v));
		size_t ns = strlen(s);
		int c = (ns == 1 ? *s : 0);

		if (ns == 0) {
		    s = _free(s);
		    continue;
		}

		if (!strcmp(s, "**")) c = (int)'P';
		if (!strcmp(s, "<<")) c = (int)'<';
		if (!strcmp(s, ">>")) c = (int)'>';
		if (!strcmp(s, "sqrt")) c = (int)'s';
		if (!strcmp(s, "abs")) c = (int)'!';
		if (!strcmp(s, "not")) c = (int)'~';
		if (!strcmp(s, "neg")) c = (int)'_';
		if (!strcmp(s, "gcd")) c = (int)'G';	/* gcd(x, y). */
		if (!strcmp(s, "invm")) c = (int)'I';	/* inverse of x (modulo m). */
		if (!strcmp(s, "sqrm")) c = (int)'S';	/* x*x (modulo m). */
#ifdef	DYING
		if (!strcmp(s, "rndm")) c = (int)'R';	/** random 1 < r < b-1. */
#endif

		switch (c) {
		case 's':	/* sqrt(x) */
		case '!':	/* abs(x) */
		case '_':	/* -x */
		case '~':	/* ~x */
assert(ix >= 0);
		    z = stack[ix];
		    stack[ix] = mpw_ops1(c, z);
		    if (freeme[ix]) z = _free(z);
		    freeme[ix] = 1;
		    break;
		case '%':	/* x % y */
		case '/':	/* x / y */
		    /* XXX divide-by-zero check. */
assert(ix > 0);
		    z = stack[ix];
assert(mpz(MPW_SIZE(z), MPW_DATA(z)) == 0);
		    /*@fallthrough@*/
		case '+':	/* x + y */
		case '-':	/* x - y */
		case '*':	/* x * y */
		case 'P':	/* x ** y */
		case 'G':	/* gcd(x, y) */
		case 'I':	/* inverse of x (modulo m) */
		case 'S':	/* x * x (modulo m) */
		case '<':	/* x << y */
		case '>':	/* x >> y */
		case '&':	/* x & y */
		case '^':	/* x ^ y */
		case '|':	/* x | y */
assert(--ix >= 0);
		    z = stack[ix];
		    stack[ix] = mpw_ops2(c, z, stack[ix+1]);
		    if (freeme[ix+1]) stack[ix+1] = _free(stack[ix+1]); freeme[ix+1] = 0;
		    if (freeme[ix]) z = _free(z);
		    freeme[ix] = 1;
		    break;
		default:
		case 0:
		    if (!strcmp(s, "addm")) c = ADDM;	/* x+x (modulo m) */
		    if (!strcmp(s, "subm")) c = SUBM;	/* x-x (modulo m) */
		    if (!strcmp(s, "mulm")) c = MULM;	/* x*x (modulo m) */
		    if (!strcmp(s, "powm")) c = POWM;	/* x**x (modulo m) */
		    if (!strcmp(s, "addp")) c = ADDP;	/* x+x (modulo p) */
		    if (!strcmp(s, "subp")) c = SUBP;	/* x-x (modulo p) */
		    if (!strcmp(s, "mulp")) c = MULP;	/* x*x (modulo p) */
		    if (!strcmp(s, "powp")) c = POWP;	/* x**x (modulo p) */
		    if (c) {
assert(--ix >= 0);
assert(--ix >= 0);
			z = stack[ix];

			switch (c) {
			case ADDM:
			case SUBM:
			case MULM:
			case POWM:
			    stack[ix] = mpw_ops3(c, z, stack[ix+1], stack[ix+2]);
			    break;
			case ADDP:
			case SUBP:
			case MULP:
			case POWP:
			    stack[ix] = mpw_pops3(c, z, stack[ix+1], stack[ix+2]);
			    break;
			}
		    
			if (freeme[ix+2]) stack[ix+2] = _free(stack[ix+2]); freeme[ix+2] = 0;
			if (freeme[ix+1]) stack[ix+1] = _free(stack[ix+1]); freeme[ix+1] = 0;
			if (freeme[ix]) z = _free(z);
			freeme[ix] = 1;
		    } else {
assert(++ix < (int)argc);
			stack[ix] = mpw_j2mpw(cx, v);
			freeme[ix] = 1;
		    }
		    
		    break;
		}
		s = _free(s);
	    } else {
assert(++ix < (int)argc);
		stack[ix] = mpw_j2mpw(cx, v);
		freeme[ix] = (!JSVAL_IS_MPW(cx, v) ? 1 : 0);
	    }
	}
	if (ix < 0) {
	    ok = JS_TRUE;
	    *vp = JSVAL_NULL;
	    o = NULL;
	    ptr = z = NULL;
	} else {
	    mpwObject * x = stack[ix];
	    ptr = z = mpw_FromMPW(MPW_SIZE(x), MPW_DATA(x), 0);
	    if (x->ob_size < 0)
		z->ob_size = -z->ob_size;
	    ok = mpw_wrap(cx, vp, z);
	    o = JSVAL_TO_OBJECT(*vp);
	}
	while (ix >= 0) {
	    if (freeme[ix]) stack[ix] = _free(stack[ix]); freeme[ix] = 0;
	    ix--;
	}
	break;
    }

if (_debug)
fprintf(stderr, "<== %s(%p,%p[%u],%p) o %p ptr %p\t", __FUNCTION__, cx, argv, (unsigned)argc, vp, o, ptr), mpfprintln(stderr, MPW_SIZE(z), MPW_DATA(z));

    return ok;
}

/* --- Class initialization */
JSClass rpmmpwClass = {
    /* XXX class should be "BeeCrypt" eventually, avoid name conflicts for now */
    "Mpw", JSCLASS_NEW_RESOLVE | JSCLASS_NEW_ENUMERATE | JSCLASS_HAS_PRIVATE,
    rpmmpw_addprop,   rpmmpw_delprop, rpmmpw_getprop, rpmmpw_setprop,
    (JSEnumerateOp)rpmmpw_enumerate, (JSResolveOp)rpmmpw_resolve,
    rpmmpw_convert,	rpmmpw_dtor,

    rpmmpw_getobjectops,	rpmmpw_checkaccess,
    rpmmpw_call,		rpmmpw_construct,
    rpmmpw_xdrobject,	rpmmpw_hasinstance,
    rpmmpw_mark,		rpmmpw_reserveslots,
};

JSObject *
rpmjs_InitMpwClass(JSContext *cx, JSObject* obj)
{
    JSObject *proto;

if (_debug)
fprintf(stderr, "==> %s(%p,%p)\n", __FUNCTION__, cx, obj);

    proto = JS_InitClass(cx, obj, NULL, &rpmmpwClass, rpmmpw_ctor, 1,
		rpmmpw_props, rpmmpw_funcs, NULL, NULL);
assert(proto != NULL);
    return proto;
}

JSObject *
rpmjs_NewMpwObject(JSContext *cx, jsval v)
{
    JSObject *obj;
    mpwObject * z;

    if ((obj = JS_NewObject(cx, &rpmmpwClass, NULL, NULL)) == NULL) {
	/* XXX error msg */
	return NULL;
    }
    if ((z = rpmmpw_init(cx, obj, v)) == NULL) {
	/* XXX error msg */
	return NULL;
    }
    return obj;
}

GPSEE_MODULE_WRAP(rpmmpw, Mpw, JS_TRUE)
