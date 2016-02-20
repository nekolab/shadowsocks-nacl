/*
 * Copyright (C) 2016  Sunny <ratsunny@gmail.com>
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

(function() { "use strict";

  /**
   * UUID Generator
   * @return {string} A generated random UUID string.
   */
  var uuid = function() {
    var d = Date.now();
    return 'xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx'.replace(/[xy]/g, function(c) {
        var r = (d + Math.random() * 16) % 16 | 0;
        d = Math.floor(d / 16);
        return (c === 'x' ? r : (r & 0x3 | 0x8)).toString(16);
    });
  };

  /**
   * Shadowsocks main module.
   * @class
   * @param {string} src - Path to the nmf file.
   */
  var Shadowsocks = function(src) {
    this._nmf_src = src;
    this._nacl_raw_events = [ 'loadstart', 'progress',
      'error', 'abort', 'load', 'loadend', 'crash', 'message' ];

    this._messageCenter = (function(self) {
      var replyPool = {}, listeners = [];

      return {
        /**
         * Send a control message to native client module.
         * @param  {string}  cmd        Control command
         * @param  {object}  arg        Command arguments
         * @param  {MessageCenter~messageCallback} [callback] Optional callback
         * @param  {object}  [context]  Optional "this" arg of callback
         */
        sendMessage: function(cmd, arg, callback, context) {
          if (!self._elem) { return; }
          var message = { cmd: cmd, arg: arg };

          if (typeof callback === 'function') {
            var msgId = uuid();
            message.msg_id = msgId;
            replyPool[msgId] = { callback: callback, context: context };
          }

          self._elem.postMessage(message);
        },
        /**
         * Callback of sendMessage.
         * @callback MessageCenter~messageCallback
         * @param {object} reply - Reply form native client module.
         */

        messageHandler: function(e) {
          // Reply message should be handled in different way.
          if (e.data && e.data.type === 'reply') {
            if (e.data.msg_id in replyPool) {
              var handler = replyPool[e.data.msg_id];
              handler.callback.call(handler.context, e.data.payload);
              delete replyPool[e.data.msg_id];
            } else {
              console.warn('Not a registered reply: ', e.data);
            }
          } else {
            listeners.forEach(function(listener) {
              if (e.type === listener.type) {   // Native client raw event ...
                listener.callback.call(listener.context, e);
              } else if (e.data && e.data.type === listener.type) {
                // ... or shadowsocks message
                listener.callback.call(listener.context, e.data);
              }
            });
          }
        },

        /**
         * Add a native client message event listener.
         * @param {string}   type       Event type.
         * @param {Function} callback   Will be called when status message arrived.
         * @param {object}   [context]  Optional "this" arg for callback.
         */
        addEventListener: function(type, callback, context) {
          listeners.push({ type: type, callback: callback, context: context });
        },

        /**
         * Remove a native client message event listener.
         * Parameter callback and context must has the same
         * reference to which passed to the addSatusListener()
         * @param  {string}   type
         * @param  {Function} callback
         * @param  {object}   [context]
         */
        removeEventListener: function(type, callback, context) {
          for (var i = 0; i < listeners.length; ++i) {
            if (listeners[i].type === type &&
                listeners[i].callback === callback &&
                listeners[i].context === context) {
              listeners.splice(i--, 1);
            }
          }
        },

        /**
         * Clear message center, remove all registered
         * native client message event listeners and empty reply pool.
         */
        clear: function() {
          replyPool = {};
          listeners = [];
        }
      };
    })(this);

  };

  /**
   * Create an <embed> element to load nacl module.
   */
  Shadowsocks.prototype._createEmbed = function() {
    this._elem = document.createElement('embed');
    this._elem.id = uuid();
    this._elem.setAttribute('width', 0);
    this._elem.setAttribute('height', 0);
    this._elem.setAttribute('src', this._nmf_src);
    this._elem.setAttribute('type', 'application/x-pnacl');
  };

  /**
   * Add a shadowsocks event listener.
   * Event name should be one of the 'loadstart', 'progress', 'error',
   *   'abort', 'load', 'loadend', 'crash', 'message' or 'status'.
   * @param  {string}   eventName - Event name
   * @param  {Shadowsocks~onCallback} callback - Event handler
   * @param  {object}   [context] - Optional "this" arg of event handler
   * @return {Shadowsocks}
   */
  Shadowsocks.prototype.addEventListener =
  Shadowsocks.prototype.on = function(eventName, callback, context) {
    this._messageCenter.addEventListener(eventName, callback, context);
    return this;
  };
  /**
   * Callback of on / addEventListener
   * For 'loadstart', 'progress', 'error', 'abort', 'load', 'loadend' and
   *   'crash' event, see https://goo.gl/NppMJn for more details.
   * For 'message' event, see https://goo.gl/pjrbZ6 for more details.
   * For 'status' event, object like
   *   { type: 'status', level: 'xx', message: 'xx' }
   *   will be passed to callback. Field 'level' should be
   *   one of the 'success', 'info', 'warning' or 'danger'.
   * @callback Shadowsocks~onCallback
   * @param {object} result - See above document
   */

  /**
   * Remove a shadowsocks event listener.
   * Parameter callback and context must has the
   * same reference to which passed to the on() method.
   * @param  {string}   eventName
   * @param  {Function} callback
   * @param  {object}   [context]
   * @return {Shadowsocks}
   */
  Shadowsocks.prototype.removeEventListener =
  Shadowsocks.prototype.off = function(eventName, callback, context) {
    this._messageCenter.removeEventListener(eventName, callback, context);
    return this;
  };

  /**
   * Load native client module to webpage.
   * Nacl element will be append to body if element is not provided.
   * @param {Element} [element] Optional container for nacl element.
   * @return {Shadowsocks}
   */
  Shadowsocks.prototype.load = function(element) {
    this._createEmbed();
    this._nacl_raw_events.forEach(function(type) {
      this._elem.addEventListener(type, this._messageCenter.messageHandler);
    }, this);

    (element || document.body).appendChild(this._elem);
    return this;
  };

  /**
   * Unload native client module from webpage.
   * It will also clear all registered event listeners.
   */
  Shadowsocks.prototype.unload = function() {
    this._nacl_raw_events.forEach(function(type) {
      this._elem.removeEventListener(type, this._messageCenter.messageHandler);
    }, this);

    this._messageCenter.clear();
    this._elem.parentNode && this._elem.parentNode.removeChild(this._elem);
    this._elem = null;
    return this;
  };

  /**
   * Get the native client element.
   * @return {HTMLEmbedElement}
   */
  Shadowsocks.prototype.getElement = function() {
    return this._elem;
  };

  /**
   * Connect to a remote server.
   * Profile should contains 'server', 'server_port',
   *   'local_port', 'method', 'password' and 'timeout' field.
   * @param {object} profile - Connect profile
   * @param {Shadowsocks~connectCallback} [callback] - Optional callback
   * @param {object} [context] - Optional "this" arg for callback
   * @return {Shadowsocks}
   */
  Shadowsocks.prototype.connect = function(profile, callback, context) {
    this._messageCenter.sendMessage('connect', profile, callback, context);
    return this;
  };
  /**
   * Callback of connect.
   * @callback Shadowsocks~connectCallback
   * @param {number} result - Currently, only 0 will be passed to callback
   */

  /**
   * Disconnect from remote server.
   * @param {Shadowsocks~disconnectCallback} [callback] - Optional callback
   * @param {object} [context] - Optional "this" arg for callback
   * @return {Shadowsocks}
   */
  Shadowsocks.prototype.disconnect = function(callback, context) {
    this._messageCenter.sendMessage('disconnect', null, callback, context);
    return this;
  };
  /**
   * Callback of disconnect
   * @callback Shadowsocks~disconnectCallback
   * @param {number} result - Currently, only 0 will be passed to callback
   */

  /**
   * Sweep timeouted connections.
   * @param {Shadowsocks~sweepCallback} [callback] - Optional callback
   * @param {object} [context] - Optional "this" arg for callback
   * @return {Shadowsocks}
   */
  Shadowsocks.prototype.sweep = function(callback, context) {
    this._messageCenter.sendMessage('sweep', null, callback, context);
    return this;
  };
  /**
   * Callback of sweep
   * @callback Shadowsocks~sweepCallback
   * @param {number} result - Currently, only 0 will be passed to callback
   */

  /**
   * Get the version of shadowsocks-nacl.
   * Version will be logged to console when call without callback.
   * @param {Shadowsocks~versionCallback} [callback] - Optional callback
   * @param {object} [context] - Optional "this" arg for callback
   * @return {Shadowsocks}
   */
  Shadowsocks.prototype.version = function(callback, context) {
    this._messageCenter.sendMessage('version', null, callback, context);
    return this;
  };
  /**
   * Callback of version
   * @callback Shadowsocks~versionCallback
   * @param {object} result - Object like {version: "xxx"} will be passed to callback
   */

  /**
   * List all supported ciphers.
   * @param  {Shadowsocks~listCipherCallback} callback
   * @param  {object}   [context] - Optional "this" arg for callback
   * @return {Shadowsocks}
   */
  Shadowsocks.prototype.listCipher = function(callback, context) {
    this._messageCenter.sendMessage('list_cipher', null, callback, context);
    return this;
  };
  /**
   * Callback of listCipher
   * @callback Shadowsocks~listCipherCallback
   * @param {array} ciphers - Array of cipher name in string form
   */

  if (typeof module === 'object' && typeof module.exports === 'object') {
    module.exports = Shadowsocks;       // CommonJS module
  } else if (typeof define === 'function' && (define.amd || define.cmd)) {
    define(Shadowsocks);                // RequireJS or SeaJS module
  } else if (typeof window !== 'undefined') {
    window.Shadowsocks = Shadowsocks;   // export Shadowsocks to global
  }

})();
