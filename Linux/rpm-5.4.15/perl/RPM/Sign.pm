package RPM::Sign;

use strict;
use Exporter;
use RPM;
use RPM::PackageIterator;
use RPM::Header;
use vars qw($AUTOLOAD);

our @ISA = qw(Exporter);

our @EXPORT = qw(
);

sub new {
    my ($class, %options) = @_;

    my $Sign;
    $Sign = {
        _signature => undef,
        name => undef,
        path => undef,
        checkrpms => 1,
        passphrase => undef,

        password_file => undef,
    };

    foreach (keys %$Sign) {
        defined($options{$_}) and $Sign->{$_} = $options{$_};
    }
    
    bless($Sign, $class);
    $Sign->getpubkey();
    $Sign->getpasswdfile();
    $Sign;
}

sub getpasswdfile {
    my ($self) = @_;
    $self->{password_file} or return 1;
    open(my $hpass, "<", $self->{password_file}) or return 0;
    $self->{passphrase} = <$hpass>;
    chomp($self->{passphrase});
    close($hpass);
    1;
}

sub adjustmacro {
    my ($self) = @_;

    defined($self->{_signature}) and RPM::add_macro("_signature $self->{_signature}");

    foreach my $macro (qw(_gpg_name _pgp_name)) {
        RPM::add_macro("$macro $self->{name}") if (defined($self->{name}));
    }
    
    foreach my $macro (qw(_gpg_path _pgp_path)) {
        RPM::add_macro("$macro $self->{path}") if (defined($self->{path}));
    }
}

sub restoremacro {
    my ($self) = @_;

    if (defined($self->{_signature})) { RPM::delete_macro('_signature'); }
    
    if (defined($self->{name})) {
        RPM::delete_macro('_gpg_name');
        RPM::delete_macro('_pgp_name');
    }

    if (defined($self->{path})) {
        RPM::delete_macro('_gpg_path');
        RPM::delete_macro('_pgp_path');
    }
}

sub getpubkey {
    my ($self) = @_;
    $self->adjustmacro();
    my $gpgcmd;
    if (RPM::expand_macro("%_signature") eq "gpg") {
        $gpgcmd = '%__gpg --homedir %_gpg_path --list-public-keys --with-colons \'%_gpg_name\'';
    }
    open(my $hgpg, RPM::expand_macro($gpgcmd) .'|') or return undef;
    while (my $l = <$hgpg>) {
        chomp($l);
        my @v = split(':', $l);
        if ($v[0] eq 'pub') {
           $self->{keyid} = $v[4];
           last;
        }
    }
    close($hgpg);
    $self->restoremacro();
}

sub rpmsign {
    my ($self, $rpm, $header) = @_;
    my $need = 1;

    $header or return -1;
    
    if (RPM::expand_macro("_signature") || "" eq "gpg") {
        my $sigid = $header->tagformat("%{DSAHEADER:pgpsig}");
        ($sigid) = $sigid =~ m/Key ID (\S+)/;
        if ($sigid && lc($sigid) eq lc($self->{keyid} || "")) { $need = 0 }
    }
    
    if ($need > 0) {
        $self->adjustmacro();
        RPM::resign($self->{passphrase}, $rpm) and $need = -1;
        $self->restoremacro();
    }

    $need;
}

sub rpmssign {
    my ($self, @rpms) = @_;
    my $ts = RPM::Transaction->new();
    $ts->vsflags("NOSIGNATURES");
    foreach my $rpm (@rpms) {
	my $header = $ts->readheader($rpm);
	defined($header) or do {
	    RPM::rpmlog("ERR", "bad rpm $rpm");
	    return;
	};
	my $res = $self->rpmsign($rpm, $header);
	if ($res > 0) { RPM::rpmlog("INFO", "$rpm has been resigned");
	} elsif ($res < 0) { RPM::rpmlog("ERR", "Can't resign $rpm"); }
    }
}

1;

__END__

=head1 NAME

RPM::Sign

=head1 SYNOPSIS

A container to massively resign packages

=head1 DESCRIPTION

This object retains gpg options and provides functions to easilly sign or
resign packages. It does not resign packages having already the proper
signature.

=head1 METHODS

=head2 new(%options)

Create a new RPM::Sign object.

Options are:

=over 4

=item name

The gpg key identity to use

=item path

the gpg homedir where keys are located

=item password_file

Use passphrase contains in this files

=item passphrase

Use this passphrase to unlock the key

=item checkrpms

Set to 0 remove the signature checking on packages

=back

=head2 rpmssign(@filelist)

Sign or resign the packages passed are arguments

=head1 SEE ALSO

L<RPM>
L<RPM::Header>

=cut
