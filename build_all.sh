# 4 steps to build sandbox
./01._make_output_folders.sh
./02._compile_all_sources.sh
./03._make_libraries_for_sandbox.sh
./04._link_all_together_to_make_sandbox.sh

# delete intermediate files
find . -type f -name "*.o" -exec rm -f '{}' \;
find . -type f -name "*.a" -exec rm -f '{}' \;
rmdir lib
