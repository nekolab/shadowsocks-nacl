#!/usr/bin/env python
# -*- coding: utf-8 -*-

import os
import sys
import time
import hashlib
import traceback
import subprocess
from selenium import webdriver
from selenium.common.exceptions import WebDriverException

CHROME_DRIVER_PATH = '/usr/local/bin/chromedriver'

CB = 'arguments[arguments.length - 1]'

TEST_MD5 = hashlib.md5(open('test.bin', 'rb').read()).hexdigest()
TEST_CIPHER_TABLE = [ 'rc4-md5', 'aes-128-cfb', 'aes-192-cfb', 'aes-256-cfb',
                      'aes-128-ctr', 'aes-192-ctr', 'aes-256-ctr', 'bf-cfb',
                      'cast5-cfb', 'des-cfb', 'rc2-cfb', 'seed-cfb', 'idea-cfb',
                      'salsa20', 'chacha20' ]

if sys.platform.startswith('linux'):
  TEST_CIPHER_TABLE.extend([ 'camellia-128-cfb', 'camellia-192-cfb',
                             'camellia-256-cfb' ])

class TColors:
  HEADER = '\033[95m'
  OKBLUE = '\033[94m'
  OKGREEN = '\033[92m'
  WARNING = '\033[93m'
  FAIL = '\033[91m'
  ENDC = '\033[0m'
  BOLD = '\033[1m'
  UNDERLINE = '\033[4m'

def run_server(server, port, method, password, ota):
  cmd = ['ss-server', '-s', server, '-p', port, '-k', password, '-m', method, '-u']
  if ota:
    cmd.append('-A')
  return subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)

def kill_server(popen):
  popen.terminate()

def run_module(driver, server, server_port, local_port, method, password, ota):
  if server == '0.0.0.0':
    server = '127.0.0.1'
  jsota = 'true' if ota else 'false'
  driver.execute_async_script('ss.connect({' \
    'server: "%s", server_port: %s, local_port: %s,' \
    'method: "%s", password: "%s", timeout: 300, one_time_auth: %s' \
  '}, '% (server, server_port, local_port, method, password, jsota) + CB + ')')

def stop_module(driver):
  driver.execute_async_script('console.log("stop");ss.disconnect(' + CB + ')')

def test_cipher(driver, server, server_port, local_port, method, password, ota):
  print 'Testing %s with%s OTA...' % (method, '' if ota else 'out')
  server_popen = run_server(server, server_port, method, password, ota)
  run_module(driver, server, server_port, local_port, method, password, ota)
  time.sleep(1)
  # Test TCP (use HTTP)
  (out, _) = subprocess.Popen('curl --socks5 %s:%s --retry 3 http://127.0.0.1:6001/test.bin | md5sum'
                             % ('127.0.0.1', local_port), shell=True, stdout=subprocess.PIPE,
                             stderr=subprocess.STDOUT).communicate()
  if TEST_MD5 in out:
    print TColors.OKGREEN + 'TCP: Passed' + TColors.ENDC
  else:
    print TColors.FAIL + 'TCP: Failed' + TColors.ENDC
  # Test UDP (use DNS)
  dig_popen = subprocess.Popen(['socksify', 'dig', '@8.8.8.8', 'www.google.com'],
                                env=dict(os.environ, SOCKS5_SERVER='127.0.0.1:1081'),
                                stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
  dig_popen.wait()
  dig_out = dig_popen.stdout.read()
  if dig_popen.returncode == 0 and 'WARNING' not in dig_out:
    print TColors.OKGREEN + 'UDP: Passed' + TColors.ENDC + '\n'
  else:
    print TColors.FAIL + 'UDP: Failed' + TColors.ENDC + '\n'

  stop_module(driver)
  kill_server(server_popen)
  time.sleep(1)

  passed = TEST_MD5 in out and dig_popen.returncode == 0 and 'WARNING' not in dig_out
  if not passed:
    print 'Curl output: %s' % out
    print 'Dig output: %s' % dig_out
    print driver.get_log('browser')
  return passed


def test():
  print TColors.HEADER + 'Preparing webdriver...' + TColors.ENDC
  # Prepare chrome options
  chromeOps = webdriver.ChromeOptions()
  # chromeOps.add_argument('--no-sandbox')
  chromeOps.add_argument('--enable-nacl')
  chromeOps.add_argument('--allow-nacl-socket-api=127.0.0.1,localhost')
  chromeOps.add_experimental_option('excludeSwitches', ['disable-component-update'])

  # Create webdriver
  try:
    driver = webdriver.Chrome(CHROME_DRIVER_PATH, chrome_options=chromeOps)
    driver.set_script_timeout(30)
  except WebDriverException as e:
    error = e.__str__()

    if 'executable needs to be in PATH' in error:
        print "ChromeDriver isn't installed. Check test/chrome/README.md " \
              "for instructions on how to install ChromeDriver"
        sys.exit(2)
    else:
        raise e

  try:
    # Initialize module
    print TColors.BOLD + 'Initializing test...' + TColors.ENDC
    driver.get('http://127.0.0.1:6001/tests/selenium.html')
    time.sleep(3)   # Wait for potential page load
    init_result = driver.execute_async_script('window.initTest(' + CB + ')')
    if init_result:
      print TColors.FAIL + 'Failed: ' + init_result + TColors.ENDC
    else:
      print TColors.OKGREEN + 'Done' + TColors.ENDC

    # Print module version
    ss_version = driver.execute_async_script('ss.version(' + CB + ')')
    print TColors.BOLD + 'Module version: ' + ss_version['version'] + TColors.ENDC

    # Print supported cipher
    cipher_list = driver.execute_async_script('ss.listCipher(' + CB + ')')
    print TColors.OKBLUE + 'Supported cipher: ' + str(cipher_list) + TColors.ENDC

    passed = True
    for cipher in cipher_list:
      if cipher not in TEST_CIPHER_TABLE:
        continue
      passed = test_cipher(driver, '0.0.0.0', '8388', '1081', cipher, '1234', True) and passed
      passed = test_cipher(driver, '0.0.0.0', '8388', '1081', cipher, '1234', False) and passed

    driver.quit()
    return passed
  except Exception as e:
    print driver.get_log('browser')
    raise e

if __name__ == '__main__':
  try:
    if not test():
      sys.exit(2)
  except Exception as e:
    traceback.print_exc()
    sys.exit(2)
