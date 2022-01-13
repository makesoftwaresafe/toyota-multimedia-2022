#!/bin/env perl

use strict;

use Test::More tests => 12;

use_ok('RPM');
like(RPM::expand_macro('%{?_rpmversion}%{?!_rpmversion:ver}'), '/^[^%]/', "can expand %_rpmversion macro");
RPM::add_macro('_test_macro I_am_set');
is(RPM::expand_macro('%_test_macro'), 'I_am_set', 'setting a macro works');
RPM::delete_macro('_test_macro');
is(RPM::expand_macro('%_test_macro'), '%_test_macro', 'deleting a macro works');

can_ok('RPM', qw(rpmlog setlogcallback setlogfile lastlogmsg setverbosity));
ok(!rpmlog('DEBUG', 'test message'), "can rpmlog()");

{
ok(!setverbosity('debug'), "can set verbosity");
my $logcall = 0;
ok(!setlogcallback(sub { $logcall++ }), "can set log callback");
rpmlog('WARNING', 'test message');
is($logcall, 1, "log callback is really call");
ok(!setlogcallback(), "can reset logcallback");
ok(!setverbosity('WARNING'), "can set verbosity");
is($logcall, 1, "log callback is no longer call");
}

