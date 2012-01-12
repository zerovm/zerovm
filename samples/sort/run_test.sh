cd ../..
echo ---------------------------------------------------- generating
./zerovm -Msamples/sort/generator.manifest
cat samples/sort/generator.stderr.log
echo ---------------------------------------------------- sorting
./zerovm -Msamples/sort/sort.manifest
cat samples/sort/sort.stderr.log
echo ---------------------------------------------------- testing
./zerovm -Msamples/sort/test.manifest
cat samples/sort/tester.stderr.log

