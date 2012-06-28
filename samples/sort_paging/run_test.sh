echo ---------------------------------------------------- generating
../../zerovm -Mgenerator.manifest -v2
cat generator.stderr.log
echo ---------------------------------------------------- sorting
../../zerovm -Msort.manifest -v2
cat sort.stderr.log
echo ---------------------------------------------------- testing
../../zerovm -Mtest.manifest -v2
cat tester.stderr.log

