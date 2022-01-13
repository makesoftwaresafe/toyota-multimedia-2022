package RPM::Problems;

use strict;
use Exporter;
use RPM;
use RPM::Transaction;

=head1 NAME

RPM::Problems

=head1 DESCRIPTION

=cut

# currently everything is in the XS code

=head1 FUNCTION

=head2 new($ts)

Create a new B<RPM::Problems> from B<$ts> RPM::Transaction

=head2 $ps->count

Return the count of problem in the RPM::Problems

=head2 $ps->print($handle)

Print into B<$handle> the list of problem

=head2 $ps->pb_info($num)

Return an hashref containing information for problem B<$num>

=over 4

=item string The problem in textual form

=item pkg_nevr The package causing the problem

=item alt_pkg_nevr The second package causing the problem, if any

=item type The type of the problem

=item key The key passed to  L<RPM::Transaction::add_install>

=back

=cut

1;

__END__

=head1 AUTHOR

Olivier Thauvin <nanardon@nanardon.zarb.org>

=head1 LICENSE

This software is under GPL, refer to rpm license for details.

=cut
