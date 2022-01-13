%undefine disttag
%undefine distepoch
Summary: test rpm for perl-RPM2 test suite
BuildArch: noarch
Name: test-rpm
Version: 1.0
Release: 1
Source0: test-rpm.spec
License: GPL
Group: Application/Development
BuildRoot: %{_tmppath}/%{name}-root

%description
test rpm

%install
rm -rf $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT/tmp
cp %SOURCE0 $RPM_BUILD_ROOT/tmp/test

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root)
/tmp/test

%changelog
* Sat Apr 13 2002 Chip Turner <cturner@localhost.localdomain>
- Initial build.


