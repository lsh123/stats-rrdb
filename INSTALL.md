INSTALLATION
=========

Compiling Boost 1.53
=========
1) Download boost-1.53.0-6.fc19.src.rpm (if you are not using REDHAT/FEDORA based
Linux then your mileage may vary).

2) Apply the "misc/boost-spec.diff" diff to the spec file to boost-1.53.0-6.fc19.src.rpm
to enable custom package name and include folders.

3) Run rpmbuild as follows (this will make a build that installs boost153 into custom folders):

rpmbuild --ba --without python3 --without mpich2 --without openmpi  -D "_includedir /usr/include/boost153" -D "_libdir /usr/lib64/boost153" boost.spec


