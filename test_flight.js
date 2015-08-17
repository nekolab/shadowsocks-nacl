/*
 * Copyright (C) 2015  Sunny <ratsunny@gmail.com>
 *
 * This file is part of Shadowsocks-NaCl.
 *
 * Shadowsocks-NaCl is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Shadowsocks-NaCl is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

window.onload = function() {

  var shadowsocks = new Shadowsocks('pnacl/Release/shadowsocks.nmf');

  shadowsocks.on('load',  function() {
    shadowsocks.connect({
      server: '127.0.0.1',
      server_port: 8388,
      local_port: 1080,
      method: 'aes-256-cfb',
      password: '1234',
      timeout: 300
    });
  });

  shadowsocks.on('status', function(e) {
    console.log(e);
  });

  shadowsocks.load();

};
