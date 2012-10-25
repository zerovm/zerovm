echo ---------------------------------------------------- generating
../../../../zerovm -e -v3 -Mgenerator.uint32_t.manifest
cat generator.stderr.log
echo ---------------------------------------------------- sorting
../../../../zerovm -e -v2 -Msort_uint_proper_with_args.manifest
cat sort.stderr.log
echo ---------------------------------------------------- testing
../../../../zerovm -e -v2 -Mtester.uint32_t.manifest
cat tester.stderr.log

