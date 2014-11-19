#!/bin/bash

WORKSPACE=$HOME/workspace
DEPS="git gcc make g++-multilib pkg-config libglib2.0-dev devscripts debhelper"
DEPS="$DEPS zvm-validator libzmq3-dev python-debian"

sudo apt-get update
sudo apt-get install --yes python-software-properties
sudo add-apt-repository ppa:zerovm-ci/zerovm-latest
sudo apt-get update
sudo apt-get install --yes $DEPS

wget https://raw.githubusercontent.com/zerovm/zvm-jenkins/master/packager.py -O /tmp/packager.py

rsync -az --exclude=contrib/jenkins/.* /jenkins/ $WORKSPACE
cd $WORKSPACE
git clean -fdx

make all
make clean
git clean -fdx
