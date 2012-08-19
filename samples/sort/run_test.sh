echo ---------------------------------------------------- generating
${ZEROVM_ROOT}/zerovm -Mgenerator.manifest -v2
cat generator.stderr.log
echo ---------------------------------------------------- sorting
${ZEROVM_ROOT}/zerovm -Msort.manifest -v2
cat sort.stderr.log
echo ---------------------------------------------------- testing
${ZEROVM_ROOT}/zerovm -Mtest.manifest -v2
cat tester.stderr.log

