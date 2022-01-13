#!/bin/env perl

use strict;
use Test::More tests => 5;
use RPM;
use RPM::Header;
use RPM::Transaction;

my $tempdbpath = `pwd`;
chomp($tempdbpath);
$tempdbpath .= '/t/tempdb';

END {
    clean();
}

sub clean {
    system(qw(rm -fr), $tempdbpath);
}

clean();

# Creating temp db
mkdir($tempdbpath) or die "Cannot create $tempdbpath";
my @rpmcmd = (qw(../rpm --macros ../macros/macros --define), "_dbpath $tempdbpath", '-qa');
system(@rpmcmd) and die "Cannot init db in $tempdbpath";
system(qw(../rpm --macros ../macros/macros), '--define', "_dbpath $tempdbpath", qw(-U test-rpm-1.0-1.noarch.rpm --justdb --nodeps));

RPM::setverbosity(3);
RPM::load_macro_file('../macros/macros');
RPM::setverbosity(6);
RPM::add_macro("_dbpath $tempdbpath");
use_ok('RPM::PackageIterator');

isa_ok(RPM::PackageIterator->new(), 'RPM::PackageIterator');

{
my $ts = RPM::Transaction->new();
my $pi = $ts->packageiterator();
isa_ok($pi, 'RPM::PackageIterator');
#ok($pi->count, "Can get Iterator count");
my $h = $pi->next;
isa_ok($h, 'RPM::Header');
is($h->tag('name'), 'test-rpm', "can get header name from db");
$ts->closedb();
}

clean();
