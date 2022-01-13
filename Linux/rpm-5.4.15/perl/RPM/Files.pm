package RPM::Files;

use strict;
use RPM;
use RPM::Header;

=head1 NAME

RPM::Files

=head1 DESCRIPTION

A files set iterator.

=head1 FUNCTIONS

=head2 new($header)

Create a new B<RPM::Files> iterator from $header. $header is a
L<RPM::Header> object.

=head2 $files->count

Return the count of files inside the iterator.

=head2 $files->init

Prepare the file iterator for first L<next> call

=head2 $files->next

Jump to next entry and return current internal index. The function return
"OEO" instead 0, which is true. Return nothing when last entry is reached.

=head2 $files->count_dir

Return the number of parent directory.

=head2 $files->init_dir

Preprare iterator for next_dir first call

=head2 $files->next_dir

Jump to next entry and return current internal index. The function return
"OEO" instead 0, which is true. Return nothing when last entry is reached.

=head2 $files->filename

Return the current filename

=head2 $files->basename

Return the current basename

=head2 $files->dirname

Return the parent directory path of current file

=head2 $files->fflags

Return the rpm file flags fo current file

=head2 $files->mode

Return the (unix) mode of current file

=head2 $files->digest

Return the digest of the file and the id of used algorythm.

=head2 $files->link

If the current file is a symlink, return the target path.

=head2 $files->user

Return the owner of the file.

=head2 $files->group

Return the group owning the file.

=head2 $files->inode

Return the inode number of the file.

=head2 $files->size

Return the size of the current file.

=head2 $files->dev

Return de device ID where the file is located.

=head2 $files->color

Return de file color of the current file.

=head2 $files->class

Return the class of the current file, aka the mime type as given
by L<file(1)>.

=head2 $files->mtime

Return the modification time of the current file.

=cut

# currently everything is in the XS code

1;

__END__

=head1 AUTHOR

Olivier Thauvin <nanardon@nanardon.zarb.org>

=head1 LICENSE

This software is under GPL, refer to rpm license for details.

=cut
