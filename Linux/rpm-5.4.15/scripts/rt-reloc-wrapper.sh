#! /bin/sh
# Identify run path, and forward to ../lib_or_lib64/$0

PROGNAME=$(basename $0)
DIR=$(cd $(dirname $0) ; pwd)

export LD_LIBRARY_PATH=$DIR/../libMark64:$DIR/../lib:$DIR/../lib64
export RPM_USRLIBRPM=$DIR/../libMark64/rpm
export RPM_ETCRPM=$DIR/../etc/rpm
export RPM_LOCALEDIR=$DIR/../share/locale

exec $DIR/../libMark64/rpm/$PROGNAME "$@"
