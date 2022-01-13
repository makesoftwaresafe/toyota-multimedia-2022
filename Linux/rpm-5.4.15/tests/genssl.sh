#!/bin/sh

top="`pwd`/tmp/SSL"
#hdir="$top/.gnupg"
plaintext="$top/plaintext"
DSA="$top/DSA"
ECDSA="$top/ECDSA"
RSA="$top/RSA"

ssl="/usr/bin/openssl"
sslopts="-new -x509 -nodes -sha1 -days 7"
DSAopts="-algorithm DSA -pkeyopt dsa_paramgen_bits:1024"
ECDSAopts="-algorithm ECDSA -pkeyopt ec_paramgen_curve:secp256k1"
RSAopts="-algorithm RSA -pkeyopt rsa_paramgen_bits:1024"

rm -rf $top
mkdir -p $top

#rm -rf $hdir
#mkdir -p $hdir
#chmod go-rwx $hdir

# C Name (2 letter code) [XX]:
# ST or Province Name (full name) []:
# L Name (eg, city) [Default City]:
# Oanization Name (eg, company) [Default Company Ltd]:
# Oanizational OU Name (eg, section) []:
# Common Name (eg, your name or your server's hostname) []:
# Email Address []:
C="US"
ST="North Carolina"
L="Chapel Hill"
O="Wraptastic!"
OU="rpm5.org"

{
$ssl genpkey -genparam ${DSAopts} -out ${DSA}.param
$ssl genpkey -paramfile ${DSA}.param -out ${DSA}.key
#$ssl dsaparam -genkey -out ${DSA}.key 1024
$ssl req -key ${DSA}.key -out ${DSA}.cert $sslopts << GO_SYSIN_DD
$C
$ST
$L
$O
$OU
Donald
donald@${OU}
GO_SYSIN_DD
} >/dev/null 2>&1
#$ssl dsa -in ${DSA}.key -pubout -text

{
#$ssl genpkey -genparam ${ECDSAopts} -out ${ECDSA}.param
#$ssl genpkey -paramfile ${ECDSA}.param -out ${ECDSA}.key
$ssl ecparam -name secp256k1 -genkey -out ${ECDSA}.key
$ssl req $sslopts -key ${ECDSA}.key -out ${ECDSA}.cert << GO_SYSIN_DD
$C
$ST
$L
$O
$OU
Eric
eric@${OU}
GO_SYSIN_DD
} >/dev/null 2>&1
#$ssl ec -in ${ECDSA}.key -pubout -text

{
#$ssl genpkey -genparam ${RSAopts} -out ${RSA}.param
#$ssl genpkey -paramfile ${RSA}.param -out ${RSA}.key
$ssl genrsa -out ${RSA}.key 1024
$ssl req -key ${RSA}.key -out ${RSA}.cert $sslopts << GO_SYSIN_DD
$C
$ST
$L
$O
$OU
Ronald
ronald@${OU}
GO_SYSIN_DD
} >/dev/null 2>&1
#$ssl rsa -in ${RSA}.key -pubout -text

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

# --- DSA-SHA1
#$gpg --detach-sign -u Donald --output - $plaintext > ${DSA}.sig
$ssl dgst -sha1 -sign ${DSA}.key $plaintext  > ${DSA}.sig
#$gpg --detach-sign -a -u Donald --output - $plaintext > ${DSA}.sigpem
$ssl smime -md sha1 -sign -nocerts -in $plaintext -nodetach \
	-inkey ${DSA}.key -signer ${DSA}.cert -text > ${DSA}.sigpem
#$gpg --clearsign -u Donald --output - $plaintext > ${DSA}.pem
$ssl smime -md sha1 -sign -nocerts -in $plaintext \
	-inkey ${DSA}.key -signer ${DSA}.cert -text > ${DSA}.pem

#$gpg --export Donald > ${DSA}.pub
$ssl dsa -in ${DSA}.key -pubout -outform DER -out ${DSA}.pub	>/dev/null 2>&1
#$gpg --export -a Donald > ${DSA}.pubpem
$ssl dsa -in ${DSA}.key -pubout -outform PEM -out ${DSA}.pubpem	>/dev/null 2>&1

echo "static const char * DSAsig = \"${DSA}.sig\";"
echo "static const char * DSAsigpem = \"${DSA}.sigpem\";"
echo "static const char * DSApem = \"${DSA}.pem\";"
echo "static const char * DSApub = \"${DSA}.pub\";"
echo "static const char * DSApubpem = \"${DSA}.pubpem\";"
echo "static const char * DSApubid = \"`$ssl x509 -noout -fingerprint -in ${DSA}.cert | sed -e 's/^[^=]*=//' -e 's/://g'`\";"

# --- ECDSA-SHA256
#$gpg --detach-sign -u Eric --output - $plaintext > ${ECDSA}.sig
$ssl dgst -sha256 -sign ${ECDSA}.key $plaintext  > ${ECDSA}.sig
#$gpg --detach-sign -a -u Eric --output - $plaintext > ${ECDSA}.sigpem
$ssl smime -md sha256 -sign -nocerts -in $plaintext -nodetach \
	-inkey ${ECDSA}.key -signer ${ECDSA}.cert -text > ${ECDSA}.sigpem
#$gpg --clearsign -u Eric --output - $plaintext > ${ECDSA}.pem
$ssl smime -md sha256 -sign -nocerts -in $plaintext \
	-inkey ${ECDSA}.key -signer ${ECDSA}.cert -text > ${ECDSA}.pem

#$gpg --export Eric > ${ECDSA}.pub
$ssl ec -in ${ECDSA}.key -pubout -outform DER -out ${ECDSA}.pub	>/dev/null 2>&1
#$gpg --export -a Eric > ${ECDSA}.pubpem
$ssl ec -in ${ECDSA}.key -pubout -outform PEM -out ${ECDSA}.pubpem >/dev/null 2>&1

echo "static const char * ECDSAsig = \"${ECDSA}.sig\";"
echo "static const char * ECDSAsigpem = \"${ECDSA}.sigpem\";"
echo "static const char * ECDSApem = \"${ECDSA}.pem\";"
echo "static const char * ECDSApub = \"${ECDSA}.pub\";"
echo "static const char * ECDSApubpem = \"${ECDSA}.pubpem\";"
echo "static const char * ECDSApubid = \"`$ssl x509 -noout -fingerprint -in ${ECDSA}.cert | sed -e 's/^[^=]*=//' -e 's/://g'`\";"

# --- RSA-SHA1
#$gpg --detach-sign -u Ronald --output - $plaintext > ${RSA}.sig
$ssl dgst -sha1 -sign ${RSA}.key $plaintext  > ${RSA}.sig
#$gpg --detach-sign -a -u Ronald --output - $plaintext > ${RSA}.sigpem
$ssl smime -md sha1 -sign -nocerts -in $plaintext -nodetach \
	-inkey ${RSA}.key -signer ${RSA}.cert -text > ${RSA}.sigpem
#$gpg --clearsign -u Ronald --output - $plaintext > ${RSA}.pem
$ssl smime -md sha1 -sign -nocerts -in $plaintext \
	-inkey ${RSA}.key -signer ${RSA}.cert -text > ${RSA}.pem

#$gpg --export Ronald > ${RSA}.pub
$ssl rsa -in ${RSA}.key -pubout -outform DER -out ${RSA}.pub	>/dev/null 2>&1
#$gpg --export -a Ronald > ${RSA}.pubpem
$ssl rsa -in ${RSA}.key -pubout -outform PEM -out ${RSA}.pubpem	>/dev/null 2>&1

echo "static const char * RSAsig = \"${RSA}.sig\";"
echo "static const char * RSAsigpem = \"${RSA}.sigpem\";"
echo "static const char * RSApem = \"${RSA}.pem\";"
echo "static const char * RSApub = \"${RSA}.pub\";"
echo "static const char * RSApubpem = \"${RSA}.pubpem\";"
echo "static const char * RSApubid = \"`$ssl x509 -noout -fingerprint -in ${RSA}.cert | sed -e 's/^[^=]*=//' -e 's/://g'`\";"
