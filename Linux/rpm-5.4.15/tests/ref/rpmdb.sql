DROP TABLE IF EXISTS Packages;
CREATE TABLE Packages (
  v	INTEGER UNIQUE PRIMARY KEY NOT NULL,
  k	BLOB NOT NULL
);

CREATE TRIGGER insert_Packages AFTER INSERT ON Packages
  BEGIN
    INSERT INTO Nvra (k,v)	VALUES (
	new.k, new.rowid );
    INSERT INTO Name (k,v)	VALUES (
	SUBSTR(new.k,  1, 4), new.rowid );
    INSERT INTO Version (k,v)	VALUES (
	SUBSTR(new.k,  6, 3), new.rowid );
    INSERT INTO Release (k,v)	VALUES (
	SUBSTR(new.k, 10, 1), new.rowid );
    INSERT INTO Arch (k,v)	VALUES (
	SUBSTR(new.k, 12), new.rowid );
  END;
CREATE TRIGGER delete_Packages BEFORE DELETE ON Packages
  BEGIN
    DELETE FROM Nvra	WHERE v = old.rowid;
    DELETE FROM Name	WHERE v = old.rowid;
    DELETE FROM Version	WHERE v = old.rowid;
    DELETE FROM Release	WHERE v = old.rowid;
    DELETE FROM Arch	WHERE v = old.rowid;
  END;

DROP TABLE IF EXISTS Nvra;
CREATE TABLE Nvra (
  k	TEXT NOT NULL,
  v	INTEGER REFERENCES Packages
);

DROP TABLE IF EXISTS Name;
CREATE TABLE Name (
  k	TEXT NOT NULL,
  v	INTEGER REFERENCES Packages
);

DROP TABLE IF EXISTS Version;
CREATE TABLE Version (
  k	TEXT NOT NULL,
  v	INTEGER REFERENCES Packages
);

DROP TABLE IF EXISTS Release;
CREATE TABLE Release (
  k	TEXT NOT NULL,
  v	INTEGER REFERENCES Packages
);

DROP TABLE IF EXISTS Arch;
CREATE TABLE Arch (
  k	TEXT NOT NULL,
  v	INTEGER REFERENCES Packages
);
