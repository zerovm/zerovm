# ZeroVM

ZeroVM is a simple virtual machine.
ZeroVM can run (and isolate) 64-bit x86 applications in a 32-bit address space.
ZeroVM works under Linux x86_64.
For more information check out [ZeroVM.org](http://zerovm.org)

## Installation:

Everything has been tested on Ubuntu 10.04 and Ubuntu 12.04. No issues with VMWare Player. 
VirtualBox [doesn't support SSE4.1](https://www.virtualbox.org/ticket/8651), which is used by several of the provided samples.

## Installing ZeroVM

### To install ZeroVM build environment see toolchain installation guide: [toolchain] (https://github.com/zerovm/toolchain/blob/master/README.md)

Proceeed only after checking the link above.

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
    ZeroVM tag1 lightweight VM manager, build 2013-10-08
    Usage: <manifest> [-l#] [-v#] [-T#] [-stFPQ]

     -l <gigabytes> file size limit (default 4Gb)
     -s skip validation
     -t <0..2> report to stdout/log/fast (default 0)
     -v <0..3> log verbosity (default 0)
     -F quit right before starting user session
     -P disable channels space preallocation
     -Q disable platform qualification
     -T enable time/call tracing

   You should see a usage message similar to the one above. If not check out the [troubleshooting](#troubleshooting) section below.

   Some samples can be found in the `tests/functional/` directory. More can be found in the [zerovm/zrt project](https://github.com/zerovm/zrt).

## Troubleshooting:
    error: expected specifier-qualifier-list before ‘AES_KEY’
**Solution:** missing libssl-dev library. Check the [system prerequisites](#install-system-prerequisites) above

    /usr/bin/ld: cannot find -lvalidator
    collect2: error: ld returned 1 exit status
    make: *** [zerovm] Error 1
**Solution:** Install [zerovm/validator][zerovm-validator] or consult the [docs above](#install-the-zerovm-validator).

