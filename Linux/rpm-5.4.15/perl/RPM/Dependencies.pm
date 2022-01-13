package RPM::Dependencies;

use strict;
use RPM;
use RPM::Header;


=head1 NAME

RPM::Dependencies

=head1 DESCRIPTION

B<RPM::Dependencies> is an iterator over a list of dependencies.
The object is designed to use only few memory.

=head1 SYNOPSIS

    my $header = rpm2header('test-rpm-1.0-1.noarch.rpm');
    my $deps = RPM::Dependencies->new($header, "REQUIRENAME");
    while ($deps->next) {
        print $deps->name . "\n";
    }

=head1 FUNCTIONS

=head2 new($header, $tag)

Create a new B<RPM::Dependencies> set from $header for $tag type dependencies.

=head2 $ds->count

Return the count of dependencies in set

=head2 $ds->index

Return the current internal index pointer

=head2 $ds->set_index($idx)

Set the internal index pointer to $idx

=cut

sub set_index() {
    my ($self, $index) = @_;
    $self->init;
    foreach (0 .. $index) {
        $self->next or return;
    }
    1;
}

=head2 $ds->init

Initialize the structure for next() call

=head2 $ds->next

Point to next dependencies inside the set. Return the previous index.

The function allways return "True", in the case previous index was 0, it return
"0E0" which is still 0 in an integer context.

=head2 $ds->name

Return the name of the dependency at current index.

=head2 $ds->flags

Return the dependency flags at current index.

=head2 $ds->evr

Return the evr part of dependency at current index if any.

=head2 $ds->color

Return the color of dependency at current index

=head2 $ds->tag

Return the tag type of the dependencies set.

=head2 $tag->create($tag, $name, $flags, $evr)

Create a new B<RPM::Dependencies> object with one entry.

=head2 $tag->add($name, $flags, $evr)

Add a new entry into the dependency set.

=head2 $ds->overlap($ds2)

Return True if both dependency at both index currently match.

=cut

# currently everything is in the XS code

1;

__END__

=head1 AUTHOR

Olivier Thauvin <nanardon@nanardon.zarb.org>

=head1 LICENSE

This software is under GPL, refer to rpm license for details.

=cut
