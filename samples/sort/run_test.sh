echo ---------------------------------------------------- generating
../../zerovm -Mgenerator.uint32_t.manifest
cat generator.stderr.log
echo ---------------------------------------------------- sorting
../../zerovm -Msort_uint_proper_with_args.manifest
cat sort.stderr.log
echo ---------------------------------------------------- testing
../../zerovm -Mtester.uint32_t.manifest
cat tester.stderr.log

