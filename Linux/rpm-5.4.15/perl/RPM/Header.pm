package RPM::Header;

use strict;
use Exporter;
use RPM;
use vars qw($AUTOLOAD);

use overload '<=>'  => \&_op_spaceship,
             'cmp'  => \&_op_spaceship,
             'bool' => \&_op_bool;

our @ISA = qw(Exporter);

our @EXPORT = qw(
    stream2header
    rpm2header
);

=head1 NAME

RPM::Header

=head1 DESCRIPTION

A module to perform action on rpm b<header>.

The header is a set of data containing information about a rpm archive.

=head1 SINOPSYS

    use RPM::Header;

    my $header = rpm2header('any-1-1.noarch.rpm');
    print $header->header_sprintf('%{NAME}\n');
    # output "any" + carriage return

=cut

=head1 FUNCTIONS

=head2 rpm2header(rpmfile, vsflags)

Read a rpm archive and return a RPM::Header object on success, return undef
on error.

B<vsflags> (optional) is either an integer value, either an arrayref
containing textual values. See L<RPM::Constant>.

=head2 stream2header(filehandle)

Read all header store into a file from an open file handle.

Two usage:

=over 4

=item stream2header(filehandle)

stream2header return an array b<RPM::Header> object.

=item stream2header(filehandle, $callback)

The function nothing, for each header read, $callback function is called
with 'RPM::Header' passed as argument.

=back

=cut

=head1 RPM::Header FUNCTIONS

In addition to the following methods, all tags have simple accessors;
$hdr->epoch() is equivalent to $hdr->tag('epoch').

The <=> and cmp operators can be used to compare versions of two packages.

=cut

# proxify calls to $header->tag()
sub AUTOLOAD {
    my ($header) = @_;

    my $tag = $AUTOLOAD;
    $tag =~ s/.*:://;
    return $header->tag($tag);
}

sub _op_bool {
    my ($self) = @_;
    return (defined($self) && ref($self) eq 'RPM::Header');
}

=head2 $hdr->listtag

Return an array listing all tag set into the header.

=head2 $hdr->hastag($tag)

Return True is B<$tag> is set into the header.

B<$tag> can be either the tagname, either the internal numeric value.

=head2 $hdr->tagformat($format)

Return a format string from header like rpm --qf.

=head2 $hdr->tag($tag)

Return tag content from the header, undef if tag is not set.

B<$tag> can be either the tagname, either the internal numeric value.

=head2 $hdr->tagtype($tag)

Return the tag type from the header, undef if tag is not set.

B<$tag> can be either the tagname, either the internal numeric value.

=head2 $hdr->removetag($tag)

Remove the tag from header.

=head2 $hdr->addtag($tag, $tagtype, @value)

Add a tag to the header.

=head2 $hdr->compare($header)

Compare two header.

=head2 $hdr->is_source_package()

Returns a true value if the package is a source package, false otherwise.

=head2 $hdr->copy

Return a B<RPM::Header> copy of the header.

=head2 $hdr->hsize

Return the size in byte of the header in it on disk rpresentation.

=head2 $hdr->string

Return a in memory string representation of $hdr

=head2 $hdr->write($handle)

Dump header into open file handle B<$handle>.

=cut

=head2 $hdr->changelog

Returns a list of hash refs containing the change log data of the package.
The hash keys represent individual change log entries, and their keys are:
C<time> (the time of the changelog entry), C<name> (the "name", ie. often
the email address of the author of the entry), and C<text> (the text of the
entry).

=cut

sub changelog {
  my $self = shift;

  if (not exists $self->{changelog}) {
    my @cltimes = $self->tag('CHANGELOGTIME');
    my @clnames = $self->tag('CHANGELOGNAME');
    my @cltexts = $self->tag('CHANGELOGTEXT');

    my @changelog;
    foreach (0 .. $#cltimes) {
      push(@changelog,
           { time => $cltimes[$_],
             name => $clnames[$_],
             text => $cltexts[$_],
           });
    }

    $self->{changelog} = \@changelog;
  }

  return @{$self->{changelog}};
}

=head2 $hdr->as_nvre

Returns a string formatted like:

   epoch:name-version-release

If epoch is undefined for this package, it and the leading colon are omitted.

=cut

sub as_nvre {
  my ($self) = @_;
  my $epoch = $self->tag('epoch');
  my $epoch_str = '';

  $epoch_str = "$epoch:" if defined $epoch;

  my $ret = $epoch_str . join("-", map { $self->tag($_) } qw/name version release/);

  return $ret;
}

=head2 $hdr->dependencies($tag)

Create a new L<RPM::Dependencies> object from RPM::Header B<$hdr>.

B<$tag> is either PROVIDENAME, REQUIRENAME, CONFLICTNAME, OBSOLETENAME
or TRIGGERNAME.

=head2 $hdr->files()

Return a L<RPM::Files> object from B<RPM::Header> $hdr.

=cut

1;

__END__

=head1 AUTHOR

Olivier Thauvin <nanardon@nanardon.zarb.org>

=cut
