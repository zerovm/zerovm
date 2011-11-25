cd ../..
echo ---------------------------------------------------- generating
./zerovm -Y2 -Msamples/sort_mapping/generator_manifest.txt
echo ---------------------------------------------------- sorting
./zerovm -Y2 -Msamples/sort_mapping/sort_manifest.txt
echo ---------------------------------------------------- testing
./zerovm -Y2 -Msamples/sort_mapping/test_manifest.txt
