#!/bin/bash -ex

pushd ~

git clone https://github.com/shadowsocks/shadowsocks-libev.git
cd shadowsocks-libev
git checkout $(git describe --tags `git rev-list --tags --max-count=1`)
./configure --disable-documentation
make
sudo make install

popd
