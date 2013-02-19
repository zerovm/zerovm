echo ---------------------------------------------------- generating
zerovm -e -Mgenerator.uint32_t.manifest
cat generator.stderr.log
echo ---------------------------------------------------- sorting
zerovm -e -Msort_uint_proper_with_args.manifest
cat sort.stderr.log
echo ---------------------------------------------------- testing
zerovm -e -Mtester.uint32_t.manifest
cat tester.stderr.log
