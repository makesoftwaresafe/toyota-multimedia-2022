/*
 * Class: Human Interface Devices (HID)
 */

/*
 * HID class define
*/

#define    HID_ITEM_TYPE_MAIN               0
#define    HID_ITEM_TYPE_GLOBAL             1
#define    HID_ITEM_TYPE_LOCAL              2

/* Main items */
#define    HID_MAIN_ITEM_INPUT              8
#define    HID_MAIN_ITEM_OUTPUT             9
#define    HID_MAIN_ITEM_COLLECTION         10
#define    HID_MAIN_ITEM_FEATURE            11
#define    HID_MAIN_ITEM_ENDCOLLECTION      12

/* Global items */
#define    HID_GLOBAL_ITEM_USAGE_PAGE       0
#define    HID_GLOBAL_ITEM_LOGICAL_MIN      1
#define    HID_GLOBAL_ITEM_LOGICAL_MAX      2
#define    HID_GLOBAL_ITEM_PHYSICAL_MIN     3
#define    HID_GLOBAL_ITEM_PHYSICAL_MAX     4
#define    HID_GLOBAL_ITEM_UNIT_EXPONENT    5
#define    HID_GLOBAL_ITEM_UNIT             6
#define    HID_GLOBAL_ITEM_REPORT_SIZE      7
#define    HID_GLOBAL_ITEM_REPORT_ID        8
#define    HID_GLOBAL_ITEM_REPORT_COUNT     9
#define    HID_GLOBAL_ITEM_PUSH             10
#define    HID_GLOBAL_ITEM_POP              11

/* Item Size */
#define    HID_ITEM_SIZE_0BYTE      0
#define    HID_ITEM_SIZE_1BYTE      1
#define    HID_ITEM_SIZE_2BYTE      2
#define    HID_ITEM_SIZE_4BYTE      3
#define    HID_ITEM_SIZE_GET(x)     ((1 << (x)) >> 1)
#define    HID_ITEM_SIZE_MASK       0x3
#define    HID_ITEM_SIZE_SHIFT      0
#define    HID_ITEM_SIZE(cr)                    \
    (((cr) >> HID_ITEM_SIZE_SHIFT) & HID_ITEM_SIZE_MASK)

/* Item Type */
#define    HID_ITEM_TYPE_MASK       0x3
#define    HID_ITEM_TYPE_SHIFT      2
#define    HID_ITEM_TYPE(cr)                    \
    (((cr) >> HID_ITEM_TYPE_SHIFT) & HID_ITEM_TYPE_MASK)

/* Item Tag */
#define    HID_ITEM_TAG_LONGITEM    0xf
#define    HID_ITEM_TAG_MASK        0xf
#define    HID_ITEM_TAG_SHIFT       4
#define    HID_ITEM_TAG(cr)                    \
    (((cr) >> HID_ITEM_TAG_SHIFT) & HID_ITEM_TAG_MASK)


#define    BIT(x, k, n)    (((x) >> (k)) & ~(~0 << (n)))

#define REPORT_LIST_MAX     256

typedef int32_t     HID_desc_t;             /* return type */
#define HID_DESC_OK         0
#define HID_DESC_ERROR      -1

#define HID_REPORT_IN       0
#define HID_REPORT_OUT      1

#define CTRL_TRANS_TOUT     1000    /* USB control transfer */

typedef struct iap2_hid_desc_rep_t {
    uint8_t     rid;                        /* report id            */
    uint8_t     dir;                        /* 0:input / 1:output   */
    uint32_t    len;                        /* report length        */
} IAP2_HID_DESC_REP, *P_IAP2_HID_DESC_REP;

typedef struct iap2_hid_desc_inf_t {
    uint8_t         *buffer;                /* buffer of descriptor */
    size_t          size;                   /* size of descriptor   */
    IAP2_HID_DESC_REP    rep[REPORT_LIST_MAX];   /* list for report      */
    uint32_t        act_size;               /* actual size          */
    uint32_t        act_sz_in;              /* actual size(in)      */
    uint32_t        act_sz_out;             /* actual size(out)     */
} IAP2_HID_DESC_INF, *P_IAP2_HID_DESC_INF;

/*
 * CPU endianness and CPU alignment requirements:
 */

#define UGETW(w)                        \
  ((w)[0] |                             \
  (((uint16_t)((w)[1])) << 8))

#define UGETDW(w)                       \
  ((w)[0] |                             \
  (((uint16_t)((w)[1])) << 8) |         \
  (((uint32_t)((w)[2])) << 16) |        \
  (((uint32_t)((w)[3])) << 24))

HID_desc_t iap2_hid_report(P_IAP2_HID_DESC_INF pinf);
HID_desc_t iap2_hid_set_report(int bTag, int bType, size_t bufsize, uint8_t *buffer, P_IAP2_HID_DESC_INF pinf);
HID_desc_t iap2_item_value(size_t bufsize, uint8_t *buffer, uint32_t *val);

