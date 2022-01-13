package URPM;

use strict;
use warnings;

# Olivier Thauvin <thauvin@aerov.jussieu.fr>
# This package extend URPM functions to permit
# URPM low level query on rpm header
# $Id: Query.pm 270395 2010-07-30 00:55:59Z nanardon $

# tag2id
# INPUT array of rpm tag name
# Return an array of ID tag

sub tag2id {
    my @l = @_;
    my %taglist = URPM::list_rpm_tag();
    map { $taglist{uc($_)} || undef } @l;
}

sub query_pkg {
   my (undef, $pkg, $query) = @_;
   my @tags = map {
	   [ $pkg->get_tag(tag2id($_)) ]
   } $query =~ m/\%\{([^{}]*)\}*/g;

   $query =~ s/\%\{[^{}]*\}/%s/g;
   $query =~ s/\\n/\n/g;
   $query =~ s/\\t/\t/g;
   my ($max, @res) = 0;

   foreach (@tags) { $max < $#{$_} and $max = $#{$_} };

   foreach my $i (0 .. $max) {
       push(@res, sprintf($query, map { ${$_}[ $#{$_} < $i ? $#{$_} : $i ] } @tags));
   }
   @res
}

1;
