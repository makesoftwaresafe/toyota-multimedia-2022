#!/bin/env perl

use Test::More;
eval "use Test::Pod::Coverage";
plan skip_all => "Test::Pod::Coverage required for testing pod coverage" if $@;

plan tests => 9;

pod_coverage_ok( "RPM", "RPM is covered" );
pod_coverage_ok( "RPM::Constant", "RPM::Constant is covered" );
pod_coverage_ok( "RPM::Header", "RPM::Header is covered" );
pod_coverage_ok( "RPM::Transaction", "RPM::Transaction is covered" );
pod_coverage_ok( "RPM::PackageIterator", "RPM::PackageIterator is covered" );
pod_coverage_ok( "RPM::Problems", "RPM::Problems is covered" );
pod_coverage_ok( "RPM::Files", "RPM::Files is covered" );
pod_coverage_ok( "RPM::Dependencies", "RPM::Dependencies is covered" );
pod_coverage_ok( "RPM::Spec", "RPM::Spec is covered" );

