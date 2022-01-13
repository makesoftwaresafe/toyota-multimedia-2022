#ifndef	H_RPMPGP
#define	H_RPMPGP

/** \ingroup rpmpgp
 * \file rpmio/rpmpgp.h
 *
 * OpenPGP constants and structures from RFC-2440.
 *
 * Text from RFC-2440 in comments is
 *	Copyright (C) The Internet Society (1998).  All Rights Reserved.
 */

#include <string.h>
#include <popt.h>
#include <rpmiotypes.h>
#include <yarn.h>

#if defined(_RPMPGP_INTERNAL)
#include <rpmsw.h>

/*@unchecked@*/
extern int _pgp_error_count;

/** \ingroup rpmpgp
 * Values parsed from OpenPGP signature/pubkey packet(s).
 */
struct pgpDigParams_s {
/*@only@*/ /*@null@*/
    const char * userid;
/*@dependent@*/ /*@null@*/
    const rpmuint8_t * hash;
    rpmuint8_t tag;

    rpmuint8_t version;		/*!< version number. */
    rpmuint8_t time[4];		/*!< time created. */
    rpmuint8_t pubkey_algo;	/*!< public key algorithm. */

    rpmuint8_t hash_algo;
    rpmuint8_t sigtype;
    size_t hashlen;
    rpmuint8_t signhash16[2];
    rpmuint8_t signid[8];
    rpmuint8_t expire[4];	/*!< signature expired after seconds). */
    rpmuint8_t keyexpire[4];	/*!< key expired after seconds). */

    rpmuint8_t saved;
#define	PGPDIG_SAVED_TIME	(1 << 0)
#define	PGPDIG_SAVED_ID		(1 << 1)

};

/** \ingroup rpmpgp
 * Container for values parsed from an OpenPGP signature and public key.
 */
struct pgpDig_s {
    struct rpmioItem_s _item;	/*!< usage mutex and pool identifier. */
    struct pgpDigParams_s signature;
    struct pgpDigParams_s pubkey;

/*@observer@*/ /*@null@*/
    const char * build_sign;
/*@observer@*/ /*@null@*/
    const char * pubkey_algoN;
/*@observer@*/ /*@null@*/
    const char * hash_algoN;

    rpmuint32_t sigtag;		/*!< Package signature tag. */
    rpmuint32_t sigtype;	/*!< Package signature data type. */
/*@relnull@*/
    const void * sig;		/*!< Package signature. */
    size_t siglen;		/*!< Package signature length. */
    const void * pub;		/*!< Package pubkey. */
    size_t publen;		/*!< Package pubkey length. */

    pgpVSFlags vsflags;		/*!< Digest/signature operation disablers. */
    struct rpmop_s dops;	/*!< Digest operation statistics. */
    struct rpmop_s sops;	/*!< Signature operation statistics. */

    int (*findPubkey) (void * _ts, /*@null@*/ void * _dig)
	/*@modifies *_ts, *_dig @*/;/*!< Find pubkey, i.e. rpmtsFindPubkey(). */
/*@null@*/
    void * _ts;			/*!< Find pubkey argument, i.e. rpmts. */

    rpmuint8_t ** ppkts;	/*!< Parsed OpenPGP 2440/4880 packets. */
    int npkts;			/*!< No. of parsed packets. */
    size_t nbytes;		/*!< No. bytes of plain text. */

/*@only@*/ /*@null@*/
    DIGEST_CTX hsha;		/*!< header hash context. */
/*@only@*/ /*@null@*/
    DIGEST_CTX hdsa;		/*!< (dsa) header sha1 hash context. */
/*@only@*/ /*@null@*/
    void * sha1;		/*!< (dsa) signature hash. */
    size_t sha1len;		/*!< (dsa) signature hash length. */

/*@only@*/ /*@null@*/
    DIGEST_CTX hecdsa;		/*!< (ecdsa) header hash context. */

/*@only@*/ /*@null@*/
    DIGEST_CTX md5ctx;		/*!< (md5) hash context. */
/*@only@*/ /*@null@*/
    DIGEST_CTX hrsa;		/*!< (rsa) header hash context. */
/*@only@*/ /*@null@*/
    void * md5;			/*!< (rsa) signature hash. */
    size_t md5len;		/*!< (rsa) signature hash length. */

/*@owned@*/ /*@relnull@*/
    void * impl;		/*!< Implementation data */

#if defined(__LCLINT__)
/*@refs@*/
    int nrefs;			/*!< (unused) keep splint happy */
#endif
};
#endif	/* _RPMPGP_INTERNAL */

/**
 */
typedef const struct pgpValTbl_s {
    int val;
/*@observer@*/
    const char * str;
} * pgpValTbl;
 
/** \ingroup rpmpgp
 * 4.3. Packet Tags
 * 
 * The packet tag denotes what type of packet the body holds. Note that
 * old format headers can only have tags less than 16, whereas new
 * format headers can have tags as great as 63.
 */
typedef enum pgpTag_e {
    PGPTAG_RESERVED		=  0, /*!< Reserved/Invalid */
    PGPTAG_PUBLIC_SESSION_KEY	=  1, /*!< Public-Key Encrypted Session Key */
    PGPTAG_SIGNATURE		=  2, /*!< Signature */
    PGPTAG_SYMMETRIC_SESSION_KEY=  3, /*!< Symmetric-Key Encrypted Session Key*/
    PGPTAG_ONEPASS_SIGNATURE	=  4, /*!< One-Pass Signature */
    PGPTAG_SECRET_KEY		=  5, /*!< Secret Key */
    PGPTAG_PUBLIC_KEY		=  6, /*!< Public Key */
    PGPTAG_SECRET_SUBKEY	=  7, /*!< Secret Subkey */
    PGPTAG_COMPRESSED_DATA	=  8, /*!< Compressed Data */
    PGPTAG_SYMMETRIC_DATA	=  9, /*!< Symmetrically Encrypted Data */
    PGPTAG_MARKER		= 10, /*!< Marker */
    PGPTAG_LITERAL_DATA		= 11, /*!< Literal Data */
    PGPTAG_TRUST		= 12, /*!< Trust */
    PGPTAG_USER_ID		= 13, /*!< User ID */
    PGPTAG_PUBLIC_SUBKEY	= 14, /*!< Public Subkey */
    PGPTAG_COMMENT_OLD		= 16, /*!< Comment (from OpenPGP draft) */
    PGPTAG_PHOTOID		= 17, /*!< PGP's photo ID */
    PGPTAG_ENCRYPTED_MDC	= 18, /*!< Integrity protected encrypted data */
    PGPTAG_MDC			= 19, /*!< Manipulaion detection code packet */
    PGPTAG_PRIVATE_60		= 60, /*!< Private or Experimental Values */
    PGPTAG_COMMENT		= 61, /*!< Comment */
    PGPTAG_PRIVATE_62		= 62, /*!< Private or Experimental Values */
    PGPTAG_CONTROL		= 63  /*!< Control (GPG) */
} pgpTag;

/** \ingroup rpmpgp
 */
/*@observer@*/ /*@unchecked@*/ /*@unused@*/
extern struct pgpValTbl_s pgpTagTbl[];

/** \ingroup rpmpgp
 * 5.1. Public-Key Encrypted Session Key Packets (Tag 1)
 *
 * A Public-Key Encrypted Session Key packet holds the session key used
 * to encrypt a message. Zero or more Encrypted Session Key packets
 * (either Public-Key or Symmetric-Key) may precede a Symmetrically
 * Encrypted Data Packet, which holds an encrypted message.  The message
 * is encrypted with the session key, and the session key is itself
 * encrypted and stored in the Encrypted Session Key packet(s).  The
 * Symmetrically Encrypted Data Packet is preceded by one Public-Key
 * Encrypted Session Key packet for each OpenPGP key to which the
 * message is encrypted.  The recipient of the message finds a session
 * key that is encrypted to their public key, decrypts the session key,
 * and then uses the session key to decrypt the message.
 *
 * The body of this packet consists of:
 *   - A one-octet number giving the version number of the packet type.
 *     The currently defined value for packet version is 3. An
 *     implementation should accept, but not generate a version of 2,
 *     which is equivalent to V3 in all other respects.
 *   - An eight-octet number that gives the key ID of the public key
 *     that the session key is encrypted to.
 *   - A one-octet number giving the public key algorithm used.
 *   - A string of octets that is the encrypted session key. This string
 *     takes up the remainder of the packet, and its contents are
 *     dependent on the public key algorithm used.
 *
 * Algorithm Specific Fields for RSA encryption
 *   - multiprecision integer (MPI) of RSA encrypted value m**e mod n.
 *
 * Algorithm Specific Fields for Elgamal encryption:
 *   - MPI of Elgamal (Diffie-Hellman) value g**k mod p.
 *   - MPI of Elgamal (Diffie-Hellman) value m * y**k mod p.
 */
typedef struct pgpPktPubkey_s {
    rpmuint8_t version;		/*!< version number (generate 3, accept 2). */
    rpmuint8_t keyid[8];	/*!< key ID of the public key for session. */
    rpmuint8_t algo;		/*!< public key algorithm used. */
} pgpPktPubkey;

/** \ingroup rpmpgp
 * 5.2.1. Signature Types
 * 
 * There are a number of possible meanings for a signature, which are
 * specified in a signature type octet in any given signature.
 */
/*@-typeuse@*/
typedef enum pgpSigType_e {
    PGPSIGTYPE_BINARY		 = 0x00, /*!< Binary document */
    PGPSIGTYPE_TEXT		 = 0x01, /*!< Canonical text document */
    PGPSIGTYPE_STANDALONE	 = 0x02, /*!< Standalone */
    PGPSIGTYPE_GENERIC_CERT	 = 0x10,
		/*!< Generic certification of a User ID & Public Key */
    PGPSIGTYPE_PERSONA_CERT	 = 0x11,
		/*!< Persona certification of a User ID & Public Key */
    PGPSIGTYPE_CASUAL_CERT	 = 0x12,
		/*!< Casual certification of a User ID & Public Key */
    PGPSIGTYPE_POSITIVE_CERT	 = 0x13,
		/*!< Positive certification of a User ID & Public Key */
    PGPSIGTYPE_SUBKEY_BINDING	 = 0x18, /*!< Subkey Binding */
    PGPSIGTYPE_KEY_BINDING	 = 0x19, /*!< Primary key Binding */
    PGPSIGTYPE_SIGNED_KEY	 = 0x1F, /*!< Signature directly on a key */
    PGPSIGTYPE_KEY_REVOKE	 = 0x20, /*!< Key revocation */
    PGPSIGTYPE_SUBKEY_REVOKE	 = 0x28, /*!< Subkey revocation */
    PGPSIGTYPE_CERT_REVOKE	 = 0x30, /*!< Certification revocation */
    PGPSIGTYPE_TIMESTAMP	 = 0x40, /*!< Timestamp */
    PGPSIGTYPE_CONFIRM		 = 0x50  /*!< Third-Party confirmation */
} pgpSigType;
/*@=typeuse@*/

/** \ingroup rpmpgp
 */
/*@observer@*/ /*@unchecked@*/ /*@unused@*/
extern struct pgpValTbl_s pgpSigTypeTbl[];

/** \ingroup rpmpgp
 * 9.1. Public Key Algorithms
 *
\verbatim
       ID           Algorithm
       --           ---------
       1          - RSA (Encrypt or Sign)
       2          - RSA Encrypt-Only
       3          - RSA Sign-Only
       16         - Elgamal (Encrypt-Only), see [ELGAMAL]
       17         - DSA (Digital Signature Standard)
       18         - Reserved for Elliptic Curve
       19         - Reserved for ECDSA
       20         - Elgamal (Encrypt or Sign)
       21         - Reserved for Diffie-Hellman (X9.42,
                    as defined for IETF-S/MIME)
       100 to 110 - Private/Experimental algorithm.
\endverbatim
 *
 * Implementations MUST implement DSA for signatures, and Elgamal for
 * encryption. Implementations SHOULD implement RSA keys.
 * Implementations MAY implement any other algorithm.
 */
/*@-typeuse@*/
typedef enum pgpPubkeyAlgo_e {
    PGPPUBKEYALGO_UNKNOWN	=  0,
    PGPPUBKEYALGO_RSA		=  1,	/*!< RSA */
    PGPPUBKEYALGO_RSA_E		=  2,	/*!< RSA(Encrypt-Only) */
    PGPPUBKEYALGO_RSA_S		=  3,	/*!< RSA(Sign-Only) */
    PGPPUBKEYALGO_ELGAMAL_E	= 16,	/*!< Elgamal(Encrypt-Only) */
    PGPPUBKEYALGO_DSA		= 17,	/*!< DSA */
    PGPPUBKEYALGO_ECDH		= 18,	/*!< ECDH */
    PGPPUBKEYALGO_ECDSA		= 19,	/*!< ECDSA */
    PGPPUBKEYALGO_ELGAMAL	= 20,	/*!< Elgamal */
    PGPPUBKEYALGO_EDDSA		= 105	/*!< EDDSA */
} pgpPubkeyAlgo;
/*@=typeuse@*/

/** \ingroup rpmpgp
 */
/*@observer@*/ /*@unchecked@*/ /*@unused@*/
extern struct pgpValTbl_s pgpPubkeyTbl[];

/** \ingroup rpmpgp
 * Symmetric key (string, value) pairs.
 */
/*@observer@*/ /*@unchecked@*/ /*@unused@*/
extern struct pgpValTbl_s pgpSymkeyTbl[];

/** \ingroup rpmpgp
 * 9.3. Compression Algorithms
 *
\verbatim
       ID           Algorithm
       --           ---------
       0          - Uncompressed
       1          - ZIP (RFC 1951)
       2          - ZLIB (RFC 1950)
       100 to 110 - Private/Experimental algorithm.
\endverbatim
 *
 * Implementations MUST implement uncompressed data. Implementations
 * SHOULD implement ZIP. Implementations MAY implement ZLIB.
 */
/*@-typeuse@*/
typedef enum pgpCompressAlgo_e {
    PGPCOMPRESSALGO_NONE	=  0,	/*!< Uncompressed */
    PGPCOMPRESSALGO_ZIP		=  1,	/*!< ZIP */
    PGPCOMPRESSALGO_ZLIB	=  2,	/*!< ZLIB */
    PGPCOMPRESSALGO_BZIP2	=  3	/*!< BZIP2 */
} pgpCompressAlgo;
/*@=typeuse@*/

/** \ingroup rpmpgp
 * Compression (string, value) pairs.
 */
/*@observer@*/ /*@unchecked@*/ /*@unused@*/
extern struct pgpValTbl_s pgpCompressionTbl[];

/** \ingroup rpmpgp
 * Hash (string, value) pairs.
 */
/*@observer@*/ /*@unchecked@*/ /*@unused@*/
extern struct pgpValTbl_s pgpHashTbl[];

/** \ingroup rpmpgp
 * 5.2.2. Version 3 Signature Packet Format
 * 
 * The body of a version 3 Signature Packet contains:
 *   - One-octet version number (3).
 *   - One-octet length of following hashed material.  MUST be 5.
 *       - One-octet signature type.
 *       - Four-octet creation time.
 *   - Eight-octet key ID of signer.
 *   - One-octet public key algorithm.
 *   - One-octet hash algorithm.
 *   - Two-octet field holding left 16 bits of signed hash value.
 *   - One or more multi-precision integers comprising the signature.
 *
 * Algorithm Specific Fields for RSA signatures:
 *   - multiprecision integer (MPI) of RSA signature value m**d.
 *
 * Algorithm Specific Fields for DSA signatures:
 *   - MPI of DSA value r.
 *   - MPI of DSA value s.
 */
typedef struct pgpPktSigV3_s {
    rpmuint8_t version;	/*!< version number (3). */
    rpmuint8_t hashlen;	/*!< length of following hashed material. MUST be 5. */
    rpmuint8_t sigtype;	/*!< signature type. */
    rpmuint8_t time[4];	/*!< 4 byte creation time. */
    rpmuint8_t signid[8];	/*!< key ID of signer. */
    rpmuint8_t pubkey_algo;	/*!< public key algorithm. */
    rpmuint8_t hash_algo;	/*!< hash algorithm. */
    rpmuint8_t signhash16[2];	/*!< left 16 bits of signed hash value. */
} * pgpPktSigV3;

/** \ingroup rpmpgp
 * 5.2.3. Version 4 Signature Packet Format
 * 
 * The body of a version 4 Signature Packet contains:
 *   - One-octet version number (4).
 *   - One-octet signature type.
 *   - One-octet public key algorithm.
 *   - One-octet hash algorithm.
 *   - Two-octet scalar octet count for following hashed subpacket
 *     data. Note that this is the length in octets of all of the hashed
 *     subpackets; a pointer incremented by this number will skip over
 *     the hashed subpackets.
 *   - Hashed subpacket data. (zero or more subpackets)
 *   - Two-octet scalar octet count for following unhashed subpacket
 *     data. Note that this is the length in octets of all of the
 *     unhashed subpackets; a pointer incremented by this number will
 *     skip over the unhashed subpackets.
 *   - Unhashed subpacket data. (zero or more subpackets)
 *   - Two-octet field holding left 16 bits of signed hash value.
 *   - One or more multi-precision integers comprising the signature.
 */
typedef struct pgpPktSigV4_s {
    rpmuint8_t version;		/*!< version number (4). */
    rpmuint8_t sigtype;		/*!< signature type. */
    rpmuint8_t pubkey_algo;	/*!< public key algorithm. */
    rpmuint8_t hash_algo;	/*!< hash algorithm. */
    rpmuint8_t hashlen[2];	/*!< length of following hashed material. */
} * pgpPktSigV4;

/** \ingroup rpmpgp
 * 5.2.3.1. Signature Subpacket Specification
 * 
 * The subpacket fields consist of zero or more signature subpackets.
 * Each set of subpackets is preceded by a two-octet scalar count of the
 * length of the set of subpackets.
 *
 * Each subpacket consists of a subpacket header and a body.  The header
 * consists of:
 *   - the subpacket length (1,  2, or 5 octets)
 *   - the subpacket type (1 octet)
 * and is followed by the subpacket specific data.
 *
 * The length includes the type octet but not this length. Its format is
 * similar to the "new" format packet header lengths, but cannot have
 * partial body lengths. That is:
\verbatim
       if the 1st octet <  192, then
           lengthOfLength = 1
           subpacketLen = 1st_octet

       if the 1st octet >= 192 and < 255, then
           lengthOfLength = 2
           subpacketLen = ((1st_octet - 192) << 8) + (2nd_octet) + 192

       if the 1st octet = 255, then
           lengthOfLength = 5
           subpacket length = [four-octet scalar starting at 2nd_octet]
\endverbatim
 *
 * The value of the subpacket type octet may be:
 *
\verbatim
       0 = reserved
       1 = reserved
       2 = signature creation time
       3 = signature expiration time
       4 = exportable certification
       5 = trust signature
       6 = regular expression
       7 = revocable
       8 = reserved
       9 = key expiration time
       10 = placeholder for backward compatibility
       11 = preferred symmetric algorithms
       12 = revocation key
       13 = reserved
       14 = reserved
       15 = reserved
       16 = issuer key ID
       17 = reserved
       18 = reserved
       19 = reserved
       20 = notation data
       21 = preferred hash algorithms
       22 = preferred compression algorithms
       23 = key server preferences
       24 = preferred key server
       25 = primary user id
       26 = policy URL
       27 = key flags
       28 = signer's user id
       29 = reason for revocation
       30 = features
       31 = signature target
       32 = embedded signature
       100 to 110 = internal or user-defined
\endverbatim
 *
 * An implementation SHOULD ignore any subpacket of a type that it does
 * not recognize.
 *
 * Bit 7 of the subpacket type is the "critical" bit.  If set, it
 * denotes that the subpacket is one that is critical for the evaluator
 * of the signature to recognize.  If a subpacket is encountered that is
 * marked critical but is unknown to the evaluating software, the
 * evaluator SHOULD consider the signature to be in error.
 */
/*@-typeuse@*/
typedef enum pgpSubType_e {
    PGPSUBTYPE_NONE		=   0, /*!< none */
    PGPSUBTYPE_SIG_CREATE_TIME	=   2, /*!< signature creation time */
    PGPSUBTYPE_SIG_EXPIRE_TIME	=   3, /*!< signature expiration time */
    PGPSUBTYPE_EXPORTABLE_CERT	=   4, /*!< exportable certification */
    PGPSUBTYPE_TRUST_SIG	=   5, /*!< trust signature */
    PGPSUBTYPE_REGEX		=   6, /*!< regular expression */
    PGPSUBTYPE_REVOCABLE	=   7, /*!< revocable */
    PGPSUBTYPE_KEY_EXPIRE_TIME	=   9, /*!< key expiration time */
    PGPSUBTYPE_ARR		=  10, /*!< additional recipient request */
    PGPSUBTYPE_PREFER_SYMKEY	=  11, /*!< preferred symmetric algorithms */
    PGPSUBTYPE_REVOKE_KEY	=  12, /*!< revocation key */
    PGPSUBTYPE_ISSUER_KEYID	=  16, /*!< issuer key ID */
    PGPSUBTYPE_NOTATION		=  20, /*!< notation data */
    PGPSUBTYPE_PREFER_HASH	=  21, /*!< preferred hash algorithms */
    PGPSUBTYPE_PREFER_COMPRESS	=  22, /*!< preferred compression algorithms */
    PGPSUBTYPE_KEYSERVER_PREFERS=  23, /*!< key server preferences */
    PGPSUBTYPE_PREFER_KEYSERVER	=  24, /*!< preferred key server */
    PGPSUBTYPE_PRIMARY_USERID	=  25, /*!< primary user id */
    PGPSUBTYPE_POLICY_URL	=  26, /*!< policy URL */
    PGPSUBTYPE_KEY_FLAGS	=  27, /*!< key flags */
    PGPSUBTYPE_SIGNER_USERID	=  28, /*!< signer's user id */
    PGPSUBTYPE_REVOKE_REASON	=  29, /*!< reason for revocation */
    PGPSUBTYPE_FEATURES		=  30, /*!< feature flags */
    PGPSUBTYPE_SIG_TARGET	=  31, /*!< signature target */
    PGPSUBTYPE_EMBEDDED_SIG	=  32, /*!< embedded signature */

    PGPSUBTYPE_INTERNAL_100	= 100, /*!< internal or user-defined */
    PGPSUBTYPE_INTERNAL_101	= 101, /*!< internal or user-defined */
    PGPSUBTYPE_INTERNAL_102	= 102, /*!< internal or user-defined */
    PGPSUBTYPE_INTERNAL_103	= 103, /*!< internal or user-defined */
    PGPSUBTYPE_INTERNAL_104	= 104, /*!< internal or user-defined */
    PGPSUBTYPE_INTERNAL_105	= 105, /*!< internal or user-defined */
    PGPSUBTYPE_INTERNAL_106	= 106, /*!< internal or user-defined */
    PGPSUBTYPE_INTERNAL_107	= 107, /*!< internal or user-defined */
    PGPSUBTYPE_INTERNAL_108	= 108, /*!< internal or user-defined */
    PGPSUBTYPE_INTERNAL_109	= 109, /*!< internal or user-defined */
    PGPSUBTYPE_INTERNAL_110	= 110, /*!< internal or user-defined */

    PGPSUBTYPE_CRITICAL		= 128  /*!< critical subpacket marker */
} pgpSubType;
/*@=typeuse@*/

/** \ingroup rpmpgp
 * Subtype (string, value) pairs.
 */
/*@observer@*/ /*@unchecked@*/ /*@unused@*/
extern struct pgpValTbl_s pgpSubTypeTbl[];

/** \ingroup rpmpgp
 * 5.2. Signature Packet (Tag 2)
 *
 * A signature packet describes a binding between some public key and
 * some data. The most common signatures are a signature of a file or a
 * block of text, and a signature that is a certification of a user ID.
 *
 * Two versions of signature packets are defined.  Version 3 provides
 * basic signature information, while version 4 provides an expandable
 * format with subpackets that can specify more information about the
 * signature. PGP 2.6.x only accepts version 3 signatures.
 *
 * Implementations MUST accept V3 signatures. Implementations SHOULD
 * generate V4 signatures.  Implementations MAY generate a V3 signature
 * that can be verified by PGP 2.6.x.
 *
 * Note that if an implementation is creating an encrypted and signed
 * message that is encrypted to a V3 key, it is reasonable to create a
 * V3 signature.
 */
typedef union pgpPktSig_u {
    struct pgpPktSigV3_s v3;
    struct pgpPktSigV4_s v4;
} * pgpPktSig;

/** \ingroup rpmpgp
 * 5.3. Symmetric-Key Encrypted Session-Key Packets (Tag 3)
 *
 * The Symmetric-Key Encrypted Session Key packet holds the symmetric-
 * key encryption of a session key used to encrypt a message.  Zero or
 * more Encrypted Session Key packets and/or Symmetric-Key Encrypted
 * Session Key packets may precede a Symmetrically Encrypted Data Packet
 * that holds an encrypted message.  The message is encrypted with a
 * session key, and the session key is itself encrypted and stored in
 * the Encrypted Session Key packet or the Symmetric-Key Encrypted
 * Session Key packet.
 *
 * If the Symmetrically Encrypted Data Packet is preceded by one or more
 * Symmetric-Key Encrypted Session Key packets, each specifies a
 * passphrase that may be used to decrypt the message.  This allows a
 * message to be encrypted to a number of public keys, and also to one
 * or more pass phrases. This packet type is new, and is not generated
 * by PGP 2.x or PGP 5.0.
 *
 * The body of this packet consists of:
 *   - A one-octet version number. The only currently defined version
 *     is 4.
 *   - A one-octet number describing the symmetric algorithm used.
 *   - A string-to-key (S2K) specifier, length as defined above.
 *   - Optionally, the encrypted session key itself, which is decrypted
 *     with the string-to-key object.
 *
 */
typedef struct pgpPktSymkey_s {
    rpmuint8_t version;	/*!< version number (4). */
    rpmuint8_t symkey_algo;
    rpmuint8_t s2k[1];
} pgpPktSymkey;

/** \ingroup rpmpgp
 * 5.4. One-Pass Signature Packets (Tag 4)
 *
 * The One-Pass Signature packet precedes the signed data and contains
 * enough information to allow the receiver to begin calculating any
 * hashes needed to verify the signature.  It allows the Signature
 * Packet to be placed at the end of the message, so that the signer can
 * compute the entire signed message in one pass.
 *
 * A One-Pass Signature does not interoperate with PGP 2.6.x or earlier.
 *
 * The body of this packet consists of:
 *   - A one-octet version number. The current version is 3.
 *   - A one-octet signature type. Signature types are described in
 *     section 5.2.1.
 *   - A one-octet number describing the hash algorithm used.
 *   - A one-octet number describing the public key algorithm used.
 *   - An eight-octet number holding the key ID of the signing key.
 *   - A one-octet number holding a flag showing whether the signature
 *     is nested.  A zero value indicates that the next packet is
 *     another One-Pass Signature packet that describes another
 *     signature to be applied to the same message data.
 *
 * Note that if a message contains more than one one-pass signature,
 * then the signature packets bracket the message; that is, the first
 * signature packet after the message corresponds to the last one-pass
 * packet and the final signature packet corresponds to the first one-
 * pass packet.
 */
typedef struct pgpPktOnepass_s {
    rpmuint8_t version;		/*!< version number (3). */
    rpmuint8_t sigtype;		/*!< signature type. */
    rpmuint8_t hash_algo;	/*!< hash algorithm. */
    rpmuint8_t pubkey_algo;	/*!< public key algorithm. */
    rpmuint8_t signid[8];	/*!< key ID of signer. */
    rpmuint8_t nested;
} * pgpPktOnepass;

/** \ingroup rpmpgp
 * 5.5.1. Key Packet Variants
 *
 * 5.5.1.1. Public Key Packet (Tag 6)
 *
 * A Public Key packet starts a series of packets that forms an OpenPGP
 * key (sometimes called an OpenPGP certificate).
 *
 * 5.5.1.2. Public Subkey Packet (Tag 14)
 *
 * A Public Subkey packet (tag 14) has exactly the same format as a
 * Public Key packet, but denotes a subkey. One or more subkeys may be
 * associated with a top-level key.  By convention, the top-level key
 * provides signature services, and the subkeys provide encryption
 * services.
 *
 * Note: in PGP 2.6.x, tag 14 was intended to indicate a comment packet.
 * This tag was selected for reuse because no previous version of PGP
 * ever emitted comment packets but they did properly ignore them.
 * Public Subkey packets are ignored by PGP 2.6.x and do not cause it to
 * fail, providing a limited degree of backward compatibility.
 *
 * 5.5.1.3. Secret Key Packet (Tag 5)
 *
 * A Secret Key packet contains all the information that is found in a
 * Public Key packet, including the public key material, but also
 * includes the secret key material after all the public key fields.
 *
 * 5.5.1.4. Secret Subkey Packet (Tag 7)
 *
 * A Secret Subkey packet (tag 7) is the subkey analog of the Secret Key
 * packet, and has exactly the same format.
 *
 * 5.5.2. Public Key Packet Formats
 *
 * There are two versions of key-material packets. Version 3 packets
 * were first generated by PGP 2.6. Version 2 packets are identical in
 * format to Version 3 packets, but are generated by PGP 2.5 or before.
 * V2 packets are deprecated and they MUST NOT be generated.  PGP 5.0
 * introduced version 4 packets, with new fields and semantics.  PGP
 * 2.6.x will not accept key-material packets with versions greater than
 * 3.
 *
 * OpenPGP implementations SHOULD create keys with version 4 format. An
 * implementation MAY generate a V3 key to ensure interoperability with
 * old software; note, however, that V4 keys correct some security
 * deficiencies in V3 keys. These deficiencies are described below. An
 * implementation MUST NOT create a V3 key with a public key algorithm
 * other than RSA.
 *
 * A version 3 public key or public subkey packet contains:
 *   - A one-octet version number (3).
 *   - A four-octet number denoting the time that the key was created.
 *   - A two-octet number denoting the time in days that this key is
 *     valid. If this number is zero, then it does not expire.
 *   - A one-octet number denoting the public key algorithm of this key
 *   - A series of multi-precision integers comprising the key
 *     material:
 *       - a multiprecision integer (MPI) of RSA public modulus n;
 *       - an MPI of RSA public encryption exponent e.
 *
 * V3 keys SHOULD only be used for backward compatibility because of
 * three weaknesses in them. First, it is relatively easy to construct a
 * V3 key that has the same key ID as any other key because the key ID
 * is simply the low 64 bits of the public modulus. Secondly, because
 * the fingerprint of a V3 key hashes the key material, but not its
 * length, which increases the opportunity for fingerprint collisions.
 * Third, there are minor weaknesses in the MD5 hash algorithm that make
 * developers prefer other algorithms. See below for a fuller discussion
 * of key IDs and fingerprints.
 *
 */
typedef struct pgpPktKeyV3_s {
    rpmuint8_t version;		/*!< version number (3). */
    rpmuint8_t time[4];		/*!< time that the key was created. */
    rpmuint8_t valid[2];	/*!< time in days that this key is valid. */
    rpmuint8_t pubkey_algo;	/*!< public key algorithm. */
} * pgpPktKeyV3;

/** \ingroup rpmpgp
 * The version 4 format is similar to the version 3 format except for
 * the absence of a validity period.  This has been moved to the
 * signature packet.  In addition, fingerprints of version 4 keys are
 * calculated differently from version 3 keys, as described in section
 * "Enhanced Key Formats."
 *
 * A version 4 packet contains:
 *   - A one-octet version number (4).
 *   - A four-octet number denoting the time that the key was created.
 *   - A one-octet number denoting the public key algorithm of this key
 *   - A series of multi-precision integers comprising the key
 *     material.  This algorithm-specific portion is:
 *
 *     Algorithm Specific Fields for RSA public keys:
 *       - multiprecision integer (MPI) of RSA public modulus n;
 *       - MPI of RSA public encryption exponent e.
 *
 *     Algorithm Specific Fields for DSA public keys:
 *       - MPI of DSA prime p;
 *       - MPI of DSA group order q (q is a prime divisor of p-1);
 *       - MPI of DSA group generator g;
 *       - MPI of DSA public key value y (= g**x where x is secret).
 *
 *     Algorithm Specific Fields for Elgamal public keys:
 *       - MPI of Elgamal prime p;
 *       - MPI of Elgamal group generator g;
 *       - MPI of Elgamal public key value y (= g**x where x is
 *         secret).
 *
 */
typedef struct pgpPktKeyV4_s {
    rpmuint8_t version;		/*!< version number (4). */
    rpmuint8_t time[4];		/*!< time that the key was created. */
    rpmuint8_t pubkey_algo;	/*!< public key algorithm. */
} * pgpPktKeyV4;

/** \ingroup rpmpgp
 * 5.5.3. Secret Key Packet Formats
 *
 * The Secret Key and Secret Subkey packets contain all the data of the
 * Public Key and Public Subkey packets, with additional algorithm-
 * specific secret key data appended, in encrypted form.
 *
 * The packet contains:
 *   - A Public Key or Public Subkey packet, as described above
 *   - One octet indicating string-to-key usage conventions.  0
 *     indicates that the secret key data is not encrypted.  255
 *     indicates that a string-to-key specifier is being given.  Any
 *     other value is a symmetric-key encryption algorithm specifier.
 *   - [Optional] If string-to-key usage octet was 255, a one-octet
 *     symmetric encryption algorithm.
 *   - [Optional] If string-to-key usage octet was 255, a string-to-key
 *     specifier.  The length of the string-to-key specifier is implied
 *     by its type, as described above.
 *   - [Optional] If secret data is encrypted, eight-octet Initial
 *     Vector (IV).
 *   - Encrypted multi-precision integers comprising the secret key
 *     data. These algorithm-specific fields are as described below.
 *   - Two-octet checksum of the plaintext of the algorithm-specific
 *     portion (sum of all octets, mod 65536).
 *
 *     Algorithm Specific Fields for RSA secret keys:
 *     - multiprecision integer (MPI) of RSA secret exponent d.
 *     - MPI of RSA secret prime value p.
 *     - MPI of RSA secret prime value q (p < q).
 *     - MPI of u, the multiplicative inverse of p, mod q.
 *
 *     Algorithm Specific Fields for DSA secret keys:
 *     - MPI of DSA secret exponent x.
 *
 *     Algorithm Specific Fields for Elgamal secret keys:
 *     - MPI of Elgamal secret exponent x.
 *
 * Secret MPI values can be encrypted using a passphrase.  If a string-
 * to-key specifier is given, that describes the algorithm for
 * converting the passphrase to a key, else a simple MD5 hash of the
 * passphrase is used.  Implementations SHOULD use a string-to-key
 * specifier; the simple hash is for backward compatibility. The cipher
 * for encrypting the MPIs is specified in the secret key packet.
 *
 * Encryption/decryption of the secret data is done in CFB mode using
 * the key created from the passphrase and the Initial Vector from the
 * packet. A different mode is used with V3 keys (which are only RSA)
 * than with other key formats. With V3 keys, the MPI bit count prefix
 * (i.e., the first two octets) is not encrypted.  Only the MPI non-
 * prefix data is encrypted.  Furthermore, the CFB state is
 * resynchronized at the beginning of each new MPI value, so that the
 * CFB block boundary is aligned with the start of the MPI data.
 *
 * With V4 keys, a simpler method is used.  All secret MPI values are
 * encrypted in CFB mode, including the MPI bitcount prefix.
 *
 * The 16-bit checksum that follows the algorithm-specific portion is
 * the algebraic sum, mod 65536, of the plaintext of all the algorithm-
 * specific octets (including MPI prefix and data).  With V3 keys, the
 * checksum is stored in the clear.  With V4 keys, the checksum is
 * encrypted like the algorithm-specific data.  This value is used to
 * check that the passphrase was correct.
 *
 */
typedef union pgpPktKey_u {
    struct pgpPktKeyV3_s v3;
    struct pgpPktKeyV4_s v4;
} pgpPktKey;

/** \ingroup rpmpgp
 * 5.6. Compressed Data Packet (Tag 8)
 *
 * The Compressed Data packet contains compressed data. Typically, this
 * packet is found as the contents of an encrypted packet, or following
 * a Signature or One-Pass Signature packet, and contains literal data
 * packets.
 *
 * The body of this packet consists of:
 *   - One octet that gives the algorithm used to compress the packet.
 *   - The remainder of the packet is compressed data.
 *
 * A Compressed Data Packet's body contains an block that compresses
 * some set of packets. See section "Packet Composition" for details on
 * how messages are formed.
 *
 * ZIP-compressed packets are compressed with raw RFC 1951 DEFLATE
 * blocks. Note that PGP V2.6 uses 13 bits of compression. If an
 * implementation uses more bits of compression, PGP V2.6 cannot
 * decompress it.
 *
 * ZLIB-compressed packets are compressed with RFC 1950 ZLIB-style
 * blocks.
 */
typedef struct pgpPktCdata_s {
    rpmuint8_t compressalgo;
    rpmuint8_t data[1];
} pgpPktCdata;

/** \ingroup rpmpgp
 * 5.7. Symmetrically Encrypted Data Packet (Tag 9)
 *
 * The Symmetrically Encrypted Data packet contains data encrypted with
 * a symmetric-key algorithm. When it has been decrypted, it will
 * typically contain other packets (often literal data packets or
 * compressed data packets).
 *
 * The body of this packet consists of:
 *   - Encrypted data, the output of the selected symmetric-key cipher
 *     operating in PGP's variant of Cipher Feedback (CFB) mode.
 *
 * The symmetric cipher used may be specified in an Public-Key or
 * Symmetric-Key Encrypted Session Key packet that precedes the
 * Symmetrically Encrypted Data Packet.  In that case, the cipher
 * algorithm octet is prefixed to the session key before it is
 * encrypted.  If no packets of these types precede the encrypted data,
 * the IDEA algorithm is used with the session key calculated as the MD5
 * hash of the passphrase.
 *
 * The data is encrypted in CFB mode, with a CFB shift size equal to the
 * cipher's block size.  The Initial Vector (IV) is specified as all
 * zeros.  Instead of using an IV, OpenPGP prefixes a 10-octet string to
 * the data before it is encrypted.  The first eight octets are random,
 * and the 9th and 10th octets are copies of the 7th and 8th octets,
 * respectively. After encrypting the first 10 octets, the CFB state is
 * resynchronized if the cipher block size is 8 octets or less.  The
 * last 8 octets of ciphertext are passed through the cipher and the
 * block boundary is reset.
 *
 * The repetition of 16 bits in the 80 bits of random data prefixed to
 * the message allows the receiver to immediately check whether the
 * session key is incorrect.
 */
typedef struct pgpPktEdata_s {
    rpmuint8_t data[1];
} pgpPktEdata;

/** \ingroup rpmpgp
 * 5.8. Marker Packet (Obsolete Literal Packet) (Tag 10)
 *
 * An experimental version of PGP used this packet as the Literal
 * packet, but no released version of PGP generated Literal packets with
 * this tag. With PGP 5.x, this packet has been re-assigned and is
 * reserved for use as the Marker packet.
 *
 * The body of this packet consists of:
 *   - The three octets 0x50, 0x47, 0x50 (which spell "PGP" in UTF-8).
 *
 * Such a packet MUST be ignored when received.  It may be placed at the
 * beginning of a message that uses features not available in PGP 2.6.x
 * in order to cause that version to report that newer software is
 * necessary to process the message.
 */
/*
 * 5.9. Literal Data Packet (Tag 11)
 *
 * A Literal Data packet contains the body of a message; data that is
 * not to be further interpreted.
 *
 * The body of this packet consists of:
 *   - A one-octet field that describes how the data is formatted.
 *
 * If it is a 'b' (0x62), then the literal packet contains binary data.
 * If it is a 't' (0x74), then it contains text data, and thus may need
 * line ends converted to local form, or other text-mode changes.  RFC
 * 1991 also defined a value of 'l' as a 'local' mode for machine-local
 * conversions.  This use is now deprecated.
 *   - File name as a string (one-octet length, followed by file name),
 *     if the encrypted data should be saved as a file.
 *
 * If the special name "_CONSOLE" is used, the message is considered to
 * be "for your eyes only".  This advises that the message data is
 * unusually sensitive, and the receiving program should process it more
 * carefully, perhaps avoiding storing the received data to disk, for
 * example.
 *   - A four-octet number that indicates the modification date of the
 *     file, or the creation time of the packet, or a zero that
 *     indicates the present time.
 *   - The remainder of the packet is literal data.
 *
 * Text data is stored with <CR><LF> text endings (i.e. network-normal
 * line endings).  These should be converted to native line endings by
 * the receiving software.
 */
typedef struct pgpPktLdata_s {
    rpmuint8_t format;
    rpmuint8_t filenamelen;
    rpmuint8_t filename[1];
} pgpPktLdata;

/** \ingroup rpmpgp
 * 5.10. Trust Packet (Tag 12)
 *
 * The Trust packet is used only within keyrings and is not normally
 * exported.  Trust packets contain data that record the user's
 * specifications of which key holders are trustworthy introducers,
 * along with other information that implementing software uses for
 * trust information.
 *
 * Trust packets SHOULD NOT be emitted to output streams that are
 * transferred to other users, and they SHOULD be ignored on any input
 * other than local keyring files.
 */
typedef struct pgpPktTrust_s {
    rpmuint8_t flag;
} pgpPktTrust;

/** \ingroup rpmpgp
 * 5.11. User ID Packet (Tag 13)
 *
 * A User ID packet consists of data that is intended to represent the
 * name and email address of the key holder.  By convention, it includes
 * an RFC 822 mail name, but there are no restrictions on its content.
 * The packet length in the header specifies the length of the user id.
 * If it is text, it is encoded in UTF-8.
 *
 */
typedef struct pgpPktUid_s {
    rpmuint8_t userid[1];
} pgpPktUid;

/** \ingroup rpmpgp
 */
/*@-typeuse@*/
typedef enum pgpArmor_e {
    PGPARMOR_ERR_CRC_CHECK		= -7,
    PGPARMOR_ERR_BODY_DECODE		= -6,
    PGPARMOR_ERR_CRC_DECODE		= -5,
    PGPARMOR_ERR_NO_END_PGP		= -4,
    PGPARMOR_ERR_UNKNOWN_PREAMBLE_TAG	= -3,
    PGPARMOR_ERR_UNKNOWN_ARMOR_TYPE	= -2,
    PGPARMOR_ERR_NO_BEGIN_PGP		= -1,
#define	PGPARMOR_ERROR	PGPARMOR_ERR_NO_BEGIN_PGP
    PGPARMOR_NONE		=  0,
    PGPARMOR_MESSAGE		=  1, /*!< MESSAGE */
    PGPARMOR_PUBKEY		=  2, /*!< PUBLIC KEY BLOCK */
    PGPARMOR_SIGNATURE		=  3, /*!< SIGNATURE */
    PGPARMOR_SIGNED_MESSAGE	=  4, /*!< SIGNED MESSAGE */
    PGPARMOR_FILE		=  5, /*!< ARMORED FILE */
    PGPARMOR_PRIVKEY		=  6, /*!< PRIVATE KEY BLOCK */
    PGPARMOR_SECKEY		=  7  /*!< SECRET KEY BLOCK */
} pgpArmor;
/*@=typeuse@*/

/** \ingroup rpmpgp
 * Armor (string, value) pairs.
 */
/*@observer@*/ /*@unchecked@*/ /*@unused@*/
extern struct pgpValTbl_s pgpArmorTbl[];

/** \ingroup rpmpgp
 */
/*@-typeuse@*/
typedef enum pgpArmorKey_e {
    PGPARMORKEY_VERSION		= 1, /*!< Version: */
    PGPARMORKEY_COMMENT		= 2, /*!< Comment: */
    PGPARMORKEY_MESSAGEID	= 3, /*!< MessageID: */
    PGPARMORKEY_HASH		= 4, /*!< Hash: */
    PGPARMORKEY_CHARSET		= 5  /*!< Charset: */
} pgpArmorKey;
/*@=typeuse@*/

/** \ingroup rpmpgp
 * Armor key (string, value) pairs.
 */
/*@observer@*/ /*@unchecked@*/ /*@unused@*/
extern struct pgpValTbl_s pgpArmorKeyTbl[];

#if defined(_RPMPGP_INTERNAL)
/** \ingroup rpmpgp
 */
union pgpPktPre_u {
    pgpPktPubkey pubkey;	/*!< 5.1. Public-Key Encrypted Session Key */
    pgpPktSig sig;		/*!< 5.2. Signature */
    pgpPktSymkey symkey;	/*!< 5.3. Symmetric-Key Encrypted Session-Key */
    pgpPktOnepass onepass;	/*!< 5.4. One-Pass Signature */
    pgpPktKey key;		/*!< 5.5. Key Material */
    pgpPktCdata cdata;		/*!< 5.6. Compressed Data */
    pgpPktEdata edata;		/*!< 5.7. Symmetrically Encrypted Data */
				/*!< 5.8. Marker (obsolete) */
    pgpPktLdata ldata;		/*!< 5.9. Literal Data */
    pgpPktTrust tdata;		/*!< 5.10. Trust */
    pgpPktUid uid;		/*!< 5.11. User ID */
};

struct pgpPkt_s {
    pgpTag tag;
    unsigned int pktlen;
    union {
	const rpmuint8_t * h;
	const pgpPktKeyV3 j;
	const pgpPktKeyV4 k;
	const pgpPktSigV3 r;
	const pgpPktSigV4 s;
	const pgpPktUid * u;
    } u;
    unsigned int hlen;
};
#endif	/* _RPMPGP_INTERNAL */

/*@-fcnuse@*/
#ifdef __cplusplus
extern "C" {
#endif

/** \ingroup rpmpgp
 * Return (native-endian) integer from big-endian representation.
 * @param s		pointer to big-endian integer
 * @param nbytes	no. of bytes
 * @return		native-endian integer
 */
/*@unused@*/ static inline
unsigned int pgpGrab(const rpmuint8_t * s, size_t nbytes)
	/*@*/
{
    unsigned int i = 0;
    size_t nb = (nbytes <= sizeof(i) ? nbytes : sizeof(i));
    while (nb--)
	i = (i << 8) | *s++;
    return i;
}

/** \ingroup rpmpgp
 * Return length of an OpenPGP packet.
 * @param s		pointer to packet
 * @retval *lenp	no. of bytes in packet
 * @return		no. of bytes in length prefix
 */
/*@unused@*/ static inline
unsigned int pgpLen(const rpmuint8_t * s, /*@out@*/ unsigned int * lenp)
	/*@modifies *lenp @*/
{
    if (*s < (rpmuint8_t)192) {
	*lenp = (unsigned int) *s++;
	return 1;
    } else if (*s < (rpmuint8_t)255) {
	*lenp = (unsigned int) ((((unsigned)s[0]) - 192) << 8) + (unsigned)s[1] + 192;
	return 2;
    } else {
	*lenp = pgpGrab(s+1, 4);
	return 5;
    }
}

/** \ingroup rpmpgp
 * Return no. of bits in a multiprecision integer.
 * @param p		pointer to multiprecision integer
 * @return		no. of bits
 */
/*@unused@*/ static inline
unsigned int pgpMpiBits(const rpmuint8_t * p)
	/*@requires maxRead(p) >= 1 @*/
	/*@*/
{
    return (unsigned int) ((p[0] << 8) | p[1]);
}

/** \ingroup rpmpgp
 * Return no. of bytes in a multiprecision integer.
 * @param p		pointer to multiprecision integer
 * @return		no. of bytes
 */
/*@unused@*/ static inline
unsigned int pgpMpiLen(const rpmuint8_t * p)
	/*@requires maxRead(p) >= 1 @*/
	/*@*/
{
    return (2 + ((pgpMpiBits(p)+7) >> 3));
}
	
/** \ingroup rpmpgp
 * Convert to hex.
 * @param t		target buffer (returned)
 * @param s		source bytes
 * @param nbytes	no. of bytes
 * @return		target buffer
 */
/*@unused@*/ static inline
char * pgpHexCvt(/*@returned@*/ char * t, const rpmuint8_t * s, size_t nbytes)
	/*@modifies *t @*/
{
    static char hex[] = "0123456789abcdef";
    while (nbytes-- > 0) {
	unsigned int i;
	i = (unsigned int) *s++;
	*t++ = hex[ (i >> 4) & 0xf ];
	*t++ = hex[ (i     ) & 0xf ];
    }
    *t = '\0';
    return t;
}

/** \ingroup rpmpgp
 * Return hex formatted representation of bytes.
 * @todo Remove static buffer. 
 * @param p		bytes
 * @param plen		no. of bytes
 * @return		hex formatted string
 */
/*@unused@*/ static inline /*@observer@*/
char * pgpHexStr(const rpmuint8_t * p, size_t plen)
	/*@*/
{
    static char prbuf[8*BUFSIZ];	/* XXX ick */
    char *t = prbuf;
    t = pgpHexCvt(t, p, plen);
    return prbuf;
}

/** \ingroup rpmpgp
 * Return hex formatted representation of a multiprecision integer.
 * @todo Remove static buffer. 
 * @param p		bytes
 * @return		hex formatted string
 */
/*@unused@*/ static inline /*@observer@*/
const char * pgpMpiStr(const rpmuint8_t * p)
	/*@requires maxRead(p) >= 3 @*/
	/*@*/
{
    static char prbuf[8*BUFSIZ];	/* XXX ick */
    char *t = prbuf;
    sprintf(t, "[%4u]: ", pgpGrab(p, 2));
    t += strlen(t);
    t = pgpHexCvt(t, p+2, pgpMpiLen(p)-2);
    return prbuf;
}

/** \ingroup rpmpgp
 * Return string representation of an OpenPGP value.
 * @param vs		table of (string,value) pairs
 * @param val		byte value to lookup
 * @return		string
 */
/*@unused@*/ static inline /*@observer@*/
const char * pgpValStr(pgpValTbl vs, rpmuint8_t val)
	/*@*/
{
    do {
	if (vs->val == (int)val)
	    break;
    } while ((++vs)->val != -1);
    return vs->str;
}

/*@unused@*/ static inline /*@observer@*/
const char * _pgpTag2Name(uint32_t tag)
	/*@*/
{
    return pgpValStr(pgpTagTbl, (rpmuint8_t)tag);
}

/*@unused@*/ static inline /*@observer@*/
const char * _pgpSigType2Name(uint32_t sigtype)
	/*@*/
{
    return pgpValStr(pgpSigTypeTbl, (rpmuint8_t)sigtype);
}

/*@unused@*/ static inline /*@observer@*/
const char * _pgpHashAlgo2Name(uint32_t algo)
	/*@*/
{
    return pgpValStr(pgpHashTbl, (rpmuint8_t)algo);
}

/*@unused@*/ static inline /*@observer@*/
const char * _pgpPubkeyAlgo2Name(uint32_t algo)
	/*@*/
{
    return pgpValStr(pgpPubkeyTbl, (rpmuint8_t)algo);
}

/** \ingroup rpmpgp
 * Return byte value of an OpenPGP string.
 * @param vs		table of (string,value) pairs
 * @param str		string to lookup
 * @return		byte (or 0 on failure)
 */
/*@unused@*/ static inline /*@observer@*/
rpmuint8_t pgpStrVal(pgpValTbl vs, const char * str)
	/*@*/
{
    do {
	if (!strcasecmp(str, vs->str))
	    break;
    } while ((++vs)->val != -1);
    return (vs->val != -1 ? vs->val : 0);
}

/** \ingroup rpmpgp
 * Return value of an OpenPGP string.
 * @param vs		table of (string,value) pairs
 * @param s		string token to lookup
 * @param se		end-of-string address
 * @return		byte value
 */
/*@unused@*/ static inline
int pgpValTok(pgpValTbl vs, const char * s, const char * se)
	/*@*/
{
    do {
	size_t vlen = strlen(vs->str);
	if (vlen <= (size_t)(se-s) && !strncmp(s, vs->str, vlen))
	    break;
    } while ((++vs)->val != -1);
    return vs->val;
}

/** \ingroup rpmpgp
 * Print an OpenPGP value.
 * @param pre		output prefix
 * @param vs		table of (string,value) pairs
 * @param val		byte value to print
 */
/*@-exportlocal@*/
void pgpPrtVal(const char * pre, pgpValTbl vs, rpmuint8_t val)
	/*@globals fileSystem @*/
	/*@modifies fileSystem @*/;
/*@=exportlocal@*/

/** \ingroup rpmpgp
 * Print/parse an OpenPGP subtype packet.
 * @param h		packet
 * @param hlen		packet length (no. of bytes)
 * @param sigtype	signature type
 * @return		0 on success
 */
#if defined(_RPMPGP_INTERNAL)
/*@-exportlocal@*/
int pgpPrtSubType(const rpmuint8_t * h, size_t hlen, pgpSigType sigtype)
	/*@globals fileSystem @*/
	/*@modifies fileSystem @*/;
/*@=exportlocal@*/
#endif

/** \ingroup rpmpgp
 * Print/parse an OpenPGP signature packet.
 * @param pp		packet tag/ptr/len
 * @return		0 on success
 */
#if defined(_RPMPGP_INTERNAL)
/*@-exportlocal@*/
int pgpPrtSig(const pgpPkt pp)
	/*@globals fileSystem, internalState @*/
	/*@modifies fileSystem, internalState @*/;

int pgpPrtSigParams(pgpDig dig, const pgpPkt pp, pgpPubkeyAlgo pubkey_algo,
                pgpSigType sigtype, const rpmuint8_t * p)
        /*@globals fileSystem @*/
        /*@modifies fileSystem @*/;

const rpmuint8_t * pgpPrtPubkeyParams(pgpDig dig, const pgpPkt pp,
                pgpPubkeyAlgo pubkey_algo, /*@returned@*/ const rpmuint8_t * p)
        /*@globals fileSystem, internalState @*/
        /*@modifies fileSystem, internalState @*/;

/*@=exportlocal@*/
#endif

/** \ingroup rpmpgp
 * Print/parse an OpenPGP key packet.
 * @param pp		packet tag/ptr/len
 * @return		0 on success
 */
#if defined(_RPMPGP_INTERNAL)
int pgpPrtKey(const pgpPkt pp)
	/*@globals fileSystem, internalState @*/
	/*@modifies fileSystem, internalState @*/;
#endif

/** \ingroup rpmpgp
 * Print/parse an OpenPGP userid packet.
 * @param pp		packet tag/ptr/len
 * @return		0 on success
 */
#if defined(_RPMPGP_INTERNAL)
/*@-exportlocal@*/
int pgpPrtUserID(const pgpPkt pp)
	/*@globals fileSystem, internalState @*/
	/*@modifies fileSystem, internalState @*/;
/*@=exportlocal@*/
#endif

/** \ingroup rpmpgp
 * Print/parse an OpenPGP comment packet.
 * @param pp		packet tag/ptr/len
 * @return		0 on success
 */
#if defined(_RPMPGP_INTERNAL)
/*@-exportlocal@*/
int pgpPrtComment(const pgpPkt pp)
	/*@globals fileSystem @*/
	/*@modifies fileSystem @*/;
/*@=exportlocal@*/
#endif

/** \ingroup rpmpgp
 * Calculate OpenPGP public key fingerprint.
 * @todo V3 non-RSA public keys not implemented.
 * @param pkt		OpenPGP packet (i.e. PGPTAG_PUBLIC_KEY)
 * @param pktlen	OpenPGP packet length (no. of bytes)
 * @retval keyid	publick key fingerprint
 * @return		0 on success, else -1
 */
/*@-exportlocal@*/
int pgpPubkeyFingerprint(const rpmuint8_t * pkt, size_t pktlen,
		/*@out@*/ rpmuint8_t * keyid)
	/*@modifies *keyid @*/;
/*@=exportlocal@*/

/** \ingroup rpmpgp
 * Extract OpenPGP public key fingerprint from base64 encoded packet.
 * @todo V3 non-RSA public keys not implemented.
 * @param b64pkt	base64 encoded openpgp packet
 * @retval keyid[8]	public key fingerprint
 * @return		8 (no. of bytes) on success, < 0 on error
 */
int pgpExtractPubkeyFingerprint(const char * b64pkt,
		/*@out@*/ rpmuint8_t * keyid)
	/*@modifies *keyid @*/;

/** \ingroup rpmpgp
 * Return lenth of a OpenPGP packet.
 * @param pkt		OpenPGP packet (i.e. PGPTAG_PUBLIC_KEY)
 * @param pleft		OpenPGP packet length (no. of bytes)
 * @retval pp		packet tag/ptr/len
 * @return		packet length, <0 on error.
 */
#if defined(_RPMPGP_INTERNAL)
int pgpPktLen(const rpmuint8_t * pkt, size_t pleft, /*@out@*/ pgpPkt pp)
	/*@modifies pp @*/;
#endif

/** \ingroup rpmpgp
 * Print/parse next OpenPGP packet.
 * @param pkt		OpenPGP packet
 * @param pleft		no. bytes remaining
 * @return		-1 on error, otherwise this packet length
 */
/*@-exportlocal@*/
int pgpPrtPkt(const rpmuint8_t * pkt, size_t pleft)
	/*@globals fileSystem, internalState @*/
	/*@modifies fileSystem, internalState @*/;
/*@=exportlocal@*/

/** \ingroup rpmpgp
 * Return array of packet pointers.
 * @param pkts		OpenPGP packet(s)
 * @param pktlen	OpenPGP packet(s) length (no. of bytes)
 * @retval *pppkts	array of packet pointers
 * @retval *pnpkts	no. of packets
 * @return		0 on success, <0 on error
 */
int pgpGrabPkts(const rpmuint8_t * pkts, size_t pktlen,
		/*@out@*/ rpmuint8_t *** pppkts, /*@out@*/ int * pnpkts)
        /*@modifies *pppkts, *pnpkts @*/;

/** \ingroup rpmpgp
 * Print/parse a OpenPGP packet(s).
 * @param pkts		OpenPGP packet(s)
 * @param pktlen	OpenPGP packet(s) length (no. of bytes)
 * @retval dig		parsed output of signature/pubkey packet parameters
 * @param printing	should packets be printed?
 * @return		-1 on error, 0 on success
 */
int pgpPrtPkts(const rpmuint8_t * pkts, size_t pktlen, pgpDig dig, int printing)
	/*@globals fileSystem, internalState @*/
	/*@modifies dig, fileSystem, internalState @*/;

/** \ingroup rpmpgp
 * Parse armored OpenPGP packets from an iob.
 * @param iob		I/O buffer
 * @retval pkt		dearmored OpenPGP packet(s)
 * @retval pktlen	dearmored OpenPGP packet(s) length in bytes
 * @return		type of armor found
 */
pgpArmor pgpArmorUnwrap(rpmiob iob,
		/*@out@*/ rpmuint8_t ** pkt, /*@out@*/ size_t * pktlen)
	/*@globals h_errno, fileSystem, internalState @*/
	/*@modifies *pkt, *pktlen, fileSystem, internalState @*/;

/** \ingroup rpmpgp
 * Parse armored OpenPGP packets from a file.
 * @param fn		file name
 * @retval pkt		dearmored OpenPGP packet(s)
 * @retval pktlen	dearmored OpenPGP packet(s) length in bytes
 * @return		type of armor found
 */
pgpArmor pgpReadPkts(const char * fn,
		/*@out@*/ rpmuint8_t ** pkt, /*@out@*/ size_t * pktlen)
	/*@globals h_errno, fileSystem, internalState @*/
	/*@modifies *pkt, *pktlen, fileSystem, internalState @*/;

/** \ingroup rpmpgp
 * Wrap a OpenPGP packets in ascii armor for transport.
 * @param atype		type of armor
 * @param s		binary pkt data
 * @param ns		binary pkt data length
 * @return		formatted string
 */
char * pgpArmorWrap(rpmuint8_t atype, const unsigned char * s, size_t ns)
	/*@*/;

/** \ingroup rpmpgp
 * Convert a hash algorithm "foo" to the internal PGPHASHALGO_FOO number.
 * @param name		name of hash algorithm
 * @param name_len		length of name or 0 for strlen(name)
 * @return		PGPHASHALGO_<name> or -1 in case of error
 */
pgpHashAlgo pgpHashAlgoStringToNumber(const char *name, size_t name_len)
	/*@*/;

/**
 * Disabler bits(s) for signature/digest checking.
 */
/*@unchecked@*/
extern pgpVSFlags pgpDigVSFlags;

/** \ingroup rpmpgp
 * Unreference a signature parameters instance.
 * @param dig		signature parameters
 * @return		NULL on last dereference
 */
/*@unused@*/ /*@null@*/
pgpDig pgpDigUnlink (/*@killref@*/ /*@only@*/ /*@null@*/ pgpDig dig)
	/*@modifies dig @*/;
#define	pgpDigUnlink(_dig)	\
    ((pgpDig)rpmioUnlinkPoolItem((rpmioItem)(_dig), __FUNCTION__, __FILE__, __LINE__))

/** \ingroup rpmpgp
 * Reference a signature parameters instance.
 * @param dig		signature parameters
 * @return		new signature parameters reference
 */
/*@unused@*/ /*@newref@*/ /*@null@*/
pgpDig pgpDigLink (/*@null@*/ pgpDig dig)
	/*@modifies dig @*/;
#define	pgpDigLink(_dig)	\
    ((pgpDig)rpmioLinkPoolItem((rpmioItem)(_dig), __FUNCTION__, __FILE__, __LINE__))

/** \ingroup rpmpgp
 * Destroy a container for parsed OpenPGP packates.
 * @param dig		signature parameters container
 * @return		NULL on last dereference
 */
/*@unused@*/ /*@null@*/
pgpDig pgpDigFree(/*@killref@*/ /*@only@*/ /*@null@*/ pgpDig dig)
	/*@modifies dig @*/;
#define pgpDigFree(_dig)	\
    ((pgpDig)rpmioFreePoolItem((rpmioItem)(_dig), __FUNCTION__, __FILE__, __LINE__))

/** \ingroup rpmpgp
 * Create a container for parsed OpenPGP packates.
 * Generate a keypair (if requested).
 * @param vsflags	verify signature flags (usually 0)
 * @param pubkey_algo	pubkey algorithm (0 disables)
 * @return		container
 */
/*@relnull@*/
pgpDig pgpDigNew(pgpVSFlags vsflags, pgpPubkeyAlgo pubkey_algo)
	/*@globals fileSystem @*/
	/*@modifies fileSystem @*/;

/** \ingroup rpmpgp
 * Release (malloc'd) data from container.
 * @param dig		signature parameters container
 */
void pgpDigClean(/*@null@*/ pgpDig dig)
	/*@modifies dig @*/;

const char * pgpHashAlgo2Name(uint32_t algo)
	/*@*/;
const char * pgpPubkeyAlgo2Name(uint32_t algo)
	/*@*/;
rpmuint8_t pgpHashName2Algo(const char * name)
	/*@*/;
rpmuint8_t pgpPubkeyName2Algo(const char * name)
	/*@*/;
int pgpDigSetAlgos(pgpDig dig)
	/*@*/;
int pgpExportPubkey(pgpDig dig)
	/*@*/;
int pgpExportSignature(pgpDig dig, /*@only@*/ DIGEST_CTX ctx)
	/*@*/;

/** \ingroup rpmpgp
 * Return OpenPGP pubkey parameters.
 * @param dig		signature parameters container
 * @return		pubkey parameters
 */
/*@exposed@*/
pgpDigParams pgpGetPubkey(const pgpDig dig)
	/*@*/;

/** \ingroup rpmpgp
 * Return OpenPGP signature parameters.
 * @param dig		signature parameters container
 * @return		signature parameters
 */
/*@exposed@*/
pgpDigParams pgpGetSignature(const pgpDig dig)
	/*@*/;

/** \ingroup rpmpgp
 * Get signature tag.
 * @param dig		signature parameters container
 * @return		signature tag
 */
rpmuint32_t pgpGetSigtag(const pgpDig dig)
	/*@*/;

/** \ingroup rpmpgp
 * Get signature tag type.
 * @param dig		signature parameters container
 * @return		signature tag type
 */
rpmuint32_t pgpGetSigtype(const pgpDig dig)
	/*@*/;

/** \ingroup rpmpgp
 * Get signature tag data, i.e. from header.
 * @param dig		signature parameters container
 * @return		signature tag data
 */
/*@observer@*/ /*@null@*/
extern const void * pgpGetSig(const pgpDig dig)
	/*@*/;

/** \ingroup rpmpgp
 * Get signature tag data length, i.e. no. of bytes of data.
 * @param dig		signature parameters container
 * @return		signature tag data length
 */
rpmuint32_t pgpGetSiglen(const pgpDig dig)
	/*@*/;

/** \ingroup rpmpgp
 * Set signature tag info, i.e. from header.
 * @param dig		signature parameters container
 * @param sigtag	signature tag
 * @param sigtype	signature tag type
 * @param sig		signature tag data
 * @param siglen	signature tag data length
 * @return		0 always
 */
int pgpSetSig(pgpDig dig,
		rpmuint32_t sigtag, rpmuint32_t sigtype,
		/*@kept@*/ /*@null@*/ const void * sig, rpmuint32_t siglen)
	/*@modifies dig @*/;

/** \ingroup rpmpgp
 * Return pgpDig container accumulator structure.
 * @param dig		signature parameters container
 * @param opx		per-container accumulator index (aka rpmtsOpX)
 * @return		per-container accumulator pointer
 */
/*@null@*/
void * pgpStatsAccumulator(pgpDig dig, int opx)
	/*@*/;

/** \ingroup rpmpgp
 * Set find pubkey vector.
 * @param dig		signature parameters container
 * @param findPubkey	routine to find a pubkey.
 * @param _ts		argument to (*findPubkey) (ts, ...)
 * @return		0 always
 */
int pgpSetFindPubkey(pgpDig dig,
		/*@null@*/ int (*findPubkey) (void *ts, /*@null@*/ void *dig),
		/*@exposed@*/ /*@null@*/ void * _ts)
	/*@modifies dig @*/;

/** \ingroup rpmpgp
 * Call find pubkey vector.
 * @param dig		signature parameters container
 * @return		rpmRC return code
 */
int pgpFindPubkey(pgpDig dig)
	/*@modifies dig @*/;

/** \ingroup rpmpgp
 * Is buffer at beginning of an OpenPGP packet?
 * @param p		buffer
 * @retval *tagp	OpenPGP tag
 * @return		1 if an OpenPGP packet, 0 otherwise
 */
/*@unused@*/ static inline
int pgpIsPkt(const rpmuint8_t * p, /*@null@*/ pgpTag * tagp)
	/*@modifies *tagp @*/
{
    unsigned int val = (unsigned int) *p++;
    pgpTag tag;
    int rc;

    /* XXX can't deal with these. */
    if (!(val & 0x80))
	return 0;

    if (val & 0x40)
	tag = (pgpTag)(val & 0x3f);
    else
	tag = (pgpTag)((val >> 2) & 0xf);

    switch (tag) {
    case PGPTAG_MARKER:
    case PGPTAG_SYMMETRIC_SESSION_KEY:
    case PGPTAG_ONEPASS_SIGNATURE:
    case PGPTAG_PUBLIC_KEY:
    case PGPTAG_SECRET_KEY:
    case PGPTAG_PUBLIC_SESSION_KEY:
    case PGPTAG_SIGNATURE:
    case PGPTAG_COMMENT:
    case PGPTAG_COMMENT_OLD:
    case PGPTAG_LITERAL_DATA:
    case PGPTAG_COMPRESSED_DATA:
    case PGPTAG_SYMMETRIC_DATA:
	rc = 1;
	break;
    case PGPTAG_PUBLIC_SUBKEY:
    case PGPTAG_SECRET_SUBKEY:
    case PGPTAG_USER_ID:
    case PGPTAG_RESERVED:
    case PGPTAG_TRUST:
    case PGPTAG_PHOTOID:
    case PGPTAG_ENCRYPTED_MDC:
    case PGPTAG_MDC:
    case PGPTAG_PRIVATE_60:
    case PGPTAG_PRIVATE_62:
    case PGPTAG_CONTROL:
    default:
	rc = 0;
	break;
    }
    if (tagp != NULL)
	*tagp = tag;
    return rc;
}

#define CRC24_INIT	0xb704ce
#define CRC24_POLY	0x1864cfb

/** \ingroup rpmpgp
 * Return CRC of a buffer.
 * @param octets	bytes
 * @param len		no. of bytes
 * @return		crc of buffer
 */
/*@unused@*/ static inline
unsigned int pgpCRC(const rpmuint8_t * octets, size_t len)
	/*@*/
{
    unsigned int crc = CRC24_INIT;
    int i;

    while (len--) {
	crc ^= (*octets++) << 16;
	for (i = 0; i < 8; i++) {
	    crc <<= 1;
	    if (crc & 0x1000000)
		crc ^= CRC24_POLY;
	}
    }
    return crc & 0xffffff;
}

/**
 */
typedef int (*pgpImplSet_t) (/*@only@*/ DIGEST_CTX ctx, pgpDig dig, pgpDigParams sigp)
        /*@modifies ctx, dig @*/;

/**
 */
typedef int (*pgpImplErrChk_t) (pgpDig dig, const char * msg, int rc, unsigned expected)
        /*@*/;

/**
 */
typedef int (*pgpImplAvailable_t) (pgpDig dig, int algo)
        /*@*/;

/**
 */
typedef int (*pgpImplGenerate_t) (pgpDig dig)
        /*@*/;

/**
 */
typedef int (*pgpImplSign_t) (pgpDig dig)
        /*@*/;

/**
 */
typedef int (*pgpImplVerify_t) (pgpDig dig)
        /*@*/;

/**
 */
typedef int (*pgpImplMpiItem_t) (const char * pre, pgpDig dig, int itemno,
		const rpmuint8_t * p, /*@null@*/ const rpmuint8_t * pend)
	/*@globals fileSystem @*/
	/*@modifies dig, fileSystem @*/;

/**
 */
typedef void (*pgpImplClean_t) (void * impl)
        /*@modifies impl @*/;

/**
 */
typedef void * (*pgpImplFree_t) (/*@only@*/ void * impl)
        /*@modifies impl @*/;

/**
 */
typedef void * (*pgpImplInit_t) (void)
        /*@*/;


/**
 */
typedef struct pgpImplVecs_s {
    const char *	_pgpNV;
    pgpImplSet_t	_pgpSetRSA;
    pgpImplSet_t	_pgpSetDSA;
    pgpImplSet_t	_pgpSetELG;
    pgpImplSet_t	_pgpSetECDSA;

    pgpImplErrChk_t	_pgpErrChk;
    pgpImplAvailable_t	_pgpAvailableCipher;
    pgpImplAvailable_t	_pgpAvailableDigest;
    pgpImplAvailable_t	_pgpAvailablePubkey;

    pgpImplVerify_t	_pgpVerify;
    pgpImplSign_t	_pgpSign;
    pgpImplGenerate_t	_pgpGenerate;

    pgpImplMpiItem_t	_pgpMpiItem;
    pgpImplClean_t	_pgpClean;
    pgpImplFree_t	_pgpFree;
    pgpImplInit_t	_pgpInit;
} pgpImplVecs_t;

/**
 */
/*@unchecked@*/
extern pgpImplVecs_t * pgpImplVecs;

/*@-mustmod@*/
/**
 */
/*@unused@*/ static inline
int pgpImplSetRSA(/*@only@*/ DIGEST_CTX ctx, pgpDig dig, pgpDigParams sigp)
	/*@modifies ctx, dig @*/
{
    return (*pgpImplVecs->_pgpSetRSA) (ctx, dig, sigp);
}

/**
 */
/*@unused@*/ static inline
int pgpImplSetDSA(/*@only@*/ DIGEST_CTX ctx, pgpDig dig, pgpDigParams sigp)
	/*@modifies ctx, dig @*/
{
    return (*pgpImplVecs->_pgpSetDSA) (ctx, dig, sigp);
}

/**
 */
/*@unused@*/ static inline
int pgpImplSetELG(/*@only@*/ DIGEST_CTX ctx, pgpDig dig, pgpDigParams sigp)
	/*@modifies ctx, dig @*/
{
    return (*pgpImplVecs->_pgpSetELG) (ctx, dig, sigp);
}

/**
 */
/*@unused@*/ static inline
int pgpImplSetECDSA(/*@only@*/ DIGEST_CTX ctx, pgpDig dig, pgpDigParams sigp)
	/*@modifies ctx, dig @*/
{
    return (*pgpImplVecs->_pgpSetECDSA) (ctx, dig, sigp);
}

/**
 */
/*@unused@*/ static inline
int pgpImplErrChk(pgpDig dig, const char * msg, int rc, unsigned expected)
	/*@*/
{
    return (pgpImplVecs->_pgpErrChk
	? (*pgpImplVecs->_pgpErrChk) (dig, msg, rc, expected)
	: rc);
}

/**
 */
/*@unused@*/ static inline
int pgpImplAvailableCipher(pgpDig dig, int algo)
	/*@*/
{
    return (pgpImplVecs->_pgpAvailableCipher
	? (*pgpImplVecs->_pgpAvailableCipher) (dig, algo)
	: 0);
}

/**
 */
/*@unused@*/ static inline
int pgpImplAvailableDigest(pgpDig dig, int algo)
	/*@*/
{
    return (pgpImplVecs->_pgpAvailableDigest
	? (*pgpImplVecs->_pgpAvailableDigest) (dig, algo)
	: 0);
}

/**
 */
/*@unused@*/ static inline
int pgpImplAvailablePubkey(pgpDig dig, int algo)
	/*@*/
{
    return (pgpImplVecs->_pgpAvailablePubkey
	? (*pgpImplVecs->_pgpAvailablePubkey) (dig, algo)
	: 0);
}

/**
 */
/*@unused@*/ static inline
int pgpImplVerify(pgpDig dig)
	/*@*/
{
    return (pgpImplVecs->_pgpVerify
	? (*pgpImplVecs->_pgpVerify) (dig)
	: 0);
}

/**
 */
/*@unused@*/ static inline
int pgpImplSign(pgpDig dig)
	/*@*/
{
    return (pgpImplVecs->_pgpSign
	? (*pgpImplVecs->_pgpSign) (dig)
	: 0);
}

/**
 */
/*@unused@*/ static inline
int pgpImplGenerate(pgpDig dig)
	/*@*/
{
    return (pgpImplVecs->_pgpGenerate
	? (*pgpImplVecs->_pgpGenerate) (dig)
	: 0);
}

/**
 */
/*@unused@*/ static inline
int pgpImplMpiItem(const char * pre, pgpDig dig, int itemno,
		const rpmuint8_t * p, /*@null@*/ const rpmuint8_t * pend)
	/*@modifies dig @*/
{
    return (*pgpImplVecs->_pgpMpiItem) (pre, dig, itemno, p, pend);
}

/**
 */
/*@unused@*/ static inline
void pgpImplClean(void * impl)
        /*@modifies impl @*/
{
/*@-noeffectuncon@*/
    (*pgpImplVecs->_pgpClean) (impl);
/*@=noeffectuncon@*/
}

/**
 */
/*@unused@*/ static inline
/*@null@*/
void * pgpImplFree(/*@only@*/ void * impl)
        /*@modifies impl @*/
{
    return (*pgpImplVecs->_pgpFree) (impl);
}

/**
 */
/*@unused@*/ static inline
void * pgpImplInit(void)
        /*@*/
{
    return (*pgpImplVecs->_pgpInit) ();
}
/*@=mustmod@*/


#ifdef __cplusplus
}
#endif
/*@=fcnuse@*/

#endif	/* H_RPMPGP */
