#!/bin/sh

hdir="`pwd`/.gnupg"
gpg="gpg2 --homedir $hdir"

rm -rf $hdir

$gpg --batch --debug-quick-random --gen-key << GO_SYSIN_DD
Key-Type: DSA
Key-Length: 1024
Key-Usage: sign
Name-Real: DSApub
Name-Comment: 1024
Name-Email: rpm-devel@rpm5.org
%commit
Key-Type: RSA
Key-Length: 1024
Key-Usage: sign
Name-Real: RSApub
Name-Comment: 1024
Name-Email: rpm-devel@rpm5.org
%commit
GO_SYSIN_DD

str="Signed\n"
echo "static const char * str = \"$str\";"

echo "static const char * DSApub ="
$gpg --export -a -u DSApub | sed -e'1,3d; $d' | sed -e's/^/"/; s/$/\\n"/; $d'
echo ";"

echo "static const char * DSAsig ="
echo -n "$str" | $gpg -sab -u DSApub | sed -e'1,3d; $d' | sed -e's/^/"/; s/$/\\n"/; $d'
echo ";"

echo "static const char * RSApub ="
$gpg --export -a -u RSApub | sed -e'1,3d; $d' | sed -e's/^/"/; s/$/\\n"/; $d'
echo ";"

echo "static const char * RSAsig ="
echo -n "$str" | $gpg -sab -u RSApub | sed -e'1,3d; $d' | sed -e's/^/"/; s/$/\\n"/; $d'
echo ";"

echo "static const char * ECDSAsig ="
cat << GO_SYSIN_DD | sed -e'1,3d; $d' | sed -e's/^/"/; s/$/\\n"/; $d'
-----BEGIN PGP SIGNATURE-----
Version: PGP SDK 3.11.0

iQBXAwUBSMjHk4QJEy1tIUM0EwhkLQEAw+SAPNMClecbCzvb8Rpx6fov5eJ3ShVs
gAtyf/w/IiwBAJNz/QD/qtHN13o1rlrJKlCXrbnfEsVl5gkMIULH+eEU
=uWBi
-----END PGP SIGNATURE-----
GO_SYSIN_DD
echo ";"

echo "static const char * ECDSApub ="
cat << GO_SYSIN_DD | sed -e'1,3d; $d' | sed -e's/^/"/; s/$/\\n"/; $d'
-----BEGIN PGP PUBLIC KEY BLOCK-----
Version: PGP SDK 3.11.0

mQBSBEjIvdoTCCqGSM49AwEHAgMEilTubmrCj7X11p6CxMO+Ifg34Bzp8UZdHh5V
40oEoS7qcjFQ0e3gjWB9DfSkiTKLaOwhDBttR+Hw8TG/v4YtubQiRUNDIFAtMjU2
IDxlY2MtcC0yNTZAYnJhaW5odWIuY29tPokArAQQEwIAVAUCSMi92gUJCWYBgDAU
gAAAAAAgAAdwcmVmZXJyZWQtZW1haWwtZW5jb2RpbmdAcGdwLmNvbXBncG1pbWUD
CwkHAhkBBRsBAAAABR4BAAAAAxUICgAKCRAY1dFUqLaa54AuAQCgn5OR4KLbGzGS
4y223be1JC0KTK2GWCuJFIOE7g+llQEAgXbpaNqOcwyOV4PDdZq5Q0bP/QAIhkh8
hSF5N742KlS5AFUESMi92xIIKoZIzj0DAQcBCAcCAwRcohj5GzIlKZaKZuPqmr9l
B4jGEMvzgCglI1syEkJitvoOrR7dpAx/n56WBcMez3GPApv4ikmSGI3qB7HoqOep
iQBqBBgTAgASBQJIyL3bBQkJZgGABRsMAAAAAAoJEBjV0VSotprnk6oBAOetPj4t
5swFTcqfMZ3R4vh1RQ9k+tzlC36u8asprtxaAQCPH8HR2z/tggdsZKpdCtZyv3zn
GRTDr8dsiFqbp9mlIbkAUgRIyL3cEwgqhkjOPQMBBwIDBLUeFv+pPbEPmHfhqywM
weyvlMpIv8q/bwOhoU4hiUs+RMbrUx4tpe73T5kFKCa+9lmeurujkjx+63qpPPd2
ptyJAMoEGBMCAHIFAkjIvd0FCQlmAYAFGwIAAABfIAQZEwgABgUCSMi93AAKCRCE
CRMtbSFDNClhAQCxtGwZqqhYTw7GX1cEtxbYsWOD9ZkocZiU+x+ryCfqBgEA+WQN
Wfw5sfwG0GRScbPwFmkjlMnKwhuFK+g2V45+124ACgkQGNXRVKi2mufzNQEAuLNz
FeS/YwMWhTaedMduGYJG/sNxZhR+lVAgYNSy/FcA/RbDyY6W4C7geTB5iuTicfuK
cJmScU/alaH8fgp3uja1
=FfP1
-----END PGP PUBLIC KEY BLOCK-----
GO_SYSIN_DD
echo ";"

echo "static const char * ECDSApvt256 ="
cat << GO_SYSIN_DD | sed -e'1,3d; $d' | sed -e's/^/"/; s/$/\\n"/; $d'
-----BEGIN PGP PRIVATE KEY BLOCK-----
Version: PGP Command Line v10.0.0 (Linux)

lQB3BE1TXyYTCCqGSM49AwEHAgMEgfu8IO6p6NHDzquwqBhZJbET0axCzVx4QDvY
PaGSNcZe1tsT2R2zRQfQEpv4iYGHjSmtv4/NFyCv23Z7s/yq/wABAKNVkW+GZeuZ
wa9I2VYLXGiJ5Sh7x1qmk6qum9sV6LP9EgS0LGVjX2RzYV9kaF8yNTZfbm9fcGFz
cyA8b3BlbnBncEBicmFpbmh1Yi5vcmc+nQB7BE1TXycSCCqGSM49AwEHAgMEIiEd
GfS4IJIHWNiV4SACr8cSps2OY4qzgIrWwPb2SCxCCaDa3BSnEIuca2ss7tA0AO5i
jWajp1NngXxuxNJ2TwMBCAcAAP9AkDzsKLFKB5MhrZ/UJqeP9Q4Om3rQ2fduu9F7
RDvhyhG2
=YXkt
-----END PGP PRIVATE KEY BLOCK-----
GO_SYSIN_DD
echo ";"

echo "static const char * ECDSAstr256 =\"This is one line\n\";"

echo "static const char * ECDSAsig256 ="
cat << GO_SYSIN_DD | sed -e'1,3d; $d' | sed -e's/^/"/; s/$/\\n"/; $d'
-----BEGIN PGP MESSAGE-----
Version: GnuPG v2.1.0-ecc (GNU/Linux)

owGbwMvMwCHMvVT3w66lc+cwrlFK4k5N1k3KT6nUK6ko8Zl8MSEkI7NYAYjy81IV
cjLzUrk64lgYhDkY2FiZQNIMXJwCMO31rxgZ+tW/zesUPxWzdKWrtLGW/LkP5rXL
V/Yvnr/EKjBbQuvZSYa/klsum6XFmTze+maVgclT6Rc6hzqqxNy6o6qdTTmLJuvp
AQA=
=GDv4
-----END PGP MESSAGE----
GO_SYSIN_DD
echo ";"

echo "static const char * ECDSApub256 ="
cat << GO_SYSIN_DD | sed -e'1,3d; $d' | sed -e's/^/"/; s/$/\\n"/; $d'
-----BEGIN PGP PUBLIC KEY BLOCK-----
Version: PGP Command Line v10.0.0 (Linux)

mQBSBE1TXyYTCCqGSM49AwEHAgMEgfu8IO6p6NHDzquwqBhZJbET0axCzVx4QDvY
PaGSNcZe1tsT2R2zRQfQEpv4iYGHjSmtv4/NFyCv23Z7s/yq/7QsZWNfZHNhX2Ro
XzI1Nl9ub19wYXNzIDxvcGVucGdwQGJyYWluaHViLm9yZz6JAKoEEBMIAFMFAk1T
XyYwFIAAAAAAIAAHcHJlZmVycmVkLWVtYWlsLWVuY29kaW5nQHBncC5jb21wZ3Bt
aW1lBAsHCQgCGQEFGwMAAAACFgIFHgEAAAAEFQgJCgAKCRDQEFX7yt0mjqQxAPYo
zS5vk00/v2zOg6zSX8+9h5C+IQ8M7wuCdI32fBEoAQCrCuL+VrPHKNgcyG/QqRfT
W245HBWY8UaOfEhnYA4rr7kAVgRNU18nEggqhkjOPQMBBwIDBCIhHRn0uCCSB1jY
leEgAq/HEqbNjmOKs4CK1sD29kgsQgmg2twUpxCLnGtrLO7QNADuYo1mo6dTZ4F8
bsTSdk8DAQgHiQBkBBgTCAAMBQJNU18nBRsMAAAAAAoJENAQVfvK3SaOfmgBAKSo
PrKWfvpxtzPQLbogAP7jcpI1+VvCx/DXKzgNspHwAQCafHFCcdvzJf4SU3l/3e9V
2dmbyLwmx1NwJpdLfZQt+g==
=cbCK
-----END PGP PUBLIC KEY BLOCK-----
GO_SYSIN_DD
echo ";"

echo "static const char * ECDSAstr384 =\"This is one line\n\";"

echo "static const char * ECDSAsig384 ="
cat << GO_SYSIN_DD | sed -e'1,3d; $d' | sed -e's/^/"/; s/$/\\n"/; $d'
-----BEGIN PGP MESSAGE-----
Version: PGP Command Line v10.2.0 (Linux)

qANQR1DIqwE7wsvMwCnM2WDcwR9SOJ/xtFISd25qcXFieqpeSUUJAxCEZGQWKwBR
fl6qQk5mXirXoXJmVgbfYC5xmC5hzsDPjHXqbDLzpXpTBXSZV3L6bAgP3Kq7Ykmo
7Ds1v4UfBS+3CSSon7Pzq79WLjzXXEH54MkjPxnrw+8cfMVnY7Bi18J702Nnsa7a
9lMv/PM0/ao9CZ3KX7Q+Tv1rllTZ5Hj4V1frw431QnHfAA==
=elKT
-----END PGP MESSAGE-----
GO_SYSIN_DD
echo ";"

echo "static const char * ECDSApub384 ="
cat << GO_SYSIN_DD | sed -e'1,3d; $d' | sed -e's/^/"/; s/$/\\n"/; $d'
-----BEGIN PGP PUBLIC KEY BLOCK-----
Version: PGP Command Line v10.2.0 (Linux)

mQBvBE1TBZITBSuBBAAiAwME9rjFrO1bhO+fSiCdsuSp37cNKMuMEOzVdnSp+lpn
OJlCti1eUTZ99Me/0/jlAP7s8H7SZaYhqOu75T6UfseMZ366FDvRUzwrNQ4cKfgj
E+HhEI66Bjvh5ksQ5pUOeZwttCRlY19kc2FfZGhfMzg0IDxvcGVucGdwQGJyYWlu
aHViLm9yZz6JAMsEEBMJAFMFAk1TBZIwFIAAAAAAIAAHcHJlZmVycmVkLWVtYWls
LWVuY29kaW5nQHBncC5jb21wZ3BtaW1lBAsJCAcCGQEFGwMAAAACFgIFHgEAAAAE
FQkKCAAKCRAJgDOID1Rxn8orAYCqNzUJaL1fEVr9jOe8exA4IhUtv/BtCvzag1Mp
UQkFuYy0abogj6q4fHQSt5nntjMBf1g2TqSA6KGj8lOgxfIsRG6L6an85iEBNu4w
gRq71JE53ii1vfjcNtBq50hXnp/1A7kAcwRNUwWSEgUrgQQAIgMDBC+qhAJKILZz
XEiX76W/tBv4W37v6rXKDLn/yOoEpGrLJVNKV3aU+eJTQKSrUiOp3R7aUwyKouZx
jbENfmclWMdzb+CTaepXOaKjVUvxbUH6pQVi8RxtObvV3/trmp7JGAMBCQmJAIQE
GBMJAAwFAk1TBZIFGwwAAAAACgkQCYAziA9UcZ+AlwGA7uem2PzuQe5PkonfF/m8
+dlV3KJcWDuUM286Ky1Jhtxc9Be40tyG90Gp4abSNsDjAX0cdldUWKDPuTroorJ0
/MZc7s16ke7INla6EyGZafBpRbSMVr0EFSw6BVPF8vS9Emc=
=I76R 
-----END PGP PUBLIC KEY BLOCK-----
GO_SYSIN_DD
echo ";"

echo "static const char * ECDSAstr521 =\"This is one line\n\";"

echo "static const char * ECDSAsig521 ="
cat << GO_SYSIN_DD | sed -e'1,3d; $d' | sed -e's/^/"/; s/$/\\n"/; $d'
-----BEGIN PGP MESSAGE-----
Version: PGP Command Line v10.2.0 (Linux)

qANQR1DIwA8BO8LLzMAlnO3Y8tB1vf4/xtNKSdy5qcXFiempeiUVJQxAEJKRWawA
RPl5qQo5mXmpXIdmMLMy+AaLnoLpEubatpeJY2Lystd7Qt32q2UcvRS5kNPWtDB7
ryufvcrWtFM7Jx8qXKDxZuqr7b9PGv1Ssk+I8TzB2O9dZC+n/jv+PAdbuu7mLe33
Gf9pLd3weV3Qno6FOqxGa5ZszQx+uer2xH3/El9x/2pVeO4l15ScsL7qWMTmffmG
Ic1RdzgeCfosMF+l/zVRchcLKzenEQA=
=ATtX
-----END PGP MESSAGE-----
GO_SYSIN_DD
echo ";"

echo "static const char * ECDSApub521 ="
cat << GO_SYSIN_DD | sed -e'1,3d; $d' | sed -e's/^/"/; s/$/\\n"/; $d'
-----BEGIN PGP PUBLIC KEY BLOCK-----
Version: PGP Command Line v10.2.0 (Linux)

mQCTBE1TFQITBSuBBAAjBCMEAWuwULfE2XoQmJhSQZ8rT5Ecr/kooudn4043gXHy
NZEdTeFfY2G7kwEaxj8TXfd1U1b4PkEoqhzKxhz/MHK/lwi2ARzW1XQiJ1/kFPsv
IUnQI1CUS099WKKQhD8JMPPyje1dKfjFjm2gzyF3TOMX1Cyy8wFyF0MiHVgB3ezb
w7C6jY+3tCRlY19kc2FfZGhfNTIxIDxvcGVucGdwQGJyYWluaHViLm9yZz6JAO0E
EBMKAFMFAk1TFQIwFIAAAAAAIAAHcHJlZmVycmVkLWVtYWlsLWVuY29kaW5nQHBn
cC5jb21wZ3BtaW1lBAsJCAcCGQEFGwMAAAACFgIFHgEAAAAEFQoJCAAKCRBrQYTh
Ra8v/sm3Agjl0YO73iEpu1z1wGtlUnACi21ti2PJNGlyi84yvDQED0+mxhhTRQYz
3ESaS1s/+4psP4aH0jeVQhce15a9RqfX+AIHam7i8K/tiKFweEjpyMCB594zLzY6
lWbUf1/1a+tNv3B6yuIwFB1LY1B4HNrze5DUnngEOkmQf2esw/4nQGB87Rm5AJcE
TVMVAhIFK4EEACMEIwQBsRFES0RLIOcCyO18cq2GaphSGXqZtyvtHQt7PKmVNrSw
UuxNClntOe8/DLdq5mYDwNsbT8vi08PyQgiNsdJkcIgAlAayAGB556GKHEmP1JC7
lCUxRi/2ecJS0bf6iTTqTqZWEFhYs2aXESwFFt3V4mga/OyTGXOpnauHZ22pVLCz
6kADAQoJiQCoBBgTCgAMBQJNUxUCBRsMAAAAAAoJEGtBhOFFry/++p0CCQFJgUCn
kiTKCNfP8Q/MO2BCp1QyESk53GJlCgIBAoa7U6X2fQxe2+OU+PNCjicJmZiSrV6x
6nYfGJ5Jx753sqJWtwIJAc9ZxCQhj4V52FmbPYexZPPneIdeCDjtowD6KUZxiS0K
eD8EzdmeJQWBQsnPtJC/JJL4zz6JyYMXf4jIb5JyGNQC
=5yaB
-----END PGP PUBLIC KEY BLOCK-----
GO_SYSIN_DD
echo ";"

rm -rf $hdir

