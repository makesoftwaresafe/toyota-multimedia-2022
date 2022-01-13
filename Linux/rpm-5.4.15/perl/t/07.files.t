#!/bin/env perl

use strict;
use Test::More tests => 3;
use RPM;
use RPM::Header;

use_ok('RPM::Files');
my $header = rpm2header('test-rpm-1.0-1.noarch.rpm');
my $files = RPM::Files->new($header);
isa_ok($files, 'RPM::Files');
is($files->count, 1, "can get file count");
