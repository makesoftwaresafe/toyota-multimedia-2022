#!/usr/bin/perl

use strict;
use warnings;
use Test::More tests => 1;
use Cwd;

chdir 't' if -d 't';
mkdir "tmp";
for (qw(BUILD SOURCES RPMS RPMS/noarch)) {
    mkdir "tmp/".$_;
}
# locally build a test rpm
system(rpmbuild => '--define', '_topdir '. Cwd::cwd() . "/tmp/", '-bb', 'test-rpm.spec');
ok( -f 'tmp/RPMS/noarch/test-rpm-1.0-1mdk.noarch.rpm', 'rpm created' );

