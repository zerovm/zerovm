#!/bin/sh

# Submit a build on the openSUSE Build Service. You need to have an
# account and have configured the osc client.

project=home:mgeisler
package=zerovm
commit=$(git rev-parse --short HEAD)
timestamp=$(date -u +%Y%m%d%H%M)
build=$timestamp.$commit

if [ ! -d $project/$package ]; then
    osc checkout $project $package
fi
osc update $project/$package

git archive HEAD -o $project/$package/$package.tar.gz

sed "s/@BUILD@/$build/" < contrib/$package.spec.tmpl \
    > $project/$package/$package.spec

osc commit --skip-validation -m "Git commit $commit" $project
