#ifndef H_HEADER_INTERNAL
#define H_HEADER_INTERNAL

/** \ingroup header
 * \file rpmdb/header_internal.h
 */

#include <rpmtag.h>
#include <yarn.h>

#if !defined(__LCLINT__)
#include <netinet/in.h>
#endif  /* __LCLINT__ */

/**
 * Sanity check on no. of tags.
 * This check imposes a limit of 16M tags.
 */
#define hdrchkTags(_ntags)      ((_ntags) & 0xff000000)

/**
 * Sanity check on type values.
 */
#define hdrchkType(_type) ((_type) < 1 || (_type) > RPM_MAX_TYPE)

/**
 * Sanity check on data size and/or offset and/or count.
 * This check imposes a limit of 1 Gb.
 */
#define hdrchkData(_nbytes) ((_nbytes) & 0xc0000000)

/**
 * Sanity check on data alignment for data type.
 */
/*@unchecked@*/ /*@observer@*/
extern int rpm_typeAlign[16];
#define hdrchkAlign(_type, _off)	((_off) & (rpm_typeAlign[_type]-1))

/**
 * Sanity check on range of data offset.
 */
#define hdrchkRange(_dl, _off)		((_off) < 0 || (_off) > (_dl))

#define	INDEX_MALLOC_SIZE	8

/*
 * Teach header.c about legacy tags.
 */
#define	HEADER_OLDFILENAMES	1027
#define	HEADER_BASENAMES	1117

/** \ingroup header
 * Description of tag data.
 */
typedef /*@abstract@*/ struct entryInfo_s * entryInfo;
struct entryInfo_s {
    rpmTag tag;		/*!< Tag identifier. */
    rpmTagType type;		/*!< Tag data type. */
    rpmint32_t offset;		/*!< Offset into data segment (ondisk only). */
    rpmTagCount count;		/*!< Number of tag elements. */
};

#define	REGION_TAG_TYPE		RPM_BIN_TYPE
#define	REGION_TAG_COUNT	sizeof(struct entryInfo_s)

#define	ENTRY_IS_REGION(_e) \
	(((_e)->info.tag >= HEADER_IMAGE) && ((_e)->info.tag < HEADER_REGIONS))
#define	ENTRY_IN_REGION(_e)	((_e)->info.offset < 0)

/** \ingroup header
 * A single tag from a Header.
 */
typedef /*@abstract@*/ struct indexEntry_s * indexEntry;
struct indexEntry_s {
    struct entryInfo_s info;	/*!< Description of tag data. */
/*@owned@*/
    void * data; 		/*!< Location of tag data. */
    size_t length;		/*!< No. bytes of data. */
    size_t rdlen;		/*!< No. bytes of data in region. */
};

/** \ingroup header
 * The Header data structure.
 */
struct headerToken_s {
    struct rpmioItem_s _item;	/*!< usage mutex and pool identifier. */
    unsigned char magic[8];	/*!< Header magic. */
/*@only@*/ /*@null@*/
    void * blob;		/*!< Header region blob. */
    size_t bloblen;		/*!< Header region blob length (in bytes). */
/*@only@*/ /*@null@*/
    const char * origin;	/*!< Header origin (e.g. path or URL). */
/*@only@*/ /*@null@*/
    const char * baseurl;	/*!< Header base URL (e.g. path or URL). */
/*@only@*/ /*@null@*/
    const char * digest;	/*!< Header digest (from origin *.rpm file) */
/*@only@*/ /*@null@*/
    const char * parent;	/*!< Parent package (e.g. parent NVRA) */
/*@null@*/
    void * rpmdb;		/*!< rpmdb pointer (or NULL). */
    struct stat sb;		/*!< Header stat(2) (from origin *.rpm file) */
    rpmuint32_t instance;	/*!< Header instance (if from rpmdb). */
    rpmuint32_t startoff;	/*!< Header starting byte offset in package. */
    rpmuint32_t endoff;	/*!< Header ending byte offset in package. */
    struct rpmop_s h_loadops;
    struct rpmop_s h_getops;
/*@owned@*/
    indexEntry index;	/*!< Array of tags. */
    size_t indexUsed;	/*!< Current size of tag array. */
    size_t indexAlloced;	/*!< Allocated size of tag array. */
    rpmuint32_t flags;
#define HEADERFLAG_SORTED	(1 << 0) /*!< Are header entries sorted? */
#define HEADERFLAG_ALLOCATED	(1 << 1) /*!< Is 1st header region allocated? */
#define HEADERFLAG_LEGACY	(1 << 2) /*!< Header came from legacy source? */
#define HEADERFLAG_DEBUG	(1 << 3) /*!< Debug this header? */
#define HEADERFLAG_SIGNATURE	(1 << 4) /*!< Signature header? */
#define HEADERFLAG_MAPPED	(1 << 5) /*!< Is 1st header region mmap'd? */
#define HEADERFLAG_RDONLY	(1 << 6) /*!< Is 1st header region rdonly? */
#if defined(__LCLINT__)
/*@refs@*/
    int nrefs;			/*!< (unused) keep splint happy */
#endif
};

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Perform simple sanity and range checks on header tag(s).
 * @param il		no. of tags in header
 * @param dl		no. of bytes in header data.
 * @param pev		1st element in tag array, big-endian
 * @param iv		failing (or last) tag element, host-endian
 * @param negate	negative offset expected?
 * @return		-1 on success, otherwise failing tag element index
 */
int headerVerifyInfo(rpmuint32_t il, rpmuint32_t dl, const void * pev, void * iv, int negate)
	/*@modifies *iv @*/;

#ifdef __cplusplus
}   
#endif

#endif  /* H_HEADER_INTERNAL */
