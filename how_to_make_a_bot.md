# Bot tutorial
here is the bot class:
```js
class Bot extends EventEmitter {
  constructor() {
    super();

    this.webSocket = null;
    
    this.id = 0;
    this.nick = "";
    this.hue = "";

    this.cursors = new Map();

    this.address = ""; // for now just replace this with the url on the website, I'll come up with an API for fetching this later
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
    this.hasConnection = true;
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
        let author = this.cursors.get(id);
        if (id == this.id) author = this;
        this.emit("chat-message", {
          content: message,
          author: author
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
          hue,
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
          this.emit('cursor-create', cursor);
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
          this.emit('cursor-update', cursor);
          break;
        }

        case 0x2: {
          // delete
          let cursor = this.cursors.get(id);
          if (cursor) {
            offset = cursor.deleteNetwork(view, offset);
            this.cursors.delete(id);
            this.emit('cursor-delete', cursor);
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
          this.emit('cursor-create', cursor);
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
        this.emit('pong', +new Date() - this.lastPing);
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
here is the Cursor class:

```js
class Cursor extends EventEmitter {
  constructor(maybeShow) {
    super();

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

      this.emit('nick', this.nick);
      this.emit'hue', this.hue);
      this.emit('create', {nick: this.nick, hue: this.hue, x, y});
    }

    this.x = x;
    this.y = y;

    this.emit('update', {x, y});

    return offset;
  }

  deleteNetwork(view, offset) {
    this.emit('delete');
    return offset;
  }
}
```

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
this is how you'd make a bot

```js
let bot = new Bot();

bot.on("open", () => {
  console.log('connected');
  bot.sendNick("test bot");
  bot.sendCursor(700, 400);
});

bot.on('cursor-create', (cursor) => {
  bot.sendChat(cursor.nick + " entered the game!");
});

bot.on('cursor-delete', (cursor) => {
  bot.sendChat(cursor.nick + " left the game.");
});

bot.on("chat-message", (msg) => {
  if (msg.content.startsWith("t!")) {
    let message = msg.content;
    let command = message.substring(2);

    if (command === "ping") {
      const listener = (latency) => {
        bot.sendChat("Pong! " + latency + "ms");
        bot.off("pong", listener);
      };

      bot.on("pong", listener);
      bot.ping();
    }
    else if(command === "help") {
      bot.sendChat(`Commands: help, ping, time, follow`);
    }
    else if (command === "time") {
      const now = new Date();
      const timeString = now.toLocaleTimeString();
      bot.sendChat("Current time: " + timeString);
    }
    else if(command.startsWith("follow")) {
      let id = parseInt(command.substring(7));
      let cursor = bot.cursors.get(id);
      if(cursor) {
        cursor.on('update', (newPos) => {
          bot.sendCursor(newPos.x, newPos.y);
        });
      }
      else {
        msg.author.on('update', (newPos) => {
          bot.sendCursor(newPos.x, newPos.y);
        });
      }
    }
  }
});
```
