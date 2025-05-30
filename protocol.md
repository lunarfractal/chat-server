# Protocol
I am writing this here so I don't forget how the networking code works
## WebSocket
Secure: `wss://[server-ip]:9091` (this is nginx proxying to ws://localhost:8081)<br>
Insecure (direct connection): `ws://[server-ip]:8081` (this is WebSocket++)
## Client -> Server messages
**0x00** (Ping): you can send this single byte packet once immediately after connecting. you MUST send it at least once after connecting (otherwise you can't enter the game)
<br>
**0x01** (hi): another packet that MUST be sent after connecting first byte is 0x01, next two bytes are u16 screen width, next two are u16 screen height
<br>
**0x02** (hi_bot): Single byte? I was planning on creating a bot protocol for this just like mpp, more on this later
<br>
**0x03** (enter_game): send this when you want to enter the game. 0x03 followed by u16 encoded nick
<br>
**0x04** (leave_game): send this single byte to leave the game
<br>
**0x05** (resize): similar to opcode_hi, just send it whenever you resize the screen
<br>
**0x06** (cursor): 0x06 followed by 4 bytes encoding your cursor's position
<br>
**0x07** (change room): 0x07 followed by u8 encoded name of the room you want to change into/create
<br>
**0x08** (ls) single byte, list all the rooms in the game
<br>
**0x09** (chat): 0x09 followed by u16 encoded message you want to send
<br>
**0x10** (ls_messages): get all the messages in your room

### Encoding types
u8: single byte unsigned int
<br>
u16: make sure this is always little endian
<br>
string: null terminated string
<br>
u16string: null terminated u16 string

## Server -> Client
**0x00** (Pong): you get this when you ping the server
<br>
**0xA0** (entered_game): 0xA0 followed by two bytes which are your u16 id
<br>
**0xA1** (events): 0xA1 followed by a byte which defines the type of event (ie chat message)<br>
if the byte is 0x1, it's a chat message followed by the id of the player who sent the message, the nick of the player, and then the content of the message
<br>
**0xB1** (history): just do this:<br>
enter an infinite loop
read the next 2 bytes, it's the author's id, read the next 8 bytes, it's the timestamp, read a string, it's the nick, read another string, it's the content of the message, stop if the offset reaches the end of the buffer.
<br>
**0xA4** (cycle_s): this is sent 30 times per second to update cursor's positions on screen. what you need to do is,<br>
start at offset 1 (after the header)<br>
enter an infinite loop<br>
read u16 id<br>
if the id is 0x00, break out of the loop<br>
read u8 flag<br>
<br>
if the flag is 0x0, it means the cursor just appeared on your screen<br>
the next 4 bytes are the cursor's x and y coordinates scaled to uint16<br>
next is null terminated u16 string which is the cursor's nick<br>
<br>
<br>
if the flag is 0x1, it means the cursor was already created, and you just need to update its position<br>
next two bytes are x and y scaled into a uint16<br>
<br>
<br>
if the flag is 0x2, it means the cursor left your screen.
<br>
<br>

And that's basically cycle_s.
<br>

