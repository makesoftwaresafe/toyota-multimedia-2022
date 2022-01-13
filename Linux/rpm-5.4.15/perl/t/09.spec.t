#!/bin/env perl

use strict;
use Test::More tests => 11;

use_ok('RPM::Spec');

my $spec = RPM::Spec->new('test-rpm.spec');
isa_ok($spec, 'RPM::Spec');
my $srcheader = $spec->srcheader;
isa_ok($srcheader, 'RPM::Header');
my @binheaders = $spec->binheader;
isa_ok($binheaders[0], 'RPM::Header');
like($spec->srcrpm, '/^\/.*\/test-rpm-1.0-1.src.rpm$/', "can get srcrpm path");
my @binrpm = $spec->binrpm;
like($binrpm[0], '/^\/.*\/test-rpm-1.0-1.noarch.rpm$/', "can get binrpm path");
ok(!$spec->check, "Can perform check");
ok(!$spec->build([ 'PREP' ]), "Can build the specfile");
is($spec->specfile, 'test-rpm.spec', "can get specfile location");
is($spec->sources, 'test-rpm.spec', "can get source list");
is($spec->sources_url, 'test-rpm.spec', "can get source list");
