'''Server List Ping''' ('''SLP''') is an interface provided by Minecraft servers which supports querying the MOTD, player count, max players and server version via the usual port. SLP is part of the [[Minecraft Wiki:Projects/wiki.vg merge/Protocol|protocol]], but it can be disabled. The Notchian client uses this interface to display the multiplayer server list, hence the name. The SLP process changed in [[Java Edition 1.7]] in a non-backwards compatible way, but modern clients and servers still support both the [[#Current|new]] and [[#1.6|old]] process.

== Current (1.7+) ==

This uses the regular client-server protocol. For the general packet format, see the [[Java Edition protocol/Packets|packets]] article.

=== Handshake ===

First, the client sends a [[Minecraft Wiki:Projects/wiki.vg merge/Protocol#Handshake|Handshake]] packet with its state set to 1.

{| class="wikitable"
 ! Packet ID
 ! Field Name
 ! Field Type
 ! Notes
 |-
 |rowspan="4"| 0x00
 | Protocol Version
 | {{Type|VarInt}}
 | See [[Minecraft Wiki:Projects/wiki.vg merge/Protocol version numbers|protocol version numbers]]. The version that the client plans on using to connect to the server (which is not important for the ping). If the client is pinging to determine what version to use, by convention <code>-1</code> should be set.
{{Warning|Setting invalid (nonexistent) version as the protocol version <i>might</i> cause some servers to close connection after this packet}}<br/>
{{Warning|See [[Minecraft Wiki:Projects/wiki.vg merge/Protocol version numbers|Protocol version numbers]] for a list of valid protocol versions.}}
 |-
 | Server Address
 | {{Type|String}}
 | Hostname or IP, e.g. localhost or 127.0.0.1, that was used to connect. The Notchian server does not use this information. Note that SRV records are a complete redirect, e.g. if _minecraft._tcp.example.com points to mc.example.org, users connecting to example.com will provide mc.example.org as server address in addition to connecting to it.
 |-
 | Server Port
 | {{Type|Unsigned Short}}
 | Default is 25565. The Notchian server does not use this information.
 |-
 | Next state
 | {{Type|VarInt}}
 | 1 for [[Minecraft Wiki:Projects/wiki.vg merge/Protocol#Status|status]], 2 for [[Minecraft Wiki:Projects/wiki.vg merge/Protocol#Login|login]], 3 for [[Minecraft Wiki:Projects/wiki.vg merge/Protocol#Login|transfer]]
 |}

=== Status Request ===

The client follows up with a [[Minecraft Wiki:Projects/wiki.vg merge/Protocol#Status Request|Status Request]] packet. This packet has no fields.
The client is also able to skip this part entirely and send a [[Minecraft Wiki:Projects/wiki.vg merge/Protocol#Ping Request|Ping Request]] instead.

{| class="wikitable"
 ! Packet ID
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | 0x00
 |colspan="3"| ''no fields''
 |}

=== Status Response ===

The server should respond with a [[Minecraft Wiki:Projects/wiki.vg merge/Protocol#Status Response|Status Response]] packet. Note that Notchian servers will for unknown reasons wait to receive the following [[Minecraft Wiki:Projects/wiki.vg merge/Protocol#Ping Request|Ping Request]] packet for 30 seconds before timing out and sending Response.

{| class="wikitable"
 ! Packet ID
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | 0x00
 | JSON Response
 | {{Type|String}}
 | See below; as with all strings this is prefixed by its length as a VarInt(3-byte max)
 |}

The JSON Response field is a [[wikipedia:JSON|JSON]] object which has the following format:

<syntaxhighlight lang="json">
{
    "version": {
        "name": "1.21.5",
        "protocol": 770
    },
    "players": {
        "max": 100,
        "online": 5,
        "sample": [
            {
                "name": "thinkofdeath",
                "id": "4566e69f-c907-48ee-8d71-d7ba5aa00d20"
            }
        ]
    },
    "description": {
        "text": "Hello, world!"
    },
    "favicon": "data:image/png;base64,<data>",
    "enforcesSecureChat": false
}
</syntaxhighlight>

The ''name'' field of the ''version'' object should be considered mandatory. On newer Notchian client versions (1.20+?), it may be omitted and will be treated as though it were the string "Old" if so.

The ''description'' field is optional. If specified, it should be a [[Minecraft Wiki:Projects/wiki.vg merge/Text formatting#Text components|text component]]. Note that the Notchian server has no way of providing actual text component data; instead section sign-based codes are embedded within the text of the object. However, third-party servers such as Spigot and Paper will return full components, so make sure you can handle both.

The ''favicon'' field is optional. If specified, it should be a [[wikipedia:Portable Network Graphics|PNG]] image that is [[wikipedia:Base64|Base64]] encoded (without newlines: <code>\n</code>, new lines no longer work since 1.13) and prepended with <code>data:image/png;base64,</code>. It should also be noted that the source image must be exactly 64x64 pixels, otherwise the Notchian client will not render the image.

The ''players'' field is optional. If omitted, "???" will be displayed in dark grey in place of the player count.

The numbers ''max'' and ''online'' in the players field have a maximum supported value of 2<sup>31</sup>-1 (2,147,483,647) by the vanilla Minecraft server. Larger values were observed to be set to the default value of 20.

If the ''sample'' field of the ''players'' object is present, the player names (but not their UUIDs) will be shown in order in the tooltip when hovering over the player count (or version name if incompatible). If empty, missing, or the wrong type, no tooltip will appear. Notchian servers will omit this field if there are no players on the server or if <code>hide-online-players</code> is false. There doesn't appear to be a hard limit on how many players may be specified.

If a client [[Minecraft Wiki:Projects/wiki.vg merge/Protocol#Client_Information_.28configuration.29|has "Allow Server Listings" set to "OFF"]], Notchian servers will report their name as "Anonymous Player" and UUID as "00000000-0000-0000-0000-000000000000". This is not reliable; making a request before the player has fully joined may cause their choice to not be respected.

On most Notchian client versions, UUIDs are mandatory and must be well formed even though it is unused by Notchian clients. Newer versions (1.20+?) will tolerate malformed or missing UUIDs and act as though ''sample'' was omitted if so.

If the server has No Chat Reports installed and hasn't disabled this feature, an additional field ''preventsChatReports'' will be added to the end with a value of true causing clients with No Chat Reports to display an icon indicating the server is a "Safe Server".

After receiving the Response packet, the client may send the next packet to help calculate the server's latency, or if it is only interested in the above information it can disconnect here.

If the client does not receive a properly formatted response (e.g. a field is missing or the wrong type), it will close the socket and instead attempt a [[Minecraft Wiki:Projects/wiki.vg merge/Server_List_Ping#1.6|legacy ping]].

=== Ping Request ===

If the process is continued, the client will now send a [[Minecraft Wiki:Projects/wiki.vg merge/Protocol#Ping Request|Ping Request]] packet containing some payload which is not important.

{| class="wikitable"
 ! Packet ID
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | 0x01
 | Payload
 | {{Type|Long}}
 | May be any number. Notchian clients use a system-dependent time value which is counted in milliseconds.
 |}

=== Pong Response ===

The server will respond with the [[Minecraft Wiki:Projects/wiki.vg merge/Protocol#Pong Response|Pong Response]] packet and then close the connection.

{| class="wikitable"
 ! Packet ID
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | 0x01
 | Payload
 | {{Type|Long}}
 | Should be the same as sent by the client
 |}

=== Ping via LAN (Open to LAN in Singleplayer) ===

In Singleplayer there is a function called "Open to LAN". Minecraft (in the serverlist) binds a UDP port and listens for connections to <code>224.0.2.60:4445</code> (Yes, that is the actual IP, no matter in what network you are or what your local IP Address is). If you click on "Open to LAN" Minecraft sends a packet every 1.5 seconds to the address: <code>[MOTD]{motd}[/MOTD][AD]{port}[/AD]</code>. Minecraft seems to check for the following Strings: <code>[MOTD]</code>, <code>[/MOTD]</code>, <code>[AD]</code>, <code>[/AD]</code>. Anything you write outside of each of the tags will be ignored. Color codes may be used in the MOTD. Between <code>[AD]</code> and <code>[/AD]</code> is the servers port. If it is not numeric, 25565 will be used. If it is out of range, an error is being displayed when trying to connect. The IP Address is the same as the senders one. The string is encoded with UTF-8.

To implement it server side, just send a packet with the text (payload) to <code>224.0.2.60:4445</code>. If you are client side, bind a UDP socket and listen for connections. You can use a <code>MulticastSocket</code> for that.

=== Examples ===

* [https://gist.github.com/csh/2480d14fbbb33b4bbae3 C#]
* [https://gist.github.com/zh32/7190955 Java]
* [https://gist.github.com/1209061 Python]
* [https://github.com/Duckulus/mc-honeypot Rust (Server)]
* [https://github.com/Sch8ill/MCClient-lib MCClient-lib (Python3)]
* [https://gist.github.com/ewized/97814f57ac85af7128bf Python3]
* [https://github.com/xPaw/PHP-Minecraft-Query PHP]
* [https://gitlab.bixilon.de/bixilon/minosoft/-/blob/43d8988ef94b6487e4da0218d87cf66ccf14a1ea/src/main/java/de/bixilon/minosoft/protocol/protocol/LANServerListener.java LAN Server Listener (Java)]
* [https://gitlab.bixilon.de/bixilon/minosoft/-/blob/master/src/main/java/de/bixilon/minosoft/protocol/protocol/LANServerListener.kt LAN Server Listener (Kotlin)]
* [https://github.com/stackotter/delta-client/blob/main/Sources/Core/Sources/Network/LANServerEnumerator.swift Swift]
* [https://github.com/dreamscached/minequery Go]
* [https://github.com/Sch8ill/mclib Go]
* [https://github.com/PauldeKoning/minecraft-server-handshake Node.js]
* [https://github.com/LhAlant/MinecraftSLP/ C]
* [https://github.com/mcstatus-io/mcutil/ Go]

== 1.6 ==

This uses a protocol which is compatible with the client-server protocol as it was before the Netty rewrite. Modern servers recognize this protocol by the starting byte of <code>fe</code> instead of the usual <code>00</code>.

=== Client to server ===

The client initiates a TCP connection to the server on the standard port. Instead of doing auth and logging in (as detailed in [[Minecraft Wiki:Projects/wiki.vg merge/Protocol|Protocol]] and [[Minecraft Wiki:Projects/wiki.vg merge/Protocol Encryption|Protocol Encryption]]), it sends the following data, expressed in hexadecimal:

# <code>FE</code> — packet identifier for a server list ping
# <code>01</code> — server list ping's payload (always 1)
# <code>FA</code> — packet identifier for a plugin message
# <code>00 0B</code> — length of following string, in characters, as a short (always 11)
# <code>00 4D 00 43 00 7C 00 50 00 69 00 6E 00 67 00 48 00 6F 00 73 00 74</code> — the string <code>MC|PingHost</code> encoded as a [http://en.wikipedia.org/wiki/UTF-16 UTF-16BE] string
# <code>XX XX</code> — length of the rest of the data, as a short. Compute as <code>7 + len(hostname)</code>, where <code>len(hostname)</code> is the number of bytes in the UTF-16BE encoded hostname.
# <code>XX</code> — [[Minecraft Wiki:Projects/wiki.vg merge/Protocol version numbers#Versions before the Netty rewrite|protocol version]], e.g. <code>4a</code> for the last version (74)
# <code>XX XX</code> — length of following string, in characters, as a short
# <code>...</code> — hostname the client is connecting to, encoded as a [http://en.wikipedia.org/wiki/UTF-16 UTF-16BE] string
# <code>XX XX XX XX</code> — port the client is connecting to, as an int.

All data types are big-endian.

Example packet dump:

 0000000: fe01 fa00 0b00 4d00 4300 7c00 5000 6900  ......M.C.|.P.i.
 0000010: 6e00 6700 4800 6f00 7300 7400 1949 0009  n.g.H.o.s.t..I..
 0000020: 006c 006f 0063 0061 006c 0068 006f 0073  .l.o.c.a.l.h.o.s
 0000030: 0074 0000 63dd                           .t..c.

'''Note: ''' All notchian servers only cares about the first 3 bytes. After reading <code>FE 01 FA</code>, the response will be sent to the client. For backward compatibility, you could only send these 3 bytes and all legacy servers(<=1.6) will respond correspondingly.

=== Server to client ===

The server responds with a 0xFF kick packet. The packet begins with a single byte identifier <code>ff</code>, then a two-byte big endian short giving the length of the following string in characters. You can actually ignore the length because the server closes the connection after the response is sent.

After the first 3 bytes, the packet is a UTF-16BE string. It begins with two characters: <code>§1</code>, followed by a null character. On the wire these look like <code>00 a7 00 31 00 00</code>.

The remainder is null character (that is <code>00 00</code>) delimited fields:

# Protocol version (e.g. <code>47</code>)
# Minecraft server version (e.g. <code>1.4.2</code>)
# Message of the day (e.g. <code>A Minecraft Server</code>)
# Current player count
# Max players

The entire packet looks something like this:

                 <---> first character
 0000000: ff00 2300 a700 3100 0000 3400 3700 0000  ....§.1...4.7...
 0000010: 3100 2e00 3400 2e00 3200 0000 4100 2000  1...4...2...A. .
 0000020: 4d00 6900 6e00 6500 6300 7200 6100 6600  M.i.n.e.c.r.a.f.
 0000030: 7400 2000 5300 6500 7200 7600 6500 7200  t. .S.e.r.v.e.r.
 0000040: 0000 3000 0000 3200 30                   ..0...2.0

'''Note: ''' When using this protocol with servers on version 1.7.x and above, the protocol version (first field) in the response will always be <code>127</code> which is not a real protocol number, so older clients will always consider this server incompatible.

=== Examples ===

* [https://gist.github.com/6281388 Ruby]
* [https://github.com/winny-/mcstat PHP]
* [https://github.com/Duckulus/mc-honeypot/blob/1514807e8af7f7cbfd13111fec334b9f4883b605/src/server/legacy.rs Rust (Server)]
* [https://github.com/dreamscached/minequery Go]
* [https://github.com/mcstatus-io/mcutil Go]

== 1.4 to 1.5 ==

Prior to the Minecraft 1.6, the client to server operation is much simpler, and only sends <code>FE 01</code>, with none of the following data.

=== Examples ===

* [https://github.com/Sch8ill/MCClient-lib Python3 (MCClient-lib)]
* [https://gist.github.com/5795159 PHP]
* [https://gist.github.com/4574114 Java]
* [https://gist.github.com/6223787 C#]
* [https://github.com/dreamscached/minequery Go]
* [https://github.com/mcstatus-io/mcutil Go]

== Beta 1.8 to 1.3 ==

Prior to Minecraft 1.4, the client only sends <code>FE</code>. 

The server should respond with a kick packet:
{| class="wikitable"
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | Packet Id
 | {{Type|Byte}}
 | The kick packet id: <code>0xFF</code>
 |-
 | Packet Length
 | {{Type|Short}}
 | The length of the following string in characters (NOT BYTES). Max is 256.
 |-
 | MOTD
 |rowspan="3"| A [http://en.wikipedia.org/wiki/UTF-16 UTF-16BE] encoded string
 | From here on out, all fields should be separated with <code>§</code>, in the same string.
 |-
 | Online Players
 | The count of players currently on the server.
 |-
 | Max Players
 | The maximum amount of players that the server is willing to accept.
 |}

The entire packet looks something like this on wire:

                 <---> first character
 0000000: ff00 1700 4100 2000 4d00 6900 6e00 6500  ....A. .M.i.n.e.
 0000010: 6300 7200 6100 6600 7400 2000 5300 6500  c.r.a.f.t. .S.e.
 0000020: 7200 7600 6500 7200 a700 3000 a700 3100  r.v.e.r.§.0.§.1.
 0000030: 30                                       0

[[Category:Protocol Details]]
[[Category:Java Edition protocol]]
{{license wiki.vg}}
