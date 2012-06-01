cd ../../..
echo ---------------------------------------------------- generating
./zerovm -Msamples/zrt/sort/generator.manifest -v99
cat samples/zrt/sort/generator.stderr.log
echo ---------------------------------------------------- sorting
./zerovm -Msamples/zrt/sort/sort.manifest -v99
cat samples/zrt/sort/sort.stderr.log
echo ---------------------------------------------------- testing
./zerovm -Msamples/zrt/sort/test.manifest -v99
cat samples/zrt/sort/tester.stderr.log

