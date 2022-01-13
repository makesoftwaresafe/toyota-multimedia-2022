#!/bin/sh

top="`pwd`/tmp"
hdir="$top/.gnupg"
plaintext="$top/plaintext"
DSA="$top/DSA"
RSA="$top/RSA"
#ECDSA="$top/ECDSA"
#EDDSA="$top/EDDSA"

passphrase="123456"
setpass="/usr/libexec/gpg-preset-passphrase --preset --passphrase $passphrase"

gpg="gpg2 --batch --homedir $hdir"

rm -rf $hdir
mkdir -p $hdir
chmod go-rwx $hdir

cat << GO_SYSIN_DD > $hdir/gpg.conf
expert
enable-dsa2
#personal-digest-preferences SHA384 SHA256 SHA1
#cert-digest-algo SHA384
#default-preference-list SHA512 SHA384 SHA256 SHA224 SHA1 AES256 AES192 AES CAST5 ZLIB BZIP2 ZIP Uncompressed
GO_SYSIN_DD

cat << GO_SYSIN_DD > $hdir/gpg-agent.conf
quiet
use-standard-socket
write-env-file $hdir/env
log-file $hdir/log
#allow-preset-passphrase
GO_SYSIN_DD

eval $(gpg-agent --batch --homedir $hdir --daemon)

$gpg --debug-quick-random --gen-key << GO_SYSIN_DD
Key-Type: DSA
Key-Length: 1024
Key-Usage: sign
Name-Real: Donald
Name-Comment: 1024
Name-Email: rpm-devel@rpm5.org
Expire-Date: 1d
%commit
Key-Type: RSA
Key-Length: 1024
Key-Usage: sign,encrypt
Name-Real: Ronald
Name-Comment: 1024
Name-Email: rpm-devel@rpm5.org
Expire-Date: 1d
%commit
GO_SYSIN_DD

#Passphrase: $passphrase
#Preferences: SHA384 SHA256 SHA224 SHA1 RIPEMD160 AES256 AES192 AES CAST5 ZLIB BZIP2 ZIP Uncompressed

#Keyserver: hkp://keys.rpm5.org
#%no-protection
#%transient-key

#Key-Type: ECDSA
#Key-Length: 256
#Key-Curve: NIST P-256
#Name-Real: Eric
#Name-Comment: 256 NIST P-256
#Name-Email: rpm-devel@rpm5.org
#Expire-Date: 1 
#Keyserver: hkp://keys.rpm5.org
#%no-protection
#%transient-key
#%commit

str="test"

# Note carefully the trailing white space on 1st line below: "${str}       "
# $ od -c plaintext 
# 0000000   t   e   s   t                              \n   t   e   s   t
# 0000020  \n
# 0000021
cat << GO_SYSIN_DD > $plaintext
${str}       
${str}
GO_SYSIN_DD

echo "static const char * plaintextfn = \"$plaintext\";"

dsa="$gpg -u Donald"
$gpg --fingerprint Donald | grep 'finger' | sed -e 's/.*print = //' -e 's/ //g' > ${DSA}.grip
#$setpass `cat ${DSA}.grip`

$dsa --detach-sign	--output - $plaintext	> ${DSA}.sig
$dsa --detach-sign -a	--output - $plaintext	> ${DSA}.sigpem
$dsa --clearsign	--output - $plaintext	> ${DSA}.pem
$gpg --export Donald				> ${DSA}.pub
$gpg --export -a Donald				> ${DSA}.pubpem

echo "static const char * DSAsig = \"${DSA}.sig\";"
echo "static const char * DSAsigpem = \"${DSA}.sigpem\";"
echo "static const char * DSApem = \"${DSA}.pem\";"
echo "static const char * DSApub = \"${DSA}.pub\";"
echo "static const char * DSApubpem = \"${DSA}.pubpem\";"
echo "static const char * DSApubid = \"`cat ${DSA}.grip`\";"

rsa="$gpg -u Ronald"
$gpg --fingerprint Ronald | grep 'finger' | sed -e 's/.*print = //' -e 's/ //g' > ${RSA}.grip
#$setpass `cat ${RSA}.grip`

$rsa --detach-sign	--output - $plaintext	> ${RSA}.sig
$rsa --detach-sign -a	--output - $plaintext	> ${RSA}.sigpem
$rsa --clearsign	--output - $plaintext	> ${RSA}.pem
$gpg --export Ronald				> ${RSA}.pub
$gpg --export -a Ronald				> ${RSA}.pubpem

echo "static const char * RSAsig = \"${RSA}.sig\";"
echo "static const char * RSAsigpem = \"${RSA}.sigpem\";"
echo "static const char * RSApem = \"${RSA}.pem\";"
echo "static const char * RSApub = \"${RSA}.pub\";"
echo "static const char * RSApubpem = \"${RSA}.pubpem\";"
echo "static const char * RSApubid = \"`cat ${RSA}.grip`\";"

#$gpg --detach-sign -u Daniel --output - $plaintext > ${EDDSA}.sig
#$gpg --detach-sign -a -u Daniel --output - $plaintext > ${EDDSA}.sigpem
#$gpg --clearsign -u Daniel --output - $plaintext > ${EDDSA}.pem
#$gpg --export Daniel > ${EDDSA}.pub
#$gpg --export -a Daniel > ${EDDSA}.pubpem

#echo "static const char * EDDSAsig = \"${EDDSA}.sig\";"
#echo "static const char * EDDSAsigpem = \"${EDDSA}.sigpem\";"
#echo "static const char * EDDSApem = \"${EDDSA}.pem\";"
#echo "static const char * EDDSApub = \"${EDDSA}.pub\";"
#echo "static const char * EDDSApubpem = \"${EDDSA}.pubpem\";"
#echo "static const char * EDDSApubid = \"`$gpg --fingerprint Daniel | grep 'finger' | sed -e 's/.*print = //' -e 's/ //g'`\";"

#$gpg --detach-sign -u Eric --output - $plaintext > ${ECDSA}.sig
#$gpg --detach-sign -a -u Eric --output - $plaintext > ${ECDSA}.sigpem
#$gpg --clearsign -u Eric --output - $plaintext > ${ECDSA}.pem
#$gpg --export Eric > ${ECDSA}.pub
#$gpg --export -a Eric > ${ECDSA}.pubpem

#echo "static const char * ECDSAsig = \"${ECDSA}.sig\";"
#echo "static const char * ECDSAsigpem = \"${ECDSA}.sigpem\";"
#echo "static const char * ECDSApem = \"${ECDSA}.pem\";"
#echo "static const char * ECDSApub = \"${ECDSA}.pub\";"
#echo "static const char * ECDSApubpem = \"${ECDSA}.pubpem\";"
#echo "static const char * ECDSApubid = \"`$gpg --fingerprint Eric | grep 'finger' | sed -e 's/.*print = //' -e 's/ //g'`\";"
