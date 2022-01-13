package RPM::Spec;

use strict;
use Exporter;
use RPM;

=head1 NAME

RPM::Spec

=head1 DESCRIPTION

An object to handle operation around specs file

=head1 FUNCTIONS

=head2 new($specfile, %options)

Create a new B<RPM::Spec> object over $specfile spec file.

Optional options are:

=over 4

=item force Don't check all sources files exists

=item passphrase The passphrase to use to sign rpm after build

=item anyarch Don't check architecture compatibility

=item verify

=item root Root directory to use to build the spec file

=back

=head2 srcheader

Return a L<RPM::Header> object about the future source rpm

=head2 binheader

Return an array of L<RPM::Header> object about future binaries rpms

=head2 srcrpm

Return the full path where srcrpm will located after build

=head2 binrpm

Return an array containing path where binaries rpms will be located after
build

=head2 check

Perform build dependencies check, return undef if everythings is fine, or
a L<RPM::Problems> object containing list of issue.

=head2 build( $flags )

Build rpms from the specfile. $flags is either an integer, either an array ref
of build step to perform.

=head2 specfile

Return the specfile location.

=head2 sources

Return an array listing sources included in the specfile.

=head2 sources_url

Return an array listing sources url included in the specfile.

=head2 icon

Return icon filename if included in the specfile.

=head2 icon_url

Return icon url if included in the specfile.
=cut

# currently everything is in the XS code

1;

__END__

=head1 AUTHOR

Olivier Thauvin <nanardon@nanardon.zarb.org>

=head1 LICENSE

This software is under GPL, refer to rpm license for details.

=cut
