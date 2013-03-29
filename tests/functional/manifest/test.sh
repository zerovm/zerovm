#!/bin/sh

echo "XQAAAAT//////////wA/kUWEaD2Jt+w/+vaX0uVChV11YbDTPTmNb6NPqLS2fHCP7AYVCu
KRazdEZ9wCOxrCfZTxlOiKtmMqCCNWydNjIlQND72trKUWU0n4hm2E6/CzJZJ3u+8xaStym4gdxT
MApjlOzBBiMrzPmDVZcfAD+8jSEWeAI1OJUVTSng/qYma+ZgbHGD8x0QR7F+v550ACSWkTmnJR/u
l1k1CT3bWip1qYJH/BNIDfFicnYNfREvBa+P0ajdAo9eDSu/x6/VTWZc3Z6GrFhVjDdvKqPt8A2z
R6b4ZY0AkeWjk5sTcJd33zW659YUX0tG8p7I6qfswpll4rGr1/iiZvRMEdffLomCf5JYLyZ0im/b
Bh9o7Txim3uX4zraQMVNihfS7/+4iaDQ=="|base64 -d|lzma -dc>dummy.nexe

for i in `ls *.manifest`; do
  $ZEROVM_ROOT/zerovm $i > /dev/null
  if [ $? = 0 ]; then
    echo "\033[01;38m$i\033[00m test has \033[02;31mfailed\033[00m"
  else
    echo "\033[01;38m$i\033[00m test has \033[01;32mpassed\033[00m"
  fi
done

rm dummy.nexe
