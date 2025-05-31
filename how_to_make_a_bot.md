# Bot tutorial
this is the bot class:
```js
class Bot extends EventEmitter {
  constructor() {
    super();
    
    this.id = 0;
    this.nick = "";
    this.hue = "";

    this.cursors = new Map();

    this.webSocket = null;

    this.address =
      "wss://4b1e-2406-8800-9014-420a-7b01-343-e6db-2ba4.ngrok-free.app/";
    this.hasConnection = false;
    this.sentHello = false;
    this.lastPing = 0;

    // Client -> Server
    this.OPCODE_PING = 0x00;
    this.OPCODE_HI = 0x01;
    this.OPCODE_HI_BOT = 0x02;
    this.OPCODE_ENTER_GAME = 0x03;
    this.OPCODE_LEAVE_GAME = 0x04;
    this.OPCODE_RESIZE = 0x05;
    this.OPCODE_CURSOR = 0x06;
    this.OPCODE_CD = 0x07;
    this.OPCODE_LS = 0x08;
    this.OPCODE_CHAT = 0x09;
    this.OPCODE_LS_MESSAGES = 0x10;

    // Server -> Client
    this.OPCODE_PONG = 0x00;
    this.OPCODE_ENTERED_GAME = 0xa0;
    this.OPCODE_CYCLE_S = 0xa4;
    this.OPCODE_EVENTS = 0xa1;
    this.OPCODE_HISTORY = 0xb1;
    this.OPCODE_CONFIG = 0xb2;

    this.connect();
  }

  connect() {
    try {
      this.webSocket = new WebSocket(this.address);
    } catch (e) {
      setTimeout(() => this.connect(), 1e3);
      console.log(e);
      return;
    }
    this.webSocket.binaryType = "arraybuffer";
    this.webSocket.onopen = this.onSocketOpen.bind(this);
    this.webSocket.onclose = this.onSocketClose.bind(this);
    this.webSocket.onerror = this.onError.bind(this);
    this.webSocket.onmessage = this.onSocketMessage.bind(this);
  }

  onSocketOpen() {
    this.hello();
    this.emit("open");
  }

  onSocketClose() {
    this.hasConnection = false;
    this.emit("close");
    setTimeout(() => this.connect(), 1e3);
  }

  onError(a) {
    this.emit("error", a);
  }

  onSocketMessage(event) {
    this.processMessage(event.data);
  }

  hello() {
    this.ping();
    this.sendHello();
  }

  processEvents(view) {
    let offset = 1;
    let type = view.getUint8(offset++);
    switch (type) {
      case 0x1: {
        let id = view.getUint16(offset, true);
        offset += 2;
        let res = getString(view, offset);
        let nick = res.nick;
        offset = res.offset;
        let res0 = getString(view, offset);
        let message = res0.nick;
        offset = res0.offset;
        let color = this.cursors.get(id)?.hue || 240;
        if (id == this.id) color = this.hue;
        this.emit("chat-message", {
          content: message,
          author: {
            nick: nick,
            id: id,
            color: color,
          },
        });
        break;
      }

      default:
        break;
    }
  }

  processConfig(view) {
    let offset = 1,
      byteLength = view.byteLength,
      lobbies = [];
    while (offset != byteLength) {
      let res = getLobbyName(view, offset);
      let name = res.nick;
      offset = res.offset;
      lobbies.push(name);
    }
    this.emit("lobbies", lobbies);
  }

  processHistory(view) {
    let offset = 1,
      byteLength = view.byteLength;
    let history = [];
    while (offset != byteLength) {
      let id = view.getUint16(offset, true);
      offset += 2;
      let hue = view.getUint16(offset, true);
      offset += 2;
      let timestamp = view.getFloat64(offset, true);
      offset += 8;
      let res = getString(view, offset);
      let nick = res.nick;
      offset = res.offset;
      res = getString(view, offset);
      let content = res.nick;
      offset = res.offset;
      history.push({
        content: content,
        author: {
          id,
          color: hue,
          nick,
        },
      });
    }
    this.emit("history", history);
  }

  processCursors(view) {
    let offset = 1;
    while (true) {
      let id = view.getUint16(offset, true);
      offset += 2;
      if (id == 0x00) break;
      let flags = view.getUint8(offset, true);
      offset++;
      switch (flags) {
        case 0x0: {
          // appear
          let cursor = new Cursor();
          cursor.id = id;
          offset = cursor.updateNetwork(view, offset, true);
          this.cursors.set(id, cursor);
          break;
        }

        case 0x1: {
          // update
          let cursor = this.cursors.get(id);
          if (cursor) {
            offset = cursor.updateNetwork(view, offset, false);
          } else {
            console.log("cursor with id: " + id + " not found");
          }
          break;
        }

        case 0x2: {
          // delete
          let cursor = this.cursors.get(id);
          if (cursor) {
            offset = cursor.deleteNetwork(view, offset);
          } else {
            console.log("unknown cursor: " + id + " can't delete it");
          }
          break;
        }

        case 0x3: {
          let cursor = new Cursor(true);
          cursor.id = id;
          offset = cursor.updateNetwork(view, offset, true);
          this.cursors.set(id, cursor);
          break;
        }

        default:
          console.log("unknown flags", flags);
      }
    }
  }

  processMessage(buffer) {
    let view = new DataView(buffer);
    let op = view.getUint8(0);
    switch (op) {
      case this.OPCODE_PONG:
        console.log("Pong", +new Date() - this.lastPing);
        break;
      case this.OPCODE_ENTERED_GAME:
        this.isInGame = true;
        this.id = view.getUint16(1, true);
        this.hue = view.getUint16(3, true);
        this.getHistory();
        break;
      case this.OPCODE_CYCLE_S:
        this.processCursors(view, op);
        break;
      case this.OPCODE_EVENTS:
        this.processEvents(view);
        break;
      case this.OPCODE_HISTORY:
        this.processHistory(view);
        break;
      case this.OPCODE_CONFIG:
        this.processConfig(view);
        break;
      default:
        console.log("unknown op:", op);
        break;
    }
  }

  ping() {
    let buffer = new ArrayBuffer(1);
    let view = new DataView(buffer);

    view.setUint8(0, this.OPCODE_PING);

    this.webSocket.send(buffer);
    this.lastPing = +new Date();
  }

  sendHello() {
    let buffer = new ArrayBuffer(1);
    let view = new DataView(buffer);

    view.setUint8(0, this.OPCODE_HI_BOT);

    this.webSocket.send(buffer);
  }

  sendResize() {
    let buffer = new ArrayBuffer(5);
    let view = new DataView(buffer);

    view.setUint8(0, this.OPCODE_RESIZE);

    view.setUint16(1, window.innerWidth, true);
    view.setUint16(3, window.innerHeight, true);

    this.webSocket.send(buffer);
  }

  sendCursor(x, y) {
    let buffer = new ArrayBuffer(5);
    let view = new DataView(buffer);

    view.setUint8(0, this.OPCODE_CURSOR);

    view.setUint16(1, x, true);
    view.setUint16(3, y, true);

    this.webSocket.send(buffer);
  }

  sendNick(nick) {
    let buffer = new ArrayBuffer(1 + 2 * nick.length + 3);
    let view = new DataView(buffer);

    view.setUint8(0, this.OPCODE_ENTER_GAME);

    for (let i = 0; i < nick.length; i++) {
      view.setUint16(1 + i * 2, nick.charCodeAt(i), true);
    }

    this.webSocket.send(buffer);
  }

  leave() {
    let buffer = new ArrayBuffer(1);
    let view = new DataView(buffer);

    view.setUint8(0, this.OPCODE_LEAVE_GAME);

    this.webSocket.send(buffer);
  }

  sendChat(value) {
    let buffer = new ArrayBuffer(1 + 2 * value.length + 3);
    let view = new DataView(buffer);

    view.setUint8(0, this.OPCODE_CHAT);

    for (let i = 0, l = value.length; i < l; i++) {
      view.setUint16(1 + i * 2, value.charCodeAt(i), true);
    }

    this.webSocket.send(buffer);
  }

  changeRoom(value) {
    let buffer = new ArrayBuffer(1 + value.length + 3);
    let view = new DataView(buffer);

    view.setUint8(0, this.OPCODE_CD);

    for (let i = 0, l = value.length; i < l; i++) {
      view.setUint8(1 + i, value.charCodeAt(i));
    }

    this.webSocket.send(buffer);
  }

  list() {
    let buffer = new ArrayBuffer(1);
    let view = new DataView(buffer);

    view.setUint8(0, this.OPCODE_LS);

    this.webSocket.send(buffer);
  }

  getHistory() {
    let buffer = new ArrayBuffer(1);
    let view = new DataView(buffer);

    view.setUint8(0, this.OPCODE_LS_MESSAGES);

    this.webSocket.send(buffer);
  }
}
```
<br>
this is the Cursor class:

```js
class Cursor {
  constructor(maybeShow) {
    this.id = 0;

    this.x = 0;
    this.y = 0;

    this.bot = false;

    this.nick = "Anonymous";
    this.hue = 240;

    this.lastUpdateTime = 0;

    if (maybeShow) {
      this.bot = true;
    }
  }

  updateNetwork(view, offset, isFull) {
    let x = (view.getUint16(offset, true) / 65535) * 1366;
    offset += 2;

    let y = (view.getUint16(offset, true) / 65535) * 768;
    offset += 2;

    if (isFull) {
      let hue = view.getUint16(offset, true);
      offset += 2;

      let res = getString(view, offset);
      offset = res.offset;

      this.nick = res.nick;
      this.hue = hue;
    }

    this.x = x;
    this.y = y;

    return offset;
  }

  deleteNetwork(view, offset) {
    return offset;
  }
}```
<br>
these are the util functions:
```js
function getString(view, offset) {
  var nick = "";
  for (;;) {
    var v = view.getUint16(offset, true);
    offset += 2;
    if (v == 0) {
      break;
    }

    nick += String.fromCharCode(v);
  }
  return {
    nick: nick,
    offset: offset,
  };
}

function getLobbyName(view, offset) {
  var nick = "";
  for (;;) {
    var v = view.getUint8(offset, true);
    offset += 1;
    if (v == 0) {
      break;
    }

    nick += String.fromCharCode(v);
  }
  return {
    nick: nick,
    offset: offset,
  };
}
```

<br>
this is how you'd do things:

```js
let bot = new Bot();

bot.on('open', () => {
  bot.sendNick('My Bot');
  bot.sendCursor(700, 400); // move anywhere you want (1336x768 screen)
});

bot.on('chat-message', (msg) => {
  /* 
    msg contains:
    content: the string which was sent
    author:
    an object which contains:
      nick: the nickname of the person who sent the message
      id: the id of the person who sent the message
      color: the color of the person who sent the messge
      (you can to `this.cursors.get(id)` to get the cursor object)
  */
  let message = msg.content;
  if(message == '!ping') {
    bot.sendChat('Pong!');
  }
});

bot.list(); // list lobbies

bot.on('lobbies', (lobbies) => {
  lobbies.forEach((lobbyName) => {
    console.log('lobby: ', lobbyName);
  });
});
```
