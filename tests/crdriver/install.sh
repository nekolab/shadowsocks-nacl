#!/bin/bash -ex

pushd ~

CR_DRIVER_VERSION=2.21

wget https://chromedriver.storage.googleapis.com/$CR_DRIVER_VERSION/chromedriver_linux64.zip
unzip chromedriver_linux64.zip
chmod +x chromedriver

sudo mv -f chromedriver /usr/local/share/chromedriver
sudo ln -s /usr/local/share/chromedriver /usr/local/bin/chromedriver
sudo ln -s /usr/local/share/chromedriver /usr/bin/chromedriver

popd
