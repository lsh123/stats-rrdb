INSTALLATION
=========

Compiling Boost 1.53
---------
Stats-RRDB depends only on one external library: Boost 1.53. You will probably be
able to compile using older Boost versions as well but I didn't try it.

If you can get Boost 1.53 from your OS packages manager or you know how to compile
it yourself then ignore the rest of this section.

To install Boost 1.53 in *custom* folders to ensure it doesn't break any of the
existing dependencies, perform the following steps:

* Download boost-1.53.0-6.fc19.src.rpm (if you are not using REDHAT/FEDORA based Linux 
then your mileage may vary).

* Apply the "misc/boost-spec.diff" diff to the spec file to boost-1.53.0-6.fc19.src.rpm 
to enable custom package name and include folders.

* Run rpmbuild as follows (this will make a build that installs boost153 into custom folders):

		rpmbuild --ba --without python3 --without mpich2 --without openmpi  -D "_includedir /usr/include/boost153" -D "_libdir /usr/lib64/boost153" boost.spec


Installing from GitHub
---------
		./autogen.sh --prefix=<path>
		make
		make install

Building dist file (.tar.gz) from GitHub
---------
		./autogen.sh --prefix=<path>
		make
		make dist
		
Building RPM from GitHub
---------
		./autogen.sh --prefix=<path>
		make
		make dist-rpm
		
Installing from dist file (.tar.gz)
---------
		./configure.sh --prefix=<path>
		make
		make install
		
Building RPM from dist file (.tar.gz)
---------
		./configure.sh --prefix=<path>
		make
		make dist-rpm

