#!/bin/env perl

use strict;
use Test::More tests => 6;
use FindBin qw($Bin);
use File::Temp qw(tempdir);
use File::Copy;
use RPM;
use RPM::Sign;
use RPM::Transaction;

my $passphrase = "RPM4";

my $testdir = tempdir( CLEANUP => 1 );

RPM::setverbosity(3);
RPM::load_macro_file('../macros/macros');
RPM::setverbosity(6);
RPM::add_macro("_dbpath $testdir");
RPM::delete_macro("_hkp_keyserver_query");

copy("test-rpm-1.0-1.noarch.rpm", "$testdir");

RPM::add_macro("_signature gpg");
RPM::add_macro("_gpg_name RPM4 test key");
RPM::add_macro("_gpg_path $Bin/gnupg");

ok(RPM::resign($passphrase, "$testdir/test-rpm-1.0-1.noarch.rpm") == 0, "can resign a rpm");
my $ts = RPM::Transaction->new();
ok(my $db = $ts->opendb(1), "Open a new database");

ok($ts->checkrpm("$testdir/test-rpm-1.0-1.noarch.rpm") != 0, "checking a rpm, key is missing");
ok($ts->checkrpm("$testdir/test-rpm-1.0-1.noarch.rpm", [ "NOSIGNATURES" ]) == 0, "checking a rpm, no checking the key");

ok($ts->importpubkey("$Bin/gnupg/test-key.gpg") == 0, "Importing a public key");

ok($ts->checkrpm("test-rpm-1.0-1.noarch.rpm") == 0, "checking a rpm file");

$db = undef;
