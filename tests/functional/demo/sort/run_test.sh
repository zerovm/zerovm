echo ---------------------------------------------------- generating
zerovm generator.uint32_t.manifest
cat generator.stderr.log
echo ---------------------------------------------------- sorting
zerovm sort_uint_proper_with_args.manifest
cat sort.stderr.log
echo ---------------------------------------------------- testing
zerovm tester.uint32_t.manifest
cat tester.stderr.log
