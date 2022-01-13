-- Configure the SQL parser parameters to known defaults.
.echo on
.explain off
.headers on
-- .log stderr
.mode list
.nullvalue ""
.output stdout
.prompt 'foo> ' '---> '
.separator "|"
.timeout 1000
.timer off
.show

-- Instantiate the rpmdb.sql schema.
PRAGMA foreign_keys = ON;
.read ref/rpmdb.sql

-- Basic rpmdbAdd/rpmdbRemove operations using SQL triggers.
BEGIN TRANSACTION;
INSERT into Packages (k) VALUES ('bing-1.2-3.noarch');
INSERT into Packages (k) VALUES ('bang-4.5-6.noarch');
INSERT into Packages (k) VALUES ('boom-7.8-9.noarch');
COMMIT TRANSACTION;

SELECT * from Packages;
SELECT * from Nvra;
SELECT * from Name;
SELECT * from Version;
SELECT * from Release;
SELECT * from Arch;

-- explicit inner join
SELECT Arch.k, Packages.k
  FROM Arch
  INNER JOIN Packages ON Arch.v = Packages.v;

-- implicit inner join
SELECT Arch.k, Packages.k
  FROM Arch, Packages
  WHERE Arch.v = Packages.v;

-- equi-join
SELECT *
  FROM Arch
  INNER JOIN Packages USING (v);

BEGIN TRANSACTION;
DELETE FROM Packages WHERE v = 2;
COMMIT TRANSACTION;

SELECT * from Packages;
SELECT * from Nvra;
SELECT * from Name;
SELECT * from Version;
SELECT * from Release;
SELECT * from Arch;

-- Basic .foo SQL metadata tests.
.backup main tmp/main.bak
.restore main tmp/main.bak
.dump main

.databases
.indices Packages
.schema
.tables

.exit
