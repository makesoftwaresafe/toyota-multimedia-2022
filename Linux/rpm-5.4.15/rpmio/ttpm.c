/*
 * (c) Copyright IBM Corporation 2006, 2010.
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 *
 * Neither the names of the IBM Corporation nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "system.h"

#define	TPM_POSIX			1
#define	TPM_V12				1
#define	TPM_NV_DISK			1
#define	TPM_MAXIMUM_KEY_SIZE		4096
#define	TPM_AES				1
#define	TPM_USE_TAG_IN_STRUCTURE	1

#include <tpmfunc.h>
#include <tpm_error.h>

#define	_RPMTPM_INTERNAL
#include <rpmtpm.h>

#include <openssl/pem.h>
#include <openssl/rsa.h>

#include "debug.h"

/*@access pgpDig @*/
/*@access pgpDigParams @*/

/*@-redecl@*/
/*@unchecked@*/
extern int _pgp_debug;

/*@unchecked@*/
extern int _pgp_print;
/*@=redecl@*/

struct get_matrix {
    uint32_t cap;
    uint32_t subcap_size;
    uint32_t result_size;
};

struct set_matrix {
    uint32_t subcap32;
    char *name;
    uint32_t type;
};

struct set_choice {
    uint32_t capArea;
    char *name;
    const struct set_matrix *smatrix;
};

#define TYPE_BOOL		(1 << 0)
#define TYPE_STRUCTURE		(1 << 1)
#define TYPE_UINT32		(1 << 2)
#define TYPE_UINT32_ARRAY	(1 << 3)
#define TYPE_VARIOUS		(1 << 4)
#define TYPE_PCR_SELECTION	(1 << 5)

static const struct get_matrix matrx[] = {
    { TPM_CAP_ORD,		4, TYPE_BOOL},
    { TPM_CAP_ALG,		4, TYPE_BOOL},
    { TPM_CAP_PID,		2, TYPE_BOOL},
    { TPM_CAP_FLAG,		4, TYPE_STRUCTURE},
    { TPM_CAP_PROPERTY,		4, TYPE_VARIOUS},
    { TPM_CAP_VERSION,		0, TYPE_UINT32},
    { TPM_CAP_KEY_HANDLE,	0, TYPE_STRUCTURE},
    { TPM_CAP_CHECK_LOADED,	4, TYPE_BOOL},
    { TPM_CAP_KEY_STATUS,	4, TYPE_BOOL},
    { TPM_CAP_NV_LIST,		0, TYPE_UINT32_ARRAY},
    { TPM_CAP_MFR,		4, TYPE_VARIOUS},
    { TPM_CAP_NV_INDEX,		4, TYPE_STRUCTURE},
    { TPM_CAP_TRANS_ALG,	4, TYPE_BOOL},
//  { TPM_CAP_GPIO_CHANNEL,	2, TYPE_BOOL},
    { TPM_CAP_HANDLE,		4, TYPE_STRUCTURE},
    { TPM_CAP_TRANS_ES,		2, TYPE_BOOL},
//  { TPM_CAP_MANUFACTURER_VER,	0, TYPE_STRUCTURE},
    { TPM_CAP_AUTH_ENCRYPT,	4, TYPE_BOOL},
    { TPM_CAP_SELECT_SIZE,	4, TYPE_BOOL},
    { TPM_CAP_VERSION_VAL,	0, TYPE_STRUCTURE},
    { TPM_CAP_FLAG_PERMANENT,	0, TYPE_STRUCTURE},
    { TPM_CAP_FLAG_VOLATILE,	0, TYPE_STRUCTURE},
    { TPM_CAP_DA_LOGIC,		2, TYPE_STRUCTURE},
    { -1,-1,-1}
};

static const struct get_matrix mfr_matrix[] = {
    { TPM_CAP_PROCESS_ID,	0, TYPE_UINT32},
    { -1,-1,-1}
};

#define _ENTRY(_v, _t)      { _v, #_v, _t }
static const struct set_matrix matrix_permflags[] = {
    _ENTRY(TPM_PF_DISABLE,		TYPE_BOOL),
    _ENTRY(TPM_PF_OWNERSHIP,		TYPE_BOOL),
    _ENTRY(TPM_PF_DEACTIVATED,		TYPE_BOOL),
    _ENTRY(TPM_PF_READPUBEK,		TYPE_BOOL),
    _ENTRY(TPM_PF_DISABLEOWNERCLEAR,	TYPE_BOOL),
    _ENTRY(TPM_PF_ALLOWMAINTENANCE,	TYPE_BOOL),
    _ENTRY(TPM_PF_PHYSICALPRESENCELIFETIMELOCK, TYPE_BOOL),
    _ENTRY(TPM_PF_PHYSICALPRESENCEHWENABLE, TYPE_BOOL),
    _ENTRY(TPM_PF_PHYSICALPRESENCECMDENABLE, TYPE_BOOL),
//    _ENTRY(TPM_PF_CEKPUSED,		TYPE_BOOL),
//    _ENTRY(TPM_PF_TPMPOST,		TYPE_BOOL),
//    _ENTRY(TPM_PF_TPMPOSTLOCK,	TYPE_BOOL),
//    _ENTRY(TPM_PF_FIPS,		TYPE_BOOL),
//    _ENTRY(TPM_PF_OPERATOR,		TYPE_BOOL),
//    _ENTRY(TPM_PF_ENABLEREVOKEEK,	TYPE_BOOL),
    _ENTRY(TPM_PF_NV_LOCKED,		TYPE_BOOL),
    _ENTRY(TPM_PF_READSRKPUB,		TYPE_BOOL),
    _ENTRY(TPM_PF_TPMESTABLISHED,	TYPE_BOOL),
    _ENTRY(TPM_PF_DISABLEFULLDALOGICINFO, TYPE_BOOL),
  { -1, NULL, 0 }
};

static const struct set_matrix matrix_permdata[] = {
    _ENTRY(TPM_PD_RESTRICTDELEGATE,	TYPE_UINT32),
  { -1, NULL, 0 }
};

static const struct set_matrix matrix_stcflags[] = {
    _ENTRY(TPM_SF_DISABLEFORCECLEAR,	TYPE_BOOL),
    _ENTRY(TPM_SF_PHYSICALPRESENCE,	TYPE_BOOL),
    _ENTRY(TPM_SF_PHYSICALPRESENCELOCK,	TYPE_BOOL),
  { -1, NULL, 0 }
};

static const struct set_matrix matrix_stcdata[] = {
    _ENTRY(TPM_SD_CONTEXTNONCEKEY,	TYPE_BOOL),
    _ENTRY(TPM_SD_COUNTID,		TYPE_BOOL),
    _ENTRY(TPM_SD_OWNERREFERENCE,	TYPE_BOOL),
    _ENTRY(TPM_SD_DISABLERESETLOCK,	TYPE_BOOL),
    _ENTRY(TPM_SD_PCR,			TYPE_BOOL),
    _ENTRY(TPM_SD_DEFERREDPHYSICALPRESENCE, TYPE_UINT32),
  { -1, NULL, 0 }
};

static const struct set_matrix matrix_stanyflags[] = {
    _ENTRY(TPM_AF_TOSPRESENT,	TYPE_BOOL),
  { -1, NULL, 0 }
};
#undef	_ENTRY

static const struct set_matrix matrix_stanydata[] = {
  { -1, NULL, 0 }
};

static const struct set_matrix matrix_vendor[] = {
  { -1, NULL, 0 }
};

static const struct set_choice choice[] = {
  { TPM_SET_PERM_FLAGS,		"TPM_PERMANENT_FLAGS",	matrix_permflags },
  { TPM_SET_PERM_DATA,		"TPM_PERMANENT_DATA",	matrix_permdata },
  { TPM_SET_STCLEAR_FLAGS,	"TPM_STCLEAR_FLAGS",	matrix_stcflags },
  { TPM_SET_STCLEAR_DATA,	"TPM_STCLEAR_DATA",	matrix_stcdata },
  { TPM_SET_STANY_FLAGS,	"TPM_STANY_FLAGS",	matrix_stanyflags },
  { TPM_SET_STANY_DATA,		"TPM_STANY_DATA",	matrix_stanydata },
  { TPM_SET_VENDOR,		"TPM_SET_VENDOR",	matrix_vendor },
  { -1, NULL, NULL }
};

static uint32_t sikeyhandle = 0;
static char * sikeypass = NULL;
static char * ownerpass = NULL;

static uint32_t cap = 0xffffffff;
static uint32_t scap = 0xffffffff;
#if 0
static uint32_t sscap = 0xffffffff;
#endif
static uint32_t val = 0xffffffff;

static int ix = -1;
static int ordinal = -1;

static const char * keyname;
static int createkey_index;		/* XXX split <pcrnum>:<digest> */

static int pkcsv15;			/* XXX TPM_BOOL 0:FALSE 1:TRUE */
static int use_oldversion;		/* XXX TPM_BOOL 0:FALSE 1:TRUE */
static int evict_all;			/* XXX TPM_BOOL 0:FALSE 1:TRUE */
static int load_keep;			/* XXX TPM_BOOL 0:FALSE 1:TRUE */
static int add_version;			/* XXX TPM_BOOL 0:FALSE 1:TRUE */

static const char * keytype = "s";

static const char *ctrpass;
static const char *keypass;
static const char *migpass;
static const char *parpass;
static const char *datpass;

enum {
    NONE,
    PCR_INFO,
    PCR_INFO_LONG
};
static int use_struct = NONE;
static uint32_t keysize = 2048;
static uint32_t parhandle;		/* handle of parent key */

static int disabled_state;		/* XXX 0:disabled 1:enabled */
static int disabled_audit;		/* XXX TPM_BOOL 0:TRUE 1:FALSE */

static const char *ifn;
static const char *ofn;
static const char *msg;

static uint32_t hahandle = 0xffffffff;
static uint32_t hkhandle = 0xffffffff;
static uint32_t restype;
static const char *lbl;

static uint32_t bitmask;
static uint32_t bitname = 0xffffffff;
static uint32_t bitvalue;

/*==============================================================*/

/*@-redef@*/
typedef struct key_s {
    uint32_t    v;
/*@observer@*/
    const char *n;
} KEY;
/*@=redef@*/

/*@observer@*/
static const char * tblName(uint32_t v, KEY * tbl, size_t ntbl)
	/*@*/
{
    const char * n = NULL;
    static char buf[256];
    size_t i;

    for (i = 0; i < ntbl; i++) {
	if (v != tbl[i].v)
	    continue;
	n = tbl[i].n;
	break;
    }
    if (n == NULL) {
	(void) snprintf(buf, sizeof(buf), "0x%x", (unsigned)v);
	n = buf;
    }
    return n;
}

/* --- 17. Ordinals rev 110 */
#define _ENTRY(_v)      { TPM_ORD_##_v, #_v }
static KEY TPM_ORD_[] = {
    _ENTRY(ActivateIdentity),
    _ENTRY(AuthorizeMigrationKey),
    _ENTRY(CertifyKey),
    _ENTRY(CertifyKey2),
    _ENTRY(CertifySelfTest),
    _ENTRY(ChangeAuth),
    _ENTRY(ChangeAuthAsymFinish),
    _ENTRY(ChangeAuthAsymStart),
    _ENTRY(ChangeAuthOwner),
    _ENTRY(CMK_ApproveMA),
    _ENTRY(CMK_ConvertMigration),
    _ENTRY(CMK_CreateBlob),
    _ENTRY(CMK_CreateKey),
    _ENTRY(CMK_CreateTicket),
    _ENTRY(CMK_SetRestrictions),
    _ENTRY(ContinueSelfTest),
    _ENTRY(ConvertMigrationBlob),
    _ENTRY(CreateCounter),
    _ENTRY(CreateEndorsementKeyPair),
    _ENTRY(CreateMaintenanceArchive),
    _ENTRY(CreateMigrationBlob),
    _ENTRY(CreateRevocableEK),
    _ENTRY(CreateWrapKey),
    _ENTRY(DAA_Join),
    _ENTRY(DAA_Sign),
    _ENTRY(Delegate_CreateKeyDelegation),
    _ENTRY(Delegate_CreateOwnerDelegation),
    _ENTRY(Delegate_LoadOwnerDelegation),
    _ENTRY(Delegate_Manage),
    _ENTRY(Delegate_ReadTable),
    _ENTRY(Delegate_UpdateVerification),
    _ENTRY(Delegate_VerifyDelegation),
    _ENTRY(DirRead),
    _ENTRY(DirWriteAuth),
    _ENTRY(DisableForceClear),
    _ENTRY(DisableOwnerClear),
    _ENTRY(DisablePubekRead),
    _ENTRY(DSAP),
    _ENTRY(EstablishTransport),
    _ENTRY(EvictKey),
    _ENTRY(ExecuteTransport),
    _ENTRY(Extend),
    _ENTRY(FieldUpgrade),
    _ENTRY(FlushSpecific),
    _ENTRY(ForceClear),
    _ENTRY(GetAuditDigest),
    _ENTRY(GetAuditDigestSigned),
    _ENTRY(GetAuditEvent),
    _ENTRY(GetAuditEventSigned),
    _ENTRY(GetCapability),
    _ENTRY(GetCapabilityOwner),
    _ENTRY(GetCapabilitySigned),
    _ENTRY(GetOrdinalAuditStatus),
    _ENTRY(GetPubKey),
    _ENTRY(GetRandom),
    _ENTRY(GetTestResult),
    _ENTRY(GetTicks),
    _ENTRY(IncrementCounter),
    _ENTRY(Init),
    _ENTRY(KeyControlOwner),
    _ENTRY(KillMaintenanceFeature),
    _ENTRY(LoadAuthContext),
    _ENTRY(LoadContext),
    _ENTRY(LoadKey),
    _ENTRY(LoadKey2),
    _ENTRY(LoadKeyContext),
    _ENTRY(LoadMaintenanceArchive),
    _ENTRY(LoadManuMaintPub),
    _ENTRY(MakeIdentity),
    _ENTRY(MigrateKey),
    _ENTRY(NV_DefineSpace),
    _ENTRY(NV_ReadValue),
    _ENTRY(NV_ReadValueAuth),
    _ENTRY(NV_WriteValue),
    _ENTRY(NV_WriteValueAuth),
    _ENTRY(OIAP),
    _ENTRY(OSAP),
    _ENTRY(OwnerClear),
    _ENTRY(OwnerReadInternalPub),
    _ENTRY(OwnerReadPubek),
    _ENTRY(OwnerSetDisable),
    _ENTRY(PCR_Reset),
    _ENTRY(PcrRead),
    _ENTRY(PhysicalDisable),
    _ENTRY(PhysicalEnable),
    _ENTRY(PhysicalSetDeactivated),
    _ENTRY(Quote),
    _ENTRY(Quote2),
    _ENTRY(ReadCounter),
    _ENTRY(ReadManuMaintPub),
    _ENTRY(ReadPubek),
    _ENTRY(ReleaseCounter),
    _ENTRY(ReleaseCounterOwner),
    _ENTRY(ReleaseTransportSigned),
    _ENTRY(Reset),
    _ENTRY(ResetLockValue),
    _ENTRY(RevokeTrust),
    _ENTRY(SaveAuthContext),
    _ENTRY(SaveContext),
    _ENTRY(SaveKeyContext),
    _ENTRY(SaveState),
    _ENTRY(Seal),
    _ENTRY(Sealx),
    _ENTRY(SelfTestFull),
    _ENTRY(SetCapability),
    _ENTRY(SetOperatorAuth),
    _ENTRY(SetOrdinalAuditStatus),
    _ENTRY(SetOwnerInstall),
    _ENTRY(SetOwnerPointer),
    _ENTRY(SetRedirection),
    _ENTRY(SetTempDeactivated),
    _ENTRY(SHA1Complete),
    _ENTRY(SHA1CompleteExtend),
    _ENTRY(SHA1Start),
    _ENTRY(SHA1Update),
    _ENTRY(Sign),
    _ENTRY(Startup),
    _ENTRY(StirRandom),
    _ENTRY(TakeOwnership),
    _ENTRY(Terminate_Handle),
    _ENTRY(TickStampBlob),
    _ENTRY(UnBind),
    _ENTRY(Unseal),
};
static size_t nTPM_ORD_ = sizeof(TPM_ORD_) / sizeof(TPM_ORD_[0]);
#undef	_ENTRY

/* --- 3.1 TPM_STRUCTURE_TAG */
#define _ENTRY(_v)      { TPM_TAG_##_v, #_v }
static KEY TPM_TAG_[] = {
    _ENTRY(CONTEXTBLOB),	/*  TPM_CONTEXT_BLOB */
    _ENTRY(CONTEXT_SENSITIVE),	/*  TPM_CONTEXT_SENSITIVE */
    _ENTRY(CONTEXTPOINTER),	/*  TPM_CONTEXT_POINTER */
    _ENTRY(CONTEXTLIST),	/*  TPM_CONTEXT_LIST */
    _ENTRY(SIGNINFO),		/*  TPM_SIGN_INFO */
    _ENTRY(PCR_INFO_LONG),	/*  TPM_PCR_INFO_LONG */
    _ENTRY(PERSISTENT_FLAGS),	/*  TPM_PERSISTENT_FLAGS (deprecated) */
    _ENTRY(VOLATILE_FLAGS),	/*  TPM_VOLATILE_FLAGS (deprecated) */
    _ENTRY(PERSISTENT_DATA),	/*  TPM_PERSISTENT_DATA (deprecated) */
    _ENTRY(VOLATILE_DATA),	/*  TPM_VOLATILE_DATA (deprecated) */
    _ENTRY(SV_DATA),		/*  TPM_SV_DATA */
    _ENTRY(EK_BLOB),		/*  TPM_EK_BLOB */
    _ENTRY(EK_BLOB_AUTH),	/*  TPM_EK_BLOB_AUTH */
    _ENTRY(COUNTER_VALUE),	/*  TPM_COUNTER_VALUE */
    _ENTRY(TRANSPORT_INTERNAL),	/*  TPM_TRANSPORT_INTERNAL */
    _ENTRY(TRANSPORT_LOG_IN),	/*  TPM_TRANSPORT_LOG_IN */
    _ENTRY(TRANSPORT_LOG_OUT),	/*  TPM_TRANSPORT_LOG_OUT */
    _ENTRY(AUDIT_EVENT_IN),	/*  TPM_AUDIT_EVENT_IN */
    _ENTRY(AUDIT_EVENT_OUT),	/*  TPM_AUDIT_EVENT_OUT */
    _ENTRY(CURRENT_TICKS),	/*  TPM_CURRENT_TICKS */
    _ENTRY(KEY),		/*  TPM_KEY */
    _ENTRY(STORED_DATA12),	/*  TPM_STORED_DATA12 */
    _ENTRY(NV_ATTRIBUTES),	/*  TPM_NV_ATTRIBUTES */
    _ENTRY(NV_DATA_PUBLIC),	/*  TPM_NV_DATA_PUBLIC */
    _ENTRY(NV_DATA_SENSITIVE),	/*  TPM_NV_DATA_SENSITIVE */
    _ENTRY(DELEGATIONS),	/*  TPM DELEGATIONS */
    _ENTRY(DELEGATE_PUBLIC),	/*  TPM_DELEGATE_PUBLIC */
    _ENTRY(DELEGATE_TABLE_ROW),	/*  TPM_DELEGATE_TABLE_ROW */
    _ENTRY(TRANSPORT_AUTH),	/*  TPM_TRANSPORT_AUTH */
    _ENTRY(TRANSPORT_PUBLIC),	/*  TPM_TRANSPORT_PUBLIC */
    _ENTRY(PERMANENT_FLAGS),	/*  TPM_PERMANENT_FLAGS */
    _ENTRY(STCLEAR_FLAGS),	/*  TPM_STCLEAR_FLAGS */
    _ENTRY(STANY_FLAGS),	/*  TPM_STANY_FLAGS */
    _ENTRY(PERMANENT_DATA),	/*  TPM_PERMANENT_DATA */
    _ENTRY(STCLEAR_DATA),	/*  TPM_STCLEAR_DATA */
    _ENTRY(STANY_DATA),		/*  TPM_STANY_DATA */
    _ENTRY(FAMILY_TABLE_ENTRY),	/*  TPM_FAMILY_TABLE_ENTRY */
    _ENTRY(DELEGATE_SENSITIVE),	/*  TPM_DELEGATE_SENSITIVE */
    _ENTRY(DELG_KEY_BLOB),	/*  TPM_DELG_KEY_BLOB */
    _ENTRY(KEY12),		/*  TPM_KEY12 */
    _ENTRY(CERTIFY_INFO2),	/*  TPM_CERTIFY_INFO2 */
    _ENTRY(DELEGATE_OWNER_BLOB),/*  TPM_DELEGATE_OWNER_BLOB */
    _ENTRY(EK_BLOB_ACTIVATE),	/*  TPM_EK_BLOB_ACTIVATE */
    _ENTRY(DAA_BLOB),		/*  TPM_DAA_BLOB */
    _ENTRY(DAA_CONTEXT),	/*  TPM_DAA_CONTEXT */
    _ENTRY(DAA_ENFORCE),	/*  TPM_DAA_ENFORCE */
    _ENTRY(DAA_ISSUER),		/*  TPM_DAA_ISSUER */
    _ENTRY(CAP_VERSION_INFO),	/*  TPM_CAP_VERSION_INFO */
    _ENTRY(DAA_SENSITIVE),	/*  TPM_DAA_SENSITIVE */
    _ENTRY(DAA_TPM),		/*  TPM_DAA_TPM */
    _ENTRY(CMK_MIGAUTH),	/*  TPM_CMK_MIGAUTH */
    _ENTRY(CMK_SIGTICKET),	/*  TPM_CMK_SIGTICKET */
    _ENTRY(CMK_MA_APPROVAL),	/*  TPM_CMK_MA_APPROVAL */
    _ENTRY(QUOTE_INFO2),	/*  TPM_QUOTE_INFO2 */
    _ENTRY(DA_INFO),		/*  TPM_DA_INFO */
    _ENTRY(DA_INFO_LIMITED),	/*  TPM_DA_INFO_LIMITED */
    _ENTRY(DA_ACTION_TYPE)	/*  TPM_DA_ACTION_TYPE */
};
static size_t nTPM_TAG_ = sizeof(TPM_TAG_) / sizeof(TPM_TAG_[0]);
#undef	_ENTRY

/* --- 4.1 TPM_RESOURCE_TYPE rev 87 */
#define _ENTRY(_v)      { TPM_RT_##_v, #_v }
static KEY TPM_RT_[] = {
    _ENTRY(KEY),		/* Key handle from LoadKey */
    _ENTRY(AUTH),		/* Authorization handle from OIAP/OSAP/DSAP */
    _ENTRY(HASH),		/* Reserved for hashes */
    _ENTRY(TRANS),		/* Transport session from TPM_EstablishTransport */
    _ENTRY(CONTEXT),		/* Resource wrapped/held w context save/restore */
    _ENTRY(COUNTER),		/* Reserved for counters */
    _ENTRY(DELEGATE),		/* Delegate row held in NVRAM */
    _ENTRY(DAA_TPM),		/* DAA TPM specific blob */
    _ENTRY(DAA_V0),		/* DAA V0 parameter */
    _ENTRY(DAA_V1)		/* DAA V1 parameter */
};
static size_t nTPM_RT_ = sizeof(TPM_RT_) / sizeof(TPM_RT_[0]);
#undef	_ENTRY

/* --- 4.2 TPM_PAYLOAD_TYPE rev 87 */
#define _ENTRY(_v)      { TPM_PT_##_v, #_v }
static KEY TPM_PT_[] = {
    _ENTRY(ASYM),		/* The entity is an asymmetric key */
    _ENTRY(BIND),		/* The entity is bound data */
    _ENTRY(MIGRATE),		/* The entity is a migration blob */
    _ENTRY(MAINT),		/* The entity is a maintenance blob */
    _ENTRY(SEAL),		/* The entity is sealed data */
    _ENTRY(MIGRATE_RESTRICTED), /* The entity is a restricted-migration asymmetric key */
    _ENTRY(MIGRATE_EXTERNAL),	/* The entity is a external migratable key */
    _ENTRY(CMK_MIGRATE)		/* The entity is a CMK migratable blob */
};
static size_t nTPM_PT_ = sizeof(TPM_PT_) / sizeof(TPM_PT_[0]);
#undef	_ENTRY

/* --- 4.3 TPM_ENTITY_TYPE rev 100 */
#define _ENTRY(_v)      { TPM_ET_##_v, #_v }
static KEY TPM_ET_[] = {
    _ENTRY(KEYHANDLE),		/* The entity is a keyHandle or key */
    _ENTRY(OWNER),		/*0x40000001 The entity is the TPM Owner */
    _ENTRY(DATA),		/* The entity is some data */
    _ENTRY(SRK),		/*0x40000000 The entity is the SRK */
    _ENTRY(KEY),		/* The entity is a key or keyHandle */
    _ENTRY(REVOKE),		/*0x40000002 The entity is RevokeTrust value */
    _ENTRY(DEL_OWNER_BLOB),	/* The entity is a delegate owner blob */
    _ENTRY(DEL_ROW),		/* The entity is a delegate row */
    _ENTRY(DEL_KEY_BLOB),	/* The entity is a delegate key blob */
    _ENTRY(COUNTER),		/* The entity is a counter */
    _ENTRY(NV),			/* The entity is a NV index */
    _ENTRY(OPERATOR),		/* The entity is the operator */
    _ENTRY(RESERVED_HANDLE)	/* Reserved. Avoids collisions w MSB. */
};
static size_t nTPM_ET_ = sizeof(TPM_ET_) / sizeof(TPM_ET_[0]);
#undef	_ENTRY

/* 4.4.1 Reserved Key Handles rev 87 */
#define _ENTRY(_v)      { TPM_KH_##_v, #_v }
static KEY TPM_KH_[] = {
    _ENTRY(SRK),
    _ENTRY(OWNER),
    _ENTRY(REVOKE),
    _ENTRY(TRANSPORT),
    _ENTRY(OPERATOR),
    _ENTRY(ADMIN),
    _ENTRY(EK),
};
static size_t nTPM_KH_ = sizeof(TPM_KH_) / sizeof(TPM_KH_[0]);
#undef	_ENTRY

/* --- 4.5 TPM_STARTUP_TYPE rev 87 */
#define _ENTRY(_v)      { TPM_ST_##_v, #_v }
static KEY TPM_ST_[] = {
    _ENTRY(CLEAR),
    _ENTRY(STATE),
    _ENTRY(DEACTIVATED)
};
static size_t nTPM_ST_ = sizeof(TPM_ST_) / sizeof(TPM_ST_[0]);
#undef	_ENTRY

/* --- 4.6 TPM_STARTUP_EFFECTS rev 101 */
#define _ENTRY(_v)      { TPM_STARTUP_EFFECTS_##_v, #_v }
static KEY TPM_STARTUP_EFFECTS_[] = {
    _ENTRY(ST_STATE_RT_DAA),
    _ENTRY(STARTUP_NO_AUDITDIGEST),
    _ENTRY(ST_CLEAR_AUDITDIGEST),
    _ENTRY(STARTUP_AUDITDIGEST),
    _ENTRY(ST_ANY_RT_KEY),
    _ENTRY(ST_STATE_RT_AUTH),
    _ENTRY(ST_STATE_RT_HASH),
    _ENTRY(ST_STATE_RT_TRANS),
    _ENTRY(ST_STATE_RT_CONTEXT)
};
static size_t nTPM_STARTUP_EFFECTS_ = sizeof(TPM_STARTUP_EFFECTS_) / sizeof(TPM_STARTUP_EFFECTS_[0]);
#undef	_ENTRY

/* --- 4.9 TPM_PHYSICAL_PRESENCE rev 87 */
#define _ENTRY(_v)      { TPM_PHYSICAL_PRESENCE_##_v, #_v }
static KEY TPM_PHYSICAL_PRESENCE_[] = {
    _ENTRY(HW_DISABLE),
    _ENTRY(CMD_DISABLE),
    _ENTRY(LIFETIME_LOCK),
    _ENTRY(HW_ENABLE),
    _ENTRY(CMD_ENABLE),
    _ENTRY(NOTPRESENT),
    _ENTRY(PRESENT),
    _ENTRY(LOCK)
};
static size_t nTPM_PHYSICAL_PRESENCE_ = sizeof(TPM_PHYSICAL_PRESENCE_) / sizeof(TPM_PHYSICAL_PRESENCE_[0]);
#undef	_ENTRY

/* --- 4.10 TPM_MIGRATE_SCHEME rev 103 */
#define _ENTRY(_v)      { TPM_MS_##_v, #_v }
static KEY TPM_MS_[] = {
    _ENTRY(MIGRATE),
    _ENTRY(REWRAP),
    _ENTRY(MAINT),
    _ENTRY(RESTRICT_MIGRATE),
    _ENTRY(RESTRICT_APPROVE)
};
static size_t nTPM_MS_ = sizeof(TPM_MS_) / sizeof(TPM_MS_[0]);
#undef	_ENTRY

/* --- 5.17 TPM_CMK_DELEGATE values rev 89 */
#define _ENTRY(_v)      { TPM_CMK_DELEGATE_##_v, #_v }
static KEY TPM_CMK_DELEGATE_[] = {
    _ENTRY(SIGNING),
    _ENTRY(STORAGE),
    _ENTRY(BIND),
    _ENTRY(LEGACY),
    _ENTRY(MIGRATE)
};
static size_t nTPM_CMK_DELEGATE_ = sizeof(TPM_CMK_DELEGATE_) / sizeof(TPM_CMK_DELEGATE_[0]);
#undef	_ENTRY

/* --- 13.1.1 TPM_TRANSPORT_ATTRIBUTES Definitions */
#define _ENTRY(_v)      { TPM_TRANSPORT_##_v, #_v }
static KEY TPM_TRANSPORT_[] = {
    _ENTRY(ENCRYPT),
    _ENTRY(LOG),
    _ENTRY(EXCLUSIVE)
};
static size_t nTPM_TRANSPORT_ = sizeof(TPM_TRANSPORT_) / sizeof(TPM_TRANSPORT_[0]);
#undef	_ENTRY

/* --- 21.1 TPM_CAPABILITY_AREA rev 115 */
#define _ENTRY(_v)      { TPM_CAP_##_v, #_v }
static KEY TPM_CAP_[] = {
    _ENTRY(ORD),		/* Boolean. TRUE indicates the TPM supports
                                   the ordinal. FALSE indicates the TPM does not
                                   support the ordinal. Unimplemented optional
                                   ordinals and unused (unassigned) ordinals
                                   return FALSE. */
    _ENTRY(ALG),		/* Boolean. TRUE means that the TPM supports
                                   the asymmetric algorithm for TPM_Sign,
                                   TPM_Seal, TPM_UnSeal and TPM_UnBind and
                                   related commands. FALSE indicates that the
                                   asymmetric algorithm is not supported.
                                   The TPM MAY return TRUE or FALSE for other
                                   than asymmetric algorithms that it supports.
                                   Unassigned and unsupported algorithm IDs
                                   return FALSE.*/
    _ENTRY(PID),		/* Boolean. TRUE indicates the TPM supports
                                   the protocol, FALSE indicates the TPM
                                   does not support the protocol.  */
    _ENTRY(FLAG),		/* Return the TPM_PERMANENT_FLAGS or the
                                              TPM_STCLEAR_FLAGS structure */
    _ENTRY(PROPERTY),		/* See following table for the subcaps */
    _ENTRY(VERSION),		/* TPM_STRUCT_VER. The Major and Minor must
                                   indicate 1.1. The firmware revision MUST
                                   indicate 0.0 */
    _ENTRY(KEY_HANDLE),		/* TPM_KEY_HANDLE_LIST. Enumerates all
                                   key handles loaded on the TPM.  */
    _ENTRY(CHECK_LOADED),	/* Boolean. TRUE indicates the TPM has
                                   enough memory available to load a key of
                                   the type specified by TPM_KEY_PARMS.
                                   FALSE indicates that the TPM does not have
                                   enough memory.  */
    _ENTRY(SYM_MODE),		/* Subcap TPM_SYM_MODE
                                   Boolean. TRUE indicates that the TPM
                                   supports the TPM_SYM_MODE, FALSE indicates
                                   the TPM does not support the mode. */
    _ENTRY(KEY_STATUS),		/* Boolean value of ownerEvict. The handle
                                   MUST point to a valid key handle.*/
    _ENTRY(NV_LIST),		/* A list of TPM_NV_INDEX values that are
                                   currently allocated NV storage through
                                   TPM_NV_DefineSpace. */
    _ENTRY(MFR),		/* Manufacturer specific. The manufacturer may
                                   provide any additional information regarding
                                   the TPM and the TPM state but MUST not expose
                                   any sensitive information.  */
    _ENTRY(NV_INDEX),		/* TPM_NV_DATA_PUBLIC. Indicates the
                                   values for the TPM_NV_INDEX. Returns
                                   TPM_BADINDEX if index is not in the
                                   TPM_CAP_NV_LIST list. */
    _ENTRY(TRANS_ALG),		/* Boolean. TRUE means the TPM supports
                                   the algorithm for TPM_EstablishTransport,
                                   TPM_ExecuteTransport and
                                   TPM_ReleaseTransportSigned. FALSE indicates
                                   the algorithm is not supported."
                                              */
    _ENTRY(HANDLE),		/* TPM_KEY_HANDLE_LIST. Enumerates all handles
                                   currently loaded in the TPM for the given
                                   resource type.  */
    _ENTRY(TRANS_ES),		/* Boolean. TRUE means the TPM supports
                                   the encryption scheme in a transport session
                                   for at least one algorithm..  */
    _ENTRY(AUTH_ENCRYPT),	/* Boolean. TRUE indicates that the TPM
                                   supports the encryption algorithm in OSAP
                                   encryption of AuthData values */
    _ENTRY(SELECT_SIZE),	/* Boolean. TRUE indicates that the TPM
                                   supports the size for the given version.
                                   For instance a request could ask for version
                                   1.1 size 2 and the TPM would indicate TRUE.
                                   For 1.1 size 3 the TPM would indicate FALSE.
                                   For 1.2 size 3 the TPM would indicate TRUE. */
    _ENTRY(DA_LOGIC),		/* (OPTIONAL)
                                   TPM_DA_INFO or TPM_DA_INFO_LIMITED.
                                   returns data for the selected entity type
                                   e.g.,
                                       TPM_ET_KEYHANDLE,
                                       TPM_ET_OWNER,
                                       TPM_ET_SRK,
                                       TPM_ET_COUNTER,
                                       TPM_ET_OPERATOR, etc.).
                                   If the implemented dictionary attack logic
                                   does not support different secret types,
                                   the entity type can be ignored. */
    _ENTRY(VERSION_VAL),	/* TPM_CAP_VERSION_INFO. The TPM fills in the
                                   structure and returns the information
                                   indicating what the TPM supports. */
    _ENTRY(FLAG_PERMANENT),	/* Return the TPM_PERMANENT_FLAGS structure */
    _ENTRY(FLAG_VOLATILE)	/* Return the TPM_STCLEAR_FLAGS structure */
};
static size_t nTPM_CAP_ = sizeof(TPM_CAP_) / sizeof(TPM_CAP_[0]);
#undef	_ENTRY

/* --- 21.2 CAP_PROPERTY Subcap values for CAP_PROPERTY rev 105 */
#define _ENTRY(_v)      { TPM_CAP_PROP_##_v, #_v }
static KEY TPM_CAP_PROP_[] = {
    _ENTRY(PCR),		/* uint32_t. Returns the number of PCR
                                   registers supported by the TPM */
    _ENTRY(DIR),		/* uint32_t. Deprecated. Returns the number of
                                   DIR, which is now fixed at 1 */
    _ENTRY(MANUFACTURER),	/* uint32_t.  Returns the vendor ID
                                   unique to each TPM manufacturer. */
    _ENTRY(KEYS),		/* uint32_t. Returns the number of 2048-
                                   bit RSA keys that can be loaded. This may
                                   vary with time and circumstances. */
    _ENTRY(MIN_COUNTER),	/* uint32_t. The minimum amount of time in
                                   10ths of a second that must pass between
                                   invocations of incrementing the monotonic
                                   counter. */
    _ENTRY(AUTHSESS),		/* uint32_t. The number of available
                                   authorization sessions. This may vary with
                                   time and circumstances. */
    _ENTRY(TRANSESS),		/* uint32_t. The number of available transport
                                   sessions. This may vary with time and
                                   circumstances.  */
    _ENTRY(COUNTERS),		/* uint32_t. The number of available monotonic
                                   counters. This may vary with time and
                                   circumstances. */
    _ENTRY(MAX_AUTHSESS),	/* uint32_t. The maximum number of loaded
                                   authorization sessions the TPM supports */
    _ENTRY(MAX_TRANSESS),	/* uint32_t. The maximum number of loaded
                                   transport sessions the TPM supports. */
    _ENTRY(MAX_COUNTERS),	/* uint32_t. MAx. no. of monotonic counters
                                   under control of TPM_CreateCounter */
    _ENTRY(MAX_KEYS),		/* uint32_t. The maximum number of 2048 RSA
                                   keys that the TPM can support. The number
                                   does not include the EK or SRK. */
    _ENTRY(OWNER),		/* BOOL. A value of TRUE indicates that the
                                   TPM has successfully installed an owner. */
    _ENTRY(CONTEXT),		/* uint32_t. The number of available saved
                                   session slots. This may vary with time and
                                   circumstances. */
    _ENTRY(MAX_CONTEXT),	/* uint32_t. Max. no. of saved session slots. */
    _ENTRY(FAMILYROWS),		/* uint32_t. The maximum number of rows in the
                                   family table */
    _ENTRY(TIS_TIMEOUT),	/* A 4 element array of uint32_t values each
                                   denoting the timeout value in microseconds
                                   for the following in this order:
                                     TIMEOUT_A, TIMEOUT_B, TIMEOUT_C, TIMEOUT_D 
                                   Where these timeouts are to be used is
                                   determined by the platform specific TPM
                                   Interface Specification. */
    _ENTRY(STARTUP_EFFECT),	/* The TPM_STARTUP_EFFECTS structure */
    _ENTRY(DELEGATE_ROW),	/* uint32_t. Max size of the delegate
                                   table in rows. */
    _ENTRY(MAX_DAASESS),	/* uint32_t. Max. number of loaded DAA sessions
                                   (join or sign) that the TPM supports */
    _ENTRY(DAASESS),		/* uint32_t. The number of available DAA
                                   sessions. Varies with time/circumstances */
    _ENTRY(CONTEXT_DIST),	/* uint32_t. The maximum distance between
                                   context count values. MUST be >= 2^16-1. */
    _ENTRY(DAA_INTERRUPT),	/* BOOL. A value of TRUE indicates that the
                                   TPM will accept ANY command while executing
                                   a DAA Join or Sign.
                                   A value of FALSE indicates that the TPM
                                   will invalidate the DAA Join or Sign upon
                                   the receipt of any command other than the
                                   next join/sign in the session or a
                                   TPM_SaveContext */
    _ENTRY(SESSIONS),		/* uint32_t. The number of available sessions
                                   from the pool. This MAY vary with time and
                                   circumstances. Pool sessions include
                                   authorization and transport sessions. */
    _ENTRY(MAX_SESSIONS),	/* uint32_t. The maximum number of sessions
                                                         the TPM supports. */
    _ENTRY(CMK_RESTRICTION),	/* uint32_t TPM_Permanent_Data -> restrictDelegate */
    _ENTRY(DURATION),		/* A 3 element array of uint32_t values each
                                   denoting the duration value in microseconds
                                   of the duration of the three classes of
                                   commands: Small, Medium and Long in the
                                   following in this order: SMALL_DURATION,
                                   MEDIUM_DURATION, LONG_DURATION */
    _ENTRY(ACTIVE_COUNTER),	/* TPM_COUNT_ID. The id of the current
                                    counter. 0xff..ff if no counter is active */
    _ENTRY(MAX_NV_AVAILABLE),	/* uint32_t. Deprecated.  The maximum number
                                   of NV space that can be allocated, MAY
                                   vary with time and circumstances.  This
                                   capability was not implemented
                                   consistently, and is replaced by
                                   TPM_NV_INDEX_TRIAL. */
    _ENTRY(INPUT_BUFFER)	/* uint32_t. no. bytes in input buffer */
};
static size_t nTPM_CAP_PROP_ = sizeof(TPM_CAP_PROP_) / sizeof(TPM_CAP_PROP_[0]);
#undef	_ENTRY

#define _ENTRY(_v)      { TPM_PID_##_v, #_v }
static KEY TPM_PID_[] = {
    _ENTRY(OIAP),
    _ENTRY(OSAP),
    _ENTRY(ADIP),
    _ENTRY(ADCP),
    _ENTRY(OWNER),
    _ENTRY(DSAP),
    _ENTRY(TRANSPORT),
};
static size_t nTPM_PID_ = sizeof(TPM_PID_) / sizeof(TPM_PID_[0]);
#undef	_ENTRY

#define _ENTRY(_v)      { TPM_ALG_##_v, #_v }
static KEY TPM_ALG_[] = {
    _ENTRY(RSA),
  { 0x2, "DES" },	/* XXX deprecated */
  { 0x3, "3DES" },	/* XXX deprecated */
    _ENTRY(SHA),
    _ENTRY(HMAC),
    _ENTRY(AES128),
    _ENTRY(MGF1),
    _ENTRY(AES192),
    _ENTRY(AES256),
    _ENTRY(XOR)
};
static size_t nTPM_ALG_ = sizeof(TPM_ALG_) / sizeof(TPM_ALG_[0]);
#undef	_ENTRY

/* --- 5.8 TPM_KEY_USAGE rev 101 */
#define _ENTRY(_v)      { TPM_KEY_##_v, #_v }
static KEY TPM_KEY_[] = {
    _ENTRY(UNINITIALIZED),
    _ENTRY(SIGNING),
    _ENTRY(STORAGE),
    _ENTRY(IDENTITY),
    _ENTRY(AUTHCHANGE),
    _ENTRY(BIND),
    _ENTRY(LEGACY),
    _ENTRY(MIGRATE)
};
static size_t nTPM_KEY_ = sizeof(TPM_KEY_) / sizeof(TPM_KEY_[0]);
#undef	_ENTRY

/* --- 5.8.1 TPM_ENC_SCHEME Mandatory Key Usage Schemes rev 99 */
#define _ENTRY(_i, _v)      { TPM_ES_##_i, _v }
static KEY TPM_ES_[] = {
    _ENTRY(NONE, "NONE"),
    _ENTRY(RSAESPKCSv15, "RSA PKCS v1.5"),
    _ENTRY(RSAESOAEP_SHA1_MGF1, "RSA OAEP SHA1 MGF1"),
    _ENTRY(SYM_CTR, "SYM CTR"),
    _ENTRY(SYM_OFB, "SYM OFB"),
};
static size_t nTPM_ES_ = sizeof(TPM_ES_) / sizeof(TPM_ES_[0]);
#undef	_ENTRY

/* --- 5.8.1 TPM_SIG_SCHEME Mandatory Key Usage Schemes rev 99 */
#define _ENTRY(_i, _v)      { TPM_SS_##_i, _v }
static KEY TPM_SS_[] = {
    _ENTRY(NONE, "NONE"),
    _ENTRY(RSASSAPKCS1v15_SHA1,	"RSA PKCS1 v1.5 SHA1"),
    _ENTRY(RSASSAPKCS1v15_DER,	"RSA PKCS1 v1.5 DER"),
    _ENTRY(RSASSAPKCS1v15_INFO,	"RSA PKCS1 v1.5 INFO"),
};
static size_t nTPM_SS_ = sizeof(TPM_SS_) / sizeof(TPM_SS_[0]);
#undef	_ENTRY

/* TPM_PERMANENT_FLAGS */
#define _ENTRY(_v)      { TPM_PF_##_v, #_v }
static KEY TPM_PF_[] = {
    _ENTRY(DISABLE),
    _ENTRY(OWNERSHIP),
    _ENTRY(DEACTIVATED),
    _ENTRY(READPUBEK),
    _ENTRY(DISABLEOWNERCLEAR),
    _ENTRY(ALLOWMAINTENANCE),
    _ENTRY(PHYSICALPRESENCELIFETIMELOCK),
    _ENTRY(PHYSICALPRESENCEHWENABLE),
    _ENTRY(PHYSICALPRESENCECMDENABLE),
    _ENTRY(CEKPUSED),
    _ENTRY(TPMPOST),
    _ENTRY(TPMPOSTLOCK),
    _ENTRY(FIPS),
    _ENTRY(OPERATOR),
    _ENTRY(ENABLEREVOKEEK),
    _ENTRY(NV_LOCKED),
    _ENTRY(READSRKPUB),
    _ENTRY(TPMESTABLISHED),
    _ENTRY(MAINTENANCEDONE),
    _ENTRY(DISABLEFULLDALOGICINFO)
};
static size_t nTPM_PF_ = sizeof(TPM_PF_) / sizeof(TPM_PF_[0]);
#undef	_ENTRY

/* TPM_STCLEAR_FLAGS */
#define _ENTRY(_v)      { TPM_SF_##_v, #_v }
static KEY TPM_SF_[] = {
    _ENTRY(DEACTIVATED),
    _ENTRY(DISABLEFORCECLEAR),
    _ENTRY(PHYSICALPRESENCE),
    _ENTRY(PHYSICALPRESENCELOCK),
    _ENTRY(BGLOBALLOCK)
};
static size_t nTPM_SF_ = sizeof(TPM_SF_) / sizeof(TPM_SF_[0]);
#undef	_ENTRY

/* TPM_STANY_FLAGS */
#define _ENTRY(_v)      { TPM_AF_##_v, #_v }
static KEY TPM_AF_[] = {
    _ENTRY(POSTINITIALISE),
    _ENTRY(LOCALITYMODIFIER),
    _ENTRY(TRANSPORTEXCLUSIVE),
    _ENTRY(TOSPRESENT)
};
static size_t nTPM_AF_ = sizeof(TPM_AF_) / sizeof(TPM_AF_[0]);
#undef	_ENTRY

/* TPM_PERMANENT_DATA */
#define _ENTRY(_v)      { TPM_PD_##_v, #_v }
static KEY TPM_PD_[] = {
    _ENTRY(REVMAJOR),
    _ENTRY(REVMINOR),
    _ENTRY(TPMPROOF),
    _ENTRY(OWNERAUTH),
    _ENTRY(OPERATORAUTH),
    _ENTRY(MANUMAINTPUB),
    _ENTRY(ENDORSEMENTKEY),
    _ENTRY(SRK),
    _ENTRY(DELEGATEKEY),
    _ENTRY(CONTEXTKEY),
    _ENTRY(AUDITMONOTONICCOUNTER),
    _ENTRY(MONOTONICCOUNTER),
    _ENTRY(PCRATTRIB),
    _ENTRY(ORDINALAUDITSTATUS),
    _ENTRY(AUTHDIR),
    _ENTRY(RNGSTATE),
    _ENTRY(FAMILYTABLE),
  { TPM_DELEGATETABLE, "DELEGATEABLE" },
    _ENTRY(EKRESET),
    _ENTRY(LASTFAMILYID),
    _ENTRY(NOOWNERNVWRITE),
    _ENTRY(RESTRICTDELEGATE),
    _ENTRY(TPMDAASEED),
    _ENTRY(DAAPROOF)
};
static size_t nTPM_PD_ = sizeof(TPM_PD_) / sizeof(TPM_PD_[0]);
#undef	_ENTRY

/* TPM_STCLEAR_DATA */
#define _ENTRY(_v)      { TPM_SD_##_v, #_v }
static KEY TPM_SD_[] = {
    _ENTRY(CONTEXTNONCEKEY),
    _ENTRY(COUNTID),
    _ENTRY(OWNERREFERENCE),
    _ENTRY(DISABLERESETLOCK),
    _ENTRY(PCR),
    _ENTRY(DEFERREDPHYSICALPRESENCE)
};
static size_t nTPM_SD_ = sizeof(TPM_SD_) / sizeof(TPM_SD_[0]);
#undef	_ENTRY

/* TPM_STANY_DATA */
#define _ENTRY(_v)      { TPM_AD_##_v, #_v }
static KEY TPM_AD_[] = {
    _ENTRY(CONTEXTNONCESESSION),
    _ENTRY(AUDITDIGEST),
    _ENTRY(CURRENTTICKS),
    _ENTRY(CONTEXTCOUNT),
    _ENTRY(CONTEXTLIST),
    _ENTRY(SESSIONS)
};
static size_t nTPM_AD_ = sizeof(TPM_AD_) / sizeof(TPM_AD_[0]);
#undef	_ENTRY

/*==============================================================*/

#ifdef	DYING
static
int rpmtpmErr(rpmtpm tpm, const char * msg, uint32_t mask, uint32_t rc)
        /*@*/
{
    uint32_t err = rc & (mask ? mask : 0xffffffff);
    if (err || _rpmtpm_debug)
        fprintf (stderr, "*** TPM_%s rc %u: %s\n", msg, rc,
		(err ? TPM_GetErrMsg(rc) : "Success"));
    return rc;
}
#endif

static inline const char * TorF(int flag)
{
    return (flag ? "TRUE" : "FALSE");
}

static void showPermanentFlags(TPM_PERMANENT_FLAGS *pf, uint32_t size)
{
    printf("Permanent flags:\n");

    /* rev 62 + */
    printf("Disabled: %s\n", TorF(pf->disable));
    printf("Ownership: %s\n", TorF(pf->ownership));
    printf("Deactivated: %s\n", TorF(pf->deactivated));
    printf("Read Pubek: %s\n", TorF(pf->readPubek));
    printf("Disable Owner Clear: %s\n",  TorF(pf->disableOwnerClear));
    printf("Allow Maintenance: %s\n", TorF(pf->allowMaintenance));
    printf("Physical Presence Lifetime Lock: %s\n", TorF(pf->physicalPresenceLifetimeLock));
    printf("Physical Presence HW Enable: %s\n", TorF(pf->physicalPresenceHWEnable));
    printf("Physical Presence CMD Enable: %s\n",  TorF(pf->physicalPresenceCMDEnable));
    printf("CEKPUsed: %s\n",  TorF(pf->CEKPUsed));
    printf("TPMpost: %s\n", TorF(pf->TPMpost));
    printf("TPMpost Lock: %s\n",  TorF(pf->TPMpostLock));
    printf("FIPS: %s\n", TorF(pf->FIPS));
    printf("Operator: %s\n",  TorF(pf->tpmOperator));
    printf("Enable Revoke EK: %s\n", TorF(pf->enableRevokeEK));

    /* Atmel rev 85 only returns 18 BOOLs */
    if (size > 19) {
	printf("NV Locked: %s\n", TorF(pf->nvLocked));
	printf("Read SRK pub: %s\n", TorF(pf->readSRKPub));
	printf("TPM established: %s\n", TorF(pf->tpmEstablished));
    }

    /* rev 85 + */
    if (size > 20)
	printf("Maintenance done: %s\n", TorF(pf->maintenanceDone));

    /* rev 103 */
    if (size > 21)
	printf("Disable full DA logic info: %s\n", TorF(pf->disableFullDALogicInfo));
}

static void showVolatileFlags(TPM_STCLEAR_FLAGS * sf)
{
    printf("Volatile flags:\n");
    printf("Deactivated: %s\n", TorF(0 == sf->deactivated));
    printf("Disable ForceClear: %s\n", TorF(0 == sf->disableForceClear));
    printf("Physical Presence: %s\n", TorF(0 == sf->physicalPresence));
    printf("Physical Presence Lock: %s\n", TorF(sf->physicalPresenceLock));
    printf("bGlobal Lock: %s\n", TorF(sf->bGlobalLock));
}

static int prepare_subcap(rpmtpm tpm, uint32_t cap,
                          struct tpm_buffer *subcap,
                          uint32_t scap)
{
    int handled = 0;
    uint32_t ret;
    if (TPM_CAP_CHECK_LOADED == cap) {
	struct keydata k = {};
	handled = 1;
	k.keyFlags = 0;
	k.keyUsage = TPM_KEY_LEGACY;
	k.pub.algorithmParms.algorithmID = scap;
	k.pub.algorithmParms.encScheme = TPM_ES_NONE;
	k.pub.algorithmParms.sigScheme = TPM_SS_RSASSAPKCS1v15_INFO;
	k.pub.algorithmParms.u.rsaKeyParms.keyLength = 2048   ;      /* RSA modulus size 2048 bits */
	k.pub.algorithmParms.u.rsaKeyParms.numPrimes = 2;            /* required */
	k.pub.algorithmParms.u.rsaKeyParms.exponentSize   = 0;       /* RSA exponent - default 0x010001 */
	k.pub.pubKey.keyLength = 0;       /* key not specified here */
	k.pub.pcrInfo.size = 0;           /* no PCR's used at this time */
	ret = rpmtpmErr(tpm, "WriteKeyInfo", ERR_MASK,
		TPM_WriteKeyInfo(subcap, &k));
    }
    return handled;
}

static unsigned int
countOnes(unsigned char l)
{
    unsigned char mask = 0x1;
    unsigned int c = 0;
    while (mask) {
        if (l & mask)
	    c++;
        mask <<= 1;
    }
    return c;
}

static unsigned int
matchPattern(unsigned char l, unsigned char pattern, unsigned char bits)
{
    int shifted = 0;
    unsigned char mask = 1;
    int c;
    unsigned int ctr = 0;
    for (c = 1; c < bits; c++) {
	mask <<= 1;
	mask |= 1;
    }
    while (shifted < 8) {
	if ((l & mask) == pattern)
	    ctr++;
	l >>= bits;
	shifted += bits;
    }
    return ctr;
}

/*==============================================================*/

static int rpmtpmGetCapabilities(rpmtpm tpm)
{
    int ec = -1;		/* assume failure */

    uint32_t ret;
    STACK_TPM_BUFFER(resp);
    int jx;
    STACK_TPM_BUFFER(subcap);
    int i;
	
    for (jx = 0; (int) matrx[jx].cap != -1; jx++) {
	if (cap == matrx[jx].cap)
	    break;
    }
    if ((int) matrx[jx].cap == -1) {
	printf("Unknown or unsupported capability!\n");
	goto exit;
    }

    subcap.used = 0;
    if (matrx[jx].subcap_size > 0) {
	if ((int) scap == -1) {
	    printf("Need subcap parameter for this capability!\n");
	    goto exit;
	}
	if (prepare_subcap(tpm, cap, &subcap, scap) == 0) {
	    if (matrx[jx].subcap_size == 2) {
		STORE16(subcap.buffer, 0, scap);
		subcap.used = 2;
	    } else if (matrx[jx].subcap_size >= 4) {
		STORE32(subcap.buffer, 0, scap);
		subcap.used = 4;
	    }
	}
    }

#if 0
    /* This was for VTPM extensions and needs retest */
    if (cap == TPM_CAP_MFR) {
	int idx2 = 0;
	while ((int) mfr_matrix[idx2].cap != -1) {
	    if (mfr_matrix[idx2].cap == scap) {
		break;
	    }
	    idx2++;
	}
	if (mfr_matrix[idx2].subcap_size > 0) {
	    uint32_t used = subcap.used + mfr_matrix[idx2].subcap_size;
	    while (subcap.used < used) {
		if (argc <= nxtarg) {
		    printf("Need one more parameter for this "
			   "capability!\n");
		    exit(-1);
		}
		if (!strncmp("0x", argv[nxtarg], 2)) {
		    sscanf(argv[nxtarg], "%x", &sscap);
		} else {
		    sscanf(argv[nxtarg], "%d", &sscap);
		}
		nxtarg++;
		if (2 == matrx[jx].subcap_size) {
		    STORE16(subcap.buffer, subcap.used, sscap);
		    subcap.used += 2;
		} else if (matrx[jx].subcap_size >= 4) {
		    STORE32(subcap.buffer, subcap.used, sscap);
		    subcap.used += 4;
		}
	    }
	}
    }
#endif

    if (sikeyhandle == 0) {
	ret = rpmtpmErr(tpm, "GetCapability", 0,
		TPM_GetCapability(cap, &subcap, &resp));

	if (ret) {
	    printf("TPM_GetCapability returned %s.\n", TPM_GetErrMsg(ret));
	    ec = ret;
	    goto exit;
	}
    } else {
	unsigned char antiReplay[TPM_HASH_SIZE];
	unsigned char signature[2048];
	uint32_t signaturelen = sizeof(signature);
	pubkeydata pubkey;
	RSA *rsa;
	unsigned char sighash[TPM_HASH_SIZE];
	unsigned char *buffer = NULL;
	unsigned char *sigkeyhashptr = NULL;
	unsigned char sigkeypasshash[TPM_HASH_SIZE];

	if (sikeypass) {
	    TSS_sha1(sikeypass, strlen(sikeypass), sigkeypasshash);
	    sigkeyhashptr = sigkeypasshash;
	}

	TSS_gennonce(antiReplay);

	ret = rpmtpmErr(tpm, "GetPubKey", 0,
		TPM_GetPubKey(sikeyhandle, sigkeyhashptr, &pubkey));

	if (ret) {
	    printf
		("Error while trying to access the signing key's public key.\n");
	    goto exit;
	}

	rsa = TSS_convpubkey(&pubkey);

	ret = rpmtpmErr(tpm, "GetCapabilitySigned", 0,
		TPM_GetCapabilitySigned(sikeyhandle,
				      sigkeyhashptr,
				      antiReplay,
				      cap,
				      &subcap,
				      &resp, signature, &signaturelen));

	if (ret) {
	    printf("TPM_GetCapabilitySigned returned %s.\n",
		   TPM_GetErrMsg(ret));
	    ec = ret;
	    goto exit;
	}

	buffer = xmalloc(resp.used + TPM_NONCE_SIZE);
	memcpy(&buffer[0], resp.buffer, resp.used);
	memcpy(&buffer[resp.used], antiReplay, TPM_NONCE_SIZE);

	TSS_sha1(buffer, resp.used + TPM_NONCE_SIZE, sighash);
	free(buffer);

	ret = RSA_verify(NID_sha1,
			 sighash, TPM_HASH_SIZE,
			 signature, signaturelen, rsa);
	if (ret != 1) {
	    printf("Error: Signature verification failed.\n");
	    goto exit;
	}
    }

    if (resp.used == 0) {
	printf("Empty response.\n");
	ec = 0;
	goto exit;
    }

    if ((int) scap == -1)
	printf("Result for TPM_CAP_%s(0x%x) is : ",
		tblName(cap, TPM_CAP_, nTPM_CAP_), cap);
    else
    if (cap == TPM_CAP_ALG
     || cap == TPM_CAP_CHECK_LOADED
     || cap == TPM_CAP_TRANS_ALG
     || cap == TPM_CAP_AUTH_ENCRYPT)
	printf("Result for TPM_CAP_%s(0x%x) %s(0x%x) is : ",
		tblName(cap, TPM_CAP_, nTPM_CAP_), cap,
		tblName(scap, TPM_ALG_, nTPM_ALG_), scap);
    else if (cap == TPM_CAP_PID)
	printf("Result for TPM_CAP_%s(0x%x) %s(0x%x) is : ",
		tblName(cap, TPM_CAP_, nTPM_CAP_), cap,
		tblName(scap, TPM_PID_, nTPM_PID_), scap);
    else
    if (cap == TPM_CAP_PROPERTY
     || cap == TPM_CAP_FLAG)
	printf("Result for TPM_CAP_%s(0x%x) %s(0x%x) is : ",
		tblName(cap, TPM_CAP_, nTPM_CAP_), cap,
		tblName(scap, TPM_CAP_PROP_, nTPM_CAP_PROP_), scap);
    else if (cap == TPM_CAP_TRANS_ES)
	printf("Result for TPM_CAP_%s(0x%x) %s(0x%x) is : ",
		tblName(cap, TPM_CAP_, nTPM_CAP_), cap,
		tblName(scap, TPM_ES_, nTPM_ES_), scap);
    else
	printf("Result for TPM_CAP_%s(0x%x), subcapability 0x%x is : ",
		tblName(cap, TPM_CAP_, nTPM_CAP_), cap, scap);

    switch (matrx[jx].result_size) {
    case TYPE_BOOL:
	printf("%s", TorF(resp.buffer[0]));
	break;
    case TYPE_UINT32:
	{
	    uint32_t rsp = LOAD32(resp.buffer, 0);
	    printf("0x%08X  = %d\n", rsp, rsp);
	}   break;
    case TYPE_UINT32_ARRAY:
	printf("\n");
	for (i = 0; (i + 3) < (int) resp.used; i += 4) {
	    uint32_t rsp = LOAD32(resp.buffer, i);
	    if (cap == TPM_CAP_NV_LIST)
		/* don't zero extend, grep needs the exact value for test suite */
		printf("%d. Index : %d = 0x%x.\n", (i / 4) + 1, rsp, rsp);
	    else if (TPM_CAP_KEY_HANDLE == cap)
		printf("%d. keyhandle : %d.\n", (i / 4) + 1, rsp);
	    else
		printf("%d. item : %d.\n", (i / 4) + 1, rsp);
	}
	break;
    case TYPE_STRUCTURE:
	switch (cap) {
	case TPM_CAP_FLAG:
	    {
		if (scap == TPM_CAP_FLAG_PERMANENT) {
		    TPM_PERMANENT_FLAGS pf;
		    STACK_TPM_BUFFER(tb);
		    TSS_SetTPMBuffer(&tb, resp.buffer, resp.used);
		    ret = rpmtpmErr(tpm, "ReadPermanentFlags", ERR_MASK,
				TPM_ReadPermanentFlags(&tb, 0, &pf, resp.used));
		    if ((ret & ERR_MASK) != 0 || ret > resp.used) {
			printf("ret=%x, responselen=%d\n", ret, resp.used);
			printf("Error parsing response!\n");
			goto exit;
		    }
		    printf("\n");
		    showPermanentFlags(&pf, resp.used);
		} else if (scap == TPM_CAP_FLAG_VOLATILE) {
		    TPM_STCLEAR_FLAGS sf;
		    STACK_TPM_BUFFER(tb);
		    TSS_SetTPMBuffer(&tb, resp.buffer, resp.used);
		    ret = rpmtpmErr(tpm, "ReadSTClearFlags", ERR_MASK,
				TPM_ReadSTClearFlags(&tb, 0, &sf));
		    if ((ret & ERR_MASK) || ret > resp.used) {
			printf("ret=%x, responselen=%d\n", ret, resp.used);
			printf("Error parsing response!\n");
			goto exit;
		    }
		    printf("\n");
		    showVolatileFlags(&sf);
		}
	    }
	    break;

	case TPM_CAP_KEY_HANDLE:
	    {
		uint16_t num = LOAD16(resp.buffer, 0);
		uint32_t handle;
		printf("\n");
		for (i = 0; i < (int) num; i++) {
		    handle = LOAD32(resp.buffer, 4 * i + 2);
		    printf("%d. handle: 0x%08X\n", i, handle);
		}
	    }
	    break;
	case TPM_CAP_NV_INDEX:
	    {
		TPM_NV_DATA_PUBLIC ndp;
		STACK_TPM_BUFFER(tb);
		TSS_SetTPMBuffer(&tb, resp.buffer, resp.used);
		ret = rpmtpmErr(tpm, "ReadNVDataPublic", ERR_MASK,
				TPM_ReadNVDataPublic(&tb, 0, &ndp));
		if (ret & ERR_MASK) {
		    printf("Could not deserialize the TPM_NV_DATA_PUBLIC structure.\n");
		    goto exit;
		}
		printf("permission.attributes : %08X\n",
			   (unsigned int) ndp.permission.attributes);
		printf("ReadSTClear           : %02X\n",
			   ndp.bReadSTClear);
		printf("WriteSTClear          : %02X\n",
			   ndp.bWriteSTClear);
		printf("WriteDefine           : %02X\n",
			   ndp.bWriteDefine);
		printf("dataSize              : %08X = %d",
			   (unsigned int) ndp.dataSize,
			   (unsigned int) ndp.dataSize);
	    }
	    break;
	case TPM_CAP_HANDLE:
	    {
		uint16_t num = LOAD16(resp.buffer, 0);
		for (i = 0; i < (int)num; i++) {
		    uint32_t handle = LOAD32(resp.buffer, 4 * i + sizeof(num));
		    printf("%02d. 0x%08X\n", i, handle);
		}
	    }
	    break;
	case TPM_CAP_VERSION_VAL:
	    {
		TPM_CAP_VERSION_INFO cvi;
		STACK_TPM_BUFFER(tb);
		TSS_SetTPMBuffer(&tb, resp.buffer, resp.used);
		ret = rpmtpmErr(tpm, "ReadCapVersionInfo", ERR_MASK,
				TPM_ReadCapVersionInfo(&tb, 0, &cvi));
		if (ret & ERR_MASK) {
		    printf ("Could not read the version info structure.\n");
		    goto exit;
		}

		printf("\n");
		printf("major      : 0x%02X\n", cvi.version.major);
		printf("minor      : 0x%02X\n", cvi.version.minor);
		printf("revMajor   : 0x%02X\n", cvi.version.revMajor);
		printf("revMinor   : 0x%02X\n", cvi.version.revMinor);
		printf("specLevel  : 0x%04X\n", cvi.specLevel);
		printf("errataRev  : 0x%02X\n", cvi.errataRev);

		printf("VendorID   : ");
		for (i = 0; i < 4; i++)
		    printf("%02X ", cvi.tpmVendorID[i]);
		printf("\n");
		/* Print vendor ID in text if printable */
		for (i = 0; i < 4; i++) {
		    if (isprint(cvi.tpmVendorID[i])) {
			if (i == 0)
			    printf("VendorID   : ");
			printf("%c", cvi.tpmVendorID[i]);
		    } else
			break;
		}
		printf("\n");

		printf("[not displaying vendor specific information]\n");
	    }
	    break;
#if 0		/* kgold: I don't think these are valid cap values */
	case TPM_CAP_FLAG_PERMANENT:
	    {
		TPM_PERMANENT_FLAGS pf;
		STACK_TPM_BUFFER(tb);
		TSS_SetTPMBuffer(&tb, resp.buffer, resp.used);

		if (resp.used == 21)
		    ret = TPM_ReadPermanentFlagsPre103(&tb, 0, &pf);
		else
		    ret = TPM_ReadPermanentFlags(&tb, 0, &pf);

		if ((ret & ERR_MASK) != 0 || ret > resp.used) {
		    printf("ret=%x, responselen=%d\n", ret, resp.used);
		    printf("Error parsing response!\n");
		    goto exit;
		}

		printf("\n");
		showPermanentFlags(&pf, resp.used);
	    }
	    break;

	case TPM_CAP_FLAG_VOLATILE:
	    {
		TPM_STCLEAR_FLAGS sf;
		STACK_TPM_BUFFER(tb);
		TSS_SetTPMBuffer(&tb, resp.buffer, resp.used);
		ret = TPM_ReadSTClearFlags(&tb, 0, &sf);
		if ((ret & ERR_MASK) != 0 || ret > resp.used) {
		    printf("ret=%x, responselen=%d\n", ret, resp.used);
		    printf("Error parsing response!\n");
		    goto exit;
		}

		printf("\n");
		showVolatileFlags(&sf);
	    }
	    break;
#endif
	case TPM_CAP_DA_LOGIC:
	    {
		TPM_BOOL lim = FALSE;
		TPM_DA_INFO dainfo;
		TPM_DA_INFO_LIMITED dainfo_lim;
		STACK_TPM_BUFFER(tb);
		TSS_SetTPMBuffer(&tb, resp.buffer, resp.used);
		ret = rpmtpmErr(tpm, "ReadDAInfo", ERR_MASK,
				TPM_ReadDAInfo(&tb, 0, &dainfo));
		if ((ret & ERR_MASK) != 0 || ret > resp.used) {
		    ret = rpmtpmErr(tpm, "ReadDAInfoLimited", ERR_MASK,
				TPM_ReadDAInfoLimited(&tb, 0, &dainfo_lim));
		    if ((ret & ERR_MASK) != 0 || ret > resp.used) {
			printf("ret=%x, responselen=%d\n", ret,
				   resp.used);
			printf("Error parsing response!\n");
			goto exit;
		    }
		    lim = TRUE;
		}

		printf("\n");
		if (lim) {
		    printf("State      : %d\n", dainfo_lim.state);
		    printf("Actions    : 0x%08x\n",
			       dainfo_lim.actionAtThreshold.actions);

		    for (i = 0; i < (int)dainfo_lim.vendorData.size; i++)
			printf("%02x ", (dainfo_lim.vendorData.buffer[i]&0xff));
		} else {
		    printf("State              : %d\n", dainfo.state);
		    printf("currentCount       : %d\n",
			       dainfo.currentCount);
		    printf("thresholdCount     : %d\n",
			       dainfo.thresholdCount);
		    printf("Actions            : 0x%08x\n",
			       dainfo.actionAtThreshold.actions);
		    printf("actionDependValue  : %d\n",
			       dainfo.actionDependValue);

#if 0
		    for (i = 0; i < (int)dainfo_lim.vendorData.size; i++)
			printf("%02x ", (dainfo_lim.vendorData.buffer[i]&0xff));
#endif
		}
	    }
	    break;
	}
	break;
    case TYPE_VARIOUS:
	switch (cap) {

	case TPM_CAP_MFR:
	    switch (scap) {
	    case TPM_CAP_PROCESS_ID:
		printf("%d\n", LOAD32(resp.buffer, 0));
		break;
	    }
	    break;		/* TPM_CAP_MFR */

	default:
	    switch (scap) {
	    case TPM_CAP_PROP_OWNER:		/* booleans */
	    case TPM_CAP_PROP_DAA_INTERRUPT:
		printf("%s", TorF(resp.buffer[0]));
		break;
#if 0
	    case TPM_CAP_PROP_TIMEOUTS:
#endif
	    case TPM_CAP_PROP_TIS_TIMEOUT:	/* array of 4 UINTs */
		for (i = 0; i < 4; i++)
		    printf("%d ", LOAD32(resp.buffer, 4 * i));
		break;
	    case TPM_CAP_PROP_STARTUP_EFFECT:
	      {
		TPM_STARTUP_EFFECTS se = 0;
		ret = rpmtpmErr(tpm, "ReadStartupEffects", ERR_MASK,
				TPM_ReadStartupEffects(resp.buffer, &se));
		if ((ret & ERR_MASK) != 0) {
		    printf("Could not read startup effects structure.\n");
		    goto exit;
		}
		printf("0x%08X=%d\n", (unsigned int) se, (unsigned int) se);
		printf("\n");
		printf("Startup effects:\n");
		printf("Effect on audit digest: %s\n",
			(se & (1 << 7)) ? "none" : "active");
		printf("Audit Digest on TPM_Startup(ST_CLEAR): %s\n",
			(se & (1 << 6)) ? "set to NULL" : "not set to NULL");

		printf("Audit Digest on TPM_Startup(any)     : %s\n",
			(se & (1 << 5)) ? "set to NULL" : "not set to NULL");
		printf("TPM_RT_KEY resource initialized on TPM_Startup(ST_ANY)     : %s\n",
			(se & (1 << 4)) ? "yes" : "no");
		printf("TPM_RT_AUTH resource initialized on TPM_Startup(ST_STATE)  : %s\n",
			(se & (1 << 3)) ? "yes" : "no");
		printf("TPM_RT_HASH resource initialized on TPM_Startup(ST_STATE)  : %s\n",
			(se & (1 << 2)) ? "yes" : "no");
		printf("TPM_RT_TRANS resource initialized on TPM_Startup(ST_STATE) : %s\n",
			(se & (1 << 1)) ? "yes" : "no");
		printf("TPM_RT_CONTEXT session initialized on TPM_Startup(ST_STATE): %s",
			(se & (1 << 0)) ? "yes" : "no");
	      }	break;
	    case TPM_CAP_PROP_DURATION:		/* array of 3 UINTs */
		for (i = 0; i < 3; i++)
		    printf("%d ", LOAD32(resp.buffer, 4 * i));
		break;
	    case TPM_CAP_PROP_ACTIVE_COUNTER:
	      {	uint32_t val = LOAD32(resp.buffer, 0);
		printf("0x%08X=%d", val, val);
		if (val == 0xffffffff)
		    printf(" (no counter is active)");
	      }	break;
	    default:	/* single UINT32 */
	      {	uint32_t val = LOAD32(resp.buffer, 0);
		printf("%u=0x%08X", val, val);
	      }	break;
	    }
	}
	printf("\n");
	break;
    }

    printf("\n");
    ec = 0;

exit:
    return ec;
}

static int rpmtpmSetCapabilities(rpmtpm tpm)
{
    int ec = -1;		/* assume failure */

    int ret;
    unsigned char *ownptr = NULL;
    unsigned char ownhash[TPM_HASH_SIZE];
    uint32_t capArea = 0;
    uint32_t subCap32 = -1;
    unsigned char serSubCap[4];
    uint32_t serSubCapLen = sizeof(serSubCap);
    STACK_TPM_BUFFER(serSetValue);
    uint32_t cap_index = 0;
    uint32_t scap_index = 0;
    const struct set_matrix *smatrix = NULL;
    TPM_PCR_SELECTION pcrsel;
    int jx;

    if (cap == 0xffffffff || scap == 0xffffffff || val == 0xffffffff) {
	printf("Missing argument\n");
	goto exit;
    }

    /*
     * Find the capability
     */
    while (choice[cap_index].name != NULL) {
	if (cap == choice[cap_index].capArea) {
	    smatrix = choice[cap_index].smatrix;
	    capArea = choice[cap_index].capArea;
	    break;
	}
	cap_index++;
    }
    if (smatrix == NULL) {
	printf("Invalid capability.\n");
	goto exit;
    }

    /*
     * Find the sub capability
     */
    while (smatrix[scap_index].name != NULL) {
	if (scap == smatrix[scap_index].subcap32) {
	    subCap32 = smatrix[scap_index].subcap32;
	    break;
	}
	scap_index++;
    }

    if ((int)subCap32 == -1) {
	printf("Invalid sub-capability.\n");
	goto exit;
    }
    STORE32(serSubCap, 0, subCap32);
    serSubCapLen = 4;

    switch (smatrix[scap_index].type) {
    case TYPE_BOOL:
	serSetValue.buffer[0] = val;
	serSetValue.used = 1;
	break;
    case TYPE_UINT32:
	STORE32(serSetValue.buffer, 0, val);
	serSetValue.used = 4;
	break;
    case TYPE_PCR_SELECTION:
	/* user provided the selection */
	memset(&pcrsel, 0x0, sizeof(pcrsel));
	jx = 0;
	while (val > 0) {
	    pcrsel.sizeOfSelect++;
	    pcrsel.pcrSelect[jx] = (uint8_t) val;
	    val >>= 8;
	    jx++;
	}
	ret = rpmtpmErr(tpm, "WritePCRSelection", ERR_MASK,
		TPM_WritePCRSelection(&serSetValue, &pcrsel));
	if (ret & ERR_MASK) {
	    printf("Error '%s' while serializing "
		   "TPM_PCR_SELECTION.\n", TPM_GetErrMsg(ret));
	    goto exit;
	}
	break;
    default:
	printf("Unkown type of value to set.\n");
	goto exit;
	break;
    }

    if (ownerpass) {
	TSS_sha1(ownerpass, strlen(ownerpass), ownhash);
	ownptr = ownhash;
    }

    ret = rpmtpmErr(tpm, "SetCapability", 0,
		TPM_SetCapability(capArea,
			    serSubCap, serSubCapLen,
			    &serSetValue, ownptr));

    if (ret) {
	printf("Error %s from SetCapability.\n", TPM_GetErrMsg(ret));
	ec = ret;
	goto exit;
    }

    printf("Capability was set successfully.\n");
    ec = 0;

exit:
    return ec;
}

/*==============================================================*/

static struct poptOption rpmtpmGetCapabilitiesOptionsTable[] = {
 { "cap", '\0', POPT_ARG_INT|POPT_ARGFLAG_ONEDASH,	&cap,	0,
	N_("get <capability>"),			N_(" <capability>") },
 { "scap", '\0', POPT_ARG_INT|POPT_ARGFLAG_ONEDASH,	&scap,	0,
	N_("get <sub-capability>"),		N_(" <sub-capability>") },
 { "hk", '\0', POPT_ARG_INT|POPT_ARGFLAG_ONEDASH,	&sikeyhandle,	0,
	N_("Use signing key <handle>"),		N_(" <handle>") },
#ifdef	DYING
 { "pwdk", '\0', POPT_ARG_STRING|POPT_ARGFLAG_ONEDASH,	&sikeypass,	0,
	N_("Use signing key <password>"),	N_(" <password>") },
#endif
  POPT_TABLEEND
};

static struct poptOption rpmtpmSetCapabilitiesOptionsTable[] = {
 { "cap", '\0', POPT_ARG_INT|POPT_ARGFLAG_ONEDASH,	&cap,	0,
	N_("set <capability>"),			N_(" <capability>") },
 { "scap", '\0', POPT_ARG_INT|POPT_ARGFLAG_ONEDASH,	&scap,	0,
	N_("set <sub-capability>"),		N_(" <sub-capability>") },
 { "val", '\0', POPT_ARG_INT|POPT_ARGFLAG_ONEDASH,	&val,	0,
	N_("set <value>"),		N_(" <value>") },
#ifdef	DYING
 { "pwdo", '\0', POPT_ARG_STRING|POPT_ARGFLAG_ONEDASH,	&ownerpass,	0,
	N_("Specify TPM owner <password>"),	N_(" <password>") },
#endif
  POPT_TABLEEND
};

static struct poptOption rpmtpmCreateKeyOptionsTable[] = {
 { "ok", '\0', POPT_ARG_STRING|POPT_ARGFLAG_ONEDASH,	&keyname,	0,
	N_("Specify the new <keyname>"),		N_(" <keyname>") },
 { "hp", '\0', POPT_ARG_INT|POPT_ARGFLAG_ONEDASH,	&parhandle,	0,
	N_("Specify the parent key hanle <pkeyhandle>"), N_(" <pkeyhandle>") },
 { "kt", '\0', POPT_ARG_STRING|POPT_ARGFLAG_ONEDASH|POPT_ARGFLAG_SHOW_DEFAULT,	&keytype,	0,
	N_("Specify the keytype"),		N_(" s|d|i|e|b|l|m") },
 { "pwdp", '\0', POPT_ARG_STRING|POPT_ARGFLAG_ONEDASH,	&parpass,	0,
	N_("Specify parent key <password>"),		N_(" <password>") },
#ifdef	DYING
 { "pwdk", '\0', POPT_ARG_STRING|POPT_ARGFLAG_ONEDASH,	&keypass,	0,
	N_("Specify new key <password>"),		N_(" <password>") },
#endif
 { "pwdm", '\0', POPT_ARG_STRING|POPT_ARGFLAG_ONEDASH,	&migpass,	0,
	N_("Specify migratable <password>"),		N_(" <password>") },
 { "sz", '\0', POPT_ARG_INT|POPT_ARGFLAG_ONEDASH|POPT_ARGFLAG_SHOW_DEFAULT,	&keysize,	0,
	N_("Specify no. bits in new key"),		N_(" <keysize>") },

 { "v1", '\0', POPT_ARG_INT|POPT_ARGFLAG_ONEDASH,	&use_oldversion, 1,
	N_("Set TPM_KEY instead of TPM_KEY12"),		NULL },
/* XXX optarg is "pkcsv15" */
 { "es", '\0', POPT_ARG_INT|POPT_ARGFLAG_ONEDASH,	&pkcsv15,	1,
	N_("XXX pkcsv15 = TRUE"),			NULL },
#ifdef	DYING
/* XXX FIXME:  <pcrnum>:<digest> split */
 { "ix", '\0', POPT_ARG_INT|POPT_ARGFLAG_ONEDASH,	&createkey_index, 0,
	N_("Used to wrap a key to values of PCR's"),	N_(" <pcrnum> <digest>") },
#endif

 { "vlong", '\0', POPT_ARG_VAL|POPT_ARGFLAG_ONEDASH,	&use_struct,	PCR_INFO_LONG,
	N_("Force usage of PCR_INFO_LONG"),	NULL },
/* XXX quote2 uses -vinfo not -addv */
 { "vinfo", '\0', POPT_ARG_VAL|POPT_ARGFLAG_ONEDASH,	&use_struct,	PCR_INFO,
	N_("Force usage of PCR_INFO"),	NULL },
  POPT_TABLEEND
};

static struct poptOption optionsTable[] = {

 { "pwdk", '\0', POPT_ARG_STRING|POPT_ARGFLAG_ONEDASH,	&keypass,	0,
	N_("Specify key <password>"),		N_(" <password>") },

/* XXX enableudit uses -p ? */
 { "pwdo", 'p', POPT_ARG_STRING|POPT_ARGFLAG_ONEDASH,	&ownerpass,	0,
	N_("Specify TPM owner <password>"),	N_(" <password>") },
 { "pwdc", '\0', POPT_ARG_STRING|POPT_ARGFLAG_ONEDASH,	&ctrpass,	0,
	N_("Specify counter <password>"),	N_(" <password>") },
 { "pwdd", '\0', POPT_ARG_STRING|POPT_ARGFLAG_ONEDASH,	&datpass,	0,
	N_("Specify data <password>"),	N_(" <password>") },

/* XXX FIXME:  <pcrnum>:<digest> split */
 { "ix", '\0', POPT_ARG_INT|POPT_ARGFLAG_ONEDASH,	&ix, 0,
	N_("Specify directory/PCR <index>"),	N_(" <index>") },

 { "en", '\0', POPT_ARG_VAL|POPT_ARGFLAG_ONEDASH,	&disabled_state, 1,
	N_("XXX OwnerSetDisable(FALSE)"),	NULL },

 { "if", '\0', POPT_ARG_STRING|POPT_ARGFLAG_ONEDASH,	&ifn,	0,
	N_("Specify <fn> of an input file"),	N_(" <fn>") },
 { "of", '\0', POPT_ARG_STRING|POPT_ARGFLAG_ONEDASH,	&ofn,	0,
	N_("Specify <fn> of an output file"),	N_(" <fn>") },
 { "ic", '\0', POPT_ARG_STRING|POPT_ARGFLAG_ONEDASH,	&msg,	0,
	N_("Specify <msg> to write into a DIR"),	N_(" <msg>") },

 { "rt", '\0', POPT_ARG_INT|POPT_ARGFLAG_ONEDASH,	&restype,	0,
	N_("Specify restource type <rt> for a context"),	N_(" <rt>") },
/* XXX counter_create uses -la asif -ix */
 { "la", '\0', POPT_ARG_STRING|POPT_ARGFLAG_ONEDASH,	&lbl,	0,
	N_("Specify <lbl> for a context"),	N_(" <lbl>") },
 { "ha", '\0', POPT_ARG_INT|POPT_ARGFLAG_ONEDASH,	&hahandle,	0,
	N_("Specify <handle> for a context"),	N_(" <handle>") },
 { "hk", '\0', POPT_ARG_INT|POPT_ARGFLAG_ONEDASH,	&hkhandle,	0,
	N_("Specify key <handle> for a context"),	N_(" <handle>") },
 { "all", '\0', POPT_ARG_VAL|POPT_ARGFLAG_ONEDASH,	&evict_all, 1,
	N_("Evict all keys"),	NULL },
 { "addversion", '\0', POPT_ARG_VAL|POPT_ARGFLAG_ONEDASH,	&add_version, 1,
	N_("XXX quote2 add version"),	NULL },
 { "keep", '\0', POPT_ARG_VAL|POPT_ARGFLAG_ONEDASH,	&load_keep, 1,
	N_("Keep resource handle when loading"),	NULL },
 { "bm", '\0', POPT_ARG_INT|POPT_ARGFLAG_ONEDASH,	&bitmask, 0,
	N_("XXX CMK_SetRestrictions(<bitmask>)"),	N_(" <bitmask>") },
 { "bn", '\0', POPT_ARG_INT|POPT_ARGFLAG_ONEDASH,	&bitname, 0,
	N_("XXX KeyControlOwner(<bitname>)"),	N_(" <bitname>") },
 { "bv", '\0', POPT_ARG_INT|POPT_ARGFLAG_ONEDASH,	&bitname, 0,
	N_("XXX KeyControlOwner(<bitvalue>)"),	N_(" <bitvalue>") },

 { "ordinal", 'o', POPT_ARG_INT|POPT_ARGFLAG_ONEDASH,	&ix, 0,
	N_("Specify <ordinal> to audit"),	N_(" <ordinal>") },
 { NULL, 'd', POPT_ARG_VAL|POPT_ARGFLAG_ONEDASH,	&disabled_audit, 1,
	N_("Disable audit"),	NULL },

  { NULL, (char)-1, POPT_ARG_INCLUDE_TABLE, rpmtpmGetCapabilitiesOptionsTable, 0,
	N_("\
======\n\
Usage: getcapability [options] -cap <capability (hex)>\n\
[-scap <sub cap (hex)>] [-scapd <sub cap (dec)>]\n\
[-hk signing key handle] [-pwdk signing key password]\n\
\n\
Possible options are:\n\
  -hk :   handle of a signing key if a signed response is requested\n\
  -pwdk : password of that signing key (if it needs one)\n\
"), NULL },

  { NULL, (char)-1, POPT_ARG_INCLUDE_TABLE, rpmtpmSetCapabilitiesOptionsTable, 0,
	N_("\
======\n\
Usage: setcapability [options] <capability (hex)> <sub cap (hex)> <value (hex)>\n\
\n\
Possible options are:\n\
  -pwdo : password of the TPM owner\n\
"), NULL },

  { NULL, (char)-1, POPT_ARG_INCLUDE_TABLE, rpmtpmCreateKeyOptionsTable, 0,
	N_("\
======\n\
Usage: createkey [<options>] -ok <keyname> -hp <pkeyhandle>\n\
\n\
   Where the arguments are...\n\
    <keyname>    is the new key name\n\
    <pkeyhandle> is the parent key handle in hex\n\
\n\
   Where the <options> are...\n\
    -kt s/d/i| e | b | l | m  keytype is s for signing (default)\n\
                                        d for signing using DER\n\
                                        i for signing using INFO sig. scheme\n\
                                        e for encryption(storage)\n\
                                        b for binding, l for legacy\n\
                                        m for migration\n\
    -pwdp <parpass>   to specify parent key use password\n\
    -pwdk <keypass>   to specify new key use password\n\
    -pwdm <migpass>   to specify new key is migratable, and specify migration password\n\
    -sz <keysize>     to specify the size of key to create; default is 2048\n\
    -v                to specify verbose output\n\
    -v1               use TPM_KEY instead of TPM_KEY12\n\
    -es pkcsv15       create a key that uses PKCSv15 encryption scheme\n\
    -ix <pcr num> <digest>    used to wrap a key to values of PCRs\n\
    -vlong, -vinfo    force usage of PCR_INFO_LONG or PCR_INFO structure\n\
    -h                print usage information (this message)\n\
"), NULL },

 { NULL, '\0', POPT_ARG_INCLUDE_TABLE, rpmioAllPoptTable, 0,
        N_("Common options:"), NULL },

  POPT_AUTOALIAS
  POPT_AUTOHELP

  POPT_TABLEEND
};

int main(int argc, char *argv[])
{
    rpmtpm tpm = _tpm;
    poptContext con;
    const char ** av = NULL;
    int ac;
    int ec = -1;		/* assume failure */

    con = rpmioInit(argc, argv, optionsTable);
    av = poptGetArgs(con);
    ac = argvCount(av);

    TPM_setlog(rpmIsVerbose() ? 1 : 0);

    if (ac == 1) {
/* --- bindfile */
/* --- calcfuturepcr */
/* --- certifykey */
/* --- certifyselftest */
/* --- chgauth */
/* --- chgtpmauth */
/* --- clearown */
	if (!strcmp(av[0], "clearown")) {
	    unsigned char ownhash[TPM_DIGEST_SIZE];
assert(ownerpass);	/* XXX FIXME */
	    TSS_sha1((unsigned char *)ownerpass, strlen(ownerpass), ownhash);
	    ec = rpmtpmErr(tpm, "OwnerClear", 0,
			TPM_OwnerClear(ownhash));
	} else
/* --- cmk_approvema */
/* --- cmk_createkey */
/* --- cmk_createticket */
/* --- cmk_loadmigrationblob */
/* --- cmk_migrate */
/* --- cmk_setrestrictions */
	if (!strcmp(av[0], "cmk_setrestrictions")) {
	    unsigned char ownhash[TPM_DIGEST_SIZE];
	    unsigned char * ownptr = NULL;
	    if (ownerpass) {
		TSS_sha1((unsigned char *)ownerpass, strlen(ownerpass), ownhash);
		ownptr = ownhash;
	    }
	    ec = rpmtpmErr(tpm, "CMK_SetRestrictions", 0,
			TPM_CMK_SetRestrictions(bitmask, ownptr));
	} else
/* --- counter_calc_incr */
	if (!strcmp(av[0], "counter_calc_incr")) {
	    unsigned char ctrValue[TPM_COUNTER_VALUE_SIZE];
	    int i;

	    if (ix == -1) {
		printf("Input parameter missing!\n");
		goto exit;
	    }

	    ec = rpmtpmErr(tpm, "ReadCounter", 0,
			TPM_ReadCounter(ix, NULL, ctrValue));
	    if (ec)
		goto exit;

	    /* Increment with carry. */
	    {	int j = sizeof(ctrValue)-1;
		ctrValue[j]++;
		for (i = 0; i < (j-1); i++) {
		    if (ctrValue[j-i])
			break;
		    ctrValue[j-i-1]++;
		}
	    }

	    printf("Value of incremented counter[%d]: ", ix);
	    rpmtpmDump(tpm, NULL, ctrValue, sizeof(ctrValue));
	} else
/* --- counter_create */
	if (!strcmp(av[0], "counter_create")) {
	    unsigned char ownhash[TPM_DIGEST_SIZE];
	    unsigned char ctrhash[TPM_DIGEST_SIZE];
	    uint32_t ctrId = 0;
	    unsigned char ctrValue[TPM_COUNTER_VALUE_SIZE];

	    if (ownerpass == NULL || ctrpass == NULL || ix == -1) {
		printf("Input parameter missing!\n");
		goto exit;
	    }

	    parhandle = 0;
	    TSS_sha1((unsigned char *)ownerpass, strlen(ownerpass), ownhash);
	    TSS_sha1((unsigned char *)ctrpass, strlen(ctrpass), ctrhash);

	    ec = rpmtpmErr(tpm, "CreateCounter", 0,
			TPM_CreateCounter(parhandle, ownhash, ix, ctrhash, &ctrId, ctrValue));
	    if (ec)
		goto exit;
	    printf("New counter id: %d\n", ctrId);
	    rpmtpmDump(tpm, NULL, ctrValue, sizeof(ctrValue));
	} else
/* --- counter_increment */
	if (!strcmp(av[0], "counter_increment")) {
	    unsigned char ctrhash[TPM_DIGEST_SIZE];
	    unsigned char ctrValue[TPM_COUNTER_VALUE_SIZE];

	    if (ctrpass == NULL || ix == -1) {
		printf("Input parameter missing!\n");
		goto exit;
	    }

	    TSS_sha1((unsigned char *)ctrpass, strlen(ctrpass), ctrhash);
	    ec = rpmtpmErr(tpm, "IncrementCounter", 0,
			TPM_IncrementCounter(ix, ctrhash, ctrValue));
	    if (ec)
		goto exit;

	    printf("Value of counter[%d]: ", ix);
	    rpmtpmDump(tpm, NULL, ctrValue, sizeof(ctrValue));
	} else
/* --- counter_read */
	if (!strcmp(av[0], "counter_calc_incr")) {
	    unsigned char ctrValue[TPM_COUNTER_VALUE_SIZE];

	    if (ix == -1) {
		printf("Input parameter missing!\n");
		goto exit;
	    }

	    ec = rpmtpmErr(tpm, "ReadCounter", 0,
			TPM_ReadCounter(ix, NULL, ctrValue));
	    if (ec)
		goto exit;

	    rpmtpmDump(tpm,"Value of the counter", ctrValue, sizeof(ctrValue));
	} else
/* --- counter_release */
	if (!strcmp(av[0], "counter_release")) {
	    unsigned char passhash[TPM_DIGEST_SIZE];

	    if ((ownerpass == NULL && ctrpass == NULL) || ix == -1) {
		printf("Input parameter missing!\n");
		goto exit;
	    }

	    if (ctrpass) {
		TSS_sha1((unsigned char *)ctrpass, strlen(ctrpass), passhash);
		ec = rpmtpmErr(tpm, "ReleaseCounter", 0,
			TPM_ReleaseCounter(ix, passhash));
	    } else
	    if (ownerpass) {
		TSS_sha1((unsigned char *)ownerpass, strlen(ownerpass), passhash);
		ec = rpmtpmErr(tpm, "ReleaseCounterOwner", 0,
			TPM_ReleaseCounterOwner(ix, passhash));
	    }
	    if (ec)
		goto exit;
	    printf("Successfully released the counter.\n");
	} else
/* --- createek */
	if (!strcmp(av[0], "createek")) {
	    uint32_t len = 0;
	    ec = rpmtpmErr(tpm, "CreateEndorsementKeyPair", 0,
			TPM_CreateEndorsementKeyPair(NULL, &len));
	} else
/* --- createkey */
#ifdef	NOTYET
	if (!strcmp(av[0], "createkey"))
	    ec = rpmtpmCreateKey(tpm);
	else
#endif
/* --- createkeydelegation */
/* --- createownerdelegation */
/* --- createrevek */
/* --- delegatemanage */
/* --- delegatereadtable */
/* --- dirread */
	if (!strcmp(av[0], "dirread")) {
	    unsigned char data[TPM_HASH_SIZE];

if (ix < 1) ix = 0;	/* XXX FIXME */
	    ec = rpmtpmErr(tpm, "DirRead", 0,
			TPM_DirRead(ix, data));
	    if (ec)
		goto exit;

	    printf("Content of DIR %d: ", tpm->ix);
	    rpmtpmDump(tpm, NULL, data, sizeof(data));

	} else
/* --- dirwrite */
	if (!strcmp(av[0], "dirwrite")) {
	    unsigned char ownhash[TPM_DIGEST_SIZE];
	    unsigned char msghash[TPM_DIGEST_SIZE];
assert(ix >= 0);	/* XXX FIXME */
assert(ownerpass);	/* XXX FIXME */
assert(msg);		/* XXX FIXME */
	    TSS_sha1((unsigned char *)ownerpass, strlen(ownerpass), ownhash);
	    TSS_sha1((unsigned char *)msg, strlen(msg), msghash);
	    ec = rpmtpmErr(tpm, "DirWriteAuth", 0,
			TPM_DirWriteAuth(ix, msghash, ownhash));
	} else
/* --- disableforceclear */
	if (!strcmp(av[0], "disableforceclear")) {
	    ec = rpmtpmErr(tpm, "DisableForceClear", 0,
			TPM_DisableForceClear());
	} else
/* --- disableownerclear */
	if (!strcmp(av[0], "disableownerclear")) {
	    unsigned char ownhash[TPM_DIGEST_SIZE];
assert(ownerpass);	/* XXX FIXME */
	    TSS_sha1((unsigned char *)ownerpass, strlen(ownerpass), ownhash);
	    ec = rpmtpmErr(tpm, "DisableOwnerClear", 0,
			TPM_DisableOwnerClear(ownhash));
	} else
/* --- disablepubek */
	if (!strcmp(av[0], "disablepubek")) {
	    unsigned char ownhash[TPM_DIGEST_SIZE];
assert(ownerpass);	/* XXX FIXME */
	    TSS_sha1((unsigned char *)ownerpass, strlen(ownerpass), ownhash);
	    ec = rpmtpmErr(tpm, "DisablePubekRead", 0,
			TPM_DisablePubekRead(ownhash));
	} else
/* --- dumpkey */
	/* XXX -ifn and TPM_ReadFile? */
/* --- enableaudit */
	if (!strcmp(av[0], "enableaudit")) {
	    unsigned char ownhash[TPM_DIGEST_SIZE];
	    if (ordinal < 0 || ownerpass == NULL) {
		printf("Missing mandatory parameter.\n");
		goto exit;
	    }
	    TSS_sha1((unsigned char *)ownerpass, strlen(ownerpass), ownhash);
	    ec = rpmtpmErr(tpm, "SetOrdinalAuditStatus", 0,
			TPM_SetOrdinalAuditStatus(ordinal, (disabled_audit ? FALSE : TRUE), ownhash));
	} else
/* --- evictkey */
	if (!strcmp(av[0], "evictkey")) {
	    if (evict_all) {
		STACK_TPM_BUFFER(response);
		ec = rpmtpmErr(tpm, "GetCapability", 0,
			TPM_GetCapability(TPM_CAP_KEY_HANDLE, NULL, &response));
		if (ec)
		    goto exit;
		else {
		    int listsize = LOAD16(response.buffer,0);
		    int offset;
		    int i;
		    for (i = 0, offset = 2; i < listsize; i++, offset += 4) {
			int ret;
			hkhandle = LOAD32(response.buffer,offset);
			ret = rpmtpmErr(tpm, "EvictKey", 0,
				TPM_EvictKey(hkhandle));
			if (ret == 0)
			    printf("Evicted key handle %08X\n", hkhandle);
		    }
		}
	    } else
	    if (hkhandle == 0xffffffff) {
		printf("Missing argument\n");
		goto exit;
	    } else
		ec = rpmtpmErr(tpm, "EvictKey", 0,
				TPM_EvictKey(hkhandle));
	} else
/* --- extend */
	if (!strcmp(av[0], "extend")) {
	    unsigned char msghash[TPM_DIGEST_SIZE];
	    unsigned char pcrhash[TPM_DIGEST_SIZE];

	    if (msg && ifn == NULL) {
		TSS_sha1((unsigned char *)msg, strlen(msg), msghash);
	    } else if (msg == NULL && ifn) {
		ec = TSS_SHAFile(ifn, msghash);
		if (ec) {
		    printf("Error %s from TSS_SHAFile.\n", TPM_GetErrMsg(ec));
		    goto exit;
		}
	    } else {
		printf("You must provide one option '-if' or '-ic'.\n");
		goto exit;
	    }

assert(ix >= 0);	/* XXX FIXME */

	    ec = rpmtpmErr(tpm, "Extend", 0,
			TPM_Extend(ix, msghash, pcrhash));
	    if (ec)
		goto exit;

	    printf("New value of PCR[%d]: ", ix);
	    rpmtpmDump(tpm, NULL, pcrhash, sizeof(pcrhash));
	} else
/* --- flushspecific */
	if (!strcmp(av[0], "flushspecific")) {
	    if (hahandle == 0xffffffff || restype == 0) {
		printf("Missing argument.\n");
		goto exit;
	    }
	    ec = rpmtpmErr(tpm, "FlushSpecific", 0,
			TPM_FlushSpecific(hahandle, restype));
	} else
/* --- forceclear */
	if (!strcmp(av[0], "forceclear")) {
	    ec = rpmtpmErr(tpm, "ForceClear", 0,
			TPM_ForceClear());
	} else
/* --- getauditdigest */
/* --- getauditdigestsigned */
/* --- getcapability */
	if (!strcmp(av[0], "getcapability"))
	    ec = rpmtpmGetCapabilities(tpm);
	else
/* --- getcontextcount */
	if (!strcmp(av[0], "getcontextcount")) {
	    unsigned char * mycontext = NULL;
	    uint32_t contextSize = 0;
	    TPM_CONTEXT_BLOB blob;
	    STACK_TPM_BUFFER(context);
assert(ifn);	/* XXX FIXME */
	    ec = rpmtpmErr(tpm, "ReadFile", ERR_MASK,
			TPM_ReadFile(ifn, &mycontext, &contextSize));
	    if ((ec & ERR_MASK))
		goto exit;
	    SET_TPM_BUFFER(&context, mycontext, contextSize);
	    ec = rpmtpmErr(tpm, "ReadContextBlob", ERR_MASK,
			TPM_ReadContextBlob(&context, 0, &blob));
	    if ((ec & ERR_MASK))
		goto exit;
	    printf("ContextCount: 0x%08X\n", blob.contextCount);
	    ec = 0;
	} else
/* --- getpubek */
	if (!strcmp(av[0], "getpubek")) {
	    unsigned char ownhash[TPM_DIGEST_SIZE];
	    pubkeydata pubek;
	    RSA *rsa;			/* OpenSSL format Public Key */
	    FILE *fp;			/* output file for public key */
	    EVP_PKEY *pkey = NULL;	/* OpenSSL public key */
	    int ret;
	    int i;

	    if (ownerpass) {
		TSS_sha1((unsigned char *)ownerpass, strlen(ownerpass), ownhash);
		ec = rpmtpmErr(tpm, "OwnerReadPubek", 0,
			TPM_OwnerReadPubek(ownhash, &pubek));
	    } else {
		ec = rpmtpmErr(tpm, "ReadPubek", 0,
			TPM_ReadPubek(&pubek));
	    }
	    if (ec)
		goto exit;

	    /* convert returned pubkey to OpenSSL and export to a file. */
	    rsa = TSS_convpubkey(&pubek);
	    if (rsa == NULL) {
		printf("Error from TSS_convpubkey\n");
		ec = -3;
		goto exit;
	    }
	    OpenSSL_add_all_algorithms();
	    pkey = EVP_PKEY_new();
	    if (pkey == NULL) {
		printf("Unable to create EVP_PKEY\n");
		ec = -4;
		goto exit;
	    }
	    ret = EVP_PKEY_assign_RSA(pkey, rsa);
	    fp = fopen("pubek.pem", "wb");
	    if (fp == NULL) {
		printf("Unable to create public key file\n");
		ec = -5;
		goto exit;
	    }
	    ret = PEM_write_PUBKEY(fp, pkey);
	    if (ret == 0) {
		printf("Unable to write public key file\n");
		ec = -6;
		goto exit;
	    }
	    printf("pubek.pem successfully written\n");
	    printf("Pubek keylength %d\nModulus:", pubek.pubKey.keyLength);
	    for(i = 0; i < (int)pubek.pubKey.keyLength; i++) {
		if((i%16) == 0)
		    printf("\n");
		printf("%02X ",pubek.pubKey.modulus[i]);
	    }
	    printf("\n");

	    fclose(fp);
	    fp = NULL;
	    EVP_PKEY_free(pkey);
	} else
/* --- getpubkey */
/* --- getticks */
	if (!strcmp(av[0], "getticks")) {
	    unsigned char tickbuffer[36];
	    TPM_CURRENT_TICKS ticks;
	    STACK_TPM_BUFFER(buffer);

	    ec = rpmtpmErr(tpm, "GetTicks", 0,
			TPM_GetTicks(tickbuffer));
	    if (ec)
		goto exit;

	    TSS_SetTPMBuffer(&buffer, tickbuffer, sizeof(tickbuffer));

#ifdef  NOTYET
	    ec = rpmtpmErr(tpm, "GetCurrentTicks", 0,
			TPM_GetCurrentTicks(&buffer, 0, &ticks));
	    if (ec)
		goto exit;
#else
	    TPM_GetCurrentTicks(&buffer, 0, &ticks);
#endif

	    printf(" Sec:         %d\n", (uint32_t)ticks.currentTicks.sec);
	    printf("uSec:         %d\n", (uint32_t)ticks.currentTicks.usec);
	    printf("tickRate:     %d\n", ticks.tickRate);
	} else
/* --- identity */
/* --- keycontrol */
	if (!strcmp(av[0], "keycontrol")) {
	    unsigned char ownhash[TPM_DIGEST_SIZE];
	    unsigned char keyhash[TPM_DIGEST_SIZE];
	    unsigned char * keyptr = NULL;
	    keydata key;

	    if (ownerpass == NULL || hkhandle == 0xffffffff || bitname == 0xffffffff) {
		printf("Missing argument\n");
		goto exit;
	    }

	    if (keypass) {
		TSS_sha1((unsigned char *)keypass, strlen(keypass), keyhash);
		keyptr = keyhash;
	    }
	    ec = rpmtpmErr(tpm, "GetPubKey", 0,
			TPM_GetPubKey(hkhandle, keyptr, &key.pub));
	    if (ec)
		goto exit;
	    TSS_sha1((unsigned char *)ownerpass, strlen(ownerpass), ownhash);
	    ec = rpmtpmErr(tpm, "KeyControlOwner", 0,
			TPM_KeyControlOwner(ownhash, hkhandle, &key, bitname, (bitvalue ? TRUE : FALSE)));
	} else
/* --- killmaintenancefeature */
	if (!strcmp(av[0], "killmaintenancefeature")) {
	    unsigned char ownhash[TPM_DIGEST_SIZE];
assert(ownerpass);	/* XXX FIXME */
	    TSS_sha1((unsigned char *)ownerpass, strlen(ownerpass), ownhash);
	    ec = rpmtpmErr(tpm, "KillMaintenanceFeature", 0,
			TPM_KillMaintenanceFeature(ownhash));
	} else
/* --- libtpm-config */
/* --- listkeys */
	if (!strcmp(av[0], "listkeys")) {
	    STACK_TPM_BUFFER(response);
	    uint32_t handle;
	    int listsize;
	    int offset;
	    int i;

	    ec = rpmtpmErr(tpm, "GetCapability", 0,
			TPM_GetCapability(TPM_CAP_KEY_HANDLE, NULL, &response));
	    if (ec)
		goto exit;

	    listsize = LOAD16(response.buffer, 0);
	    for (i = 0, offset = 2; i < listsize; i++, offset += 4) {
		handle = LOAD32(response.buffer, offset);
		printf("Key handle %02d %08x\n", i, handle);
	    }
	} else
/* --- loadauthcontext */
	if (!strcmp(av[0], "loadauthcontext")) {
	    unsigned char * mycontext = NULL;
	    uint32_t contextSize = 0;
	    uint32_t handle = 0;
assert(ifn);	/* XXX FIXME */
	    ec = rpmtpmErr(tpm, "ReadFile", ERR_MASK,
			TPM_ReadFile(ifn, &mycontext, &contextSize));
	    if ((ec & ERR_MASK))
		goto exit;
	    ec = rpmtpmErr(tpm, "LoadAuthContext", 0,
			TPM_LoadAuthContext(mycontext, contextSize, &handle));
	    if (ec)
		goto exit;
	    printf("New Handle = %08X\n", handle);
	} else
/* --- loadcontext */
	if (!strcmp(av[0], "loadcontext")) {
	    unsigned char * mycontext = NULL;
	    uint32_t contextSize = 0;
	    uint32_t handle = 0;
	    STACK_TPM_BUFFER(context);

	    if (ifn == NULL || hahandle == 0xffffffff) {
		printf("Missing argument.\n");
	    }
	    ec = rpmtpmErr(tpm, "ReadFile", ERR_MASK,
			TPM_ReadFile(ifn, &mycontext, &contextSize));
	    if ((ec & ERR_MASK))
		goto exit;
	    SET_TPM_BUFFER(&context, mycontext, contextSize);

	    ec = rpmtpmErr(tpm, "LoadContext", 0,
			TPM_LoadContext(hahandle, (load_keep ? TRUE : FALSE), &context, &handle));
	    if (ec)
		goto exit;

	    printf("New Handle = 0x%08X\n", handle);
	} else
/* --- loadkey */
/* --- loadkeycontext */
	if (!strcmp(av[0], "loadkeycontext")) {
	    unsigned char * mycontext = NULL;
	    uint32_t contextSize = 0;
	    uint32_t handle = 0;
	    STACK_TPM_BUFFER(context);
assert(ifn);	/* XXX FIXME */
	    ec = rpmtpmErr(tpm, "ReadFile", ERR_MASK,
			TPM_ReadFile(ifn, &mycontext, &contextSize));
	    if ((ec & ERR_MASK))
		goto exit;
	    SET_TPM_BUFFER(&context, mycontext, contextSize);
	    ec = rpmtpmErr(tpm, "LoadKeyContext", 0,
			TPM_LoadKeyContext(&context, &handle));
	    if (ec)
		goto exit;
	    printf("New Handle = %08X\n", handle);
	    free(mycontext);
	} else
/* --- loadmanumaintpub */
/* --- loadmigrationblob */
/* --- loadownerdelegation */
/* --- migrate */
/* --- migratekey */
/* --- nv */
/* --- nv_definespace */
/* --- nv_readvalue */
/* --- nv_writevalue */
/* --- ownerreadinternalpub */
	if (!strcmp(av[0], "ownerreadinternalpub")) {
	    unsigned char ownhash[TPM_DIGEST_SIZE];
	    STACK_TPM_BUFFER(keybuf);
	    keydata k = {};

	    if (ownerpass == NULL || hkhandle == 0xffffffff || ofn == NULL) {
		printf("Missing argument\n");
		goto exit;
	    }
	    TSS_sha1((unsigned char *)ownerpass, strlen(ownerpass), ownhash);

	    ec = rpmtpmErr(tpm, "OwnerReadInternalPub", 0,
			TPM_OwnerReadInternalPub(hkhandle, ownhash, &k.pub));
	    if (ec)
		goto exit;

	    ec = rpmtpmErr(tpm, "WriteKeyPub", 0,
			TPM_WriteKeyPub(&keybuf, &k));

	    ec = rpmtpmErr(tpm, "WriteFile", 0,
			TPM_WriteFile(ofn, keybuf.buffer, keybuf.used));
	    if (ec)
		goto exit;

	} else
/* --- ownersetdisable */
	if (!strcmp(av[0], "ownersetdisable")) {
	    unsigned char ownhash[TPM_DIGEST_SIZE];
assert(ownerpass);	/* XXX FIXME */

	    TSS_sha1((unsigned char *)ownerpass, strlen(ownerpass), ownhash);
	    ec = rpmtpmErr(tpm, "OwnerSetDisable", 0,
			TPM_OwnerSetDisable(ownhash,
					(disabled_state ? FALSE : TRUE)));
	} else
/* --- pcrread */
	if (!strcmp(av[0], "pcrread")) {
	    unsigned char data[TPM_HASH_SIZE];
	    uint32_t pcrs = 0;

	    ec = rpmtpmErr(tpm, "GetNumPCRRegisters", 0,
			TPM_GetNumPCRRegisters(&pcrs));
	    if (ec)
		goto exit;
	    if (ix < 0 || ix >= (int)pcrs) {
		printf("Index out of range.\n");
		goto exit;
	    }
	    ec = rpmtpmErr(tpm, "PcrRead", 0,
			TPM_PcrRead(ix, data));
	    if (ec)
		goto exit;

	    printf("Current value of PCR %d: ", ix);
	    rpmtpmDump(tpm, NULL, data, sizeof(data));

	} else
/* --- pcrreset */
	if (!strcmp(av[0], "pcrreset")) {
	    TPM_PCR_SELECTION selection = {};
	    uint32_t pcrs = 0;

	    ec = rpmtpmErr(tpm, "GetNumPCRRegisters", 0,
			TPM_GetNumPCRRegisters(&pcrs));
	    if (ec)
		goto exit;
	    if (ix < 0 || ix >= (int)pcrs) {
		printf("Index out of range.\n");
		goto exit;
	    }
	    selection.sizeOfSelect = pcrs / 8;
	    selection.pcrSelect[ix/8] |= (1 << ((ix & 0x07)));
	    ec = rpmtpmErr(tpm, "PcrReset", 0,
			TPM_PCRReset(&selection));
	} else
/* --- physicaldisable */
	if (!strcmp(av[0], "physicaldisable")) {
	    ec = rpmtpmErr(tpm, "PhysicalDisable", 0,
			TPM_PhysicalDisable());
	} else
/* --- physicalenable */
	if (!strcmp(av[0], "physicalenable")) {
	    ec = rpmtpmErr(tpm, "PhysicalEnable", 0,
			TPM_PhysicalEnable());
	} else
/* --- physicalpresence */
/* --- physicalsetdeactivated */
/* --- quote */
	if (!strcmp(av[0], "quote")) {
	    unsigned char keyhash[TPM_DIGEST_SIZE];
	    unsigned char nonce[TPM_NONCE_SIZE];
	    unsigned char * keyptr = NULL;
	    uint32_t pcrs = 0;
	    uint16_t sigscheme = TPM_SS_RSASSAPKCS1v15_SHA1;
	    STACK_TPM_BUFFER(signature);
	    pubkeydata pubkey;  /* public key structure */
	    TPM_PCR_COMPOSITE tpc;
	    TPM_PCR_SELECTION tps;
	    int i;

	    if (hkhandle == 0xffffffff || bitmask == 0) {
		printf("Missing argument\n");
		goto exit;
	    }

	    if (keypass) {
		TSS_sha1((unsigned char *)keypass, strlen(keypass), keyhash);
		/* for testing, use the password hash as the test nonce */
		memcpy(nonce, keyhash, sizeof(nonce));
		keyptr = keyhash;
	    } else
		memset(nonce, 0, sizeof(nonce));

	    ec = rpmtpmErr(tpm, "GetNumPCRRegisters", 0,
			TPM_GetNumPCRRegisters(&pcrs));
	    if (ec)
		goto exit;
	    if (pcrs > TPM_NUM_PCR) {
		printf("Library does not support that many PCRs.\n");
		goto exit;
	    }
	    tps.sizeOfSelect = pcrs / 8;
	    for (i = 0; i < tps.sizeOfSelect; i++) {
		tps.pcrSelect[i] = (bitmask & 0xff);
		bitmask >>= 8;
	    }

	    ec = rpmtpmErr(tpm, "Quote", 0,
			TPM_Quote(hkhandle, keyptr, nonce, &tps, &tpc, &signature));
	    if (ec)
		goto exit;

	    ec = rpmtpmErr(tpm, "GetPubKey", 0,
			TPM_GetPubKey(hkhandle, keyptr, &pubkey));
	    if (ec)
		goto exit;

	    ec = rpmtpmErr(tpm, "ValidatePCRCompositeSignature", 0,
			TPM_ValidatePCRCompositeSignature(&tpc, nonce, &pubkey, &signature, sigscheme));
	    if (ec)
		goto exit;
	    printf("Verification succeeded\n");
	} else
/* --- quote2 */
	if (!strcmp(av[0], "quote2")) {
	    unsigned char keyhash[TPM_DIGEST_SIZE];
	    unsigned char nonce[TPM_NONCE_SIZE];
	    unsigned char * keyptr = NULL;
	    uint32_t pcrs = 0;
	    uint16_t sigscheme = TPM_SS_RSASSAPKCS1v15_SHA1;
	    STACK_TPM_BUFFER(signature);
	    pubkeydata pubkey;  /* public key structure */
	    RSA *rsa;		/* openssl RSA public key */
	    STACK_TPM_BUFFER(versionblob);
	    TPM_PCR_SELECTION selection = {};
	    TPM_PCR_INFO_SHORT s1 = {};
	    TPM_QUOTE_INFO2 quoteinfo;
	    STACK_TPM_BUFFER(serQuoteInfo);
	    int i;

	    if (hkhandle == 0xffffffff || bitmask == 0) {
		printf("Missing argument\n");
		goto exit;
	    }

	    if (keypass) {
		TSS_sha1((unsigned char *)keypass, strlen(keypass), keyhash);
		/* for testing, use the password hash as the test nonce */
		memcpy(nonce, keyhash, sizeof(nonce));
		keyptr = keyhash;
	    } else
		memset(nonce, 0, sizeof(nonce));

	    ec = rpmtpmErr(tpm, "GetNumPCRRegisters", 0,
			TPM_GetNumPCRRegisters(&pcrs));
	    if (ec)
		goto exit;
	    if (pcrs > TPM_NUM_PCR) {
		printf("Library does not support that many PCRs.\n");
		goto exit;
	    }
	    selection.sizeOfSelect = pcrs / 8;
	    for (i = 0; i < selection.sizeOfSelect; i++) {
		selection.pcrSelect[i] = (bitmask & 0xff);
		bitmask >>= 8;
	    }

	    ec = rpmtpmErr(tpm, "Quote2", 0,
			TPM_Quote2(hkhandle, &selection, (add_version ? TRUE : FALSE), keyptr, nonce, &s1, &versionblob, &signature));
	    if (ec)
		goto exit;

	    ec = rpmtpmErr(tpm, "GetPubKey", 0,
			TPM_GetPubKey(hkhandle, keyptr, &pubkey));
	    if (ec)
		goto exit;
	    rsa = TSS_convpubkey(&pubkey);

	    /* fill quoteinfo and calculate hashes needed for verification */
	    quoteinfo.tag = TPM_TAG_QUOTE_INFO2;
	    memcpy(&quoteinfo.fixed, "QUT2", 4);
	    quoteinfo.infoShort = s1;
	    memcpy(&quoteinfo.externalData, nonce, TPM_NONCE_SIZE);

	    /* create hash of quoteinfo for signature verification */
	    ec = rpmtpmErr(tpm, "WriteQuoteInfo2", ERR_MASK,
			TPM_WriteQuoteInfo2(&serQuoteInfo, &quoteinfo));
	    if (ec & ERR_MASK)
		goto exit;

	    /* append version information if given in response */
	    if (add_version) {
		memcpy(serQuoteInfo.buffer + serQuoteInfo.used,
			versionblob.buffer, versionblob.used);
		serQuoteInfo.used += versionblob.used;
	    }

	    ec = rpmtpmErr(tpm, "ValidateSignature", 0,
			TPM_ValidateSignature(sigscheme, &serQuoteInfo, &signature, rsa));
	    if (ec)
		goto exit;
	    printf("Verification succeeded\n");
	} else
/* --- quote */
/* --- random */
	if (!strcmp(av[0], "random")) {
	    unsigned char buffer[1024];
	    uint32_t bufferSize;
	    unsigned int ones = 0;
	    unsigned int zeroes = 0;
	    unsigned int pattern2[4] = {0,0,0,0};
	    unsigned int i;
	    unsigned int j;
	    unsigned int c;

	    printf("Counting number of '1's and '0's of the rng.\n");
	    for (i = 0; i < 10; i++) {
		bufferSize = sizeof(buffer);
		ec = rpmtpmErr(tpm, "GetRandom", 0,
			TPM_GetRandom(bufferSize, buffer, &bufferSize));
		if (ec)
		    goto exit;
		for (j = 0; j < bufferSize; j++) {
		    c = countOnes(buffer[j]);
		    ones += c;
		    zeroes += (8-c);
		    pattern2[0] += matchPattern(buffer[j], 0x0, 2);
		    pattern2[1] += matchPattern(buffer[j], 0x1, 2);
		    pattern2[2] += matchPattern(buffer[j], 0x2, 2);
		    pattern2[3] += matchPattern(buffer[j], 0x3, 2);
		}
		ec = rpmtpmErr(tpm, "StirRandom", 0,
			TPM_StirRandom(buffer, 10));
		if (ec)
		    goto exit;
	    }
	    printf("Percentage of '1': %d percent.\n",
		(100*ones)/(ones+zeroes));
	    printf("Percentage of '00' bits:  %d percent\n",
		(200*pattern2[0])/(ones+zeroes));
	    printf("Percentage of '01' bits:  %d percent\n",
		(200*pattern2[1])/(ones+zeroes));
	    printf("Percentage of '10' bits:  %d percent\n",
		(200*pattern2[2])/(ones+zeroes));
	    printf("Percentage of '11' bits:  %d percent\n",
		(200*pattern2[3])/(ones+zeroes));
	} else
/* --- readmanumaintpub */
/* --- resetestbit */
	if (!strcmp(av[0], "resetestbit")) {
	    ec = rpmtpmErr(tpm, "ResetEstablishmentBit", 0,
			TPM_ResetEstablishmentBit());
	} else
/* --- resetlockvalue */
	if (!strcmp(av[0], "resetlockvalue")) {
	    unsigned char ownhash[TPM_DIGEST_SIZE];
assert(ownerpass);	/* XXX FIXME */
	    TSS_sha1((unsigned char *)ownerpass, strlen(ownerpass), ownhash);
	    ec = rpmtpmErr(tpm, "ResetLockValue", 0,
			TPM_ResetLockValue(ownhash));
	} else
/* --- revtrust */
	if (!strcmp(av[0], "revtrust")) {
	    unsigned char ownhash[TPM_DIGEST_SIZE];
assert(keypass);	/* XXX FIXME */
	    TSS_sha1((unsigned char *)keypass, strlen(keypass), ownhash);
	    ec = rpmtpmErr(tpm, "RevokeTrust", 0,
			TPM_RevokeTrust(ownhash));
	} else
/* --- saveauthcontext */
	if (!strcmp(av[0], "saveauthcontext")) {
	    unsigned char mycontext[2048];
	    uint32_t contextSize = sizeof(mycontext);

	    if (ofn == NULL || hahandle == 0xffffffff) {
		printf("Missing argument.\n");
		goto exit;
	    }

	    ec = rpmtpmErr(tpm, "SaveAuthContext", 0,
			TPM_SaveAuthContext(hahandle, mycontext, &contextSize));
	    if (ec)
		goto exit;

	    ec = rpmtpmErr(tpm, "WriteFile", 0,
			TPM_WriteFile(ofn, mycontext, contextSize));
	    if (ec)
		goto exit;
	} else
/* --- savecontext */
	if (!strcmp(av[0], "savecontext")) {
	    unsigned char lblhash[TPM_DIGEST_SIZE];
	    STACK_TPM_BUFFER(context);

	    if (ofn == NULL || hahandle == 0xffffffff || lbl == NULL) {
		printf("Missing argument.\n");
		goto exit;
	    }

	    TSS_sha1((unsigned char *)lbl, strlen(lbl), lblhash);

	    ec = rpmtpmErr(tpm, "SaveContext", 0,
			TPM_SaveContext(hahandle, restype, (char *)lblhash, &context));
	    if (ec)
		goto exit;

	    ec = rpmtpmErr(tpm, "WriteFile", 0,
	                TPM_WriteFile(ofn, context.buffer, context.used));
	    if (ec)
	        goto exit;
	} else
/* --- savekeycontext */
	if (!strcmp(av[0], "savekeycontext")) {
	    STACK_TPM_BUFFER(context);

	    if (ofn == NULL || hkhandle == 0xffffffff) {
		printf("Missing argument.\n");
		goto exit;
	    }

	    ec = rpmtpmErr(tpm, "SaveKeyContext", 0,
			TPM_SaveKeyContext(hkhandle, &context));
	    if (ec)
		goto exit;

	    ec = rpmtpmErr(tpm, "WriteFile", 0,
	                TPM_WriteFile(ofn, context.buffer, context.used));
	    if (ec)
	        goto exit;
	} else
/* --- savestate */
	if (!strcmp(av[0], "savestate")) {
	    ec = rpmtpmErr(tpm, "SaveState", 0,
			TPM_SaveState());
	} else
/* --- sealfile2 */
/* --- sealfile */
	if (!strcmp(av[0], "sealfile")) {
	    unsigned char keyhash[TPM_DIGEST_SIZE];
	    unsigned char * keyptr = NULL;
	    unsigned char dathash[TPM_DIGEST_SIZE];
	    unsigned char * datptr = NULL;

	    unsigned char blob[4096];
	    uint32_t bloblen = sizeof(blob);

	    if (hkhandle == 0xffffffff || ofn == NULL || ifn == NULL) {
		printf("Missing argument\n");
		goto exit;
	    }
	    if (keypass) {
		TSS_sha1((unsigned char *)keypass, strlen(keypass), keyhash);
		keyptr = keyhash;
	    }
	    if (datpass) {
		TSS_sha1((unsigned char *)datpass, strlen(datpass), dathash);
		datptr = dathash;
	    }

	    ec = rpmtpmErr(tpm, "ReadFile", ERR_MASK,
			TPM_ReadFile(ifn, &tpm->b, &tpm->nb));
	    if (ec & ERR_MASK)
		goto exit;
	    if (tpm->nb > 256) {
		printf("Data file too large for seal operation\n");
		ec = -3;
		goto exit;
	    }

	    ec = rpmtpmErr(tpm, "SealCurrPCR", 0,
			TPM_SealCurrPCR(hkhandle, 0x7f, keyptr, datptr,
				tpm->b, tpm->nb, blob, &bloblen));
	    if (ec)
		goto exit;

	    ec = rpmtpmErr(tpm, "WriteFile", 0,
	                TPM_WriteFile(ofn, blob, bloblen));
	    if (ec)
	        goto exit;

	} else
/* --- sealxfile */
/* --- selftest */
	if (!strcmp(av[0], "selftest")) {
	    ec = rpmtpmErr(tpm, "SelfTestFull", 0,
			TPM_SelfTestFull());
	    if (ec) {
		char outData[2048];
		uint32_t outDataSize = sizeof(outData);
		int i;

                ec = rpmtpmErr(tpm, "GetTestResult", 0,
				TPM_GetTestResult(outData, &outDataSize));
		if (ec)
		    goto exit;

		if (outDataSize == 0) {
		    printf("The TPM returned no test result data.\n");
		    ec = -1;
		    goto exit;
		}
		printf("Received the following test result:");
		for (i = 0; i < (int)outDataSize; i++) {
		    printf("%02X ",outData[i]);
		    if (((i+1) & 0xf) == 0)
			printf("\n");
		}
	    } else
		printf("TPM_SelfTestFull returned success.\n");
	} else
/* --- session */
/* --- setcapability */
	if (!strcmp(av[0], "setcapability"))
	    ec = rpmtpmSetCapabilities(tpm);
	else
/* --- setoperatorauth */
	if (!strcmp(av[0], "setoperatorauth")) {
	    unsigned char ownhash[TPM_DIGEST_SIZE];
assert(ownerpass);	/* XXX FIXME */
	    TSS_sha1((unsigned char *)ownerpass, strlen(ownerpass), ownhash);
	    ec = rpmtpmErr(tpm, "SetOperatorAuth", 0,
			TPM_SetOperatorAuth(ownhash));
	} else
/* --- setownerinstall */
	if (!strcmp(av[0], "setownerinstall")) {
	    ec = rpmtpmErr(tpm, "SetOwnerInstall", 0,
			TPM_SetOwnerInstall(1));
	} else
/* --- setownerpointer */
/* --- settempdeactivated */
	if (!strcmp(av[0], "settempdeactivated")) {
	    unsigned char ownhash[TPM_DIGEST_SIZE];
	    unsigned char * ownptr = NULL;
	    if (ownerpass) {
		TSS_sha1((unsigned char *)ownerpass, strlen(ownerpass), ownhash);
		ownptr = ownhash;
	    }
	    ec = rpmtpmErr(tpm, "SetTempDeactivated", 0,
			TPM_SetOperatorAuth(ownptr));
	} else
/* --- sha1parts */
/* --- sha1start */
/* --- sha */
/* --- signfile */
/* --- signmsg */
/* --- takeown */
/* --- tickstampblob */
/* --- tis_test */
/* --- tpmbios */
/* --- tpm_demo */
/* --- tpminit */
	if (!strcmp(av[0], "tpminit")) {
	    ec = rpmtpmErr(tpm, "Init", 0,
			TPM_Init());
	} else
/* --- tpmreset */
	if (!strcmp(av[0], "tpmreset")) {
	    ec = rpmtpmErr(tpm, "Reset", 0,
			TPM_Reset());
	} else
/* --- transport_test */
/* --- unbindfile */
/* --- unixiotest */
/* --- unsealfile */
	if (!strcmp(av[0], "unsealfile")) {
	    unsigned char keyhash[TPM_DIGEST_SIZE];
	    unsigned char * keyptr = NULL;
	    unsigned char dathash[TPM_DIGEST_SIZE];
	    unsigned char * datptr = NULL;

	    unsigned char databuff[256];
	    uint32_t datalen = sizeof(databuff);

	    if (hkhandle == 0xffffffff || ofn == NULL || ifn == NULL) {
		printf("Missing argument\n");
		goto exit;
	    }
	    if (keypass) {
		TSS_sha1((unsigned char *)keypass, strlen(keypass), keyhash);
		keyptr = keyhash;
	    }
	    if (datpass) {
		TSS_sha1((unsigned char *)datpass, strlen(datpass), dathash);
		datptr = dathash;
	    }

	    ec = rpmtpmErr(tpm, "ReadFile", 0,
			TPM_ReadFile(ifn, &tpm->b, &tpm->nb));
	    if (ec)
		goto exit;

	    ec = rpmtpmErr(tpm, "Unseal", 0,
			TPM_Unseal(hkhandle, keyptr, datptr,
				tpm->b, tpm->nb, databuff, &datalen));

	    if (ec)
		goto exit;

	    ec = rpmtpmErr(tpm, "WriteFile", 0,
			TPM_WriteFile(ofn, databuff, datalen));
	    if (ec)
		goto exit;

	} else
/* --- unsealxfile */
	if (!strcmp(av[0], "unsealxfile")) {
	    unsigned char keyhash[TPM_DIGEST_SIZE];
	    unsigned char * keyptr = NULL;
	    unsigned char dathash[TPM_DIGEST_SIZE];
	    unsigned char * datptr = NULL;

	    unsigned char databuff[256];
	    uint32_t  datalen = sizeof(databuff);

	    if (hkhandle == 0xffffffff || ofn == NULL || ifn == NULL) {
		printf("Missing argument\n");
		goto exit;
	    }
	    if (keypass) {
		TSS_sha1((unsigned char *)keypass, strlen(keypass), keyhash);
		keyptr = keyhash;
	    }
	    if (datpass) {
		TSS_sha1((unsigned char *)datpass, strlen(datpass), dathash);
		datptr = dathash;
	    }

	    ec = rpmtpmErr(tpm, "ReadFile", 0,
			TPM_ReadFile(ifn, &tpm->b, &tpm->nb));
	    if (ec)
		goto exit;

	    ec = rpmtpmErr(tpm, "Unsealx", 0,
			TPM_Unsealx(hkhandle, keyptr, datptr,
				tpm->b, tpm->nb, databuff, &datalen));

	    ec = rpmtpmErr(tpm, "WriteFile", 0,
			TPM_WriteFile(ofn, databuff, datalen));
	    if (ec)
		goto exit;
	} else
/* --- updateverification */
/* --- verifydelegation */
	/* XXX -ifn and TPM_ReadFile? */
/* --- verifyfile */

	{
	    fprintf(stderr, "Unknown command: %s\n", av[0]);
	    goto exit;
	}
    } else
	ec = rpmtpmGetCapabilities(tpm);

exit:
    con = rpmioFini(con);

    return ec;
}
