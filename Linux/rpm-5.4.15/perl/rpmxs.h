/* common function for perl module */

#ifndef _HAVE_RPMXS_H_
#define _HAVE_RPMXS_H_

#define CHECK_RPMDS_IX(dep) if (rpmdsIx((dep)) < 0) \
    croak("You call RPM::Dependencies method after lastest next() of before init()")

rpmTag sv2dbquerytag(SV * sv_tag);

void _rpm2header(rpmts ts, char * filename, int checkmode);

void _newiterator(rpmts ts, SV * sv_tagname, SV * sv_tagvalue, int keylen);

int sv2constant(SV * svconstant, const char * context);

int _headername_vs_dep(Header h, rpmds dep, int nopromote);

int _header_vs_dep(Header h, rpmds dep, int nopromote);

int _specbuild(rpmts ts, Spec spec, SV * sv_buildflags);

void _installsrpms(rpmts ts, char * filename);

void _newspec(rpmts ts, char * filename, SV * svpassphrase, SV * svrootdir, SV * svcookies, SV * svanyarch, SV * svforce, SV * svverify);

#endif
