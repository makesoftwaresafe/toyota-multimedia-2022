#!/bin/env perl

use strict;
use Test::More tests => 17;
use RPM;
use RPM::Header;

use_ok('RPM::Dependencies');
my $header = rpm2header('test-rpm-1.0-1.noarch.rpm');
my $deps = RPM::Dependencies->new($header, "REQUIRENAME");
isa_ok($deps, 'RPM::Dependencies');
ok($deps->count, "can get deps count");
{
my @depsname = ();
while ($deps->next()) { push @depsname, $deps->name }
is(scalar(@depsname), $deps->count, "Can get all deps using next");
ok(eq_set(\@depsname, [qw(CompressedFileNames PayloadFilesHavePrefix)]), "can get deps name");
}
$deps->set_index(1);
is($deps->index, 1, "can set and get index");
ok($deps->name, "can get name");
{
    my $n1 = $deps->name;
    $deps->set_index(0);
    ok($n1 ne $deps->name, "set_index really go next dep");
}
ok(defined($deps->flags), "can get flags");
ok($deps->tag, "can get tag");
ok($deps->overlap($deps), "overlap find matching deps");
my $deps2 = RPM::Dependencies->new($header, "PROVIDENAME");
$deps2->set_index(0); 
ok(!$deps2->overlap($deps), "overlap find non matching deps");

my $dsc = RPM::Dependencies->create('PROVIDENAME', 'foo', 8, '1-1');
isa_ok($dsc, 'RPM::Dependencies');
is($dsc->count, 1, "Can create dep");
is($dsc->name, 'foo', 'can get name');
$dsc->add('foo2', 8, '1-1');
{
my @depsname = ();
$dsc->init;
while ($dsc->next()) { push @depsname, $dsc->name }
is(scalar(@depsname), $dsc->count, "Can get all deps using next");
ok(eq_set(\@depsname, [qw(foo foo2)]), "can get deps name");
}
