package RPM;

use 5.00503;
use strict;
use DynaLoader;
use Exporter;
use Cwd qw/realpath/;
use File::Basename qw/basename dirname/;
use File::Spec ();

use vars qw/@ISA @EXPORT/;
@ISA = qw/DynaLoader Exporter/;

@EXPORT = qw(
    rpmlog
    setlogcallback
    setlogfile
    lastlogmsg
    setverbosity
    rpmvercmp
    add_macro
    delete_macro
    load_macro_file
    reset_macros
    dump_macros
    platformscore
    installsrpm
);

bootstrap RPM;

1;

__END__
# Below is stub documentation for your module. You better edit it!

=head1 NAME

RPM - Perl bindings for the RPM Package Manager API

=head1 SYNOPSIS

  use RPM;

=head1 DESCRIPTION

The RPM module provides an object-oriented interface to querying both
the installed RPM database as well as files on the filesystem.

=head1 FUNCTIONS

=head2 installsrpm($filename)

Install a source rpm and return spec file path and its cookies.
Returns undef if install is impossible.

see L<RPM::Spec>->new() for more information about cookies.

=head2 GENERICS FUNCTIONS

=head3 rpmversion

Return the rpm version which is also the module version:

    RPM::rpmversion(); # return 5.0.DEVEL currently ;)

=head2 MACROS FUNCTIONS

=head3 expand_macro($string)

Return the string after macros expansion:

    expand_macro('%_dbpath'); # will return '/var/lib/rpm' on most system,
                              # depending of your config
    expand_macro('%{?_dbpath:is set}'); # will return is set, normally... :)

=head3 add_macro($string)

Set or overide a macro, the format to use is the macro name (w/o %) follow
by its definition:

    add_macro('_anymacros anyvalue');
    print expand_macro('%_anymacros'); # show 'anyvalue'

=head3 delete_macro($macro)

Delete a macro definition:

    delete_macro('_anymacros');

=head3 dump_macros($handle)

Dump macros all macros currently defined into $handle. If $handle is missing,
STDOUT is used.

=head3 load_macro_file($file)

Load a macro file.

=head3 reset_macros

Reset all macros to default config (aka from rpm configuration).

=head2 PLATFORM FUNCTIONS

=head3 setverbosity($verbosity)

Set the global verbosity of rpmlib.

=head3 rpmvercmp($verA, $verB)

Compare two version (or release, not both) and return:
  * -1 verA < verB
  *  0 verA = verB
  *  1 verA > verB

    rpmvercmp('1', '1.1'); # return 1
    # This works but will not give the expected result:
    rpmvercmp('1-1', '1.1-2') # return -1

=head3 platformscore($platform)

Return the score of platform according your rpm configuration,
0 if not compatible.

    # on x86_64 under Mandriva Linux:
    platformscore("i586-mandriva-linux-gnu");
    # return 8 here
    platformscore("ppc-mandriva-linux-gnu");
    # return 0

=head2 LOG AND VERBOSITY FUNCTIONS

=head3 lastlogmsg

Return the last message give by rpm. In array context return both
the erreur level and message:

    my $message = rpmlog();
    my ($code, $message) = rpmlog();

=head3 rpmlog($err_level, $message)

Log a message using rpm API.

=head3 setlogcallback

DEPRECATED ?

=head3 setlogfile

DEPRECATED ?


=head1 TODO

Make package installation and removal better (-;.

Signature validation.

=head2 resign
Resign a package.

=head1 HISTORY

=over 8

=item 0.5

Massive Rework:

=item 0.01

Initial release

=back


=head1 AUTHOR

Olivier Thauvin E<lt>nanardon@rpm5.orgE<gt>

Original author:

Chip Turner E<lt>cturner@redhat.comE<gt>

=head1 SEE ALSO

L<perl>.
L<RPM::Header>
L<RPM::PackageIterator>
L<RPM::Transaction>
L<RPM::Constant>
L<RPM::Files>
L<RPM::Dependencies>

=cut
