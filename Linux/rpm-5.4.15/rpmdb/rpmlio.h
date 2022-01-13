#ifndef H_RPMLIO
#define H_RPMLIO

/*@unchecked@*/
extern int _rpmlio_debug;

int rpmlioCreat(rpmdb rpmdb, const char * fn, mode_t mode,
		const uint8_t * b, size_t blen,
		const uint8_t * d, size_t dlen, uint32_t dalgo)
	/*@*/;

int rpmlioUnlink(rpmdb rpmdb, const char * fn, mode_t mode,
		const uint8_t * b, size_t blen,
		const uint8_t * d, size_t dlen, uint32_t dalgo)
	/*@*/;

int rpmlioRename(rpmdb rpmdb, const char * oldname, const char * newname,
		mode_t mode,
		const uint8_t * b, size_t blen,
		const uint8_t * d, size_t dlen, uint32_t dalgo)
	/*@*/;

int rpmlioMkdir(rpmdb rpmdb, const char * dn, mode_t mode)
	/*@*/;

int rpmlioRmdir(rpmdb rpmdb, const char * dn, mode_t mode)
	/*@*/;

int rpmlioLsetfilecon(rpmdb rpmdb, const char * fn, const char * context)
	/*@*/;

int rpmlioChown(rpmdb rpmdb, const char * fn, uid_t uid, gid_t gid)
	/*@*/;

int rpmlioLchown(rpmdb rpmdb, const char * fn, uid_t uid, gid_t gid)
	/*@*/;

int rpmlioChmod(rpmdb rpmdb, const char * fn, mode_t mode)
	/*@*/;

int rpmlioUtime(rpmdb rpmdb, const char * fn, time_t actime, time_t modtime)
	/*@*/;

int rpmlioSymlink(rpmdb rpmdb, const char * ln, const char * fn)
	/*@*/;

int rpmlioLink(rpmdb rpmdb, const char * ln, const char * fn)
	/*@*/;

int rpmlioMknod(rpmdb rpmdb, const char * fn, mode_t mode, dev_t dev)
	/*@*/;

int rpmlioMkfifo(rpmdb rpmdb, const char * fn, mode_t mode)
	/*@*/;

int rpmlioPrein(rpmdb rpmdb, const char ** av, const char * body)
	/*@*/;

int rpmlioPostin(rpmdb rpmdb, const char ** av, const char * body)
	/*@*/;

int rpmlioPreun(rpmdb rpmdb, const char ** av, const char * body)
	/*@*/;

int rpmlioPostun(rpmdb rpmdb, const char ** av, const char * body)
	/*@*/;

#endif	/* H_RPMLIO */
