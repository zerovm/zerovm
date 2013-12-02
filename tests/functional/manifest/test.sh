#!/bin/sh

echo "XQAAAAT//////////wA/kUWEaD2Jt+w/+vaX0uVChV11YbDTPTmNb6NPqLS2fHCP7AYVCu
KRazdEZ9wCOxrCfZTxlOiKtmMqCCNWydNjIlQND72trKUWU0n4hm2E6/CzJZJ3u+8xaStym4gdxT
MApjlOzBBiMrzPmDUWufOZFWloJY80LQAZ2F1ASFI7iCirT2cdwOODiWahH6vovgh4W0mn/gCSg0
PcEmKZosEwP/+/x1hk"|base64 -d|lzma -dc>dummy.nexe

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
