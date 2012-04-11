this document contain demonstration of zerovm api
(should be added to README file)
-------------------------------------------------

to compile and run most of examples from the "samples" directory 1st you need
to have compiled zerovm and installed NaCl SDK (currently all examples use version 16, 
if you want to use other version of SDK make appropriate changes in samples' makefiles).
also it is assumed that folder where "samples" resides is "~/git/zerovm". check the path
or make changes in makefiles

"DIRECT SYSCALLS" EXAMPLE
-------------------------
cd samples/direct_syscalls
make
cd ../..
./zerovm -Msamples/direct_syscalls/direct_syscall_test.manifest
cd samples/direct_syscalls
(see new files: report.log, user.log, zerovm.log)
make clean
cd ../..

"HELLO WORLD" EXAMPLE
---------------------
cd samples/hello
make
cd ../..
./zerovm -Msamples/hello/hello_world.manifest
cd samples/hello
(see new files: hello.report.log, hello.stderr.log, hello.zerovm.log)
make clean
cd ../..

"TRAP ENGINE" EXAMPLE
---------------------
cd samples/onering
make
(create file "input.dat" with any data. for instance, '$ touch input.dat')
cd ../..
./zerovm -Msamples/onering/onering.manifest
cd samples/onering
(see new files: report.log, user.log, zerovm.log)
make clean
cd ../..

"PAGINATION" EXAMPLE
--------------------
cd samples/pagination
make
(create file "input.data" with any data)
cd ../..
./zerovm -Msamples/pagination/paging_test.manifest
cd samples/pagination
(see new files: report.log, user.log, zerovm.log, output.data (contain data from "input.data"))
make clean
cd ../..

"SYSCALLBACK ENGINE" DEMO
-------------------------
cd samples/syscallback
make
cd ../..
./zerovm -Msamples/syscallback/syscallback.manifest
./zerovm -Msamples/syscallback/zrt_mock.manifest
(see new files: report.log, user.log, zerovm.log, zrt_mock.report.log, zrt_mock.user.log, zrt_mock.zerovm.log)
make clean
make cleanlog
cd ../..

NOTE: "syscallback" folder also contain zrt demonstration. zrt is not
finished yet, but some functionality is working and you can try it.

"USER LOG" EXAMPLE
------------------
cd samples/user_log
make
cd ../..
./zerovm -Msamples/user_log/user_log.manifest
cd samples/user_log
(see new files: report.log, user.log, zerovm.log)
make clean
cd ../..

"SORT" EXAMPLE
--------------
this example consist of 3 programs: generator.uint32_t.c, sort_uint_proper_with_args.c, tester.uint32_t.c
to compile and run "sort" example issue these commands:
cd samples/sort
make

you will see results like:
---------------------------------------------------- generating
generating 16777216 of 32-bit numbers..
numbers are generated
---------------------------------------------------- sorting
number of elements = 16777216
Sorting time = 0.000sec
---------------------------------------------------- testing
number of elements = 16777216
test if array of 16777216 32-bit numbers is sorted:
given data is SORTED

after that you can find a log files: generator.log, generator.report.log,
generator.stderr.log, sort.log, sort.report.log, sort.stderr.log,
tester.log, tester.report.log, tester.stderr.log

and data files: sorted.data, unsorted.data

NOTE: to successfully run this example, besides 1gb of ram available 
your computer must have cpu with sse 4.1 or compatible.
