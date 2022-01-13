package RPM::Constant;

use strict;
use Exporter;
use RPM;

our @ISA = qw(Exporter);

our @EXPORT = qw(
    listallcontext
    listcontext
    getvalue
);

=head1 NAME

RPM::Constant

=head1 DESCRIPTION

RPM::Constant provide function to map internal C value use inside rpm
to textual value anybody can use in all RPM modules.

=head1 SYNOPSIS

    use RPM::Constant;

    my $value = getvalue("anyflagset", "anyflags");
    if (defined($value)) {
        print "Value: $value\n";
    } else {
        print STDERR "cannot find anyflags in anyflagset\n";
    }

=head1 FUNCTIONS

=head2 listallcontext

Return an array listing all existing context, aka flags type.

=head2 listcontext(contextname)

Return all textuals value handle in a context, undef if B<contextname> does
not exists.

=head2 getvalue(contextname, value)

Return the internal C value for B<value> textual value, undef B<value> in
B<contextname> cannot be found.

=cut

# currently everything is in the XS code

1;

__END__

=head1 AUTHOR

Olivier Thauvin <nanardon@nanardon.zarb.org>

=head1 LICENSE

This software is under GPL, refer to rpm license for details.

=cut
