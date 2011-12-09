cd ../..
echo ---------------------------------------------------- generating
./zerovm -Y2 -Msamples/sort/generator_manifest.txt
echo ---------------------------------------------------- sorting
./zerovm -Y2 -Msamples/sort/sort_manifest.txt
echo ---------------------------------------------------- testing
./zerovm -Y2 -Msamples/sort/test_manifest.txt
