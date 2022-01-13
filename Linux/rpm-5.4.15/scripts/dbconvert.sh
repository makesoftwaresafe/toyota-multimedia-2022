#!/bin/sh

TIMESTAMP="$(LC_TIME=C date -u +%F_%R)"
TMPDIR="/var/tmp"
BACKUP="${BACKUP:-$TMPDIR/rpmdb-$TIMESTAMP}"
NEWDB="${NEWDB:-`mktemp -d --tmpdir=$TMPDIR -t newdb-XXXXXXXXXX`}"
DBHOME="${DBHOME:-/var/lib/rpm}"
DBFORCE=${DBFORCE:-0}
DBVERBOSE=${DBVERBOSE:-1}
DBREBUILD=${DBREBUILD:-0}

# --> Find the RPM executable to use.
wdj=${wdj:-wdj}
rpm="$(which $wdj 2>/dev/null)"
if [ ".$rpm" = . ]; then
    echo "--> Can't find rpm executable $wdj"
    exit 1
fi
echo "==> found rpm executable $rpm (`$rpm --version`)"

# --> Change DBVERSION=X.Y to select which dbXY_utils to use
DBVERSION=6.1
DBERROR=0
for db_tool in db_stat db_dump db_load db_recover; do
    db_tool_versioned=$(echo $db_tool | sed -e 's/^db_/db'"${DBVERSION}"'/'  -e 's/^db\([0-9]*\)\.\([0-9]*\)/db\1\2_/')
    tool=$(which ${db_tool_versioned} || which $db_tool 2> /dev/null)
    if [ -z "$tool" -o ! -x "$tool" ]; then
	echo "Unable to locate $db_tool"
	DBERROR=1
    else
	tool_version="$($tool -V |sed 's/^Berkeley DB \([0-9]\+\.[0-9]\+\).*/\1/')"
	[ $DBVERBOSE -ne 0 ] && echo "==> found $db_tool: $tool version: $tool_version"
	if [ "$tool_version" = "$DBVERSION" ]; then
	    export $db_tool=$tool
	else
	    echo "incompatible $db_tool version ($tool_version) found, $DBVERSION.* required"
	    DBERROR=1
	fi
    fi
done
if [ $DBERROR -ne 0 ]; then
    exit $DBERROR
fi


echo "--> identifying byte order using $db_dump"
HEADER=1
DATA=0
LORDER=0
for line in `$db_dump "$DBHOME/Packages"|head`; do
    if [ $HEADER -eq 0 ]; then
	[ $DATA -eq 0 -a $((0x$line)) -eq 0 ] && continue
        DATA=`expr $DATA + 1`
	if [ $((0x$line)) -ge 10000000 ]; then
	    LORDER=1234
	else
	    LORDER=4321
	fi
	break
    fi
    if [ "$line" = "HEADER=END" ]; then
	HEADER=0
    fi
done
if [ $LORDER -eq 0 ]; then
    echo "unable to determine endianness, aborting"
    exit 1
fi
echo "==> byte order is $LORDER"

# Database is assumed to be converted, so let's ditch it
echo "--> checking for DB_BTREE using $db_stat"
if [ $($db_stat -f -d "$DBHOME/Packages" |grep -c 'Btree magic number') -ne 0 -o $LORDER -eq 4321 ] && \
    rpm --dbpath "$DBHOME" -qa >/dev/null 2>&1 && rpm --dbpath "$DBHOME" -q rpm >/dev/null 2>&1 ; then
    if [ "$DBFORCE" -eq 0 ]; then
    	[ $DBVERBOSE -ne 0 ] && echo "==> rpmdb already converted, set variable DBFORCE=1 to force"
	exit 0
    fi
fi
echo "==> rpmdb ($DBHOME) isn't DB_BTREE in 4321 order, needs conversion"

echo "--> converting $DBHOME/Packages into $NEWDB/Packages."
rm -rf "$NEWDB"
# XXXX Poor man's brace bash expansion 
for _var in "$DBHOME" "$NEWDB"
do
        for _var1 in log tmp
        do
          mkdir -p "${_var}"/"${_var1}" 2>/dev/null || :
        done
done

if [ "$DBHOME" != "/var/lib/rpm" ]; then
    if [ -f /var/lib/rpm/DB_CONFIG ]; then
	cp /var/lib/rpm/DB_CONFIG "$NEWDB/DB_CONFIG"
    else
	tee "$NEWDB/DB_CONFIG" << EOH
# ================ Environment
#add_data_dir		.
set_data_dir		.
set_create_dir		.
set_lg_dir		./log
set_tmp_dir		./tmp

# -- thread_count must be >= 8
set_thread_count	64

#set_verbose		DB_VERB_DEADLOCK
#set_verbose		DB_VERB_FILEOPS
#set_verbose		DB_VERB_FILEOPS_ALL
#set_verbose		DB_VERB_RECOVERY
#set_verbose		DB_VERB_REGISTER
#set_verbose		DB_VERB_REPLICATION
#set_verbose		DB_VERB_REP_ELECT
#set_verbose		DB_VERB_REP_LEASE
#set_verbose		DB_VERB_REP_MISC
#set_verbose		DB_VERB_REP_MSGS
#set_verbose		DB_VERB_REP_SYNC
#set_verbose		DB_VERB_REP_TEST
#set_verbose		DB_VERB_REPMGR_CONNFAIL
#set_verbose		DB_VERB_REPMGR_MISC
#set_verbose		DB_VERB_WAITSFOR

# ================ Logging

# ================ Memory Pool
#XXX initializing dbenv with set_cachesize has unimplemented prerequsites
#set_cachesize		0 1048576 0 
set_mp_mmapsize		268435456

# ================ Locking
set_lk_max_locks	163840
set_lk_max_lockers	163840
set_lk_max_objects	163840
mutex_set_max		1638400

# ================ Replication
EOH
    fi
fi

echo "--> convert Packages header instances to network 4321 order"
$db_dump "$DBHOME/Packages" \
    | sed \
    -e 's/^type=hash$/type=btree/' \
    -e '/^h_nelem=/d' \
    -e 's/^ \(..\)\(..\)\(..\)\(..\)$/ \4\3\2\1/' \
    | $db_load -c db_lorder=4321 -h "$NEWDB" Packages
echo "--> test whether the converted Packages can be queried"
$rpm -D "_dbpath $NEWDB" -qa > /dev/null
#$rpm -D "_dbpath $NEWDB" -q rpm > /dev/null
DBERROR=$?
if [ $DBERROR -ne 0 ]; then
    echo "==> conversion failed"
    exit $DBERROR
fi
echo "==> conversion successful."

if [ ! -d "$DBHOME" -o ! -w "$DBHOME" ]; then
    echo "==> unwritable $DBHOME, move $NEWDB contents manually"
    exit 0
fi

    echo "--> save previous rpmdb files in $BACKUP"
    mkdir -p "$BACKUP"
    for db in \
	Arch Filepaths Name Os Providename Release Seqno Sourcepkgid \
	Basenames Dirnames Group Nvra Packagecolor Provideversion Requirename Sha1header \
	Triggername Conflictname Filedigests Installtid Obsoletename Packages Pubkeys \
	Requireversion Sigmd5 Version; do
      test -f "$DBHOME/$db" && cp "$DBHOME/$db" "$BACKUP/$db"
    done
    echo "==> backup successful."

    echo "--> move $NEWDB/Packages and log files back to $DBHOME"
    rm -f "$DBHOME"/log/*
    mv "$NEWDB"/Packages "$DBHOME"
    if [ -f "$NEWDB"/log/log.0000000001 ]; then
    	mv "$NEWDB"/log/* "$DBHOME"/log
    	# log files will contain paths to original path where created,
    	# so need to fix these, or db_recover will PANIC
    	sed -e "s,$NEWDB,$DBHOME,g" -i "$DBHOME"/log/*
    fi
    echo "==> move successful."

    echo "--> run $db_recover"
    $db_recover -h "$DBHOME"
    rm -rf "$NEWDB"
    if [ $DBREBUILD -ne 0 ]; then
	echo "--> regenerate the $DBHOME indices with --rebuilddb"
	DBI_TXN="$($rpm --eval %__dbi_txn) nofsync"
	$rpm \
	    -D "_dbpath $DBHOME" \
	    --rebuilddb -vv \
	    -D "__dbi_txn $DBI_TXN"
    fi
