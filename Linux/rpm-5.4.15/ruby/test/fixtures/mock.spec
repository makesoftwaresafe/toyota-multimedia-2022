Name: Foo
Version: 1.0
Release: 1
Summary: A mock spec file
URL: http://www.rpm5.org
Group: Test/Mocks
License: Public Domain
Source0: foosource1.tar
Source1: %{name}-%{version}
Source3: barsource1
BuildArch: noarch
%define foobar Barbazl

%description
This is a mock spec file for testing.


%prep
%setup -cT
touch '%{tmpfile}'
echo 'prepared' > '%{tmpfile}'


%build
touch '%{tmpfile}'
echo 'built' > '%{tmpfile}'


%install
touch '%{tmpfile}'
echo 'installed' > '%{tmpfile}'


%check
touch '%{tmpfile}'
echo 'checked' > '%{tmpfile}'


%files
