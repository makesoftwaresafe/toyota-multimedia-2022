/*!\file crc.h
 * \brief CRC32, CRC64 and ADLER32 checksums.
 */

#include <sys/types.h>
#include <rpmiotypes.h>

#ifndef  _CRC_H
#define  _CRC_H

/**
 */
typedef struct {
    rpmuint32_t crc;
    rpmuint32_t (*update)  (rpmuint32_t crc, const rpmuint8_t * data, size_t size);
    rpmuint32_t (*combine) (rpmuint32_t crc1, rpmuint32_t crc2, size_t len2);
} sum32Param;

/**
 */
typedef struct {
    rpmuint64_t crc;
    rpmuint64_t (*update)  (rpmuint64_t crc, const rpmuint8_t * data, size_t size);
    rpmuint64_t (*combine) (rpmuint64_t crc1, rpmuint64_t crc2, size_t len2);
} sum64Param;

#ifdef __cplusplus
extern "C" {
#endif

/**
 */
rpmuint32_t __crc32(rpmuint32_t crc, const rpmuint8_t * data, size_t size)
        /*@*/;

/**
 */
rpmuint32_t __crc32_combine(rpmuint32_t crc1, rpmuint32_t crc2, size_t len2)
        /*@*/;

/**
 */
rpmuint64_t __crc64(rpmuint64_t crc, const rpmuint8_t * data, size_t size)
        /*@*/;

/**
 */
rpmuint64_t __crc64_combine(rpmuint64_t crc1, rpmuint64_t crc2, size_t len2)
        /*@*/;

/**
 */
rpmuint32_t __adler32(rpmuint32_t adler, const rpmuint8_t * buf, rpmuint32_t len)
        /*@*/;

/**
 */
rpmuint32_t __adler32_combine(rpmuint32_t adler1, rpmuint32_t adler2, size_t len2)
        /*@*/;

/**
 */
int sum32Reset(sum32Param * mp)
	/*@modifies *mp @*/;

/**
 */
int sum32Update(sum32Param * mp, const rpmuint8_t * data, size_t size)
	/*@modifies *mp @*/;

/**
 */
int sum32Digest(sum32Param * mp, rpmuint8_t * data)
        /*@modifies *mp, data @*/;

/**
 */
int sum64Reset(sum64Param * mp)
	/*@modifies *mp @*/;

/**
 */
int sum64Update(sum64Param * mp, const rpmuint8_t * data, size_t size)
	/*@modifies *mp @*/;

/**
 */
int sum64Digest(sum64Param * mp, rpmuint8_t * data)
        /*@modifies *mp, data @*/;

#ifdef __cplusplus
}
#endif

#endif	/* _CRC_H */
