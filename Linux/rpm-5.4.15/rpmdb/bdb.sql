CREATE DATABASE rpmdb;	/*+ CACHESIZE = 16m */

CREATE TABLE Packages(	/*+ DBTYPE = BTREE */

	Name		VARCHAR2(64),
	Nvra		VARCHAR2(64),
	G		VARCHAR2(64),

	Installtid	INTEGER,
	Packagecolor	INTEGER,
	Sigmd5		BIN(16),
	Sourcepkgid	BIN(16),
	Sha1header	VARCHAR2(41),

	Pubkeys		BIN(8),

	instance	INTEGER UNIQUE PRIMARY KEY
);

CREATE INDEX _Name on Packages(Name);			/*+ DBTYPE = BTREE */
CREATE INDEX _Nvra on Packages(Nvra);			/*+ DBTYPE = BTREE */
CREATE INDEX _Group on Packages(G);			/*+ DBTYPE = BTREE */

CREATE INDEX _Installtid on Packages(Installtid);	/*+ DBTYPE = BTREE */
CREATE INDEX _Packagecolor on Packages(Packagecolor);	/*+ DBTYPE = HASH */
CREATE INDEX _Sigmd5 on Packages(Sigmd5);		/*+ DBTYPE = HASH */
CREATE INDEX _Sourcepkgid on Packages(Sourcepkgid);	/*+ DBTYPE = HASH */
CREATE INDEX _Sha1header on Packages(Sha1header);	/*+ DBTYPE = HASH */

CREATE INDEX _Pubkeys on Packages(Pubkeys);		/*+ DBTYPE = HASH */

CREATE TABLE _Providename( /*+ DBTYPE = BTREE */
	hx		INTEGER REFERENCES Packages(instance),
	N		VARCHAR2(64) PRIMARY KEY
);

CREATE TABLE _Requirename( /*+ DBTYPE = BTREE */
	hx		INTEGER REFERENCES Packages(instance),
	N		VARCHAR2(64) PRIMARY KEY
);

CREATE TABLE _Conflictname( /*+ DBTYPE = BTREE */
	hx		INTEGER REFERENCES Packages(instance),
	N		VARCHAR2(64) PRIMARY KEY
);

CREATE TABLE _Obsoletename( /*+ DBTYPE = BTREE */
	hx		INTEGER REFERENCES Packages(instance),
	N		VARCHAR2(64) PRIMARY KEY
);

CREATE TABLE _Triggername( /*+ DBTYPE = BTREE */
	hx		INTEGER REFERENCES Packages(instance),
	N		VARCHAR2(64) PRIMARY KEY
);

CREATE TABLE _Dirnames(	/*+ DBTYPE = BTREE */
	hx		INTEGER REFERENCES Packages(instance),
	dn		VARCHAR2(64) PRIMARY KEY
);

CREATE TABLE _Filepaths( /*+ DBTYPE = BTREE */
	hx		INTEGER REFERENCES Packages(instance),
	fn		VARCHAR2(64) PRIMARY KEY
);

CREATE TABLE _Filedigests( /*+ DBTYPE = HASH */
	hx		INTEGER REFERENCES Packages(instance),
	digest		BIN(16) PRIMARY KEY
);
