#!/bin/env perl

use strict;
use Test::More tests => 5;

use_ok('RPM::Constant');
can_ok('RPM::Constant', qw(listallcontext listcontext getvalue));
my ($context) = listallcontext();
ok($context, "Can get context list");
my ($textual) = listcontext($context);
ok($textual, "can get list of value");
ok(defined(getvalue($context, $textual)), "can get value from context");
