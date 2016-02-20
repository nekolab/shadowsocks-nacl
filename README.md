Shadowsocks-NaCl [![Build Status][travis-badge]][travis-ci]
===========================================================

Shadowsocks-NaCl is a Native Client port of shadowsocks,
provides high performance crypto and network relay on
web browser which support Native Client.

**Note:** This port of shadowsocks is **for Web App developers ONLY,
NOT** for the end user. For end user, use [shadowsocks-chromeapp][] instead.


Build
-----
1. Download and install [Native Client SDK][nacl-sdk].
2. Set environment variable `NACL_SDK_ROOT` to SDK path (e.g., `~/nacl_sdk/pepper_47`).
3. Checkout [webports][], a.k.a. naclports.
4. Follow the instructions in `webports/README.md` to install webports.
5. Build and install OpenSSL to Native Client SDK. (e.g., `$ NACL_ARCH=pnacl make openssl`)
6. **For webports branch below `pepper_47`:**
   Update libsodium in webports from `0.4.5` to `1.0.3` since ChaCha20 was added in `0.6.0`.
   You may change the content of `webports/src/ports/libsodium/pkg_info` to

    ```
    NAME=libsodium
    VERSION=1.0.3
    URL=https://github.com/jedisct1/libsodium/releases/download/1.0.3/libsodium-1.0.3.tar.gz
    LICENSE=ISC
    SHA1=e44ed485842966d4e2d8f58e74a5fd78fbfbe4b0
    ```
7. Build and install libsodium to Native Client SDK. (e.g., `$ NACL_ARCH=pnacl make libsodium`)
8. Clone this repository and use `$ make` to build.


Usage
-----
You can use Shadowsocks-NaCl JavaScript API to communicate with native client
module. You just need include [`src/shadowsocks.js`][js-api] into your webapp
page, initialize it, and use it.


### Initialize
You can initialize Shadowsocks-NaCl JavaScript API like this:
```javascript
var shadowsocks = new Shadowsocks('path/to/nmf');
```
It will return a `Shadowsocks` object, then you can invoke API commands or
add event listeners.


### Events
#### Raw Native Client Progress Events
This type of events includes `loadstart`, `progress`, `error`, `abort`, `load`,
`loadend` and `crash`, they indicates the native client module load status,
see [native client document][nacl-progress-events] for more information.

#### Raw Native Client Message Event
`message` is the raw native client message event, all shadowsocks command call
rely on this event, typically you don't need to listen on this event. If you
really want, see [native client document][nacl-message-event] for more information.

#### Shadowsocks status event
`status` is the shadowsocks status event. All runtime information, like server
listen state, link error message, will be passed to the callback of `status`
event, in an object form like: `{ type: 'status', level: 'foo', message: 'bar' }`.

`level` in object could be one of the `success`, `info`, `warning` or `danger`,
and the `message` could be a string or an object.


### Profile
Profile is a JavaScript object, it can be passed to API `connect` directly.
```javascript
{
    server: "example.com",  // Domain/IP Address in string form
    server_port: 8388,      // Value must be a number
    local_port: 1080,       // Value must be a number
    method: "aes-256-cfb",  // Value must be a string and in supported cipher list
    password: "password"    // Value must be a string
    timeout: 300            // Value in seconds and must be a number
}
```


### API

* #### `shadowsocks.addEventListener(event, callback, context)`
  Add an [event](#events) listener, context is an optional `this` for callback.

* #### `shadowsocks.on(event, callback, context)`
  Alias of `shadowsocks.addEventListener(event, callback, context)`

* #### `shadowsocks.removeEventListener(event, callback, context)`
  Remove an [event](#events) listener, `event`, `callback`, `context` must
  be the same one of which passed to `addEventListener`.

* #### `shadowsocks.off(event, callback, context)`
  Alias of `shadowsocks.removeEventListener(event, callback, context)`

* #### `shadowsocks.load()`
  Load shadowsocks native client module. After invoke,
  it will trigger [progress events](#raw-native-client-progress-events).

* #### `shadowsocks.unload()`
  Unload shadowsocks native client module.
  It will also clear all registered event listeners.

* #### `shadowsocks.getElement()`
  Return the native client `<embed>` element.

* #### `shadowsocks.connect(profile, callback, context)`
  Connect to a server, `callback` will be called with argument 0.

* #### `shadowsocks.disconnect(callback, context)`
  Disconnect from a server, `callback` will be called with argument 0.

* #### `shadowsocks.sweep(callback, context)`
  Sweep timeout connection from connection pool, the native client module
  cannot do sweep automatically, so you must sweep it by yourself.

  Typically, you should invoke this function repeatedly in fixed time
  (could same as `timeout` in profile).

  The `callback` function will be called with argument 0.

* #### `shadowsocks.version(callback, context)`
  `callback` function will be called with the native client module version
  in string form.

  If `callback` is not specified, it will log version string
  into console.

* #### `shadowsocks.listCipher(callback, context)`
  `callback` function will be called with the array of supported cipher name
  in string form.


Test flight
----------
A Chromium App is provided to help testing and debugging.

You can open `chrome://extensions/`, check `Developer Mode`,
click `Load Unpacked Extension`, select the root directory of this project.

You will find a new App named `Shadowsocks NaCl Test flight` in
`chrome://extensions/`, click `Inspect views: background page` will open a
developer tools window and you can try above-mentioned command here.


License
-------
![GPLv3](https://www.gnu.org/graphics/gplv3-127x51.png)

Shadowsocks-NaCl is licensed under [GNU General Public License][gpl] Version 3.

Shadowsocks-NaCl is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Shadowsocks-NaCl is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.


[gpl]: https://www.gnu.org/licenses/gpl.html
[webports]: https://chromium.googlesource.com/webports.git/
[travis-ci]: https://travis-ci.org/meowlab/shadowsocks-nacl
[nacl-sdk]: https://developer.chrome.com/native-client/sdk/download
[travis-badge]: https://travis-ci.org/meowlab/shadowsocks-nacl.svg?branch=master
[shadowsocks-chromeapp]: https://github.com/shadowsocks/shadowsocks-chromeapp
[js-api]: https://github.com/meowlab/shadowsocks-nacl/blob/master/src/shadowsocks.js
[nacl-progress-events]: https://goo.gl/NppMJn
[nacl-message-event]: https://goo.gl/pjrbZ6
