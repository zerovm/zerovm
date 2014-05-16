#!/bin/sh

echo "XQAAAAT//////////wA/kUWEaD2JptqKzJPiTPKE6ABIn6Yq8RMybhOCmsOLjJtLU4LtvO
h0CgI3ilXVYqQKQJhr5B2p8AFk4lsGHSJTDR1FrSYDZGjkvl8VjYqZqkdC/ndeMHjnV1ENBm6iHP
DtWBYpZfGPoNz4b0zCOUhmjDcIO1tjuUtDx4CJDREEhd/C4Xsxxrdt2E1nTVuTYc+tC5ih///FJB
AA"|base64 -d|lzma -dc>dummy.nexe

for i in `ls *.manifest`; do
  $ZEROVM_ROOT/zerovm $i > /dev/null
  code=$?
  retcode=$(echo "$i" | cut -d'.' -f2)
  if [ $code = $retcode ]; then
    echo "\033[01;38m$i\033[00m test has [=$retcode] \033[01;32mpassed\033[00m"
  else
    echo "\033[01;38m$i\033[00m test has [=$retcode] \033[02;31mfailed\033[00m"
  fi
done

rm dummy.nexe
