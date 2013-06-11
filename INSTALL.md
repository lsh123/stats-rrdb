INSTALLATION
=========

Compiling Boost 1.53
=========
1) Download boost-1.53.0-6.fc19.src.rpm (if you are not using REDHAT/FEDORA based
Linux then your mileage may vary).

2) Apply the following diff to the spec file to boost-1.53.0-6.fc19.src.rpm
to enable custom package name and include folders:

--- ./SPECS/boost.spec.orig	2013-06-10 22:17:52.560984754 -0700
+++ ./SPECS/boost.spec	2013-06-10 22:29:21.998425833 -0700
@@ -30,14 +30,14 @@
 
 %bcond_without python3
 
-Name: boost
+Name: boost153
 Summary: The free peer-reviewed portable C++ source libraries
 Version: 1.53.0
 %define version_enc 1_53_0
 Release: 6%{?dist}
 License: Boost and MIT and Python
 
-%define toplev_dirname %{name}_%{version_enc}
+%define toplev_dirname boost_%{version_enc}
 URL: http://www.boost.org
 Group: System Environment/Libraries
 Source0: http://downloads.sourceforge.net/%{name}/%{toplev_dirname}.tar.bz2
@@ -668,6 +668,7 @@
     	--without-context \
 %endif
 	--prefix=$RPM_BUILD_ROOT%{_prefix} \
+	--includedir=$RPM_BUILD_ROOT%{_includedir} \
 	--libdir=$RPM_BUILD_ROOT%{_libdir} \
 	variant=release threading=single,multi debug-symbols=on pch=off \
 	python=%{python2_version} install


3) Run rpmbuild as follows (this will make a build that installs boost153 into custom folders):

rpmbuild --ba --without python3 -D "_includedir /usr/include/boost153" -D "_libdir /usr/lib64/boost153" boost.spec


