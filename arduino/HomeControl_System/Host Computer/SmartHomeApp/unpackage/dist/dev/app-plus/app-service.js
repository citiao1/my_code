if (typeof Promise !== "undefined" && !Promise.prototype.finally) {
  Promise.prototype.finally = function(callback) {
    const promise = this.constructor;
    return this.then(
      (value) => promise.resolve(callback()).then(() => value),
      (reason) => promise.resolve(callback()).then(() => {
        throw reason;
      })
    );
  };
}
;
if (typeof uni !== "undefined" && uni && uni.requireGlobal) {
  const global = uni.requireGlobal();
  ArrayBuffer = global.ArrayBuffer;
  Int8Array = global.Int8Array;
  Uint8Array = global.Uint8Array;
  Uint8ClampedArray = global.Uint8ClampedArray;
  Int16Array = global.Int16Array;
  Uint16Array = global.Uint16Array;
  Int32Array = global.Int32Array;
  Uint32Array = global.Uint32Array;
  Float32Array = global.Float32Array;
  Float64Array = global.Float64Array;
  BigInt64Array = global.BigInt64Array;
  BigUint64Array = global.BigUint64Array;
}
;
if (uni.restoreGlobal) {
  uni.restoreGlobal(Vue, weex, plus, setTimeout, clearTimeout, setInterval, clearInterval);
}
(function(vue) {
  "use strict";
  if (typeof window === "undefined") {
    globalThis.window = globalThis;
  }
  if (typeof WebSocket === "undefined") {
    globalThis.WebSocket = function(url, protocols) {
      var self = this;
      this.readyState = 0;
      this._socketTask = uni.connectSocket({
        url,
        protocols,
        complete: function() {
        }
      });
      this._socketTask.onOpen(function(res) {
        self.readyState = 1;
        if (self.onopen)
          self.onopen();
      });
      this._socketTask.onMessage(function(res) {
        if (self.onmessage)
          self.onmessage({ data: res.data });
      });
      this._socketTask.onError(function(res) {
        if (self.onerror)
          self.onerror(res);
      });
      this._socketTask.onClose(function(res) {
        self.readyState = 3;
        if (self.onclose)
          self.onclose();
      });
      this.send = function(data) {
        if (this._socketTask)
          this._socketTask.send({ data });
      };
      this.close = function() {
        if (this._socketTask)
          this._socketTask.close();
      };
    };
  }
  var Paho = {};
  "undefined" === typeof Paho && (Paho = {});
  Paho.MQTT = function(u) {
    function y(a, b, c) {
      b[c++] = a >> 8;
      b[c++] = a % 256;
      return c;
    }
    function r(a, b, c, h) {
      h = y(b, c, h);
      F(a, c, h);
      return h + b;
    }
    function m(a) {
      for (var b = 0, c = 0; c < a.length; c++) {
        var h = a.charCodeAt(c);
        2047 < h ? (55296 <= h && 56319 >= h && (c++, b++), b += 3) : 127 < h ? b += 2 : b++;
      }
      return b;
    }
    function F(a, b, c) {
      for (var h = 0; h < a.length; h++) {
        var e = a.charCodeAt(h);
        if (55296 <= e && 56319 >= e) {
          var d = a.charCodeAt(++h);
          if (isNaN(d))
            throw Error(f(g.MALFORMED_UNICODE, [e, d]));
          e = (e - 55296 << 10) + (d - 56320) + 65536;
        }
        127 >= e ? b[c++] = e : (2047 >= e ? b[c++] = e >> 6 & 31 | 192 : (65535 >= e ? b[c++] = e >> 12 & 15 | 224 : (b[c++] = e >> 18 & 7 | 240, b[c++] = e >> 12 & 63 | 128), b[c++] = e >> 6 & 63 | 128), b[c++] = e & 63 | 128);
      }
      return b;
    }
    function G(a, b, c) {
      for (var h = "", e, d = b; d < b + c; ) {
        e = a[d++];
        if (!(128 > e)) {
          var p = a[d++] - 128;
          if (0 > p)
            throw Error(f(g.MALFORMED_UTF, [e.toString(16), p.toString(16), ""]));
          if (224 > e)
            e = 64 * (e - 192) + p;
          else {
            var t = a[d++] - 128;
            if (0 > t)
              throw Error(f(g.MALFORMED_UTF, [e.toString(16), p.toString(16), t.toString(16)]));
            if (240 > e)
              e = 4096 * (e - 224) + 64 * p + t;
            else {
              var l = a[d++] - 128;
              if (0 > l)
                throw Error(f(
                  g.MALFORMED_UTF,
                  [e.toString(16), p.toString(16), t.toString(16), l.toString(16)]
                ));
              if (248 > e)
                e = 262144 * (e - 240) + 4096 * p + 64 * t + l;
              else
                throw Error(f(g.MALFORMED_UTF, [e.toString(16), p.toString(16), t.toString(16), l.toString(16)]));
            }
          }
        }
        65535 < e && (e -= 65536, h += String.fromCharCode(55296 + (e >> 10)), e = 56320 + (e & 1023));
        h += String.fromCharCode(e);
      }
      return h;
    }
    var A = function(a, b) {
      for (var c in a)
        if (a.hasOwnProperty(c))
          if (b.hasOwnProperty(c)) {
            if (typeof a[c] !== b[c])
              throw Error(f(g.INVALID_TYPE, [typeof a[c], c]));
          } else {
            var h = "Unknown property, " + c + ". Valid properties are:";
            for (c in b)
              b.hasOwnProperty(c) && (h = h + " " + c);
            throw Error(h);
          }
    }, q = function(a, b) {
      return function() {
        return a.apply(b, arguments);
      };
    }, g = {
      OK: { code: 0, text: "AMQJSC0000I OK." },
      CONNECT_TIMEOUT: { code: 1, text: "AMQJSC0001E Connect timed out." },
      SUBSCRIBE_TIMEOUT: { code: 2, text: "AMQJS0002E Subscribe timed out." },
      UNSUBSCRIBE_TIMEOUT: { code: 3, text: "AMQJS0003E Unsubscribe timed out." },
      PING_TIMEOUT: { code: 4, text: "AMQJS0004E Ping timed out." },
      INTERNAL_ERROR: { code: 5, text: "AMQJS0005E Internal error. Error Message: {0}, Stack trace: {1}" },
      CONNACK_RETURNCODE: { code: 6, text: "AMQJS0006E Bad Connack return code:{0} {1}." },
      SOCKET_ERROR: { code: 7, text: "AMQJS0007E Socket error:{0}." },
      SOCKET_CLOSE: { code: 8, text: "AMQJS0008I Socket closed." },
      MALFORMED_UTF: { code: 9, text: "AMQJS0009E Malformed UTF data:{0} {1} {2}." },
      UNSUPPORTED: { code: 10, text: "AMQJS0010E {0} is not supported by this browser." },
      INVALID_STATE: { code: 11, text: "AMQJS0011E Invalid state {0}." },
      INVALID_TYPE: { code: 12, text: "AMQJS0012E Invalid type {0} for {1}." },
      INVALID_ARGUMENT: { code: 13, text: "AMQJS0013E Invalid argument {0} for {1}." },
      UNSUPPORTED_OPERATION: { code: 14, text: "AMQJS0014E Unsupported operation." },
      INVALID_STORED_DATA: { code: 15, text: "AMQJS0015E Invalid data in local storage key={0} value={1}." },
      INVALID_MQTT_MESSAGE_TYPE: { code: 16, text: "AMQJS0016E Invalid MQTT message type {0}." },
      MALFORMED_UNICODE: { code: 17, text: "AMQJS0017E Malformed Unicode string:{0} {1}." }
    }, J = {
      0: "Connection Accepted",
      1: "Connection Refused: unacceptable protocol version",
      2: "Connection Refused: identifier rejected",
      3: "Connection Refused: server unavailable",
      4: "Connection Refused: bad user name or password",
      5: "Connection Refused: not authorized"
    }, f = function(a, b) {
      var c = a.text;
      if (b) {
        for (var h, e, d = 0; d < b.length; d++)
          if (h = "{" + d + "}", e = c.indexOf(h), 0 < e)
            var g2 = c.substring(0, e), c = c.substring(e + h.length), c = g2 + b[d] + c;
      }
      return c;
    }, B = [0, 6, 77, 81, 73, 115, 100, 112, 3], C = [0, 4, 77, 81, 84, 84, 4], n = function(a, b) {
      this.type = a;
      for (var c in b)
        b.hasOwnProperty(c) && (this[c] = b[c]);
    };
    n.prototype.encode = function() {
      var a = (this.type & 15) << 4, b = 0, c = [], h = 0;
      void 0 != this.messageIdentifier && (b += 2);
      switch (this.type) {
        case 1:
          switch (this.mqttVersion) {
            case 3:
              b += B.length + 3;
              break;
            case 4:
              b += C.length + 3;
          }
          b += m(this.clientId) + 2;
          if (void 0 != this.willMessage) {
            var b = b + (m(this.willMessage.destinationName) + 2), e = this.willMessage.payloadBytes;
            e instanceof Uint8Array || (e = new Uint8Array(g2));
            b += e.byteLength + 2;
          }
          void 0 != this.userName && (b += m(this.userName) + 2);
          void 0 != this.password && (b += m(this.password) + 2);
          break;
        case 8:
          for (var a = a | 2, d = 0; d < this.topics.length; d++)
            c[d] = m(this.topics[d]), b += c[d] + 2;
          b += this.requestedQos.length;
          break;
        case 10:
          a |= 2;
          for (d = 0; d < this.topics.length; d++)
            c[d] = m(this.topics[d]), b += c[d] + 2;
          break;
        case 6:
          a |= 2;
          break;
        case 3:
          this.payloadMessage.duplicate && (a |= 8);
          a = a |= this.payloadMessage.qos << 1;
          this.payloadMessage.retained && (a |= 1);
          var h = m(this.payloadMessage.destinationName), g2 = this.payloadMessage.payloadBytes, b = b + (h + 2) + g2.byteLength;
          g2 instanceof ArrayBuffer ? g2 = new Uint8Array(g2) : g2 instanceof Uint8Array || (g2 = new Uint8Array(g2.buffer));
      }
      var f2 = b, d = Array(1), l = 0;
      do {
        var z = f2 % 128, f2 = f2 >> 7;
        0 < f2 && (z |= 128);
        d[l++] = z;
      } while (0 < f2 && 4 > l);
      f2 = d.length + 1;
      b = new ArrayBuffer(b + f2);
      l = new Uint8Array(b);
      l[0] = a;
      l.set(d, 1);
      if (3 == this.type)
        f2 = r(this.payloadMessage.destinationName, h, l, f2);
      else if (1 == this.type) {
        switch (this.mqttVersion) {
          case 3:
            l.set(B, f2);
            f2 += B.length;
            break;
          case 4:
            l.set(C, f2), f2 += C.length;
        }
        a = 0;
        this.cleanSession && (a = 2);
        void 0 != this.willMessage && (a = a | 4 | this.willMessage.qos << 3, this.willMessage.retained && (a |= 32));
        void 0 != this.userName && (a |= 128);
        void 0 != this.password && (a |= 64);
        l[f2++] = a;
        f2 = y(this.keepAliveInterval, l, f2);
      }
      void 0 != this.messageIdentifier && (f2 = y(this.messageIdentifier, l, f2));
      switch (this.type) {
        case 1:
          f2 = r(this.clientId, m(this.clientId), l, f2);
          void 0 != this.willMessage && (f2 = r(this.willMessage.destinationName, m(this.willMessage.destinationName), l, f2), f2 = y(e.byteLength, l, f2), l.set(e, f2), f2 += e.byteLength);
          void 0 != this.userName && (f2 = r(this.userName, m(this.userName), l, f2));
          void 0 != this.password && r(this.password, m(this.password), l, f2);
          break;
        case 3:
          l.set(g2, f2);
          break;
        case 8:
          for (d = 0; d < this.topics.length; d++)
            f2 = r(this.topics[d], c[d], l, f2), l[f2++] = this.requestedQos[d];
          break;
        case 10:
          for (d = 0; d < this.topics.length; d++)
            f2 = r(
              this.topics[d],
              c[d],
              l,
              f2
            );
      }
      return b;
    };
    var H = function(a, b, c) {
      this._client = a;
      this._window = b;
      this._keepAliveInterval = 1e3 * c;
      this.isReset = false;
      var h = new n(12).encode(), e = function(a2) {
        return function() {
          return d.apply(a2);
        };
      }, d = function() {
        this.isReset ? (this.isReset = false, this._client._trace("Pinger.doPing", "send PINGREQ"), this._client.socket.send(h), this.timeout = this._window.setTimeout(e(this), this._keepAliveInterval)) : (this._client._trace("Pinger.doPing", "Timed out"), this._client._disconnected(g.PING_TIMEOUT.code, f(g.PING_TIMEOUT)));
      };
      this.reset = function() {
        this.isReset = true;
        this._window.clearTimeout(this.timeout);
        0 < this._keepAliveInterval && (this.timeout = setTimeout(e(this), this._keepAliveInterval));
      };
      this.cancel = function() {
        this._window.clearTimeout(this.timeout);
      };
    }, D = function(a, b, c, f2, e) {
      this._window = b;
      c || (c = 30);
      this.timeout = setTimeout(/* @__PURE__ */ function(a2, b2, c2) {
        return function() {
          return a2.apply(b2, c2);
        };
      }(f2, a, e), 1e3 * c);
      this.cancel = function() {
        this._window.clearTimeout(this.timeout);
      };
    }, k = function(a, b, c, h, e) {
      if (!("WebSocket" in u && null !== u.WebSocket))
        throw Error(f(
          g.UNSUPPORTED,
          ["WebSocket"]
        ));
      if (!("localStorage" in u && null !== u.localStorage))
        throw Error(f(g.UNSUPPORTED, ["localStorage"]));
      if (!("ArrayBuffer" in u && null !== u.ArrayBuffer))
        throw Error(f(g.UNSUPPORTED, ["ArrayBuffer"]));
      this._trace("Paho.MQTT.Client", a, b, c, h, e);
      this.host = b;
      this.port = c;
      this.path = h;
      this.uri = a;
      this.clientId = e;
      this._localKey = b + ":" + c + ("/mqtt" != h ? ":" + h : "") + ":" + e + ":";
      this._msg_queue = [];
      this._sentMessages = {};
      this._receivedMessages = {};
      this._notify_msg_sent = {};
      this._message_identifier = 1;
      this._sequence = 0;
      for (var d in localStorage)
        0 != d.indexOf("Sent:" + this._localKey) && 0 != d.indexOf("Received:" + this._localKey) || this.restore(d);
    };
    k.prototype.connected = false;
    k.prototype.maxMessageIdentifier = 65536;
    k.prototype._msg_queue = null;
    k.prototype.sendPinger = null;
    k.prototype.receivePinger = null;
    k.prototype.receiveBuffer = null;
    k.prototype._traceBuffer = null;
    k.prototype._MAX_TRACE_ENTRIES = 100;
    k.prototype.connect = function(a) {
      var b = this._traceMask(a, "password");
      this._trace("Client.connect", b, this.socket, this.connected);
      if (this.connected)
        throw Error(f(g.INVALID_STATE, ["already connected"]));
      if (this.socket)
        throw Error(f(g.INVALID_STATE, ["already connected"]));
      this.connectOptions = a;
      a.uris ? (this.hostIndex = 0, this._doConnect(a.uris[0])) : this._doConnect(this.uri);
    };
    k.prototype.subscribe = function(a, b) {
      this._trace("Client.subscribe", a, b);
      if (!this.connected)
        throw Error(f(g.INVALID_STATE, ["not connected"]));
      var c = new n(8);
      c.topics = [a];
      c.requestedQos = void 0 != b.qos ? [b.qos] : [0];
      b.onSuccess && (c.onSuccess = function(a2) {
        b.onSuccess({ invocationContext: b.invocationContext, grantedQos: a2 });
      });
      b.onFailure && (c.onFailure = function(a2) {
        b.onFailure({ invocationContext: b.invocationContext, errorCode: a2 });
      });
      b.timeout && (c.timeOut = new D(this, window, b.timeout, b.onFailure, [{
        invocationContext: b.invocationContext,
        errorCode: g.SUBSCRIBE_TIMEOUT.code,
        errorMessage: f(g.SUBSCRIBE_TIMEOUT)
      }]));
      this._requires_ack(c);
      this._schedule_message(c);
    };
    k.prototype.unsubscribe = function(a, b) {
      this._trace("Client.unsubscribe", a, b);
      if (!this.connected)
        throw Error(f(g.INVALID_STATE, ["not connected"]));
      var c = new n(10);
      c.topics = [a];
      b.onSuccess && (c.callback = function() {
        b.onSuccess({ invocationContext: b.invocationContext });
      });
      b.timeout && (c.timeOut = new D(this, window, b.timeout, b.onFailure, [{
        invocationContext: b.invocationContext,
        errorCode: g.UNSUBSCRIBE_TIMEOUT.code,
        errorMessage: f(g.UNSUBSCRIBE_TIMEOUT)
      }]));
      this._requires_ack(c);
      this._schedule_message(c);
    };
    k.prototype.send = function(a) {
      this._trace("Client.send", a);
      if (!this.connected)
        throw Error(f(g.INVALID_STATE, ["not connected"]));
      var wireMessage = new n(3);
      wireMessage.payloadMessage = a;
      0 < a.qos ? this._requires_ack(wireMessage) : this.onMessageDelivered && (this._notify_msg_sent[wireMessage] = this.onMessageDelivered(wireMessage.payloadMessage));
      this._schedule_message(wireMessage);
    };
    k.prototype.disconnect = function() {
      this._trace("Client.disconnect");
      if (!this.socket)
        throw Error(f(g.INVALID_STATE, ["not connecting or connected"]));
      var wireMessage = new n(14);
      this._notify_msg_sent[wireMessage] = q(this._disconnected, this);
      this._schedule_message(wireMessage);
    };
    k.prototype.getTraceLog = function() {
      if (null !== this._traceBuffer) {
        this._trace("Client.getTraceLog", /* @__PURE__ */ new Date());
        this._trace("Client.getTraceLog in flight messages", this._sentMessages.length);
        for (var a in this._sentMessages)
          this._trace("_sentMessages ", a, this._sentMessages[a]);
        for (a in this._receivedMessages)
          this._trace(
            "_receivedMessages ",
            a,
            this._receivedMessages[a]
          );
        return this._traceBuffer;
      }
    };
    k.prototype.startTrace = function() {
      null === this._traceBuffer && (this._traceBuffer = []);
      this._trace("Client.startTrace", /* @__PURE__ */ new Date(), "@VERSION@");
    };
    k.prototype.stopTrace = function() {
      delete this._traceBuffer;
    };
    k.prototype._doConnect = function(a) {
      this.connectOptions.useSSL && (a = a.split(":"), a[0] = "wss", a = a.join(":"));
      this.connected = false;
      this.socket = 4 > this.connectOptions.mqttVersion ? new WebSocket(a, ["mqttv3.1"]) : new WebSocket(a, ["mqtt"]);
      this.socket.binaryType = "arraybuffer";
      this.socket.onopen = q(this._on_socket_open, this);
      this.socket.onmessage = q(this._on_socket_message, this);
      this.socket.onerror = q(this._on_socket_error, this);
      this.socket.onclose = q(this._on_socket_close, this);
      this.sendPinger = new H(this, window, this.connectOptions.keepAliveInterval);
      this.receivePinger = new H(this, window, this.connectOptions.keepAliveInterval);
      this._connectTimeout = new D(this, window, this.connectOptions.timeout, this._disconnected, [g.CONNECT_TIMEOUT.code, f(g.CONNECT_TIMEOUT)]);
    };
    k.prototype._schedule_message = function(a) {
      this._msg_queue.push(a);
      this.connected && this._process_queue();
    };
    k.prototype.store = function(a, b) {
      var c = { type: b.type, messageIdentifier: b.messageIdentifier, version: 1 };
      switch (b.type) {
        case 3:
          b.pubRecReceived && (c.pubRecReceived = true);
          c.payloadMessage = {};
          for (var h = "", e = b.payloadMessage.payloadBytes, d = 0; d < e.length; d++)
            h = 15 >= e[d] ? h + "0" + e[d].toString(16) : h + e[d].toString(16);
          c.payloadMessage.payloadHex = h;
          c.payloadMessage.qos = b.payloadMessage.qos;
          c.payloadMessage.destinationName = b.payloadMessage.destinationName;
          b.payloadMessage.duplicate && (c.payloadMessage.duplicate = true);
          b.payloadMessage.retained && (c.payloadMessage.retained = true);
          0 == a.indexOf("Sent:") && (void 0 === b.sequence && (b.sequence = ++this._sequence), c.sequence = b.sequence);
          break;
        default:
          throw Error(f(g.INVALID_STORED_DATA, [key, c]));
      }
      localStorage.setItem(a + this._localKey + b.messageIdentifier, JSON.stringify(c));
    };
    k.prototype.restore = function(a) {
      var b = localStorage.getItem(a), c = JSON.parse(b), h = new n(c.type, c);
      switch (c.type) {
        case 3:
          for (var b = c.payloadMessage.payloadHex, e = new ArrayBuffer(b.length / 2), e = new Uint8Array(e), d = 0; 2 <= b.length; ) {
            var k2 = parseInt(b.substring(0, 2), 16), b = b.substring(2, b.length);
            e[d++] = k2;
          }
          b = new Paho.MQTT.Message(e);
          b.qos = c.payloadMessage.qos;
          b.destinationName = c.payloadMessage.destinationName;
          c.payloadMessage.duplicate && (b.duplicate = true);
          c.payloadMessage.retained && (b.retained = true);
          h.payloadMessage = b;
          break;
        default:
          throw Error(f(g.INVALID_STORED_DATA, [a, b]));
      }
      0 == a.indexOf("Sent:" + this._localKey) ? (h.payloadMessage.duplicate = true, this._sentMessages[h.messageIdentifier] = h) : 0 == a.indexOf("Received:" + this._localKey) && (this._receivedMessages[h.messageIdentifier] = h);
    };
    k.prototype._process_queue = function() {
      for (var a = null, b = this._msg_queue.reverse(); a = b.pop(); )
        this._socket_send(a), this._notify_msg_sent[a] && (this._notify_msg_sent[a](), delete this._notify_msg_sent[a]);
    };
    k.prototype._requires_ack = function(a) {
      var b = Object.keys(this._sentMessages).length;
      if (b > this.maxMessageIdentifier)
        throw Error("Too many messages:" + b);
      for (; void 0 !== this._sentMessages[this._message_identifier]; )
        this._message_identifier++;
      a.messageIdentifier = this._message_identifier;
      this._sentMessages[a.messageIdentifier] = a;
      3 === a.type && this.store("Sent:", a);
      this._message_identifier === this.maxMessageIdentifier && (this._message_identifier = 1);
    };
    k.prototype._on_socket_open = function() {
      var a = new n(1, this.connectOptions);
      a.clientId = this.clientId;
      this._socket_send(a);
    };
    k.prototype._on_socket_message = function(a) {
      this._trace("Client._on_socket_message", a.data);
      this.receivePinger.reset();
      a = this._deframeMessages(a.data);
      for (var b = 0; b < a.length; b += 1)
        this._handleMessage(a[b]);
    };
    k.prototype._deframeMessages = function(a) {
      a = new Uint8Array(a);
      if (this.receiveBuffer) {
        var b = new Uint8Array(this.receiveBuffer.length + a.length);
        b.set(this.receiveBuffer);
        b.set(a, this.receiveBuffer.length);
        a = b;
        delete this.receiveBuffer;
      }
      try {
        for (var b = 0, c = []; b < a.length; ) {
          var h;
          a: {
            var e = a, d = b, k2 = d, t = e[d], l = t >> 4, z = t & 15, d = d + 1, v = void 0, E = 0, m2 = 1;
            do {
              if (d == e.length) {
                h = [null, k2];
                break a;
              }
              v = e[d++];
              E += (v & 127) * m2;
              m2 *= 128;
            } while (0 != (v & 128));
            v = d + E;
            if (v > e.length)
              h = [null, k2];
            else {
              var w = new n(l);
              switch (l) {
                case 2:
                  e[d++] & 1 && (w.sessionPresent = true);
                  w.returnCode = e[d++];
                  break;
                case 3:
                  var k2 = z >> 1 & 3, r2 = 256 * e[d] + e[d + 1], d = d + 2, u2 = G(e, d, r2), d = d + r2;
                  0 < k2 && (w.messageIdentifier = 256 * e[d] + e[d + 1], d += 2);
                  var q2 = new Paho.MQTT.Message(e.subarray(d, v));
                  1 == (z & 1) && (q2.retained = true);
                  8 == (z & 8) && (q2.duplicate = true);
                  q2.qos = k2;
                  q2.destinationName = u2;
                  w.payloadMessage = q2;
                  break;
                case 4:
                case 5:
                case 6:
                case 7:
                case 11:
                  w.messageIdentifier = 256 * e[d] + e[d + 1];
                  break;
                case 9:
                  w.messageIdentifier = 256 * e[d] + e[d + 1], d += 2, w.returnCode = e.subarray(d, v);
              }
              h = [w, v];
            }
          }
          var x2 = h[0], b = h[1];
          if (null !== x2)
            c.push(x2);
          else
            break;
        }
        b < a.length && (this.receiveBuffer = a.subarray(b));
      } catch (y2) {
        this._disconnected(g.INTERNAL_ERROR.code, f(g.INTERNAL_ERROR, [y2.message, y2.stack.toString()]));
        return;
      }
      return c;
    };
    k.prototype._handleMessage = function(a) {
      this._trace("Client._handleMessage", a);
      try {
        switch (a.type) {
          case 2:
            this._connectTimeout.cancel();
            if (this.connectOptions.cleanSession) {
              for (var b in this._sentMessages) {
                var c = this._sentMessages[b];
                localStorage.removeItem("Sent:" + this._localKey + c.messageIdentifier);
              }
              this._sentMessages = {};
              for (b in this._receivedMessages) {
                var h = this._receivedMessages[b];
                localStorage.removeItem("Received:" + this._localKey + h.messageIdentifier);
              }
              this._receivedMessages = {};
            }
            if (0 === a.returnCode)
              this.connected = true, this.connectOptions.uris && (this.hostIndex = this.connectOptions.uris.length);
            else {
              this._disconnected(g.CONNACK_RETURNCODE.code, f(g.CONNACK_RETURNCODE, [a.returnCode, J[a.returnCode]]));
              break;
            }
            a = [];
            for (var e in this._sentMessages)
              this._sentMessages.hasOwnProperty(e) && a.push(this._sentMessages[e]);
            a = a.sort(function(a2, b2) {
              return a2.sequence - b2.sequence;
            });
            e = 0;
            for (var d = a.length; e < d; e++)
              if (c = a[e], 3 == c.type && c.pubRecReceived) {
                var k2 = new n(6, { messageIdentifier: c.messageIdentifier });
                this._schedule_message(k2);
              } else
                this._schedule_message(c);
            if (this.connectOptions.onSuccess)
              this.connectOptions.onSuccess({ invocationContext: this.connectOptions.invocationContext });
            this._process_queue();
            break;
          case 3:
            this._receivePublish(a);
            break;
          case 4:
            if (c = this._sentMessages[a.messageIdentifier]) {
              if (delete this._sentMessages[a.messageIdentifier], localStorage.removeItem("Sent:" + this._localKey + a.messageIdentifier), this.onMessageDelivered)
                this.onMessageDelivered(c.payloadMessage);
            }
            break;
          case 5:
            if (c = this._sentMessages[a.messageIdentifier])
              c.pubRecReceived = true, k2 = new n(6, { messageIdentifier: a.messageIdentifier }), this.store("Sent:", c), this._schedule_message(k2);
            break;
          case 6:
            h = this._receivedMessages[a.messageIdentifier];
            localStorage.removeItem("Received:" + this._localKey + a.messageIdentifier);
            h && (this._receiveMessage(h), delete this._receivedMessages[a.messageIdentifier]);
            var m2 = new n(7, { messageIdentifier: a.messageIdentifier });
            this._schedule_message(m2);
            break;
          case 7:
            c = this._sentMessages[a.messageIdentifier];
            delete this._sentMessages[a.messageIdentifier];
            localStorage.removeItem("Sent:" + this._localKey + a.messageIdentifier);
            if (this.onMessageDelivered)
              this.onMessageDelivered(c.payloadMessage);
            break;
          case 9:
            if (c = this._sentMessages[a.messageIdentifier]) {
              c.timeOut && c.timeOut.cancel();
              a.returnCode.indexOf = Array.prototype.indexOf;
              if (-1 !== a.returnCode.indexOf(128)) {
                if (c.onFailure)
                  c.onFailure(a.returnCode);
              } else if (c.onSuccess)
                c.onSuccess(a.returnCode);
              delete this._sentMessages[a.messageIdentifier];
            }
            break;
          case 11:
            if (c = this._sentMessages[a.messageIdentifier])
              c.timeOut && c.timeOut.cancel(), c.callback && c.callback(), delete this._sentMessages[a.messageIdentifier];
            break;
          case 13:
            this.sendPinger.reset();
            break;
          case 14:
            this._disconnected(g.INVALID_MQTT_MESSAGE_TYPE.code, f(g.INVALID_MQTT_MESSAGE_TYPE, [a.type]));
            break;
          default:
            this._disconnected(g.INVALID_MQTT_MESSAGE_TYPE.code, f(g.INVALID_MQTT_MESSAGE_TYPE, [a.type]));
        }
      } catch (l) {
        this._disconnected(
          g.INTERNAL_ERROR.code,
          f(g.INTERNAL_ERROR, [l.message, l.stack.toString()])
        );
      }
    };
    k.prototype._on_socket_error = function(a) {
      this._disconnected(g.SOCKET_ERROR.code, f(g.SOCKET_ERROR, [a.data]));
    };
    k.prototype._on_socket_close = function() {
      this._disconnected(g.SOCKET_CLOSE.code, f(g.SOCKET_CLOSE));
    };
    k.prototype._socket_send = function(a) {
      if (1 == a.type) {
        var b = this._traceMask(a, "password");
        this._trace("Client._socket_send", b);
      } else
        this._trace("Client._socket_send", a);
      this.socket.send(a.encode());
      this.sendPinger.reset();
    };
    k.prototype._receivePublish = function(a) {
      switch (a.payloadMessage.qos) {
        case "undefined":
        case 0:
          this._receiveMessage(a);
          break;
        case 1:
          var b = new n(4, { messageIdentifier: a.messageIdentifier });
          this._schedule_message(b);
          this._receiveMessage(a);
          break;
        case 2:
          this._receivedMessages[a.messageIdentifier] = a;
          this.store("Received:", a);
          a = new n(5, { messageIdentifier: a.messageIdentifier });
          this._schedule_message(a);
          break;
        default:
          throw Error("Invaild qos=" + wireMmessage.payloadMessage.qos);
      }
    };
    k.prototype._receiveMessage = function(a) {
      if (this.onMessageArrived)
        this.onMessageArrived(a.payloadMessage);
    };
    k.prototype._disconnected = function(a, b) {
      this._trace("Client._disconnected", a, b);
      this.sendPinger.cancel();
      this.receivePinger.cancel();
      this._connectTimeout && this._connectTimeout.cancel();
      this._msg_queue = [];
      this._notify_msg_sent = {};
      this.socket && (this.socket.onopen = null, this.socket.onmessage = null, this.socket.onerror = null, this.socket.onclose = null, 1 === this.socket.readyState && this.socket.close(), delete this.socket);
      if (this.connectOptions.uris && this.hostIndex < this.connectOptions.uris.length - 1)
        this.hostIndex++, this._doConnect(this.connectOptions.uris[this.hostIndex]);
      else if (void 0 === a && (a = g.OK.code, b = f(g.OK)), this.connected) {
        if (this.connected = false, this.onConnectionLost)
          this.onConnectionLost({ errorCode: a, errorMessage: b });
      } else if (4 === this.connectOptions.mqttVersion && false === this.connectOptions.mqttVersionExplicit)
        this._trace("Failed to connect V4, dropping back to V3"), this.connectOptions.mqttVersion = 3, this.connectOptions.uris ? (this.hostIndex = 0, this._doConnect(this.connectOptions.uris[0])) : this._doConnect(this.uri);
      else if (this.connectOptions.onFailure)
        this.connectOptions.onFailure({ invocationContext: this.connectOptions.invocationContext, errorCode: a, errorMessage: b });
    };
    k.prototype._trace = function() {
      if (this.traceFunction) {
        for (var a in arguments)
          "undefined" !== typeof arguments[a] && (arguments[a] = JSON.stringify(arguments[a]));
        a = Array.prototype.slice.call(arguments).join("");
        this.traceFunction({ severity: "Debug", message: a });
      }
      if (null !== this._traceBuffer) {
        a = 0;
        for (var b = arguments.length; a < b; a++)
          this._traceBuffer.length == this._MAX_TRACE_ENTRIES && this._traceBuffer.shift(), 0 === a ? this._traceBuffer.push(arguments[a]) : "undefined" === typeof arguments[a] ? this._traceBuffer.push(arguments[a]) : this._traceBuffer.push("  " + JSON.stringify(arguments[a]));
      }
    };
    k.prototype._traceMask = function(a, b) {
      var c = {}, f2;
      for (f2 in a)
        a.hasOwnProperty(f2) && (c[f2] = f2 == b ? "******" : a[f2]);
      return c;
    };
    var I = function(a, b, c, h) {
      var e;
      if ("string" !== typeof a)
        throw Error(f(g.INVALID_TYPE, [typeof a, "host"]));
      if (2 == arguments.length) {
        h = b;
        e = a;
        var d = e.match(/^(wss?):\/\/((\[(.+)\])|([^\/]+?))(:(\d+))?(\/.*)$/);
        if (d)
          a = d[4] || d[2], b = parseInt(d[7]), c = d[8];
        else
          throw Error(f(g.INVALID_ARGUMENT, [a, "host"]));
      } else {
        3 == arguments.length && (h = c, c = "/mqtt");
        if ("number" !== typeof b || 0 > b)
          throw Error(f(g.INVALID_TYPE, [typeof b, "port"]));
        if ("string" !== typeof c)
          throw Error(f(g.INVALID_TYPE, [typeof c, "path"]));
        e = "ws://" + (-1 != a.indexOf(":") && "[" != a.slice(0, 1) && "]" != a.slice(-1) ? "[" + a + "]" : a) + ":" + b + c;
      }
      for (var p = d = 0; p < h.length; p++) {
        var m2 = h.charCodeAt(p);
        55296 <= m2 && 56319 >= m2 && p++;
        d++;
      }
      if ("string" !== typeof h || 65535 < d)
        throw Error(f(
          g.INVALID_ARGUMENT,
          [h, "clientId"]
        ));
      var l = new k(e, a, b, c, h);
      this._getHost = function() {
        return a;
      };
      this._setHost = function() {
        throw Error(f(g.UNSUPPORTED_OPERATION));
      };
      this._getPort = function() {
        return b;
      };
      this._setPort = function() {
        throw Error(f(g.UNSUPPORTED_OPERATION));
      };
      this._getPath = function() {
        return c;
      };
      this._setPath = function() {
        throw Error(f(g.UNSUPPORTED_OPERATION));
      };
      this._getURI = function() {
        return e;
      };
      this._setURI = function() {
        throw Error(f(g.UNSUPPORTED_OPERATION));
      };
      this._getClientId = function() {
        return l.clientId;
      };
      this._setClientId = function() {
        throw Error(f(g.UNSUPPORTED_OPERATION));
      };
      this._getOnConnectionLost = function() {
        return l.onConnectionLost;
      };
      this._setOnConnectionLost = function(a2) {
        if ("function" === typeof a2)
          l.onConnectionLost = a2;
        else
          throw Error(f(g.INVALID_TYPE, [typeof a2, "onConnectionLost"]));
      };
      this._getOnMessageDelivered = function() {
        return l.onMessageDelivered;
      };
      this._setOnMessageDelivered = function(a2) {
        if ("function" === typeof a2)
          l.onMessageDelivered = a2;
        else
          throw Error(f(g.INVALID_TYPE, [typeof a2, "onMessageDelivered"]));
      };
      this._getOnMessageArrived = function() {
        return l.onMessageArrived;
      };
      this._setOnMessageArrived = function(a2) {
        if ("function" === typeof a2)
          l.onMessageArrived = a2;
        else
          throw Error(f(g.INVALID_TYPE, [typeof a2, "onMessageArrived"]));
      };
      this._getTrace = function() {
        return l.traceFunction;
      };
      this._setTrace = function(a2) {
        if ("function" === typeof a2)
          l.traceFunction = a2;
        else
          throw Error(f(g.INVALID_TYPE, [typeof a2, "onTrace"]));
      };
      this.connect = function(a2) {
        a2 = a2 || {};
        A(a2, {
          timeout: "number",
          userName: "string",
          password: "string",
          willMessage: "object",
          keepAliveInterval: "number",
          cleanSession: "boolean",
          useSSL: "boolean",
          invocationContext: "object",
          onSuccess: "function",
          onFailure: "function",
          hosts: "object",
          ports: "object",
          mqttVersion: "number"
        });
        void 0 === a2.keepAliveInterval && (a2.keepAliveInterval = 60);
        if (4 < a2.mqttVersion || 3 > a2.mqttVersion)
          throw Error(f(g.INVALID_ARGUMENT, [a2.mqttVersion, "connectOptions.mqttVersion"]));
        void 0 === a2.mqttVersion ? (a2.mqttVersionExplicit = false, a2.mqttVersion = 4) : a2.mqttVersionExplicit = true;
        if (void 0 === a2.password && void 0 !== a2.userName)
          throw Error(f(
            g.INVALID_ARGUMENT,
            [a2.password, "connectOptions.password"]
          ));
        if (a2.willMessage) {
          if (!(a2.willMessage instanceof x))
            throw Error(f(g.INVALID_TYPE, [a2.willMessage, "connectOptions.willMessage"]));
          a2.willMessage.stringPayload;
          if ("undefined" === typeof a2.willMessage.destinationName)
            throw Error(f(g.INVALID_TYPE, [typeof a2.willMessage.destinationName, "connectOptions.willMessage.destinationName"]));
        }
        "undefined" === typeof a2.cleanSession && (a2.cleanSession = true);
        if (a2.hosts) {
          if (!(a2.hosts instanceof Array))
            throw Error(f(g.INVALID_ARGUMENT, [
              a2.hosts,
              "connectOptions.hosts"
            ]));
          if (1 > a2.hosts.length)
            throw Error(f(g.INVALID_ARGUMENT, [a2.hosts, "connectOptions.hosts"]));
          for (var b2 = false, d2 = 0; d2 < a2.hosts.length; d2++) {
            if ("string" !== typeof a2.hosts[d2])
              throw Error(f(g.INVALID_TYPE, [typeof a2.hosts[d2], "connectOptions.hosts[" + d2 + "]"]));
            if (/^(wss?):\/\/((\[(.+)\])|([^\/]+?))(:(\d+))?(\/.*)$/.test(a2.hosts[d2]))
              if (0 == d2)
                b2 = true;
              else {
                if (!b2)
                  throw Error(f(g.INVALID_ARGUMENT, [a2.hosts[d2], "connectOptions.hosts[" + d2 + "]"]));
              }
            else if (b2)
              throw Error(f(g.INVALID_ARGUMENT, [a2.hosts[d2], "connectOptions.hosts[" + d2 + "]"]));
          }
          if (b2)
            a2.uris = a2.hosts;
          else {
            if (!a2.ports)
              throw Error(f(g.INVALID_ARGUMENT, [a2.ports, "connectOptions.ports"]));
            if (!(a2.ports instanceof Array))
              throw Error(f(g.INVALID_ARGUMENT, [a2.ports, "connectOptions.ports"]));
            if (a2.hosts.length != a2.ports.length)
              throw Error(f(g.INVALID_ARGUMENT, [a2.ports, "connectOptions.ports"]));
            a2.uris = [];
            for (d2 = 0; d2 < a2.hosts.length; d2++) {
              if ("number" !== typeof a2.ports[d2] || 0 > a2.ports[d2])
                throw Error(f(g.INVALID_TYPE, [typeof a2.ports[d2], "connectOptions.ports[" + d2 + "]"]));
              var b2 = a2.hosts[d2], h2 = a2.ports[d2];
              e = "ws://" + (-1 != b2.indexOf(":") ? "[" + b2 + "]" : b2) + ":" + h2 + c;
              a2.uris.push(e);
            }
          }
        }
        l.connect(a2);
      };
      this.subscribe = function(a2, b2) {
        if ("string" !== typeof a2)
          throw Error("Invalid argument:" + a2);
        b2 = b2 || {};
        A(b2, { qos: "number", invocationContext: "object", onSuccess: "function", onFailure: "function", timeout: "number" });
        if (b2.timeout && !b2.onFailure)
          throw Error("subscribeOptions.timeout specified with no onFailure callback.");
        if ("undefined" !== typeof b2.qos && 0 !== b2.qos && 1 !== b2.qos && 2 !== b2.qos)
          throw Error(f(g.INVALID_ARGUMENT, [
            b2.qos,
            "subscribeOptions.qos"
          ]));
        l.subscribe(a2, b2);
      };
      this.unsubscribe = function(a2, b2) {
        if ("string" !== typeof a2)
          throw Error("Invalid argument:" + a2);
        b2 = b2 || {};
        A(b2, { invocationContext: "object", onSuccess: "function", onFailure: "function", timeout: "number" });
        if (b2.timeout && !b2.onFailure)
          throw Error("unsubscribeOptions.timeout specified with no onFailure callback.");
        l.unsubscribe(a2, b2);
      };
      this.send = function(a2, b2, c2, d2) {
        var e2;
        if (0 == arguments.length)
          throw Error("Invalid argument.length");
        if (1 == arguments.length) {
          if (!(a2 instanceof x) && "string" !== typeof a2)
            throw Error("Invalid argument:" + typeof a2);
          e2 = a2;
          if ("undefined" === typeof e2.destinationName)
            throw Error(f(g.INVALID_ARGUMENT, [e2.destinationName, "Message.destinationName"]));
        } else
          e2 = new x(b2), e2.destinationName = a2, 3 <= arguments.length && (e2.qos = c2), 4 <= arguments.length && (e2.retained = d2);
        l.send(e2);
      };
      this.disconnect = function() {
        l.disconnect();
      };
      this.getTraceLog = function() {
        return l.getTraceLog();
      };
      this.startTrace = function() {
        l.startTrace();
      };
      this.stopTrace = function() {
        l.stopTrace();
      };
      this.isConnected = function() {
        return l.connected;
      };
    };
    I.prototype = {
      get host() {
        return this._getHost();
      },
      set host(a) {
        this._setHost(a);
      },
      get port() {
        return this._getPort();
      },
      set port(a) {
        this._setPort(a);
      },
      get path() {
        return this._getPath();
      },
      set path(a) {
        this._setPath(a);
      },
      get clientId() {
        return this._getClientId();
      },
      set clientId(a) {
        this._setClientId(a);
      },
      get onConnectionLost() {
        return this._getOnConnectionLost();
      },
      set onConnectionLost(a) {
        this._setOnConnectionLost(a);
      },
      get onMessageDelivered() {
        return this._getOnMessageDelivered();
      },
      set onMessageDelivered(a) {
        this._setOnMessageDelivered(a);
      },
      get onMessageArrived() {
        return this._getOnMessageArrived();
      },
      set onMessageArrived(a) {
        this._setOnMessageArrived(a);
      },
      get trace() {
        return this._getTrace();
      },
      set trace(a) {
        this._setTrace(a);
      }
    };
    var x = function(a) {
      var b;
      if ("string" === typeof a || a instanceof ArrayBuffer || a instanceof Int8Array || a instanceof Uint8Array || a instanceof Int16Array || a instanceof Uint16Array || a instanceof Int32Array || a instanceof Uint32Array || a instanceof Float32Array || a instanceof Float64Array)
        b = a;
      else
        throw f(g.INVALID_ARGUMENT, [a, "newPayload"]);
      this._getPayloadString = function() {
        return "string" === typeof b ? b : G(b, 0, b.length);
      };
      this._getPayloadBytes = function() {
        if ("string" === typeof b) {
          var a2 = new ArrayBuffer(m(b)), a2 = new Uint8Array(a2);
          F(b, a2, 0);
          return a2;
        }
        return b;
      };
      var c = void 0;
      this._getDestinationName = function() {
        return c;
      };
      this._setDestinationName = function(a2) {
        if ("string" === typeof a2)
          c = a2;
        else
          throw Error(f(g.INVALID_ARGUMENT, [a2, "newDestinationName"]));
      };
      var h = 0;
      this._getQos = function() {
        return h;
      };
      this._setQos = function(a2) {
        if (0 === a2 || 1 === a2 || 2 === a2)
          h = a2;
        else
          throw Error("Invalid argument:" + a2);
      };
      var e = false;
      this._getRetained = function() {
        return e;
      };
      this._setRetained = function(a2) {
        if ("boolean" === typeof a2)
          e = a2;
        else
          throw Error(f(g.INVALID_ARGUMENT, [a2, "newRetained"]));
      };
      var d = false;
      this._getDuplicate = function() {
        return d;
      };
      this._setDuplicate = function(a2) {
        d = a2;
      };
    };
    x.prototype = {
      get payloadString() {
        return this._getPayloadString();
      },
      get payloadBytes() {
        return this._getPayloadBytes();
      },
      get destinationName() {
        return this._getDestinationName();
      },
      set destinationName(a) {
        this._setDestinationName(a);
      },
      get qos() {
        return this._getQos();
      },
      set qos(a) {
        this._setQos(a);
      },
      get retained() {
        return this._getRetained();
      },
      set retained(a) {
        this._setRetained(a);
      },
      get duplicate() {
        return this._getDuplicate();
      },
      set duplicate(a) {
        this._setDuplicate(a);
      }
    };
    return { Client: I, Message: x };
  }(typeof window !== "undefined" ? window : globalThis);
  const Paho$1 = Paho;
  const _export_sfc = (sfc, props) => {
    const target = sfc.__vccOpts || sfc;
    for (const [key2, val] of props) {
      target[key2] = val;
    }
    return target;
  };
  const _sfc_main$1 = {
    data() {
      return {
        client: null,
        isConnected: false,
        timer: null,
        // 自动轮询定时器
        lastUpdate: "--:--",
        // 状态数据
        env: { temp: "--", hum: "--" },
        light: { isOn: false, val: 0 },
        door: { isOpen: false, lastMsg: "", isAlert: false },
        logs: [],
        // MQTT 配置 (完全复用 Web V3 的配置)
        config: {
          host: "broker.emqx.io",
          port: 8084,
          // WSS 端口
          path: "/mqtt",
          // 订阅主题
          subHome: "China/Beijing/huayuan/302/home/status",
          subDoor: "China/Beijing/huayuan/302/door/status",
          // 发布主题
          pubCmd: "China/Beijing/huayuan/302/command",
          pubDoor: "China/Beijing/huayuan/302/door/status"
          // 门控指令专用
        }
      };
    },
    onLoad() {
      setTimeout(() => {
        this.connectMQTT();
      }, 500);
    },
    onUnload() {
      if (this.timer)
        clearInterval(this.timer);
      if (this.client && this.isConnected)
        this.client.disconnect();
    },
    methods: {
      // --- 连接逻辑 ---
      connectMQTT() {
        this.addLog("正在连接服务器...");
        let clientId = "AppV3-" + Math.random().toString(16).substr(2, 8);
        try {
          this.client = new Paho$1.MQTT.Client(this.config.host, this.config.port, this.config.path, clientId);
        } catch (e) {
          this.addLog("初始化失败: " + e.message);
          return;
        }
        this.client.onConnectionLost = (res) => {
          this.isConnected = false;
          this.addLog("连接断开:" + res.errorMessage);
          setTimeout(() => this.connectMQTT(), 5e3);
        };
        this.client.onMessageArrived = (message) => {
          this.handleMessage(message.destinationName, message.payloadString);
        };
        this.client.connect({
          useSSL: true,
          cleanSession: true,
          keepAliveInterval: 60,
          onSuccess: () => {
            this.isConnected = true;
            this.addLog("✅ 连接成功!");
            this.client.subscribe(this.config.subHome);
            this.client.subscribe(this.config.subDoor);
            this.manualQuery();
            this.startAutoQuery();
          },
          onFailure: (e) => {
            this.isConnected = false;
            this.addLog("❌ 连接失败: " + e.errorMessage);
          }
        });
      },
      // --- 核心：消息处理 (复用 Web V3 逻辑) ---
      handleMessage(topic, msg) {
        if (topic === this.config.subHome) {
          this.lastUpdate = (/* @__PURE__ */ new Date()).toLocaleTimeString();
          if (msg.includes("温度"))
            this.env.temp = this.extractNum(msg);
          if (msg.includes("湿度"))
            this.env.hum = this.extractNum(msg);
          if (msg.includes("灯光"))
            this.light.isOn = msg.includes("开启");
          if (msg.includes("亮度")) {
            let val = parseInt(this.extractNum(msg));
            if (!isNaN(val))
              this.light.val = val;
          }
          if (msg.includes("大门"))
            this.door.isOpen = msg.includes("开启");
        }
        if (topic === this.config.subDoor) {
          this.door.lastMsg = msg + " (" + (/* @__PURE__ */ new Date()).toLocaleTimeString() + ")";
          if (msg.includes("客人") || msg.includes("错误")) {
            this.triggerAlert();
            uni.vibrateLong();
          }
          if (msg.includes("door open"))
            this.door.isOpen = true;
          if (msg.includes("door close"))
            this.door.isOpen = false;
        }
      },
      // --- 控制指令 ---
      // 1. 通用设备指令
      sendCmd(cmd) {
        this.publish(this.config.pubCmd, cmd);
      },
      // 2. 门禁专用指令 (发到 door/status)
      sendDoorCmd(cmd) {
        this.publish(this.config.pubDoor, cmd);
      },
      // 3. 调节亮度
      onSliderChange(e) {
        this.light.val = e.detail.value;
        this.sendCmd("light:" + this.light.val);
      },
      // 4. 手动/自动查询
      manualQuery() {
        this.sendCmd("get_status");
        uni.showToast({ title: "已发送查询", icon: "none" });
      },
      startAutoQuery() {
        if (this.timer)
          clearInterval(this.timer);
        this.timer = setInterval(() => {
          if (this.isConnected)
            this.sendCmd("get_status");
        }, 1e4);
      },
      // --- 辅助函数 ---
      publish(topic, payload) {
        if (this.client && this.isConnected) {
          let message = new Paho$1.MQTT.Message(payload);
          message.destinationName = topic;
          this.client.send(message);
          this.addLog("发送: " + payload);
        } else {
          uni.showToast({ title: "未连接", icon: "none" });
        }
      },
      extractNum(str) {
        let match = str.match(/-?\d+(\.\d+)?/);
        return match ? match[0] : "--";
      },
      triggerAlert() {
        this.door.isAlert = true;
        setTimeout(() => {
          this.door.isAlert = false;
        }, 3e3);
      },
      addLog(txt) {
        let time = (/* @__PURE__ */ new Date()).toLocaleTimeString();
        this.logs.unshift(`[${time}] ${txt}`);
        if (this.logs.length > 20)
          this.logs.pop();
      }
    }
  };
  function _sfc_render(_ctx, _cache, $props, $setup, $data, $options) {
    return vue.openBlock(), vue.createElementBlock("div", { class: "content" }, [
      vue.createElementVNode("div", { class: "header" }, [
        vue.createElementVNode("h2", { class: "title" }, "🏠 智能家居 V3.0"),
        vue.createElementVNode("div", { class: "status-bar" }, [
          vue.createElementVNode(
            "span",
            {
              class: vue.normalizeClass(["badge", { online: $data.isConnected }])
            },
            vue.toDisplayString($data.isConnected ? "☁️ 已连接云端" : "❌ 等待连接..."),
            3
            /* TEXT, CLASS */
          ),
          vue.createElementVNode("button", {
            class: "refresh-btn",
            onClick: _cache[0] || (_cache[0] = (...args) => $options.manualQuery && $options.manualQuery(...args)),
            disabled: !$data.isConnected
          }, " 🔄 刷新 ", 8, ["disabled"])
        ])
      ]),
      vue.createElementVNode("div", { class: "card env-card" }, [
        vue.createElementVNode("div", { class: "card-title" }, "🌡️ 环境监测"),
        vue.createElementVNode("div", { class: "env-grid" }, [
          vue.createElementVNode("div", { class: "env-item" }, [
            vue.createElementVNode(
              "text",
              { class: "env-val temp" },
              vue.toDisplayString($data.env.temp),
              1
              /* TEXT */
            ),
            vue.createElementVNode("text", { class: "env-unit" }, "℃"),
            vue.createElementVNode("text", { class: "env-label" }, "室内温度")
          ]),
          vue.createElementVNode("div", { class: "env-item" }, [
            vue.createElementVNode(
              "text",
              { class: "env-val hum" },
              vue.toDisplayString($data.env.hum),
              1
              /* TEXT */
            ),
            vue.createElementVNode("text", { class: "env-unit" }, "%"),
            vue.createElementVNode("text", { class: "env-label" }, "空气湿度")
          ])
        ]),
        vue.createElementVNode(
          "div",
          { class: "update-time" },
          "更新时间: " + vue.toDisplayString($data.lastUpdate),
          1
          /* TEXT */
        )
      ]),
      vue.createElementVNode("div", { class: "card light-card" }, [
        vue.createElementVNode("div", { class: "card-header" }, [
          vue.createElementVNode("text", { class: "card-title" }, "💡 灯光控制"),
          vue.createElementVNode(
            "text",
            {
              class: vue.normalizeClass(["state-text", { on: $data.light.isOn }])
            },
            vue.toDisplayString($data.light.isOn ? "已开启" : "已关闭"),
            3
            /* TEXT, CLASS */
          )
        ]),
        vue.createElementVNode("div", { class: "btn-group" }, [
          vue.createElementVNode("button", {
            class: "btn btn-yellow",
            onClick: _cache[1] || (_cache[1] = ($event) => $options.sendCmd("ON"))
          }, "全开"),
          vue.createElementVNode("button", {
            class: "btn btn-gray",
            onClick: _cache[2] || (_cache[2] = ($event) => $options.sendCmd("OFF"))
          }, "全关")
        ]),
        vue.createElementVNode("div", { class: "slider-area" }, [
          vue.createElementVNode(
            "text",
            null,
            "亮度: " + vue.toDisplayString($data.light.val) + "%",
            1
            /* TEXT */
          ),
          vue.createElementVNode("slider", {
            value: $data.light.val,
            onChange: _cache[3] || (_cache[3] = (...args) => $options.onSliderChange && $options.onSliderChange(...args)),
            min: "0",
            max: "100",
            "active-color": "#ffc107",
            "block-size": "24"
          }, null, 40, ["value"])
        ])
      ]),
      vue.createElementVNode(
        "div",
        {
          class: vue.normalizeClass(["card door-card", { alert: $data.door.isAlert }])
        },
        [
          vue.createElementVNode("div", { class: "card-header" }, [
            vue.createElementVNode("text", { class: "card-title" }, "🔒 门禁系统"),
            vue.createElementVNode(
              "text",
              {
                class: vue.normalizeClass(["state-text", { open: $data.door.isOpen }])
              },
              vue.toDisplayString($data.door.isOpen ? "🔓 已开启" : "🔒 已关闭"),
              3
              /* TEXT, CLASS */
            )
          ]),
          vue.createElementVNode("div", { class: "btn-group" }, [
            vue.createElementVNode("button", {
              class: "btn btn-green",
              onClick: _cache[4] || (_cache[4] = ($event) => $options.sendDoorCmd("door open"))
            }, "远程开门"),
            vue.createElementVNode("button", {
              class: "btn btn-red",
              onClick: _cache[5] || (_cache[5] = ($event) => $options.sendDoorCmd("door close"))
            }, "远程关门")
          ]),
          $data.door.lastMsg ? (vue.openBlock(), vue.createElementBlock(
            "div",
            {
              key: 0,
              class: "msg-box"
            },
            " 📢 最新消息: " + vue.toDisplayString($data.door.lastMsg),
            1
            /* TEXT */
          )) : vue.createCommentVNode("v-if", true)
        ],
        2
        /* CLASS */
      ),
      vue.createElementVNode("div", { class: "log-box" }, [
        (vue.openBlock(true), vue.createElementBlock(
          vue.Fragment,
          null,
          vue.renderList($data.logs, (log, index) => {
            return vue.openBlock(), vue.createElementBlock(
              "view",
              {
                key: index,
                class: "log-item"
              },
              vue.toDisplayString(log),
              1
              /* TEXT */
            );
          }),
          128
          /* KEYED_FRAGMENT */
        ))
      ])
    ]);
  }
  const PagesIndexIndex = /* @__PURE__ */ _export_sfc(_sfc_main$1, [["render", _sfc_render], ["__file", "C:/Users/123/Desktop/my_code/arduino/HomeControl_System/Host Computer/SmartHomeApp/pages/index/index.vue"]]);
  __definePage("pages/index/index", PagesIndexIndex);
  function formatAppLog(type, filename, ...args) {
    if (uni.__log__) {
      uni.__log__(type, filename, ...args);
    } else {
      console[type].apply(console, [...args, filename]);
    }
  }
  const _sfc_main = {
    onLaunch: function() {
      formatAppLog("log", "at App.vue:4", "App Launch");
    },
    onShow: function() {
      formatAppLog("log", "at App.vue:7", "App Show");
    },
    onHide: function() {
      formatAppLog("log", "at App.vue:10", "App Hide");
    }
  };
  const App = /* @__PURE__ */ _export_sfc(_sfc_main, [["__file", "C:/Users/123/Desktop/my_code/arduino/HomeControl_System/Host Computer/SmartHomeApp/App.vue"]]);
  function createApp() {
    const app = vue.createVueApp(App);
    return {
      app
    };
  }
  const { app: __app__, Vuex: __Vuex__, Pinia: __Pinia__ } = createApp();
  uni.Vuex = __Vuex__;
  uni.Pinia = __Pinia__;
  __app__.provide("__globalStyles", __uniConfig.styles);
  __app__._component.mpType = "app";
  __app__._component.render = () => {
  };
  __app__.mount("#app");
})(Vue);
