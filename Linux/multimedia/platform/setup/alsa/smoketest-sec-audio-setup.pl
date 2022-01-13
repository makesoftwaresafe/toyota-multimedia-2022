#!/usr/bin/perl

my $testname="audio-coldplug-setup";

my $systemd_service_dir="/lib/systemd/system";
my @services = (
"audio-setup-imx6q-sabreauto-masca.service",
"audio-setup-imx6q-sabresd.service",
"audio-setup-imx6qdl-audioboard.service",
"audio-setup-imx6qdls-sabreauto.service",
"audio-setup-mxc-asrc.service",
"audio-setup-x86-vmware.service",
"audio-setup-x86-virtualbox.service");

my $result = 0;

foreach $service (@services) {
    $filename = $systemd_service_dir."/".$service;
    if (-e $filename) {
	    `systemctl start $service`;
	    my $res = $?;

	    if ($res != 0) {
		    print "Unexpected returnvalue when starting $service\n";
		    $result = 1;
	    } else {
		    my $systemd_status = `systemctl --no-pager status $service`;
		    if ($systemd_status =~ m/Active:\ inactive.*ConditionPathExistsGlob=.*\ was\ not\ met/ms) {
			    print "$service not relevant for this board\n";
		    } else {

			    if ($systemd_status =~ m/Active:\ active/m) {
				    print "Testing $service\n";

				    my $kmod_conf = get_kmodules_device_sync_conf ($service);
				    print "  found $kmod_conf";
				    if ($kmod_conf eq "") {
					    print "  no kmodules config found\n";
					    $result = 1;
				    } else {
					    print "  found $kmod_conf\n";

					    my $checkoutput=`/opt/platform/SEC_SMOKE_HELP/sec_smk_check_coldplug_udev_rules.pl $kmod_conf`;
					    print "  Checker output : $checkoutput\n";
                        if ($? != 0) {
                            $result = 1;
                        }
				    }

			    } else {
				    print "Unexpected status for $service\n";
				    $result = 1;
			    }
		    }
	    }
    } else {
		    print "$service not deployed\n";
    }
}

if ($result == 0) {
	print "\n$testname PASS $result\n";
	exit $result;
} else {
	print "\n$testname FAIL $result\n";
	exit $result;
}


sub get_kmodules_device_sync_conf {
	my $service = shift;
	my $kmod_devsy_conf = "";

	open FILE, $systemd_service_dir."/".$service or $result=1;
	while (<FILE>) {
		if ($_ =~ m/^\s*ExecStart\s*=\s*\/bin\/device_sync\s+(\S+)/) {
			$kmod_devsy_conf = $1;
		}
		if ($_ =~ m/^\s*ExecStart\s*=\s*\/bin\/kmodule_loader\s+-c\s+(\S+)/) {
			$kmod_devsy_conf = $1;
		}
	}
	close FILE;

	return $kmod_devsy_conf;
}
