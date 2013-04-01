# ZeroVM

ZeroVM is a simple virtual machine.
ZeroVM can run (and isolate) 64-bit x86 applications in a 32-bit address space.
ZeroVM works under Linux x86_64.
For more information check out [ZeroVM.org](http://zerovm.org)

## Installation:

Everything has been tested on Ubuntu 10.04 and Ubuntu 12.04. No issues with VMWare Player. 
VirtualBox [doesn't support SSE4.1](https://www.virtualbox.org/ticket/8651), which is used by several of the provided samples.

## Prerequisites
### Install system prerequisites:
    $ sudo apt-get install build-essential gcc-multilib g++-multilib libsdl1.2-dev texinfo libcrypto++-dev libssl-dev m4 lib32z1 uuid-dev

### Set environment variables (add to ~/.bashrc or equivalent)
    export NACL_SDK_ROOT=/some/path/to/nacl_sdk/pepper_23
    export ZEROVM_ROOT=/some/path/to/zerovm
    export ZRT_ROOT=/some/path/to/zrt
    export ZVM_PREFIX=/opt/zerovm

### Install zeromq & packages required by zeromq
    $ wget http://download.zeromq.org/zeromq-2.2.0.tar.gz
    $ tar xvfz zeromq-2.2.0.tar.gz
    $ cd zeromq-2.2.0 && ./configure
    $ make
    $ sudo make install

### Install NaCl SDK. 

Consult the [NaCL installation docs][google-nacl-devcenter] for the exact procedure. The following process should work in most cases:

    $ wget http://commondatastorage.googleapis.com/nativeclient-mirror/nacl/nacl_sdk/nacl_sdk.zip 
    $ unzip nacl_sdk.zip && cd nacl_sdk 
    $ ./naclsdk update pepper_23

   Now you can use the newly installed custom GNU toolchain to build the attached samples or your own programs.
   
[google-nacl-devcenter]: https://developers.google.com/native-client/sdk/download

## Installing ZeroVM

### Install the ZeroVM Validator

Get the [validator][zerovm-validator]:

    $ git clone git://github.com/zerovm/validator.git && cd validator
    $ make validator install
    # OR
    $ sudo -E make validator install

[zerovm-validator]: https://github.com/zerovm/validator

### Console installation (not needed if compiling from Eclipse):
    
    $ wget https://github.com/zerovm/zerovm/zipball/master
    $ unzip master
    $ mkdir zerovm
    $ mv zerovm-zerovm-#######/* zerovm/
    
    # OR
    
    $ git clone git://github.com/zerovm/zerovm.git
    
    # THEN
    
    $ cd zerovm
    $ make
    $ sudo ldconfig
    
### Eclispe CDT installation
   1. If your system doesn't have JRE, install a recent JRE. 
   2. Download Eclipse CDT from the [Eclipse download site][eclipse-dl]. No installation necessary, just unpack and run executable.
   3. Download and install [EGit plugin][egit-plugin] for Eclipse
   4. Point EGit to `git://github.com/zerovm/zerovm.git` and pull the most recent ZeroVM sources.
   5. Select 'create C/C++ project from makefile" to create a new Eclipse project.
   6. Run the makefile to build the project and run unit tests.

   If everything is ok you will see output of the unit tests. Now if you've got `RUN OK PASSED` messages, you have successfully compiled ZeroVM for your platform! You'll find the `zerovm` executable in the project root directory.

[eclipse-dl]: http://www.eclipse.org/downloads/
[egit-plugin]: http://www.eclipse.org/egit/download/

## Running ZeroVM and samples

    $ cd ~/zerovm
    $ ./zerovm
    ZeroVM lightweight VM manager, build 2013-03-27
    Usage: <manifest> [-e#] [-l#] [-v#] [-sFPSQ]

    <manifest> load settings from manifest file
    -e <level> enable the data control. where levels are
        1 - channels, 2 - memory, 3 - both 
    -l set a new storage limit (in Gb)
    -s skip validation
    -v <level> verbosity -F fuzz testing; quit right before starting user app
    -P disable channels space preallocation
    -Q disable platform qualification (dangerous!)
    -S disable signal handling

   You should see a usage message similar to the one above. If not check out the [troubleshooting](#troubleshooting) section below.

   Some samples can be found in the `tests/functional/` directory. More can be found in the [zerovm/zrt project](https://github.com/zerovm/zrt).

## Troubleshooting:
    error: expected specifier-qualifier-list before ‘AES_KEY’
**Solution:** missing libssl-dev library. Check the [system prerequisites](#install-system-prerequisites) above

    /usr/bin/ld: cannot find -lvalidator
    collect2: error: ld returned 1 exit status
    make: *** [zerovm] Error 1
**Solution:** Install [zerovm/validator][zerovm-validator] or consult the [docs above](#install-the-zerovm-validator).

