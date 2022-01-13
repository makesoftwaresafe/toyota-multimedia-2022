Summary: The rpm that provides
Name: provider
Version: 1.0
Release: 1
License: LGPL
Group: Development/Test
AutoReqProv: no
BuildRoot: %{_tmppath}/%{name}-%{version}-buildroot
BuildArch: noarch
Provides: provided
%description
RPM non-versioned provides of provided

%prep

%build

%install
rm -rf $RPM_BUILD_ROOT
install -d -m 755 $RPM_BUILD_ROOT/usr/opt/%{name}
echo "%{name} %{version}" > $RPM_BUILD_ROOT/usr/opt/%{name}/%{name}


%clean

%files
%defattr(0644,root,root,0755)
%dir /usr/opt/%{name}
     /usr/opt/%{name}/%{name}


