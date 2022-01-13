package MY; # so that "SUPER" works right

#X use strict;
use ExtUtils::MakeMaker;
# See lib/ExtUtils/MakeMaker.pm for details of how to influence
# the contents of the Makefile that is written.

# where to find the rpm utility

my $rpm_path = $ENV{RPM_PATH}; # this overrides

unless (defined $rpm_path) {
    for (qw(/bin/rpm /usr/bin/rpm /usr/local/bin/rpm /opt/local/bin/rpm )) {
	if (-x) {
	    $rpm_path = $_;
	    last;
	}
    }
}

defined $rpm_path or die "Can't find rpm on this system\n";

#X sub hexversion {
#X 	my ($major, $minor, $micro) = (@_[0] =~ /(\d+)\.(\d+)\.?(\d+)?/);
#X 	return int($major<<16) + int($minor<<8) + int($micro<<0)
#X }

#X my $version = `pkg-config --modversion rpm`;
#X my $hversion = hexversion($version);
#X $hversion ge hexversion("5.3") or die "Unable to build URPM with too old (or undetected) rpm version $version\n";

my $version = qw(5.4.8);

#X # to generate the ChangeLog depending on the checkout layout
#X my $commonusername = "../common/";
#X -d $commonusername or do {
#X     $commonusername = "../../common/";
#X     -d $commonusername or do {
#X 	$commonusername = "../../../common/";
#X 	-d $commonusername or $commonusername = "";
#X     };
#X };

#X sub MY::postamble {
#X     <<"**MM**";
#X .PHONY: ChangeLog

#X ChangeLog:
#X 	LC_ALL=C svn2cl --accum --strip-prefix=soft/rpm/perl-URPM/trunk --authors ${commonusername}username.xml
#X 	rm -f *.bak
#X **MM**
#X }

my @libdir = qw(. build lib popt rpmdb rpmio misc rpmconstant);

my @ldaddp = map { '-L../' . $_ . '/.libs' } @libdir;
my @ldadd = map { '-l' . $_ } qw(rpmmisc rpmio rpmbuild rpm popt rpmdb rpmconstant);

my @cppflags = qw(
    -I.
    -I..
    -I../lib
    -I../lib
);

my @objects = qw(
    URPM.o 
);

# teach MakeMaker some Mac OS X LDFLAGS:
my @LDFLAGS = qw(                                             -L/usr/lib64/perl5/CORE -Wl,-rpath,/usr/lib64/perl5/CORE -L/usr/java/jdk1.8.0_11/jre/lib/amd64/server -Wl,-rpath,/usr/java/jdk1.8.0_11/jre/lib/amd64/server     -lpthread -lrt  -lelf $(CCCDLFLAGS) $(CCDLFLAGS) -L$(PERL_ARCHLIB)/CORE -lperl );
my $uname = `uname`; chomp($uname);
foreach (@LDFLAGS) {
    if ("$uname" eq "Darwin" && m/^(-arch|ppc|i386|ppc64|x86_64)$/) {
         push (@ldflags, $&);
    } elsif ("$uname" eq "Darwin" && m/-Wl,-search_paths_first/) {
         push (@ldflags, $&);
    } else {
         push (@ldlibs, $_);    
    }
}

# teach MakeMaker some VPATH $< manners:
sub c_o {
    my $inherited = shift->SUPER::c_o(@_);
    $inherited =~ s/(\.\w+\$\(OBJ_EXT\)\:\n\t.*?)\$\*\.\w+/$1\$\</g;
    $inherited;
}
sub xs_c {
    my $inherited = shift->SUPER::xs_c(@_);
    $inherited =~ s/(\.xs\.c\:\n\t.*?)\$\*\.\w+/$1\$\</g;
    $inherited;
}
sub xs_o {
    my $inherited = shift->SUPER::xs_o(@_);
    $inherited =~ s/(\.\w+\$\(OBJ_EXT\)\:\n\t.*?)\$\*\.\w+/$1\$\</g;
    $inherited;
}

#X my $ldflags = `pkg-config --libs rpm`;
#X chomp($ldflags);
#X $ldflags .= " -lmagic -ldb";
my $ldflags = join(' ', @ldaddp, @ldadd, @ldlibs);

#X my $includes = `pkg-config --cflags-only-I rpm`;
my $includes = join(' ', @cppflags, map { '-I../'. $_ } @libdir) . '  -DRPM_OS_LINUX=020632   -I/usr/include/nss3 -I/usr/include/nspr4       -DLTM_DESC -I${prefix}/include/tommath         -I/usr/include/pakchois     -I/usr/include/apr-1 -I/usr/include/subversion-1 -I/usr/include/db61 -I/usr/include/libxml2   -I/usr/lib64/perl5/CORE -I/usr/include/python2.6 -I/usr/include/python2.6 -I/usr/lib64/ruby/1.8/x86_64-linux -I/usr/lib64/ruby/1.8/x86_64-linux -I/usr/java/jdk1.8.0_11/include -I/usr/java/jdk1.8.0_11/include/linux -I/usr/java/jdk1.8.0_11/include -I/usr/java/jdk1.8.0_11/include -I/usr/include/squirrel   -I/usr/include/gpsee -I/usr/include/gpsee -I${prefix}/include/js';

#X my $ccflags = join(' ', '-Wall -Wextra -fno-strict-aliasing -std=gnu99', `pkg-config --cflags-only-other rpm`);
#X chomp($ccflags);
my $ccflags = join(' ', '-g -O2 -fPIC -DPIC -Wall -W -Wpointer-arith -Wstrict-prototypes -Wmissing-prototypes -Wno-char-subscripts -Wno-unused-parameter -Wformat-security -Wno-override-init -g3 -fno-delete-null-pointer-checks -D_FORTIFY_SOURCE=2 -fstack-protector -fexceptions -fexceptions -Wno-missing-field-initializers -D_GNU_SOURCE -D_REENTRANT                        ', '-std=gnu99', '-Wno-unused-variable', '-Wno-missing-prototypes', '-Wno-implicit', '-Wno-strict-prototypes' );

#X print "Found RPM version $version (compiling with flags: $ccflags)\n";

WriteMakefile(
    'NAME'	=> 'URPM',
    'OBJECT'    => join(' ', @objects),
    'VERSION_FROM' => 'URPM.pm',
    'MAKEFILE'	=> 'Makefile.perl',
    'CCFLAGS'	=> $ccflags,
    'dynamic_lib' => { OTHERLDFLAGS => $ldflags },
    'OPTIMIZE'	=> '-g',
    'INC'	=> $includes,
    'INSTALLMAN1DIR' => '${prefix}/share/man/man1',
    'INSTALLMAN3DIR' => '${prefix}/share/man/man3',

    'PREREQ_PM'	=> {
	'MDV::Packdrakeng' => '1.00',
    },
    'dist'	=> { COMPRESS => "xz -f", SUFFIX => ".xz" },
    'realclean'	=> { FILES => "t/RPMS/noarch/*" },
);
