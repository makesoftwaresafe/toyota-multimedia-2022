package RPM::PackageIterator;

use strict;
use RPM;
use RPM::Header;

=head1 NAME

RPM::PackageIterator

=head1 DESCRIPTION

An iterator over rpm in current transaction.

=head1 FUNCTIONS

=head2 new()

Create a new iterator over an empty transaction.

This is useful to query rpm currently existing in rpmdb. To get an iterator
over a pending transaction see B<packageiterator> function from
L<RPM::Transaction>.

To filter result and using rpmdb index, the function can be called with the
tag to filter and the wanted value:

    RPM::PackageIterator->new("PROVIDESNAME", "rpm");

Will return all installed rpm having "rpm" as PROVIDES tag

=head2 $iter->next()

Return next L<RPM::Header> object found by iterator, nothing if last entry has
been reached.

=head2 $iter->count()

Return the count of entry found by iterator.

=head2 $iter->getoffset

Return the offset of last header return by last L<next> call.

=head2 $iter->prune(@offset)

Filter also rpm located in rpmdb at offset list in @offset.

This is useful from a L<RPM::Transaction> to filter pending removed rpms
when solving dependencies.

=cut

sub _find_all {
  return RPM::PackageIterator->new();
}

sub _find_by_name {
  my ($name) = @_;

  return RPM::PackageIterator->new("NAME", $name);
}

sub _find_by_provides {
  my ($name) = @_;

  return RPM::PackageIterator->new("PROVIDES", $name);
}

sub _find_by_requires {
  my $name = shift;

  return RPM::PackageIterator->new("REQUIRENAME", $name);
}

sub _find_by_file {
  my $name = shift;

  return RPM::PackageIterator->new("BASENAMES", $name);
}

1;
