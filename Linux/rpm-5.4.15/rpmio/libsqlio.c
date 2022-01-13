#include "system.h"

#define _RPMSQL_INTERNAL
#define _RPMVT_INTERNAL
#define _RPMVC_INTERNAL
#include <rpmsql.h>

#include <sqlite3.h>	/* XXX sqlite3_module */

#include "debug.h"

/*==============================================================*/
#ifdef	REFERENCE
/* --- MacPorts registry schema */

BEGIN;

/* -- metadata table */
CREATE TABLE registry.metadata (
    key UNIQUE,
    value
);
INSERT INTO registry.metadata (key, value) VALUES ('version', 1.000);
INSERT INTO registry.metadata (key, value) VALUES ('created', strftime('%s', 'now'));

/* -- ports table */
CREATE TABLE registry.ports (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    name TEXT COLLATE NOCASE,
    portfile CLOB,
    url TEXT,
    location TEXT,
    epoch INTEGER,
    version TEXT COLLATE VERSION,
    revision INTEGER,
    variants TEXT,
    negated_variants TEXT,
    state TEXT,
    date DATETIME,
    installtype TEXT,
    archs TEXT,
    requested INT,
    os_platform TEXT,
    os_major INTEGER,
    UNIQUE (name, epoch, version, revision, variants),
    UNIQUE (url, epoch, version, revision, variants)
);
CREATE INDEX registry.port_name ON ports
   (name, epoch, version, revision, variants);
CREATE INDEX registry.port_url ON ports
   (url, epoch, version, revision, variants);
CREATE INDEX registry.port_state ON ports (state);

/* -- file map */
CREATE TABLE registry.files (
    id INTEGER,
    path TEXT,
    actual_path TEXT,
    active INT,
    mtime DATETIME,
    md5sum TEXT,
    editable INT,
   FOREIGN KEY(id) REFERENCES ports(id));
CREATE INDEX registry.file_port ON files (id);
CREATE INDEX registry.file_path ON files(path);
CREATE INDEX registry.file_actual ON files(actual_path);

/* -- dependency map */
CREATE TABLE registry.dependencies (
    id INTEGER,
    name TEXT,
    variants TEXT,
   FOREIGN KEY(id) REFERENCES ports(id));
CREATE INDEX registry.dep_name ON dependencies (name);

COMMIT;

/* -- MacPorts temporaray tables. */
BEGIN;

/* -- items cache */
CREATE TEMPORARY TABLE items (refcount, proc UNIQUE, name, url, path,
            worker, options, variants);

/* -- indexes list */
CREATE TEMPORARY TABLE indexes (file, name, attached);

COMMIT;

#endif

/*==============================================================*/
#ifdef	REFERENCE
/* --- Nix sqlite schema */
create table if not exists ValidPaths (
    id               integer primary key autoincrement not null,
    path             text unique not null,
    hash             text not null,
    registrationTime integer not null,
    deriver          text
);

create table if not exists Refs (
    referrer  integer not null,
    reference integer not null,
    primary key (referrer, reference),
    foreign key (referrer) references ValidPaths(id) on delete cascade,
    foreign key (reference) references ValidPaths(id) on delete restrict
);

create index if not exists IndexReferrer on Refs(referrer);
create index if not exists IndexReference on Refs(reference);

-- Paths can refer to themselves, causing a tuple (N, N) in the Refs
-- table.  This causes a deletion of the corresponding row in
-- ValidPaths to cause a foreign key constraint violation (due to `on
-- delete restrict' on the `reference' column).  Therefore, explicitly
-- get rid of self-references.
create trigger if not exists DeleteSelfRefs before delete on ValidPaths
  begin
    delete from Refs where referrer = old.id and reference = old.id;
  end;

create table if not exists DerivationOutputs (
    drv  integer not null,
    id   text not null, -- symbolic output id, usually "out"
    path text not null,
    primary key (drv, id),
    foreign key (drv) references ValidPaths(id) on delete cascade
);

create index if not exists IndexDerivationOutputs on DerivationOutputs(path);

create table if not exists FailedPaths (
    path text primary key not null,
    time integer not null
);
#endif

static struct rpmvd_s _nixdbVD = {
        .dbpath = "/nix/var/nix/db/db.sqlite",
	/* XXX glob needs adjustment for base32 and possibly length. */
	.prefix = "/nix/store/[a-z0-9]*-",
	.split	= "/-",
	.parse	= "dir/instance-name",
	.regex	= "^(.+/)([^-]+)-(.*)$",
	.idx	= 2,
};

static int nixdbCreateConnect(void * _db, void * pAux,
		int argc, const char *const * argv,
		rpmvt * vtp, char ** pzErr)
{
    return rpmvtLoadArgv(rpmvtNew(_db, pAux, argv, &_nixdbVD), vtp);
}

struct sqlite3_module nixdbModule = {
    .xCreate	= (void *) nixdbCreateConnect,
    .xConnect	= (void *) nixdbCreateConnect,
};

/*==============================================================*/

static struct rpmsqlVMT_s __VMT[] = {
  { "Nixdb",		&nixdbModule, NULL },
  { NULL,               NULL, NULL }
};

extern int sqlite3_extension_init(void * _db);
int sqlite3_extension_init(void * _db)
{
    int rc = 0;		/* SQLITE_OK */
SQLDBG((stderr, "--> %s(%p)\n", __FUNCTION__, _db));
    rc = _rpmsqlLoadVMT(_db, __VMT);
SQLDBG((stderr, "<-- %s(%p) rc %d\n", __FUNCTION__, _db, rc));
    return rc;
}
