{{DISPLAYTITLE:''Java Edition'' protocol/Packets}}
{{about|the protocol for a stable release of {{JE}}|the protocol used in development versions of {{JE}}|Java Edition protocol/Development version|the protocol used in {{BE}}|Bedrock Edition protocol|the protocol used in old ''[[Pocket Edition]]'' versions|Pocket Edition protocol}}
{{See also|Java Edition protocol/FAQ|title1=Protocol FAQ}}
{{exclusive|java}}
{{Info|While you may use the contents of this page without restriction to create servers, clients, bots, etc; keep in mind that the contents of this page are distributed under the terms of [https://creativecommons.org/licenses/by-sa/3.0/ CC BY-SA 3.0 Unported]. Reproductions and derivative works must be distributed accordingly.}}

This article presents a dissection of the current {{JE}} '''protocol''' for [[Minecraft Wiki:Projects/wiki.vg merge/Protocol version numbers|1.21.5, protocol 770]].

The changes between versions may be viewed at [[Minecraft Wiki:Projects/wiki.vg merge/Protocol History|Protocol History]].

== Definitions ==

The Minecraft server accepts connections from TCP clients and communicates with them using ''packets''. A packet is a sequence of bytes sent over the TCP connection. The meaning of a packet depends both on its packet ID and the current state of the connection. The initial state of each connection is [[#Handshaking|Handshaking]], and state is switched using the packets [[#Handshake|Handshake]] and [[#Login Success|Login Success]].

=== Data types ===

{{:Protocol data types}} <!-- Transcluded contents of Data types article in here — go to that page if you want to edit it -->

=== Other definitions ===

{| class="wikitable"
 |-
 ! Term
 ! Definition
 |-
 | Player
 | When used in the singular, Player always refers to the client connected to the server.
 |-
 | Entity
 | Entity refers to any item, player, mob, minecart or boat etc. See [[Entity|the Minecraft Wiki article]] for a full list.
 |-
 | EID
 | An EID — or Entity ID — is a 4-byte sequence used to identify a specific entity. An entity's EID is unique on the entire server.
 |-
 | XYZ
 | In this document, the axis names are the same as those shown in the debug screen (F3). Y points upwards, X points east, and Z points south.
 |-
 | Meter
 | The meter is Minecraft's base unit of length, equal to the length of a vertex of a solid block. The term “block” may be used to mean “meter” or “cubic meter”.
 |-
 | Registry
 | A table describing static, gameplay-related objects of some kind, such as the types of entities, block states or biomes. The entries of a registry are typically associated with textual or numeric identifiers, or both.

Minecraft has a unified registry system used to implement most of the registries, including blocks, items, entities, biomes and dimensions. These "ordinary" registries associate entries with both namespaced textual identifiers (see [[#Identifier]]), and signed (positive) 32-bit numeric identifiers. There is also a registry of registries listing all of the registries in the registry system. Some other registries, most notably the [[Minecraft Wiki:Projects/wiki.vg merge/Chunk Format#Block state registry|block state registry]], are however implemented in a more ad-hoc fashion.

Some registries, such as biomes and dimensions, can be customized at runtime by the server (see [[Minecraft Wiki:Projects/wiki.vg merge/Registry Data|Registry Data]]), while others, such as blocks, items and entities, are hardcoded. The contents of the hardcoded registries can be extracted via the built-in [[Minecraft Wiki:Projects/wiki.vg merge/Data Generators|Data Generators]] system.
 |-
 | Block state
 | Each block in Minecraft has 0 or more properties, which in turn may have any number of possible values. These represent, for example, the orientations of blocks, poweredness states of redstone components, and so on. Each of the possible permutations of property values for a block is a distinct block state. The block state registry assigns a numeric identifier to every block state of every block.

A current list of properties and state ID ranges is found on [https://pokechu22.github.io/Burger/1.21.html burger].

Alternatively, the vanilla server now includes an option to export the current block state ID mapping, by running <code>java -DbundlerMainClass=net.minecraft.data.Main -jar minecraft_server.jar --reports</code>.  See [[Minecraft Wiki:Projects/wiki.vg merge/Data Generators|Data Generators]] for more information.
 |-
 | Vanilla
 | The official implementation of Minecraft as developed and released by Mojang.
 |-
 | Sequence
 | The action number counter for local block changes, incremented by one when clicking a block with a hand, right clicking an item, or starting or finishing digging a block. Counter handles latency to avoid applying outdated block changes to the local world.  Also is used to revert ghost blocks created when placing blocks, using buckets, or breaking blocks.
 |}

== Packet format ==

Packets cannot be larger than 2<sup>21</sup> &minus; 1 or 2097151 bytes (the maximum that can be sent in a 3-byte {{Type|VarInt}}). Moreover, the length field must not be longer than 3 bytes, even if the encoded value is within the limit. Unnecessarily long encodings at 3 bytes or below are still allowed.  For compressed packets, this applies to the Packet Length field, i.e. the compressed length.

=== Without compression ===

{| class="wikitable"
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | Length
 | {{Type|VarInt}}
 | Length of Packet ID + Data
 |-
 | Packet ID
 | {{Type|VarInt}}
 | Corresponds to <code>protocol_id</code> from [[Minecraft Wiki:Projects/wiki.vg merge/Data Generators#Packets report|the server's packet report]]
 |-
 | Data
 | {{Type|Byte Array}}
 | Depends on the connection state and packet ID, see the sections below
 |}

=== With compression ===

Once a [[#Set Compression|Set Compression]] packet (with a non-negative threshold) is sent, [[wikipedia:Zlib|zlib]] compression is enabled for all following packets. The format of a packet changes slightly to include the size of the uncompressed packet.

{| class=wikitable
 ! Present?
 ! Compressed?
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | always
 | No
 | Packet Length
 | {{Type|VarInt}}
 | Length of (Data Length) + length of compressed (Packet ID + Data)
 |-
 | rowspan="3"| if size >= threshold
 | No
 | Data Length
 | {{Type|VarInt}}
 | Length of uncompressed (Packet ID + Data)
 |-
 | rowspan="2"| Yes
 | Packet ID
 | {{Type|VarInt}}
 | zlib compressed packet ID (see the sections below)
 |-
 | Data
 | {{Type|Byte Array}}
 | zlib compressed packet data (see the sections below)
 |-
 | rowspan="3"| if size < threshold
 | rowspan="3"| No
 | Data Length
 | {{Type|VarInt}}
 | 0 to indicate uncompressed
 |-
 | Packet ID
 | {{Type|VarInt}}
 | packet ID (see the sections below)
 |-
 | Data
 | {{Type|Byte Array}}
 | packet data (see the sections below)
 |}

For serverbound packets, the uncompressed length of (Packet ID + Data) must not be greater than 2<sup>23</sup> or 8388608 bytes. Note that a length equal to 2<sup>23</sup> is permitted, which differs from the compressed length limit. The vanilla client, on the other hand, has no limit for the uncompressed length of incoming compressed packets.

If the size of the buffer containing the packet data and ID (as a {{Type|VarInt}}) is smaller than the threshold specified in the packet [[#Set Compression|Set Compression]]. It will be sent as uncompressed. This is done by setting the data length as 0. (Comparable to sending a non-compressed format with an extra 0 between the length, and packet data).

If it's larger than or equal to the threshold, then it follows the regular compressed protocol format.

The vanilla server (but not client) rejects compressed packets smaller than the threshold. Uncompressed packets exceeding the threshold, however, are accepted.

Compression can be disabled by sending the packet [[#Set Compression|Set Compression]] with a negative Threshold, or not sending the Set Compression packet at all.

== Handshaking ==

=== Clientbound ===

There are no clientbound packets in the Handshaking state, since the protocol immediately switches to a different state after the client sends the first packet.

=== Serverbound ===

==== Handshake ====

This packet causes the server to switch into the target state. It should be sent right after opening the TCP connection to prevent the server from disconnecting.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="4"| ''protocol:''<br/><code>0x00</code><br/><br/>''resource:''<br/><code>intention</code>
 | rowspan="4"| Handshaking
 | rowspan="4"| Server
 | Protocol Version
 | {{Type|VarInt}}
 | See [[Minecraft Wiki:Projects/wiki.vg merge/Protocol version numbers|protocol version numbers]] (currently 770 in Minecraft 1.21.5).
 |-
 | Server Address
 | {{Type|String}} (255)
 | Hostname or IP, e.g. localhost or 127.0.0.1, that was used to connect. The vanilla server does not use this information. Note that SRV records are a simple redirect, e.g. if _minecraft._tcp.example.com points to mc.example.org, users connecting to example.com will provide example.org as server address in addition to connecting to it.
 |-
 | Server Port
 | {{Type|Unsigned Short}}
 | Default is 25565. The vanilla server does not use this information.
 |-
 | Next State
 | {{Type|VarInt}} {{Type|Enum}}
 | 1 for [[#Status|Status]], 2 for [[#Login|Login]], 3 for [[#Login|Transfer]].
 |}

==== Legacy Server List Ping ====

{{Warning|This packet uses a nonstandard format. It is never length-prefixed, and the packet ID is an {{Type|Unsigned Byte}} instead of a {{Type|VarInt}}.}}

While not technically part of the current protocol, (legacy) clients may send this packet to initiate [[Minecraft Wiki:Projects/wiki.vg merge/Server List Ping|Server List Ping]], and modern servers should handle it correctly.
The format of this packet is a remnant of the pre-Netty age, before the switch to Netty in 1.7 brought the standard format that is recognized now. This packet merely exists to inform legacy clients that they can't join our modern server.

Modern clients (tested with 1.21.5 + 1.21.4) also send this packet when the server does not send any response within a 30 seconds time window or when the connection is immediately closed.
{{Warning|The client does not close the connection with the legacy packet on its own!
It only gets closed when the Minecraft client is closed.}}
{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | 0xFE
 | Handshaking
 | Server
 | Payload
 | {{Type|Unsigned Byte}}
 | always 1 (<code>0x01</code>).
 |}

See [[Minecraft Wiki:Projects/wiki.vg merge/Server List Ping#1.6|Server List Ping#1.6]] for the details of the protocol that follows this packet.
== Status ==
{{Main|Minecraft Wiki:Projects/wiki.vg merge/Server List Ping}}

=== Clientbound ===

==== Status Response ====

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | ''protocol:''<br/><code>0x00</code><br/><br/>''resource:''<br/><code>status_response</code>
 | Status
 | Client
 | JSON Response
 | {{Type|String}} (32767)
 | See [[Minecraft Wiki:Projects/wiki.vg merge/Server List Ping#Status Response|Server List Ping#Status Response]]; as with all strings this is prefixed by its length as a {{Type|VarInt}}.
 |}

==== Pong Response (status) ====

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | ''protocol:''<br/><code>0x01</code><br/><br/>''resource:''<br/><code>pong_response</code>
 | Status
 | Client
 | Timestamp
 | {{Type|Long}}
 | Should match the one sent by the client.
 |}

=== Serverbound ===

==== Status Request ====

The status can only be requested once immediately after the handshake, before any ping. The server won't respond otherwise.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | ''protocol:''<br/><code>0x00</code><br/><br/>''resource:''<br/><code>status_request</code>
 | Status
 | Server
 | colspan="3"| ''no fields''
 |}

==== Ping Request (status) ====

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | ''protocol:''<br/><code>0x01</code><br/><br/>''resource:''<br/><code>ping_request</code>
 | Status
 | Server
 | Timestamp
 | {{Type|Long}}
 | May be any number, but vanilla clients use will always use the timestamp in milliseconds.
 |}

== Login ==

The login process is as follows:

# C→S: [[#Handshake|Handshake]] with Next State set to 2 (login)
# C→S: [[#Login Start|Login Start]]
# S→C: [[#Encryption Request|Encryption Request]]
# Client auth (if enabled)
# C→S: [[#Encryption Response|Encryption Response]]
# Server auth (if enabled)
# Both enable encryption
# S→C: [[#Set Compression|Set Compression]] (optional)
# S→C: [[#Login Success|Login Success]]
# C→S: [[#Login Acknowledged|Login Acknowledged]]

Set Compression, if present, must be sent before Login Success. Note that anything sent after Set Compression must use the [[#With compression|Post Compression packet format]].

Three modes of operation are possible depending on how the packets are sent:
* Online-mode with encryption
* Offline-mode with encryption
* Offline-mode without encryption

For online-mode servers (the ones with authentication enabled), encryption is always mandatory, and the entire process described above needs to be followed.

For offline-mode servers (the ones with authentication disabled), encryption is optional, and part of the process can be skipped. In that case [[#Login Start|Login Start]] is directly followed by [[#Login Success|Login Success]]. The vanilla server only uses UUID v3 for offline player UUIDs, deriving it from the string <code>OfflinePlayer:<player's name></code> For example, Notch’s offline UUID would be chosen from the string <code>OfflinePlayer:Notch</code>. This is not a requirement however, the UUID can be set to anything.

As of 1.21, the vanilla server never uses encryption in offline mode.

See [[protocol encryption]] for details.

=== Clientbound ===

==== Disconnect (login) ====

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | ''protocol:''<br/><code>0x00</code><br/><br/>''resource:''<br/><code>login_disconnect</code>
 | Login
 | Client
 | Reason
 | {{Type|JSON Text Component}}
 | The reason why the player was disconnected.
 |}

==== Encryption Request ====

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="4"| ''protocol:''<br/><code>0x01</code><br/><br/>''resource:''<br/><code>hello</code>
 | rowspan="4"| Login
 | rowspan="4"| Client
 | Server ID
 | {{Type|String}} (20)
 | Always empty when sent by the vanilla server.
 |-
 | Public Key
 | {{Type|Prefixed Array}} of {{Type|Byte}}
 | The server's public key, in bytes.
 |-
 | Verify Token
 | {{Type|Prefixed Array}} of {{Type|Byte}}
 | A sequence of random bytes generated by the server.
 |-
 | Should authenticate
 | {{Type|Boolean}}
 | Whether the client should attempt to [[Minecraft Wiki:Projects/wiki.vg merge/Protocol_Encryption#Authentication|authenticate through mojang servers]].
 |}

See [[protocol encryption]] for details.

==== Login Success ====

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! colspan="2"| Field Name
 ! colspan="2"| Field Type
 ! Notes
 |-
 | rowspan="5"| ''protocol:''<br/><code>0x02</code><br/><br/>''resource:''<br/><code>login_finished</code>
 | rowspan="5"| Login
 | rowspan="5"| Client
 | colspan="2"| UUID
 | colspan="2"| {{Type|UUID}}
 | colspan="2"|
 |-
 | colspan="2"| Username
 | colspan="2"| {{Type|String}} (16)
 | colspan="2"|
 |-
 | rowspan="3"| Property
 | Name
 | rowspan="3"| {{Type|Prefixed Array}} (16)
 | {{Type|String}} (64)
 | colspan="2"|
 |-
 | Value
 | {{Type|String}} (32767)
 | colspan="1"|
 |-
 | Signature
 | {{Type|Prefixed Optional}} {{Type|String}} (1024)
 |
 |}

The Property field looks like response of [[Minecraft Wiki:Projects/wiki.vg merge/Mojang API#UUID to Profile and Skin/Cape|Mojang API#UUID to Profile and Skin/Cape]], except using the protocol format instead of JSON. That is, each player will usually have one property with Name being “textures” and Value being a base64-encoded JSON string, as documented at [[Minecraft Wiki:Projects/wiki.vg merge/Mojang API#UUID to Profile and Skin/Cape|Mojang API#UUID to Profile and Skin/Cape]]. An empty properties array is also acceptable, and will cause clients to display the player with one of the two default skins depending their UUID (again, see the Mojang API page).

==== Set Compression ====

Enables compression.  If compression is enabled, all following packets are encoded in the [[#With compression|compressed packet format]].  Negative values will disable compression, meaning the packet format should remain in the [[#Without compression|uncompressed packet format]].  However, this packet is entirely optional, and if not sent, compression will also not be enabled (the vanilla server does not send the packet when compression is disabled).

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | ''protocol:''<br/><code>0x03</code><br/><br/>''resource:''<br/><code>login_compression</code>
 | Login
 | Client
 | Threshold
 | {{Type|VarInt}}
 | Maximum size of a packet before it is compressed.
 |}

==== Login Plugin Request ====

Used to implement a custom handshaking flow together with [[#Login Plugin Response|Login Plugin Response]].

Unlike plugin messages in "play" mode, these messages follow a lock-step request/response scheme, where the client is expected to respond to a request indicating whether it understood. The vanilla client always responds that it hasn't understood, and sends an empty payload.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="3"| ''protocol:''<br/><code>0x04</code><br/><br/>''resource:''<br/><code>custom_query</code>
 | rowspan="3"| Login
 | rowspan="3"| Client
 | Message ID
 | {{Type|VarInt}}
 | Generated by the server - should be unique to the connection.
 |-
 | Channel
 | {{Type|Identifier}}
 | Name of the [[Minecraft Wiki:Projects/wiki.vg merge/Plugin channels|plugin channel]] used to send the data.
 |-
 | Data
 | {{Type|Byte Array}} (1048576)
 | Any data, depending on the channel. The length of this array must be inferred from the packet length.
 |}

==== Cookie Request (login) ====

Requests a cookie that was previously stored.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! colspan="2"| Field Name
 ! colspan="2"| Field Type
 ! Notes
 |-
 | rowspan="1"| ''protocol:''<br/><code>0x05</code><br/><br/>''resource:''<br/><code>cookie_request</code>
 | rowspan="1"| Login
 | rowspan="1"| Client
 | colspan="2"| Key
 | colspan="2"| {{Type|Identifier}}
 | The identifier of the cookie.
 |}

=== Serverbound ===

==== Login Start ====

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="2"| ''protocol:''<br/><code>0x00</code><br/><br/>''resource:''<br/><code>hello</code>
 | rowspan="2"| Login
 | rowspan="2"| Server
 | Name
 | {{Type|String}} (16)
 | Player's Username.
 |-
 | Player UUID
 | {{Type|UUID}}
 | The {{Type|UUID}} of the player logging in. Unused by the vanilla server.
 |}

==== Encryption Response ====

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="2"| ''protocol:''<br/><code>0x01</code><br/><br/>''resource:''<br/><code>key</code>
 | rowspan="2"| Login
 | rowspan="2"| Server
 | Shared Secret
 | {{Type|Prefixed Array}} of {{Type|Byte}}
 | Shared Secret value, encrypted with the server's public key.
 |-
 | Verify Token
 | {{Type|Prefixed Array}} of {{Type|Byte}}
 | Verify Token value, encrypted with the same public key as the shared secret.
 |}

See [[protocol encryption]] for details.

==== Login Plugin Response ====

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="2"| ''protocol:''<br/><code>0x02</code><br/><br/>''resource:''<br/><code>custom_query_answer</code>
 | rowspan="2"| Login
 | rowspan="2"| Server
 | Message ID
 | {{Type|VarInt}}
 | Should match ID from server.
 |-
 | Data
 | {{Type|Prefixed Optional}} {{Type|Byte Array}} (1048576)
 | Any data, depending on the channel. The length of this array must be inferred from the packet length. Only present if the client understood the request.
 |}

==== Login Acknowledged ====

Acknowledgement to the [[Java Edition protocol#Login_Success|Login Success]] packet sent by the server.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | ''protocol:''<br/><code>0x03</code><br/><br/>''resource:''<br/><code>login_acknowledged</code>
 | Login
 | Server
 | colspan="3"| ''no fields''
 |}

This packet switches the connection state to [[#Configuration|configuration]].

==== Cookie Response (login) ====

Response to a [[#Cookie_Request_(login)|Cookie Request (login)]] from the server. The vanilla server only accepts responses of up to 5 kiB in size.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="2"| ''protocol:''<br/><code>0x04</code><br/><br/>''resource:''<br/><code>cookie_response</code>
 | rowspan="2"| Login
 | rowspan="2"| Server
 | Key
 | {{Type|Identifier}}
 | The identifier of the cookie.
 |-
 | Payload
 | {{Type|Prefixed Optional}} {{Type|Prefixed Array}} (5120) of {{Type|Byte}}
 | The data of the cookie.
 |}

== Configuration ==

=== Clientbound ===

==== Cookie Request (configuration) ====

Requests a cookie that was previously stored.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! colspan="2"| Field Name
 ! colspan="2"| Field Type
 ! Notes
 |-
 | rowspan="1"| ''protocol:''<br/><code>0x00</code><br/><br/>''resource:''<br/><code>cookie_request</code>
 | rowspan="1"| Configuration
 | rowspan="1"| Client
 | colspan="2"| Key
 | colspan="2"| {{Type|Identifier}}
 | The identifier of the cookie.
 |}

==== Clientbound Plugin Message (configuration) ====

{{Main|Minecraft Wiki:Projects/wiki.vg merge/Plugin channels}}

Mods and plugins can use this to send their data. Minecraft itself uses several [[Minecraft Wiki:Projects/wiki.vg merge/Plugin channels|plugin channels]]. These internal channels are in the <code>minecraft</code> namespace.

More information on how it works on [https://web.archive.org/web/20220831140929/https://dinnerbone.com/blog/2012/01/13/minecraft-plugin-channels-messaging/ Dinnerbone's blog]. More documentation about internal and popular registered channels are [[Minecraft Wiki:Projects/wiki.vg merge/Plugin channels|here]].

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="2"| ''protocol:''<br/><code>0x01</code><br/><br/>''resource:''<br/><code>custom_payload</code>
 | rowspan="2"| Configuration
 | rowspan="2"| Client
 | Channel
 | {{Type|Identifier}}
 | Name of the [[Minecraft Wiki:Projects/wiki.vg merge/Plugin channels|plugin channel]] used to send the data.
 |-
 | Data
 | {{Type|Byte Array}} (1048576)
 | Any data. The length of this array must be inferred from the packet length.
 |}

In vanilla clients, the maximum data length is 1048576 bytes.

==== Disconnect (configuration) ====

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | ''protocol:''<br/><code>0x02</code><br/><br/>''resource:''<br/><code>disconnect</code>
 | Configuration 
 | Client
 | Reason
 | {{Type|Text Component}}
 | The reason why the player was disconnected.
 |}

==== Finish Configuration ====

Sent by the server to notify the client that the configuration process has finished. The client answers with [[#Acknowledge_Finish_Configuration|Acknowledge Finish Configuration]] whenever it is ready to continue.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="1"| ''protocol:''<br/><code>0x03</code><br/><br/>''resource:''<br/><code>finish_configuration</code>
 | rowspan="1"| Configuration
 | rowspan="1"| Client
 | colspan="3"| ''no fields''
 |}

This packet switches the connection state to [[#Play|play]].

==== Clientbound Keep Alive (configuration) ====

The server will frequently send out a keep-alive, each containing a random ID. The client must respond with the same payload (see [[#Serverbound Keep Alive (configuration)|Serverbound Keep Alive]]). If the client does not respond to a Keep Alive packet within 15 seconds after it was sent, the server kicks the client. Vice versa, if the server does not send any keep-alives for 20 seconds, the client will disconnect and yields a "Timed out" exception.

The vanilla server uses a system-dependent time in milliseconds to generate the keep alive ID value.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | ''protocol:''<br/><code>0x04</code><br/><br/>''resource:''<br/><code>keep_alive</code>
 | Configuration
 | Client
 | Keep Alive ID
 | {{Type|Long}}
 |
 |}

==== Ping (configuration) ====

Packet is not used by the vanilla server. When sent to the client, client responds with a [[#Pong (configuration)|Pong]] packet with the same id.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | ''protocol:''<br/><code>0x05</code><br/><br/>''resource:''<br/><code>ping</code>
 | Configuration
 | Client
 | ID
 | {{Type|Int}}
 |
 |}

==== Reset Chat ====

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | ''protocol:''<br/><code>0x06</code><br/><br/>''resource:''<br/><code>reset_chat</code>
 | Configuration
 | Client
 | colspan="3"| ''no fields''
 |}

==== Registry Data ====

Represents certain registries that are sent from the server and are applied on the client.

See [[Minecraft Wiki:Projects/wiki.vg merge/Registry_Data|Registry Data]] for details.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! colspan="2"| Field Name
 ! colspan="2"| Field Type
 ! Notes
 |-
 | rowspan="3"| ''protocol:''<br/><code>0x07</code><br/><br/>''resource:''<br/><code>registry_data</code>
 | rowspan="3"| Configuration
 | rowspan="3"| Client
 | colspan="2"| Registry ID
 | colspan="2"| {{Type|Identifier}}
 | 
 |-
 | rowspan="2"| Entries
 | Entry ID
 | rowspan="2"| {{Type|Prefixed Array}}
 | {{Type|Identifier}}
 |
 |-
 | Data
 | {{Type|Prefixed Optional}} {{Type|NBT}}
 | Entry data.
 |}

==== Remove Resource Pack (configuration) ====

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="1"| ''protocol:''<br/><code>0x08</code><br/><br/>''resource:''<br/><code>resource_pack_pop</code>
 | rowspan="1"| Configuration
 | rowspan="1"| Client
 | UUID
 | {{Type|Prefixed Optional}} {{Type|UUID}}
 | The {{Type|UUID}} of the resource pack to be removed. If not present every resource pack will be removed.
 |}

==== Add Resource Pack (configuration) ====

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="5"| ''protocol:''<br/><code>0x09</code><br/><br/>''resource:''<br/><code>resource_pack_push</code>
 | rowspan="5"| Configuration
 | rowspan="5"| Client
 | UUID
 | {{Type|UUID}}
 | The unique identifier of the resource pack.
 |-
 | URL
 | {{Type|String}} (32767)
 | The URL to the resource pack.
 |-
 | Hash
 | {{Type|String}} (40)
 | A 40 character hexadecimal, case-insensitive [[wikipedia:SHA-1|SHA-1]] hash of the resource pack file.<br />If it's not a 40 character hexadecimal string, the client will not use it for hash verification and likely waste bandwidth.
 |-
 | Forced
 | {{Type|Boolean}}
 | The vanilla client will be forced to use the resource pack from the server. If they decline they will be kicked from the server.
 |-
 | Prompt Message
 | {{Type|Prefixed Optional}} {{Type|Text Component}}
 | This is shown in the prompt making the client accept or decline the resource pack (only if present).
 |}

==== Store Cookie (configuration) ====

Stores some arbitrary data on the client, which persists between server transfers. The vanilla client only accepts cookies of up to 5 kiB in size.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! colspan="2"| Field Name
 ! colspan="2"| Field Type
 ! Notes
 |-
 | rowspan="2"| ''protocol:''<br/><code>0x0A</code><br/><br/>''resource:''<br/><code>store_cookie</code>
 | rowspan="2"| Configuration
 | rowspan="2"| Client
 | colspan="2"| Key
 | colspan="2"| {{Type|Identifier}}
 | The identifier of the cookie.
 |-
 | colspan="2"| Payload
 | colspan="2"| {{Type|Prefixed Array}} (5120) of {{Type|Byte}}
 | The data of the cookie.
 |}

==== Transfer (configuration) ====

Notifies the client that it should transfer to the given server. Cookies previously stored are preserved between server transfers.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! colspan="2"| Field Name
 ! colspan="2"| Field Type
 ! Notes
 |-
 | rowspan="2"| ''protocol:''<br/><code>0x0B</code><br/><br/>''resource:''<br/><code>transfer</code>
 | rowspan="2"| Configuration
 | rowspan="2"| Client
 | colspan="2"| Host
 | colspan="2"| {{Type|String}} (32767)
 | The hostname or IP of the server.
 |-
 | colspan="2"| Port
 | colspan="2"| {{Type|VarInt}}
 | The port of the server.
 |}

==== Feature Flags ====

Used to enable and disable features, generally experimental ones, on the client.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="1"| ''protocol:''<br/><code>0x0C</code><br/><br/>''resource:''<br/><code>update_enabled_features</code>
 | rowspan="1"| Configuration
 | rowspan="1"| Client
 | Feature Flags
 | {{Type|Prefixed Array}} of {{Type|Identifier}}
 |
 |}

There is one special feature flag, which is in most versions:
* minecraft:vanilla - enables vanilla features

For the other feature flags, which may change between versions, see [[Experiments#Java_Edition]].

==== Update Tags (configuration) ====

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! colspan="2"| Field Name
 ! colspan="2"| Field Type
 ! Notes
 |-
 | rowspan="2"| ''protocol:''<br/><code>0x0D</code><br/><br/>''resource:''<br/><code>update_tags</code>
 | rowspan="2"| Configuration
 | rowspan="2"| Client
 | rowspan="2"| Array of tags
 | Registry
 | rowspan="2"| {{Type|Prefixed Array}}
 | {{Type|Identifier}}
 | Registry identifier (Vanilla expects tags for the registries <code>minecraft:block</code>, <code>minecraft:item</code>, <code>minecraft:fluid</code>, <code>minecraft:entity_type</code>, and <code>minecraft:game_event</code>)
 |-
 | Array of Tag
 | (See below)
 |
 |}

Tag arrays look like:

{| class="wikitable"
 ! colspan="2"| Field Name
 ! colspan="2"| Field Type
 ! Notes
 |-
 | rowspan="2"| Tags
 | Tag name
 | rowspan="2"| {{Type|Prefixed Array}}
 | {{Type|Identifier}}
 |
 |-
 | Entries
 | {{Type|Prefixed Array}} of {{Type|VarInt}}
 | Numeric IDs of the given type (block, item, etc.). This list replaces the previous list of IDs for the given tag. If some preexisting tags are left unmentioned, a warning is printed.
 |}

See [[Tag]] on the Minecraft Wiki for more information, including a list of vanilla tags.

==== Clientbound Known Packs ====

Informs the client of which data packs are present on the server.
The client is expected to respond with its own [[#Serverbound_Known_Packs|Serverbound Known Packs]] packet.
The vanilla server does not continue with Configuration until it receives a response.

The vanilla client requires the <code>minecraft:core</code> pack with version <code>1.21.5</code> for a normal login sequence. This packet must be sent before the Registry Data packets.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! colspan="2"| Field Name
 ! colspan="2"| Field Type
 ! Notes
 |-
 | rowspan="3"| ''protocol:''<br/><code>0x0E</code><br/><br/>''resource:''<br/><code>select_known_packs</code>
 | rowspan="3"| Configuration
 | rowspan="3"| Client
 | rowspan="3"| Known Packs
 | Namespace
 | rowspan="3"| {{Type|Prefixed Array}}
 | {{Type|String}}
 |
 |-
 | ID
 | {{Type|String}}
 |
 |-
 | Version
 | {{Type|String}}
 |
 |}

==== Custom Report Details (configuration) ====

Contains a list of key-value text entries that are included in any crash or disconnection report generated during connection to the server.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! colspan="2"| Field Name
 ! colspan="2"| Field Type
 ! Notes
 |-
 | rowspan="2"| ''protocol:''<br/><code>0x0F</code><br/><br/>''resource:''<br/><code>custom_report_details</code>
 | rowspan="2"| Configuration
 | rowspan="2"| Client
 | rowspan="2"| Details
 | Title
 | rowspan="2"| {{Type|Prefixed Array}} (32)
 | {{Type|String}} (128)
 |
 |-
 | Description
 | {{Type|String}} (4096)
 |
|}

==== Server Links (configuration) ====

This packet contains a list of links that the vanilla client will display in the menu available from the pause menu. Link labels can be built-in or custom (i.e., any text).

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! colspan="2"| Field Name
 ! colspan="2"| Field Type
 ! Notes
 |-
 | rowspan="3"| ''protocol:''<br/><code>0x10</code><br/><br/>''resource:''<br/><code>server_links</code>
 | rowspan="3"| Configuration
 | rowspan="3"| Client
 | rowspan="3"| Links
 | Is built-in
 | rowspan="3"| {{Type|Prefixed Array}}
 | {{Type|Boolean}}
 | True if Label is an enum (built-in label), false if it's a text component (custom label).
 |-
 | Label
 | {{Type|VarInt}} {{Type|Enum}} / {{Type|Text Component}}
 | See below.
 |-
 | URL
 | {{Type|String}}
 | Valid URL.
|}


{| class="wikitable"
 ! ID
 ! Name
 ! Notes
 |-
 | 0
 | Bug Report
 | Displayed on connection error screen; included as a comment in the disconnection report.
 |-
 | 1
 | Community Guidelines
 | 
 |-
 | 2
 | Support
 | 
 |-
 | 3
 | Status
 | 
 |-
 | 4
 | Feedback
 | 
 |-
 | 5
 | Community
 | 
 |-
 | 6
 | Website
 | 
 |-
 | 7
 | Forums
 | 
 |-
 | 8
 | News
 | 
 |-
 | 9
 | Announcements
 | 
 |-
 |}

=== Serverbound ===

==== Client Information (configuration) ====

Sent when the player connects, or when settings are changed.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="9"| ''protocol:''<br/><code>0x00</code><br/><br/>''resource:''<br/><code>client_information</code>
 | rowspan="9"| Configuration
 | rowspan="9"| Server
 | Locale
 | {{Type|String}} (16)
 | e.g. <code>en_GB</code>.
 |-
 | View Distance
 | {{Type|Byte}}
 | Client-side render distance, in chunks.
 |-
 | Chat Mode
 | {{Type|VarInt}} {{Type|Enum}}
 | 0: enabled, 1: commands only, 2: hidden.  See [[Minecraft Wiki:Projects/wiki.vg merge/Chat#Client chat mode|Chat#Client chat mode]] for more information.
 |-
 | Chat Colors
 | {{Type|Boolean}}
 | “Colors” multiplayer setting. The vanilla server stores this value but does nothing with it (see [https://bugs.mojang.com/browse/MC-64867 MC-64867]). Third-party servers such as Hypixel disable all coloring in chat and system messages when it is false.
 |-
 | Displayed Skin Parts
 | {{Type|Unsigned Byte}}
 | Bit mask, see below.
 |-
 | Main Hand
 | {{Type|VarInt}} {{Type|Enum}}
 | 0: Left, 1: Right.
 |-
 | Enable text filtering
 | {{Type|Boolean}}
 | Enables filtering of text on signs and written book titles. The vanilla client sets this according to the <code>profanityFilterPreferences.profanityFilterOn</code> account attribute indicated by the [[Minecraft Wiki:Projects/wiki.vg merge/Mojang API#Player Attributes|<code>/player/attributes</code> Mojang API endpoint]]. In offline mode it is always false.
 |-
 | Allow server listings
 | {{Type|Boolean}}
 | Servers usually list online players, this option should let you not show up in that list.
 |-
 | Particle Status
 | {{Type|VarInt}} {{Type|Enum}}
 | 0: all, 1: decreased, 2: minimal
 |}

''Displayed Skin Parts'' flags:

* Bit 0 (0x01): Cape enabled
* Bit 1 (0x02): Jacket enabled
* Bit 2 (0x04): Left Sleeve enabled
* Bit 3 (0x08): Right Sleeve enabled
* Bit 4 (0x10): Left Pants Leg enabled
* Bit 5 (0x20): Right Pants Leg enabled
* Bit 6 (0x40): Hat enabled

The most significant bit (bit 7, 0x80) appears to be unused.

==== Cookie Response (configuration) ====

Response to a [[#Cookie_Request_(configuration)|Cookie Request (configuration)]] from the server. The vanilla server only accepts responses of up to 5 kiB in size.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="2"| ''protocol:''<br/><code>0x01</code><br/><br/>''resource:''<br/><code>cookie_response</code>
 | rowspan="2"| Configuration
 | rowspan="2"| Server
 | Key
 | {{Type|Identifier}}
 | The identifier of the cookie.
 |-
 | Payload
 | {{Type|Prefixed Optional}} {{Type|Prefixed Array}} (5120) of {{Type|Byte}}
 | The data of the cookie.
 |}

==== Serverbound Plugin Message (configuration) ====

{{Main|Minecraft Wiki:Projects/wiki.vg merge/Plugin channels}}

Mods and plugins can use this to send their data. Minecraft itself uses some [[Minecraft Wiki:Projects/wiki.vg merge/Plugin channels|plugin channels]]. These internal channels are in the <code>minecraft</code> namespace.

More documentation on this: [https://dinnerbone.com/blog/2012/01/13/minecraft-plugin-channels-messaging/ https://dinnerbone.com/blog/2012/01/13/minecraft-plugin-channels-messaging/]

Note that the length of Data is known only from the packet length, since the packet has no length field of any kind.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="2"| ''protocol:''<br/><code>0x02</code><br/><br/>''resource:''<br/><code>custom_payload</code>
 | rowspan="2"| Configuration
 | rowspan="2"| Server
 | Channel
 | {{Type|Identifier}}
 | Name of the [[Minecraft Wiki:Projects/wiki.vg merge/Plugin channels|plugin channel]] used to send the data.
 |-
 | Data
 | {{Type|Byte Array}} (32767)
 | Any data, depending on the channel. <code>minecraft:</code> channels are documented [[Minecraft Wiki:Projects/wiki.vg merge/Plugin channels|here]]. The length of this array must be inferred from the packet length.
 |}

In vanilla server, the maximum data length is 32767 bytes.

==== Acknowledge Finish Configuration ====

Sent by the client to notify the server that the configuration process has finished. It is sent in response to the server's [[#Finish_Configuration|Finish Configuration]].

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="1"| ''protocol:''<br/><code>0x03</code><br/><br/>''resource:''<br/><code>finish_configuration</code>
 | rowspan="1"| Configuration
 | rowspan="1"| Server
 | colspan="3"| ''no fields''
 |}

This packet switches the connection state to [[#Play|play]].

==== Serverbound Keep Alive (configuration) ====

The server will frequently send out a keep-alive (see [[#Clientbound Keep Alive (configuration)|Clientbound Keep Alive]]), each containing a random ID. The client must respond with the same packet.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | ''protocol:''<br/><code>0x04</code><br/><br/>''resource:''<br/><code>keep_alive</code>
 | Configuration
 | Server
 | Keep Alive ID
 | {{Type|Long}}
 |
 |}

==== Pong (configuration) ====

Response to the clientbound packet ([[#Ping (configuration)|Ping]]) with the same id.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | ''protocol:''<br/><code>0x05</code><br/><br/>''resource:''<br/><code>pong</code>
 | Configuration
 | Server
 | ID
 | {{Type|Int}}
 |
 |}

==== Resource Pack Response (configuration) ====

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="2" | ''protocol:''<br/><code>0x06</code><br/><br/>''resource:''<br/><code>resource_pack</code>
 | rowspan="2" | Configuration
 | rowspan="2" | Server
 | UUID
 | {{Type|UUID}}
 | The unique identifier of the resource pack received in the [[#Add_Resource_Pack_(configuration)|Add Resource Pack (configuration)]] request.
 |-
 | Result
 | {{Type|VarInt}} {{Type|Enum}}
 | Result ID (see below).
 |}

Result can be one of the following values:

{| class="wikitable"
 ! ID
 ! Result
 |-
 | 0
 | Successfully downloaded
 |-
 | 1
 | Declined
 |-
 | 2
 | Failed to download
 |-
 | 3
 | Accepted
 |-
 | 4
 | Downloaded
 |-
 | 5
 | Invalid URL
 |-
 | 6
 | Failed to reload
 |-
 | 7
 | Discarded
 |}

==== Serverbound Known Packs ====

Informs the server of which data packs are present on the client. The client sends this in response to [[#Clientbound_Known_Packs|Clientbound Known Packs]].

If the client specifies a pack in this packet, the server should omit its contained data from the [[#Registry_Data_2|Registry Data]] packet.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! colspan="2"| Field Name
 ! colspan="2"| Field Type
 ! Notes
 |-
 | rowspan="3"| ''protocol:''<br/><code>0x07</code><br/><br/>''resource:''<br/><code>select_known_packs</code>
 | rowspan="3"| Configuration
 | rowspan="3"| Server
 | rowspan="3"| Known Packs
 | Namespace
 | rowspan="3"| {{Type|Prefixed Array}}
 | {{Type|String}}
 |
 |-
 | ID
 | {{Type|String}}
 |
 |-
 | Version
 | {{Type|String}}
 |
 |}

== Play ==

=== Clientbound ===

==== Bundle Delimiter ====

The delimiter for a bundle of packets. When received, the client should store every subsequent packet it receives, and wait until another delimiter is received. Once that happens, the client is guaranteed to process every packet in the bundle on the same tick, and the client should stop storing packets.

As of 1.20.6, the vanilla server only uses this to ensure [[#Spawn_Entity|Spawn Entity]] and associated packets used to configure the entity happen on the same tick. Each entity gets a separate bundle.

The vanilla client doesn't allow more than 4096 packets in the same bundle.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | ''protocol:''<br/><code>0x00</code><br/><br/>''resource:''<br/><code>bundle_delimiter</code>
 | Play
 | Client
 | colspan="3"| ''no fields''
 |}

==== Spawn Entity ====

Sent by the server when an entity (aside from [[#Spawn_Experience_Orb|Experience Orb]]) is created.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="13"| ''protocol:''<br/><code>0x01</code><br/><br/>''resource:''<br/><code>add_entity</code>
 | rowspan="13"| Play
 | rowspan="13"| Client
 | Entity ID
 | {{Type|VarInt}}
 | A unique integer ID mostly used in the protocol to identify the entity.
 |-
 | Entity UUID
 | {{Type|UUID}}
 | A unique identifier that is mostly used in persistence and places where the uniqueness matters more.
 |-
 | Type
 | {{Type|VarInt}}
 | ID in the <code>minecraft:entity_type</code> registry (see "type" field in [[Java Edition protocol/Entity metadata#Entities|Entity metadata#Entities]]).
 |-
 | X
 | {{Type|Double}}
 |
 |-
 | Y
 | {{Type|Double}}
 |
 |-
 | Z
 | {{Type|Double}}
 |
 |-
 | Pitch
 | {{Type|Angle}}
 | To get the real pitch, you must divide this by (256.0F / 360.0F)
 |-
 | Yaw
 | {{Type|Angle}}
 | To get the real yaw, you must divide this by (256.0F / 360.0F)
 |-
 | Head Yaw
 | {{Type|Angle}}
 | Only used by living entities, where the head of the entity may differ from the general body rotation.
 |-
 | Data
 | {{Type|VarInt}}
 | Meaning dependent on the value of the Type field, see [[Minecraft Wiki:Projects/wiki.vg merge/Object Data|Object Data]] for details.
 |-
 | Velocity X
 | {{Type|Short}}
 | rowspan="3"| Same units as [[#Set Entity Velocity|Set Entity Velocity]].
 |-
 | Velocity Y
 | {{Type|Short}}
 |-
 | Velocity Z
 | {{Type|Short}}
 |}

{{warning|The points listed below should be considered when this packet is used to spawn a player entity.}}
When in [[Server.properties#online-mode|online mode]], the UUIDs must be valid and have valid skin blobs.
In offline mode, the vanilla server uses [[Wikipedia:Universally unique identifier#Versions 3 and 5 (namespace name-based)|UUID v3]] and chooses the player's UUID by using the String <code>OfflinePlayer:&lt;player name&gt;</code>, encoding it in UTF-8 (and case-sensitive), then processes it with <code>[https://github.com/AdoptOpenJDK/openjdk-jdk8u/blob/9a91972c76ddda5c1ce28b50ca38cbd8a30b7a72/jdk/src/share/classes/java/util/UUID.java#L153-L175 UUID.nameUUIDFromBytes]</code>.

For NPCs UUID v2 should be used. Note:

 <+Grum> i will never confirm this as a feature you know that :)

In an example UUID, <code>xxxxxxxx-xxxx-Yxxx-xxxx-xxxxxxxxxxxx</code>, the UUID version is specified by <code>Y</code>. So, for UUID v3, <code>Y</code> will always be <code>3</code>, and for UUID v2, <code>Y</code> will always be <code>2</code>.

==== Entity Animation ====

Sent whenever an entity should change animation.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="2"| ''protocol:''<br/><code>0x02</code><br/><br/>''resource:''<br/><code>animate</code>
 | rowspan="2"| Play
 | rowspan="2"| Client
 | Entity ID
 | {{Type|VarInt}}
 | Player ID.
 |-
 | Animation
 | {{Type|Unsigned Byte}}
 | Animation ID (see below).
 |}

Animation can be one of the following values:

{| class="wikitable"
 ! ID
 ! Animation
 |-
 | 0
 | Swing main arm
 |-
 | 2
 | Leave bed
 |-
 | 3
 | Swing offhand
 |-
 | 4
 | Critical effect
 |-
 | 5
 | Magic critical effect
 |}

==== Award Statistics ====

Sent as a response to [[#Client Status|Client Status]] (id 1). Will only send the changed values if previously requested.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! colspan="2"| Field Name
 ! colspan="2"| Field Type
 ! Notes
 |-
 | rowspan="3"| ''protocol:''<br/><code>0x03</code><br/><br/>''resource:''<br/><code>award_stats</code>
 | rowspan="3"| Play
 | rowspan="3"| Client
 | rowspan="3"| Statistic
 | Category ID
 | rowspan="3"| {{Type|Prefixed Array}}
 | {{Type|VarInt}}
 | See below.
 |-
 | Statistic ID
 | {{Type|VarInt}}
 | See below.
 |-
 | Value
 | {{Type|VarInt}}
 | The amount to set it to.
 |}

Categories (these are namespaced, but with <code>:</code> replaced with <code>.</code>):

{| class="wikitable"
 ! Name
 ! ID
 ! Registry
 |-
 | <code>minecraft.mined</code>
 | 0
 | Blocks
 |-
 | <code>minecraft.crafted</code>
 | 1
 | Items
 |-
 | <code>minecraft.used</code>
 | 2
 | Items
 |-
 | <code>minecraft.broken</code>
 | 3
 | Items
 |-
 | <code>minecraft.picked_up</code>
 | 4
 | Items
 |-
 | <code>minecraft.dropped</code>
 | 5
 | Items
 |-
 | <code>minecraft.killed</code>
 | 6
 | Entities
 |-
 | <code>minecraft.killed_by</code>
 | 7
 | Entities
 |-
 | <code>minecraft.custom</code>
 | 8
 | Custom
 |}

Blocks, Items, and Entities use block (not block state), item, and entity ids.

Custom has the following (unit only matters for clients):

{| class="wikitable"
 ! Name
 ! ID
 ! Unit
 |-
 | <code>minecraft.leave_game</code>
 | 0
 | None
 |-
 | <code>minecraft.play_time</code>
 | 1
 | Time
 |-
 | <code>minecraft.total_world_time</code>
 | 2
 | Time
 |-
 | <code>minecraft.time_since_death</code>
 | 3
 | Time
 |-
 | <code>minecraft.time_since_rest</code>
 | 4
 | Time
 |-
 | <code>minecraft.sneak_time</code>
 | 5
 | Time
 |-
 | <code>minecraft.walk_one_cm</code>
 | 6
 | Distance
 |-
 | <code>minecraft.crouch_one_cm</code>
 | 7
 | Distance
 |-
 | <code>minecraft.sprint_one_cm</code>
 | 8
 | Distance
 |-
 | <code>minecraft.walk_on_water_one_cm</code>
 | 9
 | Distance
 |-
 | <code>minecraft.fall_one_cm</code>
 | 10
 | Distance
 |-
 | <code>minecraft.climb_one_cm</code>
 | 11
 | Distance
 |-
 | <code>minecraft.fly_one_cm</code>
 | 12
 | Distance
 |-
 | <code>minecraft.walk_under_water_one_cm</code>
 | 13
 | Distance
 |-
 | <code>minecraft.minecart_one_cm</code>
 | 14
 | Distance
 |-
 | <code>minecraft.boat_one_cm</code>
 | 15
 | Distance
 |-
 | <code>minecraft.pig_one_cm</code>
 | 16
 | Distance
 |-
 | <code>minecraft.horse_one_cm</code>
 | 17
 | Distance
 |-
 | <code>minecraft.aviate_one_cm</code>
 | 18
 | Distance
 |-
 | <code>minecraft.swim_one_cm</code>
 | 19
 | Distance
 |-
 | <code>minecraft.strider_one_cm</code>
 | 20
 | Distance
 |-
 | <code>minecraft.jump</code>
 | 21
 | None
 |-
 | <code>minecraft.drop</code>
 | 22
 | None
 |-
 | <code>minecraft.damage_dealt</code>
 | 23
 | Damage
 |-
 | <code>minecraft.damage_dealt_absorbed</code>
 | 24
 | Damage
 |-
 | <code>minecraft.damage_dealt_resisted</code>
 | 25
 | Damage
 |-
 | <code>minecraft.damage_taken</code>
 | 26
 | Damage
 |-
 | <code>minecraft.damage_blocked_by_shield</code>
 | 27
 | Damage
 |-
 | <code>minecraft.damage_absorbed</code>
 | 28
 | Damage
 |-
 | <code>minecraft.damage_resisted</code>
 | 29
 | Damage
 |-
 | <code>minecraft.deaths</code>
 | 30
 | None
 |-
 | <code>minecraft.mob_kills</code>
 | 31
 | None
 |-
 | <code>minecraft.animals_bred</code>
 | 32
 | None
 |-
 | <code>minecraft.player_kills</code>
 | 33
 | None
 |-
 | <code>minecraft.fish_caught</code>
 | 34
 | None
 |-
 | <code>minecraft.talked_to_villager</code>
 | 35
 | None
 |-
 | <code>minecraft.traded_with_villager</code>
 | 36
 | None
 |-
 | <code>minecraft.eat_cake_slice</code>
 | 37
 | None
 |-
 | <code>minecraft.fill_cauldron</code>
 | 38
 | None
 |-
 | <code>minecraft.use_cauldron</code>
 | 39
 | None
 |-
 | <code>minecraft.clean_armor</code>
 | 40
 | None
 |-
 | <code>minecraft.clean_banner</code>
 | 41
 | None
 |-
 | <code>minecraft.clean_shulker_box</code>
 | 42
 | None
 |-
 | <code>minecraft.interact_with_brewingstand</code>
 | 43
 | None
 |-
 | <code>minecraft.interact_with_beacon</code>
 | 44
 | None
 |-
 | <code>minecraft.inspect_dropper</code>
 | 45
 | None
 |-
 | <code>minecraft.inspect_hopper</code>
 | 46
 | None
 |-
 | <code>minecraft.inspect_dispenser</code>
 | 47
 | None
 |-
 | <code>minecraft.play_noteblock</code>
 | 48
 | None
 |-
 | <code>minecraft.tune_noteblock</code>
 | 49
 | None
 |-
 | <code>minecraft.pot_flower</code>
 | 50
 | None
 |-
 | <code>minecraft.trigger_trapped_chest</code>
 | 51
 | None
 |-
 | <code>minecraft.open_enderchest</code>
 | 52
 | None
 |-
 | <code>minecraft.enchant_item</code>
 | 53
 | None
 |-
 | <code>minecraft.play_record</code>
 | 54
 | None
 |-
 | <code>minecraft.interact_with_furnace</code>
 | 55
 | None
 |-
 | <code>minecraft.interact_with_crafting_table</code>
 | 56
 | None
 |-
 | <code>minecraft.open_chest</code>
 | 57
 | None
 |-
 | <code>minecraft.sleep_in_bed</code>
 | 58
 | None
 |-
 | <code>minecraft.open_shulker_box</code>
 | 59
 | None
 |-
 | <code>minecraft.open_barrel</code>
 | 60
 | None
 |-
 | <code>minecraft.interact_with_blast_furnace</code>
 | 61
 | None
 |-
 | <code>minecraft.interact_with_smoker</code>
 | 62
 | None
 |-
 | <code>minecraft.interact_with_lectern</code>
 | 63
 | None
 |-
 | <code>minecraft.interact_with_campfire</code>
 | 64
 | None
 |-
 | <code>minecraft.interact_with_cartography_table</code>
 | 65
 | None
 |-
 | <code>minecraft.interact_with_loom</code>
 | 66
 | None
 |-
 | <code>minecraft.interact_with_stonecutter</code>
 | 67
 | None
 |-
 | <code>minecraft.bell_ring</code>
 | 68
 | None
 |-
 | <code>minecraft.raid_trigger</code>
 | 69
 | None
 |-
 | <code>minecraft.raid_win</code>
 | 70
 | None
 |-
 | <code>minecraft.interact_with_anvil</code>
 | 71
 | None
 |-
 | <code>minecraft.interact_with_grindstone</code>
 | 72
 | None
 |-
 | <code>minecraft.target_hit</code>
 | 73
 | None
 |-
 | <code>minecraft.interact_with_smithing_table</code>
 | 74
 | None
 |}
 
Units:

* None: just a normal number (formatted with 0 decimal places)
* Damage: value is 10 times the normal amount
* Distance: a distance in centimeters (hundredths of blocks)
* Time: a time span in ticks

==== Acknowledge Block Change ====

Acknowledges a user-initiated block change. After receiving this packet, the client will display the block state sent by the server instead of the one predicted by the client.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="1"| ''protocol:''<br/><code>0x04</code><br/><br/>''resource:''<br/><code>block_changed_ack</code>
 | rowspan="1"| Play
 | rowspan="1"| Client
 | Sequence ID
 | {{Type|VarInt}}
 | Represents the sequence to acknowledge, this is used for properly syncing block changes to the client after interactions.
 |}

==== Set Block Destroy Stage ====

0–9 are the displayable destroy stages and each other number means that there is no animation on this coordinate.

Block break animations can still be applied on air; the animation will remain visible although there is no block being broken.  However, if this is applied to a transparent block, odd graphical effects may happen, including water losing its transparency.  (An effect similar to this can be seen in normal gameplay when breaking ice blocks)

If you need to display several break animations at the same time you have to give each of them a unique Entity ID. The entity ID does not need to correspond to an actual entity on the client. It is valid to use a randomly generated number.

When removing break animation, you must use the ID of the entity that set it.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="3"| ''protocol:''<br/><code>0x05</code><br/><br/>''resource:''<br/><code>block_destruction</code>
 | rowspan="3"| Play
 | rowspan="3"| Client
 | Entity ID
 | {{Type|VarInt}}
 | The ID of the entity breaking the block.
 |-
 | Location
 | {{Type|Position}}
 | Block Position.
 |-
 | Destroy Stage
 | {{Type|Unsigned Byte}}
 | 0–9 to set it, any other value to remove it.
 |}

==== Block Entity Data ====

Sets the block entity associated with the block at the given location.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="3"| ''protocol:''<br/><code>0x06</code><br/><br/>''resource:''<br/><code>block_entity_data</code>
 | rowspan="3"| Play
 | rowspan="3"| Client
 | Location
 | {{Type|Position}}
 |
 |-
 | Type
 | {{Type|VarInt}}
 | The type of the block entity
 |-
 | NBT Data
 | {{Type|NBT}}
 | Data to set.
 |}

==== Block Action ====

This packet is used for a number of actions and animations performed by blocks, usually non-persistent.  The client ignores the provided block type and instead uses the block state in their world.

See [[Minecraft Wiki:Projects/wiki.vg merge/Block Actions|Block Actions]] for a list of values.

{{warning|This packet uses a block ID from the <code>minecraft:block</code> registry, not a block state.}}

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="4"| ''protocol:''<br/><code>0x07</code><br/><br/>''resource:''<br/><code>block_event</code>
 | rowspan="4"| Play
 | rowspan="4"| Client
 | Location
 | {{Type|Position}}
 | Block coordinates.
 |-
 | Action ID (Byte 1)
 | {{Type|Unsigned Byte}}
 | Varies depending on block — see [[Minecraft Wiki:Projects/wiki.vg merge/Block Actions|Block Actions]].
 |-
 | Action Parameter (Byte 2)
 | {{Type|Unsigned Byte}}
 | Varies depending on block — see [[Minecraft Wiki:Projects/wiki.vg merge/Block Actions|Block Actions]].
 |-
 | Block Type
 | {{Type|VarInt}}
 | The block type ID for the block. This value is unused by the vanilla client, as it will infer the type of block based on the given position.
 |}

==== Block Update ====

Fired whenever a block is changed within the render distance.

{{warning|Changing a block in a chunk that is not loaded is not a stable action.  The vanilla client currently uses a ''shared'' empty chunk which is modified for all block changes in unloaded chunks; while in 1.9 this chunk never renders in older versions the changed block will appear in all copies of the empty chunk.  Servers should avoid sending block changes in unloaded chunks and clients should ignore such packets.}}

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="2"| ''protocol:''<br/><code>0x08</code><br/><br/>''resource:''<br/><code>block_update</code>
 | rowspan="2"| Play
 | rowspan="2"| Client
 | Location
 | {{Type|Position}}
 | Block Coordinates.
 |-
 | Block ID
 | {{Type|VarInt}}
 | The new block state ID for the block as given in the [[Minecraft Wiki:Projects/wiki.vg merge/Chunk Format#Block state registry|block state registry]].
 |}

==== Boss Bar ====

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! colspan="2"| Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="14"| ''protocol:''<br/><code>0x09</code><br/><br/>''resource:''<br/><code>boss_event</code>
 | rowspan="14"| Play
 | rowspan="14"| Client
 | colspan="2"| UUID
 | {{Type|UUID}}
 | Unique ID for this bar.
 |-
 | colspan="2"| Action
 | {{Type|VarInt}} {{Type|Enum}}
 | Determines the layout of the remaining packet.
 |-
 ! Action
 ! Field Name
 !
 !
 |-
 | rowspan="5"| 0: add
 | Title
 | {{Type|Text Component}}
 |
 |-
 | Health
 | {{Type|Float}}
 | From 0 to 1. Values greater than 1 do not crash a vanilla client, and start [https://i.johni0702.de/nA.png rendering part of a second health bar] at around 1.5.
 |-
 | Color
 | {{Type|VarInt}} {{Type|Enum}}
 | Color ID (see below).
 |-
 | Division
 | {{Type|VarInt}} {{Type|Enum}}
 | Type of division (see below).
 |-
 | Flags
 | {{Type|Unsigned Byte}}
 | Bit mask. 0x01: should darken sky, 0x02: is dragon bar (used to play end music), 0x04: create fog (previously was also controlled by 0x02).
 |-
 | 1: remove
 | ''no fields''
 | ''no fields''
 | Removes this boss bar.
 |-
 | 2: update health
 | Health
 | {{Type|Float}}
 | ''as above''
 |-
 | 3: update title
 | Title
 | {{Type|Text Component}}
 |
 |-
 | rowspan="2"| 4: update style
 | Color
 | {{Type|VarInt}} {{Type|Enum}}
 | Color ID (see below).
 |-
 | Dividers
 | {{Type|VarInt}} {{Type|Enum}}
 | ''as above''
 |-
 | 5: update flags
 | Flags
 | {{Type|Unsigned Byte}}
 | ''as above''
 |}

{| class="wikitable"
 ! ID
 ! Color
 |-
 | 0
 | Pink
 |-
 | 1
 | Blue
 |-
 | 2
 | Red
 |-
 | 3
 | Green
 |-
 | 4
 | Yellow
 |-
 | 5
 | Purple
 |-
 | 6
 | White
 |}

{| class="wikitable"
 ! ID
 ! Type of division
 |-
 | 0
 | No division
 |-
 | 1
 | 6 notches
 |-
 | 2
 | 10 notches
 |-
 | 3
 | 12 notches
 |-
 | 4
 | 20 notches
 |}

==== Change Difficulty ====

Changes the difficulty setting in the client's option menu

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="2"| ''protocol:''<br/><code>0x0A</code><br/><br/>''resource:''<br/><code>change_difficulty</code>
 | rowspan="2"| Play
 | rowspan="2"| Client
 | Difficulty
 | {{Type|Unsigned Byte}} {{Type|Enum}}
 | 0: peaceful, 1: easy, 2: normal, 3: hard.
 |-
 | Difficulty locked?
 | {{Type|Boolean}}
 |
 |}

==== Chunk Batch Finished ====

Marks the end of a chunk batch. The vanilla client marks the time it receives this packet and calculates the elapsed duration since the [[#Chunk Batch Start|beginning of the chunk batch]]. The server uses this duration and the batch size received in this packet to estimate the number of milliseconds elapsed per chunk received. This value is then used to calculate the desired number of chunks per tick through the formula <code>25 / millisPerChunk</code>, which is reported to the server through [[#Chunk Batch Received|Chunk Batch Received]]. This likely uses <code>25</code> instead of the normal tick duration of <code>50</code> so chunk processing will only use half of the client's and network's bandwidth.

The vanilla client uses the samples from the latest 15 batches to estimate the milliseconds per chunk number.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="1"| ''protocol:''<br/><code>0x0B</code><br/><br/>''resource:''<br/><code>chunk_batch_finished</code>
 | rowspan="1"| Play
 | rowspan="1"| Client
 | Batch size
 | {{Type|VarInt}}
 | Number of chunks.
 |}

==== Chunk Batch Start ====

Marks the start of a chunk batch. The vanilla client marks and stores the time it receives this packet.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | ''protocol:''<br/><code>0x0C</code><br/><br/>''resource:''<br/><code>chunk_batch_start</code>
 | Play
 | Client
 | colspan="3"| ''no fields''
 |}

==== Chunk Biomes ====

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! colspan="2"| Field Name
 ! colspan="2"| Field Type
 ! Notes
 |-
 | rowspan="3"| ''protocol:''<br/><code>0x0D</code><br/><br/>''resource:''<br/><code>chunks_biomes</code>
 | rowspan="3"| Play
 | rowspan="3"| Client
 | rowspan="3"| Chunk biome data
 | Chunk Z
 | rowspan="3"| {{Type|Prefixed Array}}
 | {{Type|Int}}
 | Chunk coordinate (block coordinate divided by 16, rounded down)
 |-
 | Chunk X
 | {{Type|Int}}
 | Chunk coordinate (block coordinate divided by 16, rounded down)
 |-
 | Data
 | {{Type|Prefixed Array}} of {{Type|Byte}}
 | Chunk [[Minecraft Wiki:Projects/wiki.vg merge/Chunk Format#Data structure|data structure]], with [[Minecraft Wiki:Projects/wiki.vg merge/Chunk Format#Chunk_Section|sections]] containing only the <code>Biomes</code> field
 |}

Note: The order of X and Z is inverted, because the client reads them as one big-endian {{Type|Long}}, with Z being the upper 32 bits.

==== Clear Titles ====

Clear the client's current title information, with the option to also reset it.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="1"| ''protocol:''<br/><code>0x0E</code><br/><br/>''resource:''<br/><code>clear_titles</code>
 | rowspan="1"| Play
 | rowspan="1"| Client
 | Reset
 | {{Type|Boolean}}
 |
 |}

==== Command Suggestions Response ====

The server responds with a list of auto-completions of the last word sent to it. In the case of regular chat, this is a player username. Command names and parameters are also supported. The client sorts these alphabetically before listing them.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! colspan="2"| Field Name
 ! colspan="2"| Field Type
 ! Notes
 |-
 | rowspan="5"| ''protocol:''<br/><code>0x0F</code><br/><br/>''resource:''<br/><code>command_suggestions</code>
 | rowspan="5"| Play
 | rowspan="5"| Client
 | colspan="2"| ID
 | colspan="2"| {{Type|VarInt}}
 | Transaction ID.
 |-
 | colspan="2"| Start
 | colspan="2"| {{Type|VarInt}}
 | Start of the text to replace.
 |-
 | colspan="2"| Length
 | colspan="2"| {{Type|VarInt}}
 | Length of the text to replace.
 |-
 | rowspan="2"| Matches
 | Match
 | rowspan="2"| {{Type|Prefixed Array}}
 | {{Type|String}} (32767)
 | One eligible value to insert, note that each command is sent separately instead of in a single string, hence the need for Count.  Note that for instance this doesn't include a leading <code>/</code> on commands.
 |-
 | Tooltip
 | {{Type|Prefixed Optional}} {{Type|Text Component}}
 | Tooltip to display.
 |}

==== Commands ====

Lists all of the commands on the server, and how they are parsed.

This is a directed graph, with one root node.  Each redirect or child node must refer only to nodes that have already been declared.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="2"| ''protocol:''<br/><code>0x10</code><br/><br/>''resource:''<br/><code>commands</code>
 | rowspan="2"| Play
 | rowspan="2"| Client
 | Nodes
 | {{Type|Prefixed Array}} of [[Minecraft Wiki:Projects/wiki.vg merge/Command Data#Node Format|Node]]
 | An array of nodes.
 |-
 | Root index
 | {{Type|VarInt}}
 | Index of the <code>root</code> node in the previous array.
 |}

For more information on this packet, see the [[Minecraft Wiki:Projects/wiki.vg merge/Command Data|Command Data]] article.

==== Close Container ====

This packet is sent from the server to the client when a window is forcibly closed, such as when a chest is destroyed while it's open. The vanilla client disregards the provided window ID and closes any active window.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | ''protocol:''<br/><code>0x11</code><br/><br/>''resource:''<br/><code>container_close</code>
 | Play
 | Client
 | Window ID
 | {{Type|VarInt}}
 | This is the ID of the window that was closed. 0 for inventory.
 |}

==== Set Container Content ====

[[File:Inventory-slots.png|thumb|The inventory slots]]

Replaces the contents of a container window. Sent by the server upon initialization of a container window or the player's inventory, and in response to state ID mismatches (see [[#Click Container]]).

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="4"| ''protocol:''<br/><code>0x12</code><br/><br/>''resource:''<br/><code>container_set_content</code>
 | rowspan="4"| Play
 | rowspan="4"| Client
 | Window ID
 | {{Type|VarInt}}
 | The ID of window which items are being sent for. 0 for player inventory. The client ignores any packets targeting a Window ID other than the current one. However, an exception is made for the player inventory, which may be targeted at any time. (The vanilla server does not appear to utilize this special case.)
 |-
 | State ID
 | {{Type|VarInt}}
 | A server-managed sequence number used to avoid desynchronization; see [[#Click Container]].
 |-
 | Slot Data
 | {{Type|Prefixed Array}} of {{Type|Slot}}
 |-
 | Carried Item
 | {{Type|Slot}}
 | Item being dragged with the mouse.
 |}

See [[Minecraft Wiki:Projects/wiki.vg merge/Inventory#Windows|inventory windows]] for further information about how slots are indexed.
Use [[#Open Screen|Open Screen]] to open the container on the client.

==== Set Container Property ====

This packet is used to inform the client that part of a GUI window should be updated.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="3"| ''protocol:''<br/><code>0x13</code><br/><br/>''resource:''<br/><code>container_set_data</code>
 | rowspan="3"| Play
 | rowspan="3"| Client
 | Window ID
 | {{Type|VarInt}}
 |
 |-
 | Property
 | {{Type|Short}}
 | The property to be updated, see below.
 |-
 | Value
 | {{Type|Short}}
 | The new value for the property, see below.
 |}

The meaning of the Property field depends on the type of the window. The following table shows the known combinations of window type and property, and how the value is to be interpreted.

{| class="wikitable"
 |-
 ! Window type
 ! Property
 ! Value
 |-
 | rowspan="4"| Furnace
 | 0: Fire icon (fuel left)
 | counting from fuel burn time down to 0 (in-game ticks)
 |-
 | 1: Maximum fuel burn time
 | fuel burn time or 0 (in-game ticks)
 |-
 | 2: Progress arrow
 | counting from 0 to maximum progress (in-game ticks)
 |-
 | 3: Maximum progress
 | always 200 on the vanilla server
 |-
 | rowspan="10"| Enchantment Table
 | 0: Level requirement for top enchantment slot
 | rowspan="3"| The enchantment's xp level requirement
 |-
 | 1: Level requirement for middle enchantment slot
 |-
 | 2: Level requirement for bottom enchantment slot
 |-
 | 3: The enchantment seed
 | Used for drawing the enchantment names (in [[Wikipedia:Standard Galactic Alphabet|SGA]]) clientside.  The same seed ''is'' used to calculate enchantments, but some of the data isn't sent to the client to prevent easily guessing the entire list (the seed value here is the regular seed bitwise and <code>0xFFFFFFF0</code>).
 |-
 | 4: Enchantment ID shown on mouse hover over top enchantment slot
 | rowspan="3"| The enchantment id (set to -1 to hide it), see below for values
 |-
 | 5: Enchantment ID shown on mouse hover over middle enchantment slot
 |-
 | 6: Enchantment ID shown on mouse hover over bottom enchantment slot
 |-
 | 7: Enchantment level shown on mouse hover over the top slot
 | rowspan="3"| The enchantment level (1 = I, 2 = II, 6 = VI, etc.), or -1 if no enchant
 |-
 | 8: Enchantment level shown on mouse hover over the middle slot
 |-
 | 9: Enchantment level shown on mouse hover over the bottom slot
 |-
 | rowspan="3"| Beacon
 | 0: Power level
 | 0-4, controls what effect buttons are enabled
 |-
 | 1: First potion effect
 | [[Data values#Status effects|Potion effect ID]] for the first effect, or -1 if no effect
 |-
 | 2: Second potion effect
 | [[Data values#Status effects|Potion effect ID]] for the second effect, or -1 if no effect
 |-
 | Anvil
 | 0: Repair cost
 | The repair's cost in xp levels
 |-
 | rowspan="2"| Brewing Stand
 | 0: Brew time
 | 0 – 400, with 400 making the arrow empty, and 0 making the arrow full
 |-
 | 1: Fuel time
 | 0 - 20, with 0 making the arrow empty, and 20 making the arrow full
 |-
 | Stonecutter
 | 0: Selected recipe
 | The index of the selected recipe. -1 means none is selected.
 |-
 | Loom
 | 0: Selected pattern
 | The index of the selected pattern. 0 means none is selected, 0 is also the internal id of the "base" pattern.
 |-
 | Lectern
 | 0: Page number
 | The current page number, starting from 0.
 |}

For an enchanting table, the following numerical IDs are used:

{| class="wikitable"
 ! Numerical ID
 ! Enchantment ID
 ! Enchantment Name
 |-
 | 0
 | minecraft:protection
 | Protection
 |-
 | 1
 | minecraft:fire_protection
 | Fire Protection
 |-
 | 2
 | minecraft:feather_falling
 | Feather Falling
 |-
 | 3
 | minecraft:blast_protection
 | Blast Protection
 |-
 | 4
 | minecraft:projectile_protection
 | Projectile Protection
 |-
 | 5
 | minecraft:respiration
 | Respiration
 |-
 | 6
 | minecraft:aqua_affinity
 | Aqua Affinity
 |-
 | 7
 | minecraft:thorns
 | Thorns
 |-
 | 8
 | minecraft:depth_strider
 | Depth Strider
 |-
 | 9
 | minecraft:frost_walker
 | Frost Walker
 |-
 | 10
 | minecraft:binding_curse
 | Curse of Binding
 |-
 | 11
 | minecraft:soul_speed
 | Soul Speed
 |-
 | 12
 | minecraft:swift_sneak
 | Swift Sneak
 |-
 | 13
 | minecraft:sharpness
 | Sharpness
 |-
 | 14
 | minecraft:smite
 | Smite
 |-
 | 15
 | minecraft:bane_of_arthropods
 | Bane of Arthropods
 |-
 | 16
 | minecraft:knockback
 | Knockback
 |-
 | 17
 | minecraft:fire_aspect
 | Fire Aspect
 |-
 | 18
 | minecraft:looting
 | Looting
 |-
 | 19
 | minecraft:sweeping_edge
 | Sweeping Edge
 |-
 | 20
 | minecraft:efficiency
 | Efficiency
 |-
 | 21
 | minecraft:silk_touch
 | Silk Touch
 |-
 | 22
 | minecraft:unbreaking
 | Unbreaking
 |-
 | 23
 | minecraft:fortune
 | Fortune
 |-
 | 24
 | minecraft:power
 | Power
 |-
 | 25
 | minecraft:punch
 | Punch
 |-
 | 26
 | minecraft:flame
 | Flame
 |-
 | 27
 | minecraft:infinity
 | Infinity
 |-
 | 28
 | minecraft:luck_of_the_sea
 | Luck of the Sea
 |-
 | 29
 | minecraft:lure
 | Lure
 |-
 | 30
 | minecraft:loyalty
 | Loyalty
 |-
 | 31
 | minecraft:impaling
 | Impaling
 |-
 | 32
 | minecraft:riptide
 | Riptide
 |-
 | 33
 | minecraft:channeling
 | Channeling
 |-
 | 34
 | minecraft:multishot
 | Multishot
 |-
 | 35
 | minecraft:quick_charge
 | Quick Charge
 |-
 | 36
 | minecraft:piercing
 | Piercing
 |-
 | 37
 | minecraft:density
 | Density
 |-
 | 38
 | minecraft:breach
 | Breach
 |-
 | 39
 | minecraft:wind_burst
 | Wind Burst
 |-
 | 40
 | minecraft:mending
 | Mending
 |-
 | 41
 | minecraft:vanishing_curse
 | Curse of Vanishing
 |}

==== Set Container Slot ====

Sent by the server when an item in a slot (in a window) is added/removed.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="4"| ''protocol:''<br/><code>0x14</code><br/><br/>''resource:''<br/><code>container_set_slot</code>
 | rowspan="4"| Play
 | rowspan="4"| Client
 | Window ID
 | {{Type|VarInt}}
 | The window which is being updated. 0 for player inventory. The client ignores any packets targeting a Window ID other than the current one; see below for exceptions.
 |-
 | State ID
 | {{Type|VarInt}}
 | A server-managed sequence number used to avoid desynchronization; see [[#Click Container]].
 |-
 | Slot
 | {{Type|Short}}
 | The slot that should be updated.
 |-
 | Slot Data
 | {{Type|Slot}}
 |
 |}

If Window ID is 0, the hotbar and offhand slots (slots 36 through 45) may be updated even when a different container window is open. (The vanilla server does not appear to utilize this special case.) Updates are also restricted to those slots when the player is looking at a creative inventory tab other than the survival inventory. (The vanilla server does ''not'' handle this restriction in any way, leading to [https://bugs.mojang.com/browse/MC-242392 MC-242392].)

If Window ID is -1, the item being dragged with the mouse is set. In this case, State ID and Slot are ignored.

If Window ID is -2, any slot in the player's inventory can be updated irrespective of the current container window. In this case, State ID is ignored, and the vanilla server uses a bogus value of 0. Used by the vanilla server to implement the [[#Pick Item]] functionality.

When a container window is open, the server never sends updates targeting Window ID 0&mdash;all of the [[Minecraft Wiki:Projects/wiki.vg merge/Inventory|window types]] include slots for the player inventory. The client must automatically apply changes targeting the inventory portion of a container window to the main inventory; the server does not resend them for ID 0 when the window is closed. However, since the armor and offhand slots are only present on ID 0, updates to those slots occurring while a window is open must be deferred by the server until the window's closure.

==== Cookie Request (play) ====

Requests a cookie that was previously stored.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! colspan="2"| Field Name
 ! colspan="2"| Field Type
 ! Notes
 |-
 | rowspan="1"| ''protocol:''<br/><code>0x15</code><br/><br/>''resource:''<br/><code>cookie_request</code>
 | rowspan="1"| Play
 | rowspan="1"| Client
 | colspan="2"| Key
 | colspan="2"| {{Type|Identifier}}
 | The identifier of the cookie.
 |}

==== Set Cooldown ====

Applies a cooldown period to all items with the given type.  Used by the vanilla server with enderpearls.  This packet should be sent when the cooldown starts and also when the cooldown ends (to compensate for lag), although the client will end the cooldown automatically. Can be applied to any item, note that interactions still get sent to the server with the item but the client does not play the animation nor attempt to predict results (i.e block placing).

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="2"| ''protocol:''<br/><code>0x16</code><br/><br/>''resource:''<br/><code>cooldown</code>
 | rowspan="2"| Play
 | rowspan="2"| Client
 | Item ID
 | {{Type|VarInt}}
 | Numeric ID of the item to apply a cooldown to.
 |-
 | Cooldown Ticks
 | {{Type|VarInt}}
 | Number of ticks to apply a cooldown for, or 0 to clear the cooldown.
 |}

==== Chat Suggestions ====

Unused by the vanilla server. Likely provided for custom servers to send chat message completions to clients.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="2"| ''protocol:''<br/><code>0x17</code><br/><br/>''resource:''<br/><code>custom_chat_completions</code>
 | rowspan="2"| Play
 | rowspan="2"| Client
 | Action
 | {{Type|VarInt}} {{Type|Enum}}
 | 0: Add, 1: Remove, 2: Set
 |-
 | Entries
 | {{Type|Prefixed Array}} of {{Type|String}} (32767)
 |
 |}

==== Clientbound Plugin Message (play) ====

{{Main|Minecraft Wiki:Projects/wiki.vg merge/Plugin channels}}

Mods and plugins can use this to send their data. Minecraft itself uses several [[Minecraft Wiki:Projects/wiki.vg merge/Plugin channels|plugin channels]]. These internal channels are in the <code>minecraft</code> namespace.

More information on how it works on [https://dinnerbone.com/blog/2012/01/13/minecraft-plugin-channels-messaging/ Dinnerbone's blog]. More documentation about internal and popular registered channels are [[Minecraft Wiki:Projects/wiki.vg merge/Plugin channels|here]].

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="2"| ''protocol:''<br/><code>0x18</code><br/><br/>''resource:''<br/><code>custom_payload</code>
 | rowspan="2"| Play
 | rowspan="2"| Client
 | Channel
 | {{Type|Identifier}}
 | Name of the [[Minecraft Wiki:Projects/wiki.vg merge/Plugin channels|plugin channel]] used to send the data.
 |-
 | Data
 | {{Type|Byte Array}} (1048576)
 | Any data. The length of this array must be inferred from the packet length.
 |}

In vanilla clients, the maximum data length is 1048576 bytes.

==== Damage Event ====

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! colspan="2"| Field Name
 ! colspan="2"| Field Type
 ! Notes
 |-
 | rowspan="9"| ''protocol:''<br/><code>0x19</code><br/><br/>''resource:''<br/><code>damage_event</code>
 | rowspan="9"| Play
 | rowspan="9"| Client
 |-
 | colspan="2"| Entity ID
 | colspan="2"| {{Type|VarInt}}
 | The ID of the entity taking damage
 |-
 | colspan="2"| Source Type ID
 | colspan="2"| {{Type|VarInt}}
 | The type of damage in the <code>minecraft:damage_type</code> registry, defined by the [[Java Edition protocol#Registry_Data|Registry Data]] packet.
 |-
 | colspan="2"| Source Cause ID
 | colspan="2"| {{Type|VarInt}}
 | The ID + 1 of the entity responsible for the damage, if present. If not present, the value is 0
 |-
 | colspan="2"| Source Direct ID
 | colspan="2"| {{Type|VarInt}}
 | The ID + 1 of the entity that directly dealt the damage, if present. If not present, the value is 0. If this field is present:
* and damage was dealt indirectly, such as by the use of a projectile, this field will contain the ID of such projectile;
* and damage was dealt dirctly, such as by manually attacking, this field will contain the same value as Source Cause ID.
 |-
 | colspan="2"| Has Source Position
 | colspan="2"| {{Type|Boolean}}
 | Indicates the presence of the three following fields.
The vanilla server sends the Source Position when the damage was dealt by the /damage command and a position was specified
 |-
 | colspan="2"| Source Position X
 | colspan="2"| {{Type|Optional}} {{Type|Double}}
 | Only present if Has Source Position is true
 |-
 | colspan="2"| Source Position Y
 | colspan="2"| {{Type|Optional}} {{Type|Double}}
 | Only present if Has Source Position is true
 |-
 | colspan="2"| Source Position Z
 | colspan="2"| {{Type|Optional}} {{Type|Double}}
 | Only present if Has Source Position is true
 |}

==== Debug Sample ====

Sample data that is sent periodically after the client has subscribed with [[#Debug_Sample_Subscription|Debug Sample Subscription]].

The vanilla server only sends debug samples to players that are server operators.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="2"| ''protocol:''<br/><code>0x1A</code><br/><br/>''resource:''<br/><code>debug_sample</code>
 | rowspan="2"| Play
 | rowspan="2"| Client
 | Sample
 | {{Type|Prefixed Array}} of {{Type|Long}}
 | Array of type-dependent samples.
 |-
 | Sample Type
 | {{Type|VarInt}} {{Type|Enum}}
 | See below.
 |}

Types:

{| class="wikitable"
 ! Id !! Name !! Description
 |-
 | 0 || Tick time || Four different tick-related metrics, each one represented by one long on the array.
They are measured in nano-seconds, and are as follows:
* 0: Full tick time: Aggregate of the three times below;
* 1: Server tick time: Main server tick logic;
* 2: Tasks time: Tasks scheduled to execute after the main logic;
* 3: Idle time: Time idling to complete the full 50ms tick cycle.
Note that the vanilla client calculates the timings used for min/max/average display by subtracting the idle time from the full tick time. This can cause the displayed values to go negative if the idle time is (nonsensically) greater than the full tick time.
 |}

==== Delete Message ====

Removes a message from the client's chat. This only works for messages with signatures, system messages cannot be deleted with this packet.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="2"| ''protocol:''<br/><code>0x1B</code><br/><br/>''resource:''<br/><code>delete_chat</code>
 | rowspan="2"| Play
 | rowspan="2"| Client
 | Message ID
 | {{Type|VarInt}}
 | The message Id + 1, used for validating message signature. The next field is present only when value of this field is equal to 0.
 |-
 | Signature
 | {{Type|Optional}} {{Type|Byte Array}} (256)
 | The previous message's signature. Always 256 bytes and not length-prefixed.
 |}

==== Disconnect (play) ====

Sent by the server before it disconnects a client. The client assumes that the server has already closed the connection by the time the packet arrives.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | ''protocol:''<br/><code>0x1C</code><br/><br/>''resource:''<br/><code>disconnect</code>
 | Play
 | Client
 | Reason
 | {{Type|Text Component}}
 | Displayed to the client when the connection terminates.
 |}

==== Disguised Chat Message ====

{{Main|Minecraft_Wiki:Projects/wiki.vg_merge/Chat}}

Sends the client a chat message, but without any message signing information.

The vanilla server uses this packet when the console is communicating with players through commands, such as <code>/say</code>, <code>/tell</code>, <code>/me</code>, among others.

{| class="wikitable
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="4"| ''protocol:''<br/><code>0x1D</code><br/><br/>''resource:''<br/><code>disguised_chat</code>
 | rowspan="4"| Play
 | rowspan="4"| Client
 | Message
 | {{Type|Text Component}}
 | This is used as the <code>content</code> parameter when formatting the message on the client.
 |-
 | Chat Type
 | {{Type|ID}} or {{Type|Chat Type}}
 | Either the type of chat in the <code>minecraft:chat_type</code> registry, defined by the [[Java Edition protocol#Registry_Data|Registry Data]] packet, or an inline definition.
 |-
 | Sender Name
 | {{Type|Text Component}}
 | The name of the one sending the message, usually the sender's display name.
This is used as the <code>sender</code> parameter when formatting the message on the client.
 |-
 | Target Name
 | {{Type|Prefixed Optional}} {{Type|Text Component}}
 | The name of the one receiving the message, usually the receiver's display name.
This is used as the <code>target</code> parameter when formatting the message on the client.
 |}

==== Entity Event ====

Entity statuses generally trigger an animation for an entity.  The available statuses vary by the entity's type (and are available to subclasses of that type as well).

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="2"| ''protocol:''<br/><code>0x1E</code><br/><br/>''resource:''<br/><code>entity_event</code>
 | rowspan="2"| Play
 | rowspan="2"| Client
 | Entity ID
 | {{Type|Int}}
 |
 |-
 | Entity Status
 | {{Type|Byte}} {{Type|Enum}}
 | See [[Minecraft Wiki:Projects/wiki.vg merge/Entity statuses|Entity statuses]] for a list of which statuses are valid for each type of entity.
 |}

==== Teleport Entity ====

{{warning|The Mojang-specified name of this packet was changed in 1.21.2 from <code>teleport_entity</code> to <code>entity_position_sync</code>. There is a new <code>teleport_entity</code>, which this document more appropriately calls [[#Synchronize Vehicle Position|Synchronize Vehicle Position]]. That packet has a different function and will lead to confusing results if used in place of this one.}}

This packet is sent by the server when an entity moves more than 8 blocks. 

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="10"| ''protocol:''<br/><code>0x1F</code><br/><br/>''resource:''<br/><code>entity_position_sync</code>
 | rowspan="10"| Play
 | rowspan="10"| Client
 | Entity ID
 | {{Type|VarInt}}
 |
 |-
 | X
 | {{Type|Double}}
 | 
 |-
 | Y
 | {{Type|Double}}
 |
 |-
 | Z
 | {{Type|Double}}
 |
 |-
 | Velocity X
 | {{Type|Double}}
 |
 |-
 | Velocity Y
 | {{Type|Double}}
 |
 |-
 | Velocity Z
 | {{Type|Double}}
 |
 |-
 | Yaw
 | {{Type|Float}}
 | Rotation on the X axis, in degrees.
 |-
 | Pitch
 | {{Type|Float}}
 | Rotation on the Y axis, in degrees.
 |-
 | On Ground
 | {{Type|Boolean}}
 |
|}

==== Explosion ====

Sent when an explosion occurs (creepers, TNT, and ghast fireballs).

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="10"| ''protocol:''<br/><code>0x20</code><br/><br/>''resource:''<br/><code>explode</code>
 | rowspan="10"| Play
 | rowspan="10"| Client
 | X
 | {{Type|Double}}
 |
 |-
 | Y
 | {{Type|Double}}
 |
 |-
 | Z
 | {{Type|Double}}
 |
 |-
 | Has Player Velocity
 | {{Type|Boolean}}
 |
 |-
 | Player Velocity X
 | {{Type|Optional}} {{Type|Double}}
 | X velocity of the player being pushed by the explosion. Only present if Has Player Velocity is true.
 |-
 | Player Velocity Y
 | {{Type|Optional}} {{Type|Double}}
 | Y velocity of the player being pushed by the explosion. Only present if Has Player Velocity is true.
 |-
 | Player Velocity Z
 | {{Type|Optional}} {{Type|Double}}
 | Z velocity of the player being pushed by the explosion. Only present if Has Player Velocity is true.
 |-
 | Explosion Particle ID
 | {{Type|VarInt}}
 | The particle ID listed in [[Minecraft Wiki:Projects/wiki.vg merge/Particles|Particles]].
 |-
 | Explosion Particle Data
 | Varies
 | Particle data as specified in [[Minecraft Wiki:Projects/wiki.vg merge/Particles|Particles]].
 |-
 | Explosion Sound
 | {{Type|ID or}} {{Type|Sound Event}}
 | ID in the <code>minecraft:sound_event</code> registry, or an inline definition.
 |}

==== Unload Chunk ====

Tells the client to unload a chunk column.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="2"| ''protocol:''<br/><code>0x21</code><br/><br/>''resource:''<br/><code>forget_level_chunk</code>
 | rowspan="2"| Play
 | rowspan="2"| Client
 | Chunk Z
 | {{Type|Int}}
 | Block coordinate divided by 16, rounded down.
 |-
 | Chunk X
 | {{Type|Int}}
 | Block coordinate divided by 16, rounded down.
 |}

Note: The order is inverted, because the client reads this packet as one big-endian {{Type|Long}}, with Z being the upper 32 bits.

It is legal to send this packet even if the given chunk is not currently loaded.

==== Game Event ====

Used for a wide variety of game events, from weather to bed use to game mode to demo messages.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="2"| ''protocol:''<br/><code>0x22</code><br/><br/>''resource:''<br/><code>game_event</code>
 | rowspan="2"| Play
 | rowspan="2"| Client
 | Event
 | {{Type|Unsigned Byte}}
 | See below.
 |-
 | Value
 | {{Type|Float}}
 | Depends on Event.
 |}

''Events'':

{| class="wikitable"
 ! Event
 ! Effect
 ! Value
 |-
 | 0
 | No respawn block available
 | Note: Displays message 'block.minecraft.spawn.not_valid' (You have no home bed or charged respawn anchor, or it was obstructed) to the player.
 |-
 | 1
 | Begin raining
 |
 |-
 | 2
 | End raining
 |
 |-
 | 3
 | Change game mode
 | 0: Survival, 1: Creative, 2: Adventure, 3: Spectator.
 |-
 | 4
 | Win game
 | 0: Just respawn player.<br>1: Roll the credits and respawn player.<br>Note that 1 is only sent by vanilla server when player has not yet achieved advancement "The end?", else 0 is sent.
 |-
 | 5
 | Demo event
 | 0: Show welcome to demo screen.<br>101: Tell movement controls.<br>102: Tell jump control.<br>103: Tell inventory control.<br>104: Tell that the demo is over and print a message about how to take a screenshot.
 |-
 | 6
 | Arrow hit player
 | Note: Sent when any player is struck by an arrow.
 |-
 | 7
 | Rain level change
 | Note: Seems to change both sky color and lighting.<br>Rain level ranging from 0 to 1.
 |-
 | 8
 | Thunder level change
 | Note: Seems to change both sky color and lighting (same as Rain level change, but doesn't start rain). It also requires rain to render by vanilla client.<br>Thunder level ranging from 0 to 1.
 |-
 | 9
 | Play pufferfish sting sound
 |-
 | 10
 | Play elder guardian mob appearance (effect and sound)
 |
 |-
 | 11
 | Enable respawn screen
 |  0: Enable respawn screen.<br>1: Immediately respawn (sent when the <code>doImmediateRespawn</code> gamerule changes).
 |-
 | 12
 | Limited crafting
 | 0: Disable limited crafting.<br>1: Enable limited crafting (sent when the <code>doLimitedCrafting</code> gamerule changes).
 |-
 | 13
 | Start waiting for level chunks
 | Instructs the client to begin the waiting process for the level chunks.<br>Sent by the server after the level is cleared on the client and is being re-sent (either during the first, or subsequent reconfigurations).
 |}

==== Open Horse Screen ====

This packet is used exclusively for opening the horse GUI. [[#Open Screen|Open Screen]] is used for all other GUIs.  The client will not open the inventory if the Entity ID does not point to an horse-like animal.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="3"| ''protocol:''<br/><code>0x23</code><br/><br/>''resource:''<br/><code>horse_screen_open</code>
 | rowspan="3"| Play
 | rowspan="3"| Client
 | Window ID
 | {{Type|VarInt}}
 | Same as the field of [[#Open Screen|Open Screen]].
 |-
 | Inventory columns count
 | {{Type|VarInt}}
 | How many columns of horse inventory slots exist in the GUI, 3 slots per column.
 |-
 | Entity ID
 | {{Type|Int}}
 | The "owner" entity of the GUI. The client should close the GUI if the owner entity dies or is cleared.
 |}

==== Hurt Animation ====

Plays a bobbing animation for the entity receiving damage.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="2"| ''protocol:''<br/><code>0x24</code><br/><br/>''resource:''<br/><code>hurt_animation</code>
 | rowspan="2"| Play
 | rowspan="2"| Client
 | Entity ID
 | {{Type|VarInt}}
 | The ID of the entity taking damage
 |-
 | Yaw
 | {{Type|Float}}
 | The direction the damage is coming from in relation to the entity
 |}

==== Initialize World Border ====

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="8"| ''protocol:''<br/><code>0x25</code><br/><br/>''resource:''<br/><code>initialize_border</code>
 | rowspan="8"| Play
 | rowspan="8"| Client
 | X
 | {{Type|Double}}
 |
 |-
 | Z
 | {{Type|Double}}
 |
 |-
 | Old Diameter
 | {{Type|Double}}
 | Current length of a single side of the world border, in meters.
 |-
 | New Diameter
 | {{Type|Double}}
 | Target length of a single side of the world border, in meters.
 |-
 | Speed
 | {{Type|VarLong}}
 | Number of real-time ''milli''seconds until New Diameter is reached. It appears that vanilla server does not sync world border speed to game ticks, so it gets out of sync with server lag. If the world border is not moving, this is set to 0.
 |-
 | Portal Teleport Boundary
 | {{Type|VarInt}}
 | Resulting coordinates from a portal teleport are limited to ±value. Usually 29999984.
 |-
 | Warning Blocks
 | {{Type|VarInt}}
 | In meters.
 |-
 | Warning Time
 | {{Type|VarInt}}
 | In seconds as set by <code>/worldborder warning time</code>.
 |}

The vanilla client determines how solid to display the warning by comparing to whichever is higher, the warning distance or whichever is lower, the distance from the current diameter to the target diameter or the place the border will be after warningTime seconds. In pseudocode:

<syntaxhighlight lang="java">
distance = max(min(resizeSpeed * 1000 * warningTime, abs(targetDiameter - currentDiameter)), warningDistance);
if (playerDistance < distance) {
    warning = 1.0 - playerDistance / distance;
} else {
    warning = 0.0;
}
</syntaxhighlight>

==== Clientbound Keep Alive (play) ====

The server will frequently send out a keep-alive, each containing a random ID. The client must respond with the same payload (see [[#Serverbound Keep Alive (play)|Serverbound Keep Alive]]). If the client does not respond to a Keep Alive packet within 15 seconds after it was sent, the server kicks the client. Vice versa, if the server does not send any keep-alives for 20 seconds, the client will disconnect and yields a "Timed out" exception.

The vanilla server uses a system-dependent time in milliseconds to generate the keep alive ID value.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | ''protocol:''<br/><code>0x26</code><br/><br/>''resource:''<br/><code>keep_alive</code>
 | Play
 | Client
 | Keep Alive ID
 | {{Type|Long}}
 |
 |}

==== Chunk Data and Update Light ====

{{Main|Minecraft Wiki:Projects/wiki.vg merge/Chunk Format}}
{{See also|#Unload Chunk}}

Sent when a chunk comes into the client's view distance, specifying its terrain, lighting and block entities.

The chunk must be within the view area previously specified with [[#Set Center Chunk|Set Center Chunk]]; see that packet for details.

It is not strictly necessary to send all block entities in this packet; it is still legal to send them with [[#Block Entity Data|Block Entity Data]] later.
{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="4"| ''protocol:''<br/><code>0x27</code><br/><br/>''resource:''<br/><code>level_chunk_with_light</code>
 | rowspan="4"| Play
 | rowspan="4"| Client
 | Chunk X
 | {{Type|Int}}
 | Chunk coordinate (block coordinate divided by 16, rounded down)
 |-
 | Chunk Z
 | {{Type|Int}}
 | Chunk coordinate (block coordinate divided by 16, rounded down)
 |-
 | Data
 | {{Type|Chunk Data}}
 | 
 |-
 | Light
 | {{Type|Light Data}}
 | 
 |}

Unlike the [[#Update Light|Update Light]] packet which uses the same format, setting the bit corresponding to a section to 0 in both of the block light or sky light masks does not appear to be useful, and the results in testing have been highly inconsistent.

==== World Event ====

Sent when a client is to play a sound or particle effect.

By default, the Minecraft client adjusts the volume of sound effects based on distance. The final boolean field is used to disable this, and instead the effect is played from 2 blocks away in the correct direction. Currently this is only used for effect 1023 (wither spawn), effect 1028 (enderdragon death), and effect 1038 (end portal opening); it is ignored on other effects.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="4"| ''protocol:''<br/><code>0x28</code><br/><br/>''resource:''<br/><code>level_event</code>
 | rowspan="4"| Play
 | rowspan="4"| Client
 | Event
 | {{Type|Int}}
 | The event, see below.
 |-
 | Location
 | {{Type|Position}}
 | The location of the event.
 |-
 | Data
 | {{Type|Int}}
 | Extra data for certain events, see below.
 |-
 | Disable Relative Volume
 | {{Type|Boolean}}
 | See above.
 |}

Events:

{| class="wikitable"
 ! ID
 ! Name
 ! Data
 |-
 ! colspan="3"| Sound
 |-
 | 1000
 | Dispenser dispenses
 |
 |-
 | 1001
 | Dispenser fails to dispense
 |
 |-
 | 1002
 | Dispenser shoots
 |
 |-
 | 1004
 | Firework shot
 |
 |-
 | 1009
 | Fire extinguished
 |
 |-
 | 1010
 | Play record
 | An ID in the <code>minecraft:item</code> registry, corresponding to a [[Music Disc|record item]]. If the ID doesn't correspond to a record, the packet is ignored. Any record already being played at the given location is overwritten. See [[Minecraft Wiki:Projects/wiki.vg merge/Data Generators|Data Generators]] for information on item IDs.
 |-
 | 1011
 | Stop record
 | 
 |-
 | 1015
 | Ghast warns
 |
 |-
 | 1016
 | Ghast shoots
 |
 |-
 | 1017
 | Ender dragon shoots
 |
 |-
 | 1018
 | Blaze shoots
 |
 |-
 | 1019
 | Zombie attacks wooden door
 |
 |-
 | 1020
 | Zombie attacks iron door
 |
 |-
 | 1021
 | Zombie breaks wooden door
 |
 |-
 | 1022
 | Wither breaks block
 |
 |-
 | 1023
 | Wither spawned
 |
 |-
 | 1024
 | Wither shoots
 |
 |-
 | 1025
 | Bat takes off
 |
 |-
 | 1026
 | Zombie infects
 |
 |-
 | 1027
 | Zombie villager converted
 |
 |-
 | 1028
 | Ender dragon dies
 |
 |-
 | 1029
 | Anvil destroyed
 |
 |-
 | 1030
 | Anvil used
 |
 |-
 | 1031
 | Anvil lands
 |
 |-
 | 1032
 | Portal travel
 |
 |-
 | 1033
 | Chorus flower grows
 |
 |-
 | 1034
 | Chorus flower dies
 |
 |-
 | 1035
 | Brewing stand brews
 |
 |-
 | 1038
 | End portal created
 |
 |-
 | 1039
 | Phantom bites
 |
 |-
 | 1040
 | Zombie converts to drowned
 |
 |-
 | 1041
 | Husk converts to zombie by drowning
 |
 |-
 | 1042
 | Grindstone used
 |
 |-
 | 1043
 | Book page turned
 |
 |-
 | 1044
 | Smithing table used
 |
 |-
 | 1045
 | Pointed dripstone landing
 |
 |-
 | 1046
 | Lava dripping on cauldron from dripstone
 |
 |-
 | 1047
 | Water dripping on cauldron from dripstone
 |
 |-
 | 1048
 | Skeleton converts to stray
 |
 |-
 | 1049
 | Crafter successfully crafts item
 | 
 |-
 | 1050
 | Crafter fails to craft item
 |
 |-
 ! colspan="3"| Particle
 |-
 | 1500
 | Composter composts
 |
 |-
 | 1501
 | Lava converts block (either water to stone, or removes existing blocks such as torches)
 |
 |-
 | 1502
 | Redstone torch burns out
 |
 |-
 | 1503
 | Ender eye placed in end portal frame
 |
 |-
 | 1504
 | Fluid drips from dripstone
 |
 |-
 | 1505
 | Bone meal particles and sound
 | How many particles to spawn.
 |-
 | 2000
 | Dispenser activation smoke
 | Direction, see below.
 |-
 | 2001
 | Block break + block break sound
 | Block state ID (see [[Minecraft Wiki:Projects/wiki.vg merge/Chunk Format#Block state registry|Chunk Format#Block state registry]]).
 |-
 | 2002
 | Splash potion. Particle effect + glass break sound.
 | RGB color as an integer (e.g. 8364543 for #7FA1FF).
 |-
 | 2003
 | Eye of ender entity break animation — particles and sound
 |
 |-
 | 2004
 | Spawner spawns mob: smoke + flames
 |
 |-
 | 2006
 | Dragon breath
 |
 |-
 | 2007
 | Instant splash potion. Particle effect + glass break sound.
 | RGB color as an integer (e.g. 8364543 for #7FA1FF).
 |-
 | 2008
 | Ender dragon destroys block
 |
 |-
 | 2009
 | Wet sponge vaporizes
 |
 |-
 | 2010
 | Crafter activation smoke
 | Direction, see below.
 |-
 | 2011
 | Bee fertilizes plant
 | How many particles to spawn.
 |-
 | 2012
 | Turtle egg placed
 | How many particles to spawn.
 |-
 | 2013
 | Smash attack (mace)
 | How many particles to spawn.
 |-
 | 3000
 | End gateway spawns
 |
 |-
 | 3001
 | Ender dragon resurrected
 |
 |-
 | 3002
 | Electric spark
 |
 |-
 | 3003
 | Copper apply wax
 |
 |-
 | 3004
 | Copper remove wax
 |
 |-
 | 3005
 | Copper scrape oxidation
 |
 |-
 | 3006
 | Sculk charge
 |
 |-
 | 3007
 | Sculk shrieker shriek
 |
 |-
 | 3008
 | Block finished brushing
 | Block state ID (see [[Minecraft Wiki:Projects/wiki.vg merge/Chunk Format#Block state registry|Chunk Format#Block state registry]])
 |-
 | 3009
 | Sniffer egg cracks
 | If 1, 3-6, if any other number, 1-3 particles will be spawned.
 |-
 | 3011
 | Trial spawner spawns mob (at spawner)
 |
 |-
 | 3012
 | Trial spawner spawns mob (at spawn location)
 |
 |-
 | 3013
 | Trial spawner detects player
 | Number of players nearby
 |-
 | 3014
 | Trial spawner ejects item
 | 
 |-
 | 3015
 | Vault activates
 | 
 |-
 | 3016
 | Vault deactivates
 | 
 |-
 | 3017
 | Vault ejects item
 | 
 |-
 | 3018
 | Cobweb weaved
 | 
 |-
 | 3019
 | Ominous trial spawner detects player
 | Number of players nearby
 |-
 | 3020
 | Trial spawner turns ominous
 | If 0, the sound will be played at 0.3 volume. Otherwise, it is played at full volume.
 |-
 | 3021
 | Ominous item spawner spawns item
 | 
 |}

Smoke directions:

{| class="wikitable"
 ! ID
 ! Direction
 |-
 | 0
 | Down
 |-
 | 1
 | Up
 |-
 | 2
 | North
 |-
 | 3
 | South
 |-
 | 4
 | West
 |-
 | 5
 | East
 |}

==== Particle ====

Displays the named particle

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="12"| ''protocol:''<br/><code>0x29</code><br/><br/>''resource:''<br/><code>level_particles</code>
 | rowspan="12"| Play
 | rowspan="12"| Client
 | Long Distance
 | {{Type|Boolean}}
 | If true, particle distance increases from 256 to 65536.
 |-
 | Always Visible
 | {{Type|Boolean}}
 | Whether this particle should always be visible.
 |-
 | X
 | {{Type|Double}}
 | X position of the particle.
 |-
 | Y
 | {{Type|Double}}
 | Y position of the particle.
 |-
 | Z
 | {{Type|Double}}
 | Z position of the particle.
 |-
 | Offset X
 | {{Type|Float}}
 | This is added to the X position after being multiplied by <code>random.nextGaussian()</code>.
 |-
 | Offset Y
 | {{Type|Float}}
 | This is added to the Y position after being multiplied by <code>random.nextGaussian()</code>.
 |-
 | Offset Z
 | {{Type|Float}}
 | This is added to the Z position after being multiplied by <code>random.nextGaussian()</code>.
 |-
 | Max Speed
 | {{Type|Float}}
 |
 |-
 | Particle Count
 | {{Type|Int}}
 | The number of particles to create.
 |-
 | Particle ID
 | {{Type|VarInt}}
 | The particle ID listed in [[Minecraft Wiki:Projects/wiki.vg merge/Particles|Particles]].
 |-
 | Data
 | Varies
 | Particle data as specified in [[Minecraft Wiki:Projects/wiki.vg merge/Particles|Particles]].
 |}

==== Update Light ====

Updates light levels for a chunk.  See [[Light]] for information on how lighting works in Minecraft.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="3"| ''protocol:''<br/><code>0x2A</code><br/><br/>''resource:''<br/><code>light_update</code>
 | rowspan="3"| Play
 | rowspan="3"| Client
 | Chunk X
 | {{Type|VarInt}}
 | Chunk coordinate (block coordinate divided by 16, rounded down)
 |-
 | Chunk Z
 | {{Type|VarInt}}
 | Chunk coordinate (block coordinate divided by 16, rounded down)
 |-
 | Data
 | {{Type|Light Data}}
 | 
 |}

A bit will never be set in both the block light mask and the empty block light mask, though it may be present in neither of them (if the block light does not need to be updated for the corresponding chunk section).  The same applies to the sky light mask and the empty sky light mask.

==== Login (play) ====

{{missing info|section|Although the number of portal cooldown ticks is included in this packet, the whole portal usage process is still dictated entirely by the server. What kind of effect does this value have on the client, if any?}}

See [[protocol encryption]] for information on logging in.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="22"| ''protocol:''<br/><code>0x2B</code><br/><br/>''resource:''<br/><code>login</code>
 | rowspan="22"| Play
 | rowspan="22"| Client
 | Entity ID
 | {{Type|Int}}
 | The player's Entity ID (EID).
 |-
 | Is hardcore
 | {{Type|Boolean}}
 |
 |-
 | Dimension Names
 | {{Type|Prefixed Array}} of {{Type|Identifier}}
 | Identifiers for all dimensions on the server.
 |-
 | Max Players
 | {{Type|VarInt}}
 | Was once used by the client to draw the player list, but now is ignored.
 |-
 | View Distance
 | {{Type|VarInt}}
 | Render distance (2-32).
 |-
 | Simulation Distance
 | {{Type|VarInt}}
 | The distance that the client will process specific things, such as entities.
 |-
 | Reduced Debug Info
 | {{Type|Boolean}}
 | If true, a vanilla client shows reduced information on the [[debug screen]].  For servers in development, this should almost always be false.
 |-
 | Enable respawn screen
 | {{Type|Boolean}}
 | Set to false when the doImmediateRespawn gamerule is true.
 |-
 | Do limited crafting
 | {{Type|Boolean}}
 | Whether players can only craft recipes they have already unlocked. Currently unused by the client.
 |-
 | Dimension Type
 | {{Type|VarInt}}
 | The ID of the type of dimension in the <code>minecraft:dimension_type</code> registry, defined by the Registry Data packet.
 |-
 | Dimension Name
 | {{Type|Identifier}}
 | Name of the dimension being spawned into.
 |-
 | Hashed seed
 | {{Type|Long}}
 | First 8 bytes of the SHA-256 hash of the world's seed. Used client side for biome noise 
 |-
 | Game mode
 | {{Type|Unsigned Byte}}
 | 0: Survival, 1: Creative, 2: Adventure, 3: Spectator.
 |-
 | Previous Game mode
 | {{Type|Byte}}
 | -1: Undefined (null), 0: Survival, 1: Creative, 2: Adventure, 3: Spectator. The previous game mode. Vanilla client uses this for the debug (F3 + N & F3 + F4) game mode switch. (More information needed)
 |-
 | Is Debug
 | {{Type|Boolean}}
 | True if the world is a [[debug mode]] world; debug mode worlds cannot be modified and have predefined blocks.
 |-
 | Is Flat
 | {{Type|Boolean}}
 | True if the world is a [[superflat]] world; flat worlds have different void fog and a horizon at y=0 instead of y=63.
 |-
 | Has death location
 | {{Type|Boolean}}
 | If true, then the next two fields are present.
 |-
 | Death dimension name
 | {{Type|Optional}} {{Type|Identifier}}
 | Name of the dimension the player died in.
 |-
 | Death location
 | {{Type|Optional}} {{Type|Position}}
 | The location that the player died at.
 |-
 | Portal cooldown
 | {{Type|VarInt}}
 | The number of ticks until the player can use the portal again.
 |-
 | Sea level
 | {{Type|VarInt}}
 |
 |-
 | Enforces Secure Chat
 | {{Type|Boolean}}
 |
 |}

==== Map Data ====

Updates a rectangular area on a [[map]] item.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! colspan="2"| Field Name
 ! colspan="2"| Field Type
 ! Notes
 |-
 | rowspan="13"| ''protocol:''<br/><code>0x2C</code><br/><br/>''resource:''<br/><code>map_item_data</code>
 | rowspan="13"| Play
 | rowspan="13"| Client
 | colspan="2"| Map ID
 | colspan="2"| {{Type|VarInt}}
 | Map ID of the map being modified
 |-
 | colspan="2"| Scale
 | colspan="2"| {{Type|Byte}}
 | From 0 for a fully zoomed-in map (1 block per pixel) to 4 for a fully zoomed-out map (16 blocks per pixel)
 |-
 | colspan="2"| Locked
 | colspan="2"| {{Type|Boolean}}
 | True if the map has been locked in a cartography table
 |-
 | rowspan="5"| Icon
 | Type
 | rowspan="5"| {{Type|Prefixed Optional}} {{Type|Prefixed Array}}
 | {{Type|VarInt}} {{Type|Enum}}
 | See below
 |-
 | X
 | {{Type|Byte}}
 | Map coordinates: -128 for furthest left, +127 for furthest right
 |-
 | Z
 | {{Type|Byte}}
 | Map coordinates: -128 for highest, +127 for lowest
 |-
 | Direction
 | {{Type|Byte}}
 | 0-15
 |-
 | Display Name
 | {{Type|Prefixed Optional}} {{Type|Text Component}}
 | 
 |-
 | rowspan="5"| Color Patch
 | Columns
 | colspan="2"| {{Type|Unsigned Byte}}
 | Number of columns updated
 |-
 | Rows
 | colspan="2"| {{Type|Optional}} {{Type|Unsigned Byte}}
 | Only if Columns is more than 0; number of rows updated
 |-
 | X
 | colspan="2"| {{Type|Optional}} {{Type|Unsigned Byte}}
 | Only if Columns is more than 0; x offset of the westernmost column
 |-
 | Z
 | colspan="2"| {{Type|Optional}} {{Type|Unsigned Byte}}
 | Only if Columns is more than 0; z offset of the northernmost row
 |-
 | Data
 | colspan="2"| {{Type|Optional}} {{Type|Prefixed Array}} of {{Type|Unsigned Byte}}
 | Only if Columns is more than 0; see [[Map item format]]
 |}

For icons, a direction of 0 is a vertical icon and increments by 22.5&deg; (360/16).

Types are based off of rows and columns in <code>map_icons.png</code>:

{| class="wikitable"
 |-
 ! Icon type
 ! Result
 |-
 | 0
 | White arrow (players)
 |-
 | 1
 | Green arrow (item frames)
 |-
 | 2
 | Red arrow
 |-
 | 3
 | Blue arrow
 |-
 | 4
 | White cross
 |-
 | 5
 | Red pointer
 |-
 | 6
 | White circle (off-map players)
 |-
 | 7
 | Small white circle (far-off-map players)
 |-
 | 8
 | Mansion
 |-
 | 9
 | Monument
 |-
 | 10
 | White Banner
 |-
 | 11
 | Orange Banner
 |-
 | 12
 | Magenta Banner
 |-
 | 13
 | Light Blue Banner
 |-
 | 14
 | Yellow Banner
 |-
 | 15
 | Lime Banner
 |-
 | 16
 | Pink Banner
 |-
 | 17
 | Gray Banner
 |-
 | 18
 | Light Gray Banner
 |-
 | 19
 | Cyan Banner
 |-
 | 20
 | Purple Banner
 |-
 | 21
 | Blue Banner
 |-
 | 22
 | Brown Banner
 |-
 | 23
 | Green Banner
 |-
 | 24
 | Red Banner
 |-
 | 25
 | Black Banner
 |-
 | 26
 | Treasure marker
 |-
 | 27
 | Desert Village
 |-
 | 28
 | Plains Village
 |-
 | 29
 | Savanna Village
 |-
 | 30
 | Snowy Village
 |-
 | 31
 | Taiga Village
 |-
 | 32
 | Jungle Temple
 |-
 | 33
 | Swamp Hut
 |-
 | 34
 | Trial Chambers
 |}

==== Merchant Offers ====

The list of trades a villager NPC is offering.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! colspan="2"| Field Name
 ! colspan="2"| Field Type
 ! Notes
 |-
 | rowspan="15"| ''protocol:''<br/><code>0x2D</code><br/><br/>''resource:''<br/><code>merchant_offers</code>
 | rowspan="15"| Play
 | rowspan="15"| Client
 | colspan="2"| Window ID
 | colspan="2"| {{Type|VarInt}}
 | The ID of the window that is open; this is an int rather than a byte.
 |-
 | rowspan="10"| Trades
 | Input item 1
 | rowspan="10"| {{Type|Prefixed Array}}
 | Trade Item
 | See below. The first item the player has to supply for this villager trade. The count of the item stack is the default "price" of this trade.
 |-
 | Output item
 | {{Type|Slot}}
 | The item the player will receive from this villager trade.
 |-
 | Input item 2
 | {{Type|Prefixed Optional}} Trade Item
 | The second item the player has to supply for this villager trade.
 |-
 | Trade disabled
 | {{Type|Boolean}}
 | True if the trade is disabled; false if the trade is enabled.
 |-
 | Number of trade uses
 | {{Type|Int}}
 | Number of times the trade has been used so far. If equal to the maximum number of trades, the client will display a red X.
 |-
 | Maximum number of trade uses
 | {{Type|Int}}
 | Number of times this trade can be used before it's exhausted.
 |-
 | XP
 | {{Type|Int}}
 | Amount of XP the villager will earn each time the trade is used.
 |-
 | Special Price
 | {{Type|Int}}
 | Can be zero or negative. The number is added to the price when an item is discounted due to player reputation or other effects.
 |-
 | Price Multiplier
 | {{Type|Float}}
 | Can be low (0.05) or high (0.2). Determines how much demand, player reputation, and temporary effects will adjust the price.
 |-
 | Demand
 | {{Type|Int}}
 | If positive, causes the price to increase. Negative values seem to be treated the same as zero.
 |-
 | colspan="2"| Villager level
 | colspan="2"| {{Type|VarInt}}
 | Appears on the trade GUI; meaning comes from the translation key <code>merchant.level.</code> + level.
1: Novice, 2: Apprentice, 3: Journeyman, 4: Expert, 5: Master.
 |-
 | colspan="2"| Experience
 | colspan="2"| {{Type|VarInt}}
 | Total experience for this villager (always 0 for the wandering trader).
 |-
 | colspan="2"| Is regular villager
 | colspan="2"| {{Type|Boolean}}
 | True if this is a regular villager; false for the wandering trader.  When false, hides the villager level and some other GUI elements.
 |-
 | colspan="2"| Can restock
 | colspan="2"| {{Type|Boolean}}
 | True for regular villagers and false for the wandering trader. If true, the "Villagers restock up to two times per day." message is displayed when hovering over disabled trades.
 |}

Trade Item:
{| class="wikitable"
 ! colspan="2"| Field Name
 ! colspan="2"| Field Type
 ! Meaning
 |-
 | colspan="2"| Item ID
 | colspan="2"| {{Type|VarInt}}
 | The [[Java Edition data values#Blocks|item ID]]. Item IDs are distinct from block IDs; see [[Minecraft Wiki:Projects/wiki.vg merge/Data Generators|Data Generators]] for more information.
 |-
 | colspan="2"| Item Count
 | colspan="2"| {{Type|VarInt}}
 | The item count.
 |-
 | rowspan="2"| Components
 | Component type
 | rowspan="2"| {{Type|Prefixed Array}}
 | {{Type|VarInt}} {{Type|Enum}}
 | The type of component. See [[Java_Edition_protocol/Slot_Data#Structured_components|Structured components]] for more detail.
 |-
 | Component data
 | Varies
 | The component-dependent data. See [[Java_Edition_protocol/Slot_Data#Structured_components|Structured components]] for more detail.
 |-
 |}

Modifiers can increase or decrease the number of items for the first input slot. The second input slot and the output slot never change the number of items. The number of items may never be less than 1, and never more than the stack size. If special price and demand are both zero, only the default price is displayed. If either is non-zero, then the adjusted price is displayed next to the crossed-out default price. The adjusted prices is calculated as follows:

Adjusted price = default price + floor(default price x multiplier x demand) + special price

[[File:1.14-merchant-slots.png|thumb|The merchant UI, for reference]]
{{-}}

==== Update Entity Position ====

This packet is sent by the server when an entity moves a small distance. The change in position is represented as a [[#Fixed-point numbers|fixed-point number]] with 12 fraction bits and 4 integer bits. As such, the maximum movement distance along each axis is 8 blocks in the negative direction, or 7.999755859375 blocks in the positive direction. If the movement exceeds these limits, [[#Teleport Entity|Teleport Entity]] should be sent instead.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="5"| ''protocol:''<br/><code>0x2E</code><br/><br/>''resource:''<br/><code>move_entity_pos</code>
 | rowspan="5"| Play
 | rowspan="5"| Client
 | Entity ID
 | {{Type|VarInt}}
 |
 |-
 | Delta X
 | {{Type|Short}}
 | Change in X position as <code>currentX * 4096 - prevX * 4096</code>.
 |-
 | Delta Y
 | {{Type|Short}}
 | Change in Y position as <code>currentY * 4096 - prevY * 4096</code>.
 |-
 | Delta Z
 | {{Type|Short}}
 | Change in Z position as <code>currentZ * 4096 - prevZ * 4096</code>.
 |-
 | On Ground
 | {{Type|Boolean}}
 |
 |}

==== Update Entity Position and Rotation ====

This packet is sent by the server when an entity rotates and moves. See [[#Update Entity Position]] for how the position is encoded.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="7"| ''protocol:''<br/><code>0x2F</code><br/><br/>''resource:''<br/><code>move_entity_pos_rot</code>
 | rowspan="7"| Play
 | rowspan="7"| Client
 | Entity ID
 | {{Type|VarInt}}
 |
 |-
 | Delta X
 | {{Type|Short}}
 | Change in X position as <code>currentX * 4096 - prevX * 4096</code>.
 |-
 | Delta Y
 | {{Type|Short}}
 | Change in Y position as <code>currentY * 4096 - prevY * 4096</code>.
 |-
 | Delta Z
 | {{Type|Short}}
 | Change in Z position as <code>currentZ * 4096 - prevZ * 4096</code>.
 |-
 | Yaw
 | {{Type|Angle}}
 | New angle, not a delta.
 |-
 | Pitch
 | {{Type|Angle}}
 | New angle, not a delta.
 |-
 | On Ground
 | {{Type|Boolean}}
 |
 |}

==== Move Minecart Along Track ====

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! colspan="2"| Field Name
 ! colspan="2"| Field Type
 ! colspan="2"| Notes
 |-
 | rowspan="10"| ''protocol:''<br/><code>0x30</code><br/><br/>''resource:''<br/><code>move_minecart_along_track</code>
 | rowspan="10"| Play
 | rowspan="10"| Client
 | colspan="2"| Entity ID
 | colspan="2"| {{Type|VarInt}}
 |
 |-
 | rowspan="9"| Steps
 | X
 | rowspan="9"| {{Type|Prefixed Array}}
 | {{Type|Double}}
 | 
 |-
 | Y
 | {{Type|Double}}
 | 
 |-
 | Z
 | {{Type|Double}}
 | 
 |-
 | Velocity X
 | {{Type|Double}}
 |
 |-
 | Velocity Y
 | {{Type|Double}}
 |
 |-
 | Velocity Z
 | {{Type|Double}}
 |
 |-
 | Yaw
 | {{Type|Angle}}
 |
 |-
 | Pitch
 | {{Type|Angle}}
 |
 |-
 | Weight
 | {{Type|Float}}
|}

==== Update Entity Rotation ====

This packet is sent by the server when an entity rotates.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="4"| ''protocol:''<br/><code>0x31</code><br/><br/>''resource:''<br/><code>move_entity_rot</code>
 | rowspan="4"| Play
 | rowspan="4"| Client
 | Entity ID
 | {{Type|VarInt}}
 |
 |-
 | Yaw
 | {{Type|Angle}}
 | New angle, not a delta.
 |-
 | Pitch
 | {{Type|Angle}}
 | New angle, not a delta.
 |-
 | On Ground
 | {{Type|Boolean}}
 |
 |}

==== Move Vehicle ====

Note that all fields use absolute positioning and do not allow for relative positioning.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="5"| ''protocol:''<br/><code>0x32</code><br/><br/>''resource:''<br/><code>move_vehicle</code>
 | rowspan="5"| Play
 | rowspan="5"| Client
 | X
 | {{Type|Double}}
 | Absolute position (X coordinate).
 |-
 | Y
 | {{Type|Double}}
 | Absolute position (Y coordinate).
 |-
 | Z
 | {{Type|Double}}
 | Absolute position (Z coordinate).
 |-
 | Yaw
 | {{Type|Float}}
 | Absolute rotation on the vertical axis, in degrees.
 |-
 | Pitch
 | {{Type|Float}}
 | Absolute rotation on the horizontal axis, in degrees.
 |}

==== Open Book ====

Sent when a player right clicks with a signed book. This tells the client to open the book GUI.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | ''protocol:''<br/><code>0x33</code><br/><br/>''resource:''<br/><code>open_book</code>
 | Play
 | Client
 | Hand
 | {{Type|VarInt}} {{Type|Enum}}
 | 0: Main hand, 1: Off hand .
 |}

==== Open Screen ====

This is sent to the client when it should open an inventory, such as a chest, workbench, furnace, or other container. Resending this packet with already existing window id, will update the window title and window type without closing the window.

This message is not sent to clients opening their own inventory, nor do clients inform the server in any way when doing so. From the server's perspective, the inventory is always "open" whenever no other windows are.

For horses, use [[#Open Horse Screen|Open Horse Screen]].

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="3"| ''protocol:''<br/><code>0x34</code><br/><br/>''resource:''<br/><code>open_screen</code>
 | rowspan="3"| Play
 | rowspan="3"| Client
 | Window ID
 | {{Type|VarInt}}
 | An identifier for the window to be displayed. vanilla server implementation is a counter, starting at 1. There can only be one window at a time; this is only used to ignore outdated packets targeting already-closed windows. Note also that the Window ID field in most other packets is only a single byte, and indeed, the vanilla server wraps around after 100.
 |-
 | Window Type
 | {{Type|VarInt}}
 | The window type to use for display. Contained in the <code>minecraft:menu</code> registry; see [[Minecraft Wiki:Projects/wiki.vg merge/Inventory|Inventory]] for the different values.
 |-
 | Window Title
 | {{Type|Text Component}}
 | The title of the window.
 |}

==== Open Sign Editor ====

Sent when the client has placed a sign and is allowed to send [[#Update Sign|Update Sign]].  There must already be a sign at the given location (which the client does not do automatically) - send a [[#Block Update|Block Update]] first.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="2" | ''protocol:''<br/><code>0x35</code><br/><br/>''resource:''<br/><code>open_sign_editor</code>
 | rowspan="2" | Play
 | rowspan="2" | Client
 | Location
 | {{Type|Position}}
 |
 |-
 | Is Front Text
 | {{Type|Boolean}}
 | Whether the opened editor is for the front or on the back of the sign
 |}

==== Ping (play) ====

Packet is not used by the vanilla server. When sent to the client, client responds with a [[#Pong (play)|Pong]] packet with the same id.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | ''protocol:''<br/><code>0x36</code><br/><br/>''resource:''<br/><code>ping</code>
 | Play
 | Client
 | ID
 | {{Type|Int}}
 |
 |}

==== Ping Response (play) ====

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | ''protocol:''<br/><code>0x37</code><br/><br/>''resource:''<br/><code>pong_response</code>
 | Play
 | Client
 | Payload
 | {{Type|Long}}
 | Should be the same as sent by the client.
 |}

==== Place Ghost Recipe ====

Response to the serverbound packet ([[#Place Recipe|Place Recipe]]), with the same recipe ID. Appears to be used to notify the UI.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="2"| ''protocol:''<br/><code>0x38</code><br/><br/>''resource:''<br/><code>place_ghost_recipe</code>
 | rowspan="2"| Play
 | rowspan="2"| Client
 | Window ID
 | {{Type|VarInt}}
 |
 |-
 | Recipe Display
 | {{Type|Recipe Display}}
 |
 |}

==== Player Abilities (clientbound) ====

The latter 2 floats are used to indicate the flying speed and field of view respectively, while the first byte is used to determine the value of 4 booleans.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="3"| ''protocol:''<br/><code>0x39</code><br/><br/>''resource:''<br/><code>player_abilities</code>
 | rowspan="3"| Play
 | rowspan="3"| Client
 | Flags
 | {{Type|Byte}}
 | Bit field, see below.
 |-
 | Flying Speed
 | {{Type|Float}}
 | 0.05 by default.
 |-
 | Field of View Modifier
 | {{Type|Float}}
 | Modifies the field of view, like a speed potion. A vanilla server will use the same value as the movement speed sent in the [[#Update Attributes|Update Attributes]] packet, which defaults to 0.1 for players.
 |}

About the flags:

{| class="wikitable"
 |-
 ! Field
 ! Bit
 |-
 | Invulnerable
 | 0x01
 |-
 | Flying
 | 0x02
 |-
 | Allow Flying
 | 0x04
 |-
 | Creative Mode (Instant Break)
 | 0x08
 |}

If Flying is set but Allow Flying is unset, the player is unable to stop flying.

==== Player Chat Message ====

{{Main|Minecraft_Wiki:Projects/wiki.vg_merge/Chat}}

Sends the client a chat message from a player. 

Currently a lot is unknown about this packet, blank descriptions are for those that are unknown

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Sector
 ! colspan="2"| Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="15"| ''protocol:''<br/><code>0x3A</code><br/><br/>''resource:''<br/><code>player_chat</code>
 | rowspan="15"| Play
 | rowspan="15"| Client
 | rowspan="4"| Header
 | colspan="2"| Global Index
 | {{Type|VarInt}}
 | 
 |-
 | colspan="2"| Sender
 | {{Type|UUID}}
 | Used by the vanilla client for the disableChat launch option. Setting both longs to 0 will always display the message regardless of the setting.
 |-
 | colspan="2"| Index
 | {{Type|VarInt}}
 | 
 |-
 | colspan="2"| Message Signature bytes
 | {{Type|Prefixed Optional}} {{Type|Byte Array}} (256)
 | Cryptography, the signature consists of the Sender UUID, Session UUID from the [[#Player Session|Player Session]] packet, Index, Salt, Timestamp in epoch seconds, the length of the original chat content, the original content itself, the length of Previous Messages, and all of the Previous message signatures. These values are hashed with [https://en.wikipedia.org/wiki/SHA-2 SHA-256] and signed using the [https://en.wikipedia.org/wiki/RSA_(cryptosystem) RSA] cryptosystem. Modifying any of these values in the packet will cause this signature to fail. This buffer is always 256 bytes long and it is not length-prefixed.
 |-
 | rowspan="3"| Body
 | colspan="2"| Message
 | {{Type|String}} (256)
 | Raw (optionally) signed sent message content.
This is used as the <code>content</code> parameter when formatting the message on the client.
 |-
 | colspan="2"| Timestamp
 | {{Type|Long}}
 | Represents the time the message was signed as milliseconds since the [https://en.wikipedia.org/wiki/Unix_time epoch], used to check if the message was received within 2 minutes of it being sent.
 |-
 | colspan="2"| Salt
 | {{Type|Long}}
 | Cryptography, used for validating the message signature. 
 |-
 | rowspan="2"| {{Type|Prefixed Array}} (20)
 | Message ID
 | {{Type|VarInt}}
 | The message Id + 1, used for validating message signature. The next field is present only when value of this field is equal to 0.
 |-
 | Signature
 | {{Type|Optional}} {{Type|Byte Array}} (256)
 | The previous message's signature. Contains the same type of data as <code>Message Signature bytes</code> (256 bytes) above. Not length-prefxied.
 |-
 | rowspan="3"| Other
 | colspan="2"| Unsigned Content
 | {{Type|Prefixed Optional}} {{Type|Text Component}}
 | 
 |-
 | colspan="2"| Filter Type
 | {{Type|VarInt}} {{Type|Enum}}
 | If the message has been filtered
 |-
 | colspan="2"| Filter Type Bits
 | {{Type|Optional}} {{Type|BitSet}}
 | Only present if the Filter Type is Partially Filtered. Specifies the indexes at which characters in the original message string should be replaced with the <code>#</code> symbol (i.e. filtered) by the vanilla client
 |-
 | rowspan="3"| Chat Formatting
 | colspan="2"| Chat Type
 | {{Type|ID or}} {{Type|Chat Type}}
 | Either the type of chat in the <code>minecraft:chat_type</code> registry, defined by the [[Java Edition protocol#Registry_Data|Registry Data]] packet, or an inline definition.
 |-
 | colspan="2"| Sender Name
 | {{Type|Text Component}}
 | The name of the one sending the message, usually the sender's display name.
This is used as the <code>sender</code> parameter when formatting the message on the client.
 |-
 | colspan="2"| Target Name
 | {{Type|Prefixed Optional}} {{Type|Text Component}}
 | The name of the one receiving the message, usually the receiver's display name.
This is used as the <code>target</code> parameter when formatting the message on the client.
 |}
[[File:MinecraftChat.drawio4.png|thumb|Player Chat Handling Logic]]

Filter Types:

The filter type mask should NOT be specified unless partially filtered is selected

{| class="wikitable"
 ! ID
 ! Name
 ! Description
 |-
 | 0
 | PASS_THROUGH
 | Message is not filtered at all
 |-
 | 1
 | FULLY_FILTERED
 | Message is fully filtered
 |-
 | 2
 | PARTIALLY_FILTERED
 | Only some characters in the message are filtered
 |}

==== End Combat ====

Unused by the vanilla client.  This data was once used for twitch.tv metadata circa 1.8.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="1"| ''protocol:''<br/><code>0x3B</code><br/><br/>''resource:''<br/><code>player_combat_end</code>
 | rowspan="1"| Play
 | rowspan="1"| Client
 | Duration
 | {{Type|VarInt}}
 | Length of the combat in ticks.
 |}

==== Enter Combat ====

Unused by the vanilla client.  This data was once used for twitch.tv metadata circa 1.8.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="1"| ''protocol:''<br/><code>0x3C</code><br/><br/>''resource:''<br/><code>player_combat_enter</code>
 | rowspan="1"| Play
 | rowspan="1"| Client
 | colspan="3"| ''no fields''
 |}

==== Combat Death ====

Used to send a respawn screen.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="2"| ''protocol:''<br/><code>0x3D</code><br/><br/>''resource:''<br/><code>player_combat_kill</code>
 | rowspan="2"| Play
 | rowspan="2"| Client
 | Player ID
 | {{Type|VarInt}}
 | Entity ID of the player that died (should match the client's entity ID).
 |-
 | Message
 | {{Type|Text Component}}
 | The death message.
 |}

==== Player Info Remove ====

Used by the server to remove players from the player list.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | ''protocol:''<br/><code>0x3E</code><br/><br/>''resource:''<br/><code>player_info_remove</code>
 | Play
 | Client
 | UUIDs
 | {{Type|Prefixed Array}} of {{Type|UUID}}
 | UUIDs of players to remove.
 |}

==== Player Info Update ====

Sent by the server to update the user list (<tab> in the client).

{{Warning|The EnumSet type is only used here and it is currently undocumented}}

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! colspan="2"| Field Name
 ! colspan="2"| Field Type
 ! Notes
 |-
 | rowspan="3"| ''protocol:''<br/><code>0x3F</code><br/><br/>''resource:''<br/><code>player_info_update</code>
 | rowspan="3"| Play
 | rowspan="3"| Client
 | colspan="2"| Actions
 | colspan="2"| {{Type|EnumSet}}
 | Determines what actions are present.
 |-
 | rowspan="2" | Players
 | UUID
 | rowspan="2" | {{Type|Prefixed Array}}
 | {{Type|UUID}}
 | The player UUID
 |-
 | Player Actions
 | {{Type|Array}} of [[#player-info:player-actions|Player&nbsp;Actions]]
 | The length of this array is determined by the number of [[#player-info:player-actions|Player Actions]] that give a non-zero value when applying its mask to the actions flag. For example given the decimal number 5, binary 00000101. The masks 0x01 and 0x04 would return a non-zero value, meaning the Player Actions array would include two actions: Add Player and Update Game Mode.
 |}
 

{| class="wikitable"
 |+ id="player-info:player-actions" | Player Actions
 ! Action
 ! Mask
 ! colspan="2" | Field Name
 ! colspan="2" | Type
 ! Notes
 |-
 | rowspan="4" | Add Player
 | rowspan="4" | 0x01
 | colspan="2" | Name
 | colspan="2" | {{Type|String}} (16)
 |-
 | rowspan="3" | Property
 | Name
 | rowspan="3"| {{Type|Prefixed Array}} (16)
 | {{Type|String}} (64)
 |
 |-
 | Value
 | {{Type|String}} (32767)
 |
 |-
 | Signature
 | {{Type|Prefixed Optional}} {{Type|String}} (1024)
 | 
 |-
 | rowspan="4" | Initialize Chat
 | rowspan="4" | 0x02
 | rowspan="4" | Data
 | Chat session ID
 | rowspan="4" | {{Type|Prefixed Optional}}
 | {{Type|UUID}}
 | 
 |-
 | Public key expiry time
 | {{Type|Long}}
 | Key expiry time, as a UNIX timestamp in milliseconds. Only sent if Has Signature Data is true.
 |-
 | Encoded public key
 | {{Type|Prefixed Array}} (512) of {{Type|Byte}}
 | The player's public key, in bytes. Only sent if Has Signature Data is true.
 |-
 | Public key signature
 | {{Type|Prefixed Array}} (4096) of {{Type|Byte}}
 | The public key's digital signature. Only sent if Has Signature Data is true.
 |-
 | Update Game Mode
 | 0x04
 | colspan="2" | Game Mode
 | colspan="2" | {{Type|VarInt}}
 |-
 | Update Listed
 | 0x08
 | colspan="2" | Listed
 | colspan="2" | {{Type|Boolean}}
 | Whether the player should be listed on the player list.
 |-
 | Update Latency
 | 0x10
 | colspan="2" | Ping
 | colspan="2" | {{Type|VarInt}}
 | Measured in milliseconds.
 |-
 | Update Display Name
 | 0x20
 | colspan="2" | Display Name
 | colspan="2" | {{Type|Prefixed Optional}} {{Type|Text Component}}
 | Only sent if Has Display Name is true.
 |-
 | Update List Priority
 | 0x40
 | colspan="2" | Priority
 | colspan="2" | {{Type|VarInt}}
 | See below.
 |-
 | Update Hat
 | 0x80
 | colspan="2" | Visible
 | colspan="2" | {{Type|Boolean}}
 | Whether the player's hat skin layer is shown.
 |}

The properties included in this packet are the same as in [[#Login Success|Login Success]], for the current player.

Ping values correspond with icons in the following way:
* A ping that negative (i.e. not known to the server yet) will result in the no connection icon.
* A ping under 150 milliseconds will result in 5 bars
* A ping under 300 milliseconds will result in 4 bars
* A ping under 600 milliseconds will result in 3 bars
* A ping under 1000 milliseconds (1 second) will result in 2 bars
* A ping greater than or equal to 1 second will result in 1 bar.

The order of players in the player list is determined as follows:
* Players with higher priorities are sorted before those with lower priorities.
* Among players of equal priorities, spectators are sorted after non-spectators.
* Within each of those groups, players are sorted into teams. The teams are ordered case-sensitively by team name in ascending order. Players with no team are listed first.
* The players of each team (and non-team) are sorted case-insensitively by name in ascending order.

==== Look At ====

Used to rotate the client player to face the given location or entity (for <code>/teleport [<targets>] <x> <y> <z> facing</code>).

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="8"| ''protocol:''<br/><code>0x40</code><br/><br/>''resource:''<br/><code>player_look_at</code>
 | rowspan="8"| Play
 | rowspan="8"| Client
 |-
 | Feet/eyes
 | {{Type|VarInt}} {{Type|Enum}}
 | Values are feet=0, eyes=1.  If set to eyes, aims using the head position; otherwise aims using the feet position.
 |-
 | Target x
 | {{Type|Double}}
 | x coordinate of the point to face towards.
 |-
 | Target y
 | {{Type|Double}}
 | y coordinate of the point to face towards.
 |-
 | Target z
 | {{Type|Double}}
 | z coordinate of the point to face towards.
 |-
 | Is entity
 | {{Type|Boolean}}
 | If true, additional information about an entity is provided.
 |-
 | Entity ID
 | {{Type|Optional}} {{Type|VarInt}}
 | Only if is entity is true &mdash; the entity to face towards.
 |-
 | Entity feet/eyes
 | {{Type|Optional}} {{Type|VarInt}} {{Type|Enum}}
 | Whether to look at the entity's eyes or feet.  Same values and meanings as before, just for the entity's head/feet.
 |}

If the entity given by entity ID cannot be found, this packet should be treated as if is entity was false.

==== Synchronize Player Position ====

Teleports the client, e.g. during login, when using an ender pearl, in response to invalid move packets, etc.

Due to latency, the server may receive outdated movement packets sent before the client was aware of the teleport. To account for this, the server ignores all movement packets from the client until a [[#Confirm Teleportation|Confirm Teleportation]] packet with an ID matching the one sent in the teleport packet is received.

Yaw is measured in degrees, and does not follow classical trigonometry rules. The unit circle of yaw on the XZ-plane starts at (0, 1) and turns counterclockwise, with 90 at (-1, 0), 180 at (0, -1) and 270 at (1, 0). Additionally, yaw is not clamped to between 0 and 360 degrees; any number is valid, including negative numbers and numbers greater than 360 (see [https://bugs.mojang.com/browse/MC-90097 MC-90097]).

Pitch is measured in degrees, where 0 is looking straight ahead, -90 is looking straight up, and 90 is looking straight down.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="10"| ''protocol:''<br/><code>0x41</code><br/><br/>''resource:''<br/><code>player_position</code>
 | rowspan="10"| Play
 | rowspan="10"| Client
 | Teleport ID
 | {{Type|VarInt}}
 | Client should confirm this packet with [[#Confirm Teleportation|Confirm Teleportation]] containing the same Teleport ID.
 |-
 | X
 | {{Type|Double}}
 | Absolute or relative position, depending on Flags.
 |-
 | Y
 | {{Type|Double}}
 | Absolute or relative position, depending on Flags.
 |-
 | Z
 | {{Type|Double}}
 | Absolute or relative position, depending on Flags.
 |-
 | Velocity X
 | {{Type|Double}}
 |
 |-
 | Velocity Y
 | {{Type|Double}}
 |
 |-
 | Velocity Z
 | {{Type|Double}}
 |
 |-
 | Yaw
 | {{Type|Float}}
 | Absolute or relative rotation on the X axis, in degrees.
 |-
 | Pitch
 | {{Type|Float}}
 | Absolute or relative rotation on the Y axis, in degrees.
 |-
 | Flags
 | {{Type|Teleport Flags}}
 |
 |}

==== Player Rotation ====

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="2"| ''protocol:''<br/><code>0x42</code><br/><br/>''resource:''<br/><code>player_rotation</code>
 | rowspan="2"| Play
 | rowspan="2"| Client
 | Yaw
 | {{Type|Float}}
 | Rotation on the X axis, in degrees.
 |-
 | Pitch
 | {{Type|Float}}
 | Rotation on the Y axis, in degrees.
 |}

==== Recipe Book Add ====

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! colspan="2"| Field Name
 ! colspan="2"| Field Type
 ! colspan="2"| Notes
 |-
 | rowspan="7"| ''protocol:''<br/><code>0x43</code><br/><br/>''resource:''<br/><code>recipe_book_add</code>
 | rowspan="7"| Play
 | rowspan="7"| Client
 | rowspan="6"| Recipes
 | Recipe ID
 | rowspan="6"| {{Type|Prefixed Array}}
 | {{Type|VarInt}}
 | ID to assign to the recipe.
 |-
 | Display
 | {{Type|Recipe Display}}
 |
 |-
 | Group ID
 | {{Type|VarInt}}
 |
 |-
 | Category ID
 | {{Type|VarInt}}
 | ID in the <code>minecraft:recipe_book_category</code> registry.
 |-
 | Ingredients
 | {{Type|Prefixed Optional}} {{Type|Prefixed Array}} of {{Type|ID Set}}
 | IDs in the <code>minecraft:item</code> registry, or an inline definition.
 |-
 | Flags
 | {{Type|Byte}}
 | 0x01: show notification; 0x02: highlight as new
 |-
 | colspan="2"| Replace
 | colspan="2"| {{Type|Boolean}}
 | Replace or Add to known recipes
 |}

==== Recipe Book Remove ====

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | ''protocol:''<br/><code>0x44</code><br/><br/>''resource:''<br/><code>recipe_book_remove</code>
 | Play
 | Client
 | Recipes
 | {{Type|Prefixed Array}} of {{Type|VarInt}}
 | IDs of recipes to remove.
 |}

==== Recipe Book Settings ====

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="8"| ''protocol:''<br/><code>0x45</code><br/><br/>''resource:''<br/><code>recipe_book_settings</code>
 | rowspan="8"| Play
 | rowspan="8"| Client
 | Crafting Recipe Book Open
 | {{Type|Boolean}}
 | If true, then the crafting recipe book will be open when the player opens its inventory.
 |-
 | Crafting Recipe Book Filter Active
 | {{Type|Boolean}}
 | If true, then the filtering option is active when the players opens its inventory.
 |-
 | Smelting Recipe Book Open
 | {{Type|Boolean}}
 | If true, then the smelting recipe book will be open when the player opens its inventory.
 |-
 | Smelting Recipe Book Filter Active
 | {{Type|Boolean}}
 | If true, then the filtering option is active when the players opens its inventory.
 |-
 | Blast Furnace Recipe Book Open
 | {{Type|Boolean}}
 | If true, then the blast furnace recipe book will be open when the player opens its inventory.
 |-
 | Blast Furnace Recipe Book Filter Active
 | {{Type|Boolean}}
 | If true, then the filtering option is active when the players opens its inventory.
 |-
 | Smoker Recipe Book Open
 | {{Type|Boolean}}
 | If true, then the smoker recipe book will be open when the player opens its inventory.
 |-
 | Smoker Recipe Book Filter Active
 | {{Type|Boolean}}
 | If true, then the filtering option is active when the players opens its inventory.
 |}

==== Remove Entities ====

Sent by the server when an entity is to be destroyed on the client.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | ''protocol:''<br/><code>0x46</code><br/><br/>''resource:''<br/><code>remove_entities</code>
 | Play
 | Client
 | Entity IDs
 | {{Type|Prefixed Array}} of {{Type|VarInt}}
 | The list of entities to destroy.
 |}

==== Remove Entity Effect ====

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="2"| ''protocol:''<br/><code>0x47</code><br/><br/>''resource:''<br/><code>remove_mob_effect</code>
 | rowspan="2"| Play
 | rowspan="2"| Client
 | Entity ID
 | {{Type|VarInt}}
 |
 |-
 | Effect ID
 | {{Type|VarInt}}
 | See [[Status effect#Effect list|this table]].
 |}

==== Reset Score ====

This is sent to the client when it should remove a scoreboard item.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="2"| ''protocol:''<br/><code>0x48</code><br/><br/>''resource:''<br/><code>reset_score</code>
 | rowspan="2"| Play
 | rowspan="2"| Client
 | Entity Name
 | {{Type|String}} (32767)
 | The entity whose score this is. For players, this is their username; for other entities, it is their UUID.
 |-
 | Objective Name
 | {{Type|Prefixed Optional}} {{Type|String}} (32767)
 | The name of the objective the score belongs to.
 |}

==== Remove Resource Pack (play) ====

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | ''protocol:''<br/><code>0x49</code><br/><br/>''resource:''<br/><code>resource_pack_pop</code>
 | Play
 | Client
 | UUID
 | {{Type|Optional}} {{Type|UUID}}
 | The UUID of the resource pack to be removed.
 |}

==== Add Resource Pack (play) ====

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="5"| ''protocol:''<br/><code>0x4A</code><br/><br/>''resource:''<br/><code>resource_pack_push</code>
 | rowspan="5"| Play
 | rowspan="5"| Client
 | UUID
 | {{Type|UUID}}
 | The unique identifier of the resource pack.
 |-
 | URL
 | {{Type|String}} (32767)
 | The URL to the resource pack.
 |-
 | Hash
 | {{Type|String}} (40)
 | A 40 character hexadecimal, case-insensitive [[wikipedia:SHA-1|SHA-1]] hash of the resource pack file.<br />If it's not a 40 character hexadecimal string, the client will not use it for hash verification and likely waste bandwidth.
 |-
 | Forced
 | {{Type|Boolean}}
 | The vanilla client will be forced to use the resource pack from the server. If they decline they will be kicked from the server.
 |-
 | Prompt Message
 | {{Type|Prefixed Optional}} {{Type|Text Component}}
 | This is shown in the prompt making the client accept or decline the resource pack.
 |}

==== Respawn ====

{{missing info|section|Although the number of portal cooldown ticks is included in this packet, the whole portal usage process is still dictated entirely by the server. What kind of effect does this value have on the client, if any?}}

To change the player's dimension (overworld/nether/end), send them a respawn packet with the appropriate dimension, followed by prechunks/chunks for the new dimension, and finally a position and look packet. You do not need to unload chunks, the client will do it automatically.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="13"| ''protocol:''<br/><code>0x4B</code><br/><br/>''resource:''<br/><code>respawn</code>
 | rowspan="13"| Play
 | rowspan="13"| Client
 | Dimension Type
 | {{Type|VarInt}}
 | The ID of type of dimension in the <code>minecraft:dimension_type</code> registry, defined by the [[Java Edition protocol#Registry_data|Registry Data]] packet.
 |-
 | Dimension Name
 | {{Type|Identifier}}
 | Name of the dimension being spawned into.
 |-
 | Hashed seed
 | {{Type|Long}}
 | First 8 bytes of the SHA-256 hash of the world's seed. Used client side for biome noise
 |-
 | Game mode
 | {{Type|Unsigned Byte}}
 | 0: Survival, 1: Creative, 2: Adventure, 3: Spectator.
 |-
 | Previous Game mode
 | {{Type|Byte}}
 | -1: Undefined (null), 0: Survival, 1: Creative, 2: Adventure, 3: Spectator. The previous game mode. Vanilla client uses this for the debug (F3 + N & F3 + F4) game mode switch. (More information needed)
 |-
 | Is Debug
 | {{Type|Boolean}}
 | True if the world is a [[debug mode]] world; debug mode worlds cannot be modified and have predefined blocks.
 |-
 | Is Flat
 | {{Type|Boolean}}
 | True if the world is a [[superflat]] world; flat worlds have different void fog and a horizon at y=0 instead of y=63.
 |-
 | Has death location
 | {{Type|Boolean}}
 | If true, then the next two fields are present.
 |-
 | Death dimension Name
 | {{Type|Optional}} {{Type|Identifier}}
 | Name of the dimension the player died in.
 |-
 | Death location
 | {{Type|Optional}} {{Type|Position}}
 | The location that the player died at.
 |-
 | Portal cooldown
 | {{Type|VarInt}}
 | The number of ticks until the player can use the portal again.
 |-
 | Sea level
 | {{Type|VarInt}}
 |
 |- 
 | Data kept
 | {{Type|Byte}}
 | Bit mask. 0x01: Keep attributes, 0x02: Keep metadata. Tells which data should be kept on the client side once the player has respawned.
In the vanilla implementation, this is context dependent:
* normal respawns (after death) keep no data;
* exiting the end poem/credits keeps the attributes;
* other dimension changes (portals or teleports) keep all data.
 |}

{{warning|Avoid changing player's dimension to same dimension they were already in unless they are dead. If you change the dimension to one they are already in, weird bugs can occur, such as the player being unable to attack other players in new world (until they die and respawn).

Before 1.16, if you must respawn a player in the same dimension without killing them, send two respawn packets, one to a different world and then another to the world you want. You do not need to complete the first respawn; it only matters that you send two packets.}}

==== Set Head Rotation ====

Changes the direction an entity's head is facing.

While sending the Entity Look packet changes the vertical rotation of the head, sending this packet appears to be necessary to rotate the head horizontally.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="2"| ''protocol:''<br/><code>0x4C</code><br/><br/>''resource:''<br/><code>rotate_head</code>
 | rowspan="2"| Play
 | rowspan="2"| Client
 | Entity ID
 | {{Type|VarInt}}
 |
 |-
 | Head Yaw
 | {{Type|Angle}}
 | New angle, not a delta.
 |}

==== Update Section Blocks ====

Fired whenever 2 or more blocks are changed within the same chunk on the same tick.

{{Warning|Changing blocks in chunks not loaded by the client is unsafe (see note on [[#Block Update|Block Update]]).}}

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="2"| ''protocol:''<br/><code>0x4D</code><br/><br/>''resource:''<br/><code>section_blocks_update</code>
 | rowspan="2"| Play
 | rowspan="2"| Client
 | Chunk section position
 | {{Type|Long}}
 | Chunk section coordinate (encoded chunk x and z with each 22 bits, and section y with 20 bits, from left to right).
 |-
 | Blocks
 | {{Type|Prefixed Array}} of {{Type|VarLong}}
 | Each entry is composed of the block state id, shifted left by 12, and the relative block position in the chunk section (4 bits for x, z, and y, from left to right).
 |}

Chunk section position is encoded:
<syntaxhighlight lang="java">
((sectionX & 0x3FFFFF) << 42) | (sectionY & 0xFFFFF) | ((sectionZ & 0x3FFFFF) << 20);
</syntaxhighlight>
and decoded:
<syntaxhighlight lang="java">
sectionX = long >> 42;
sectionY = long << 44 >> 44;
sectionZ = long << 22 >> 42;
</syntaxhighlight>

Blocks are encoded:
<syntaxhighlight lang="java">
blockStateId << 12 | (blockLocalX << 8 | blockLocalZ << 4 | blockLocalY)
//Uses the local position of the given block position relative to its respective chunk section
</syntaxhighlight>
and decoded:
<syntaxhighlight lang="java">
blockStateId = long >> 12;
blockLocalX = (long >> 8) & 0xF;
blockLocalY = long & 0xF;
blockLocalZ = (long >> 4) & 0xF;
</syntaxhighlight>

==== Select Advancements Tab ====

Sent by the server to indicate that the client should switch advancement tab. Sent either when the client switches tab in the GUI or when an advancement in another tab is made.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | ''protocol:''<br/><code>0x4E</code><br/><br/>''resource:''<br/><code>select_advancements_tab</code>
 | Play
 | Client
 | Identifier
 | {{Type|Prefixed Optional}} {{Type|Identifier}}
 | See below.
 |}

The {{Type|Identifier}} must be one of the following if no custom data pack is loaded:

{| class="wikitable"
 ! Identifier
 |-
 | minecraft:story/root
 |-
 | minecraft:nether/root
 |-
 | minecraft:end/root
 |-
 | minecraft:adventure/root
 |-
 | minecraft:husbandry/root
 |}

If no or an invalid identifier is sent, the client will switch to the first tab in the GUI.

==== Server Data ====

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="2"| ''protocol:''<br/><code>0x4F</code><br/><br/>''resource:''<br/><code>server_data</code>
 | rowspan="2"| Play
 | rowspan="2"| Client
 | MOTD
 | {{Type|Text Component}}
 |
 |-
 | Icon
 | {{Type|Prefixed Optional}} {{Type|Prefixed Array}} of {{Type|Byte}}
 | Icon bytes in the PNG format.
 |}

==== Set Action Bar Text ====

Displays a message above the hotbar. Equivalent to [[#System Chat Message|System Chat Message]] with Overlay set to true, except that [[Minecraft Wiki:Projects/wiki.vg merge/Chat#Social Interactions (blocking)|chat message blocking]] isn't performed. Used by the vanilla server only to implement the <code>/title</code> command.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="1"| ''protocol:''<br/><code>0x50</code><br/><br/>''resource:''<br/><code>set_action_bar_text</code>
 | rowspan="1"| Play
 | rowspan="1"| Client
 | Action bar text
 | {{Type|Text Component}}
 |}

==== Set Border Center ====

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="2"| ''protocol:''<br/><code>0x51</code><br/><br/>''resource:''<br/><code>set_border_center</code>
 | rowspan="2"| Play
 | rowspan="2"| Client
 | X
 | {{Type|Double}}
 |
 |-
 | Z
 | {{Type|Double}}
 |
 |}

==== Set Border Lerp Size ====

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="3"| ''protocol:''<br/><code>0x52</code><br/><br/>''resource:''<br/><code>set_border_lerp_size</code>
 | rowspan="3"| Play
 | rowspan="3"| Client
 | Old Diameter
 | {{Type|Double}}
 | Current length of a single side of the world border, in meters.
 |-
 | New Diameter
 | {{Type|Double}}
 | Target length of a single side of the world border, in meters.
 |-
 | Speed
 | {{Type|VarLong}}
 | Number of real-time ''milli''seconds until New Diameter is reached. It appears that vanilla server does not sync world border speed to game ticks, so it gets out of sync with server lag. If the world border is not moving, this is set to 0.
 |}

==== Set Border Size ====

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="1"| ''protocol:''<br/><code>0x53</code><br/><br/>''resource:''<br/><code>set_border_size</code>
 | rowspan="1"| Play
 | rowspan="1"| Client
 | Diameter
 | {{Type|Double}}
 | Length of a single side of the world border, in meters.
 |}

==== Set Border Warning Delay ====

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="1"| ''protocol:''<br/><code>0x54</code><br/><br/>''resource:''<br/><code>set_border_warning_delay</code>
 | rowspan="1"| Play
 | rowspan="1"| Client
 | Warning Time
 | {{Type|VarInt}}
 | In seconds as set by <code>/worldborder warning time</code>.
 |}

==== Set Border Warning Distance ====

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="1"| ''protocol:''<br/><code>0x55</code><br/><br/>''resource:''<br/><code>set_border_warning_distance</code>
 | rowspan="1"| Play
 | rowspan="1"| Client
 | Warning Blocks
 | {{Type|VarInt}}
 | In meters.
 |}

==== Set Camera ====

Sets the entity that the player renders from. This is normally used when the player left-clicks an entity while in spectator mode.

The player's camera will move with the entity and look where it is looking. The entity is often another player, but can be any type of entity.  The player is unable to move this entity (move packets will act as if they are coming from the other entity).

If the given entity is not loaded by the player, this packet is ignored.  To return control to the player, send this packet with their entity ID.

The vanilla server resets this (sends it back to the default entity) whenever the spectated entity is killed or the player sneaks, but only if they were spectating an entity. It also sends this packet whenever the player switches out of spectator mode (even if they weren't spectating an entity).

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | ''protocol:''<br/><code>0x56</code><br/><br/>''resource:''<br/><code>set_camera</code>
 | Play
 | Client
 | Camera ID
 | {{Type|VarInt}}
 | ID of the entity to set the client's camera to.
 |}

The vanilla client also loads certain shaders for given entities:

* Creeper &rarr; <code>shaders/post/creeper.json</code>
* Spider (and cave spider) &rarr; <code>shaders/post/spider.json</code>
* Enderman &rarr; <code>shaders/post/invert.json</code>
* Anything else &rarr; the current shader is unloaded

==== Set Center Chunk ====

Sets the center position of the client's chunk loading area. The area is square-shaped, spanning 2 &times; server view distance + 7 chunks on both axes (width, not radius!). Since the area's width is always an odd number, there is no ambiguity as to which chunk is the center.

The vanilla client ignores attempts to send chunks located outside the loading area, and immediately unloads any existing chunks no longer inside it.

The center chunk is normally the chunk the player is in, but apart from the implications on chunk loading, the (vanilla) client takes no issue with this not being the case. Indeed, as long as chunks are sent only within the default loading area centered on the world origin, it is not necessary to send this packet at all. This may be useful for servers with small bounded worlds, such as minigames, since it ensures chunks never need to be resent after the client has joined, saving on bandwidth.

The vanilla server sends this packet whenever the player moves across a chunk border horizontally, and also (according to testing) for any integer change in the vertical axis, even if it doesn't go across a chunk section border.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="2"| ''protocol:''<br/><code>0x57</code><br/><br/>''resource:''<br/><code>set_chunk_cache_center</code>
 | rowspan="2"| Play
 | rowspan="2"| Client
 | Chunk X
 | {{Type|VarInt}}
 | Chunk X coordinate of the loading area center.
 |-
 | Chunk Z
 | {{Type|VarInt}}
 | Chunk Z coordinate of the loading area center.
 |}

==== Set Render Distance ====

Sent by the integrated singleplayer server when changing render distance.  This packet is sent by the server when the client reappears in the overworld after leaving the end.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | ''protocol:''<br/><code>0x58</code><br/><br/>''resource:''<br/><code>set_chunk_cache_radius</code>
 | Play
 | Client
 | View Distance
 | {{Type|VarInt}}
 | Render distance (2-32).
 |}

==== Set Cursor Item ====

Replaces or sets the inventory item that's being dragged with the mouse.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | ''protocol:''<br/><code>0x59</code><br/><br/>''resource:''<br/><code>set_cursor_item</code>
 | Play
 | Client
 | Carried item
 | {{Type|Slot}}
 |
 |}

==== Set Default Spawn Position ====

Sent by the server after login to specify the coordinates of the spawn point (the point at which players spawn at, and which the compass points to). It can be sent at any time to update the point compasses point at.

The client uses this as the default position of the player upon spawning, though it's a good idea to always override this default by sending [[#Synchronize Player Position|Synchronize Player Position]]. When converting the position to floating point, 0.5 is added to the x and z coordinates and 1.0 to the y coordinate, so as to place the player centered on top of the specified block position.

Before receiving this packet, the client uses the default position 8, 64, 8, and angle 0.0 (resulting in a default player spawn position of 8.5, 65.0, 8.5).

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="2"| ''protocol:''<br/><code>0x5A</code><br/><br/>''resource:''<br/><code>set_default_spawn_position</code>
 | rowspan="2"| Play
 | rowspan="2"| Client
 | Location
 | {{Type|Position}}
 | Spawn location.
 |-
 | Angle
 | {{Type|Float}}
 | The angle at which to respawn at.
 |}

==== Display Objective ====

This is sent to the client when it should display a scoreboard.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="2"| ''protocol:''<br/><code>0x5B</code><br/><br/>''resource:''<br/><code>set_display_objective</code>
 | rowspan="2"| Play
 | rowspan="2"| Client
 | Position
 | {{Type|VarInt}}
 | The position of the scoreboard. 0: list, 1: sidebar, 2: below name, 3 - 18: team specific sidebar, indexed as 3 + team color.
 |-
 | Score Name
 | {{Type|String}} (32767)
 | The unique name for the scoreboard to be displayed.
 |}

==== Set Entity Metadata ====

Updates one or more [[Java Edition protocol/Entity_metadata#Entity Metadata Format|metadata]] properties for an existing entity. Any properties not included in the Metadata field are left unchanged.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="2"| ''protocol:''<br/><code>0x5C</code><br/><br/>''resource:''<br/><code>set_entity_data</code>
 | rowspan="2"| Play
 | rowspan="2"| Client
 | Entity ID
 | {{Type|VarInt}}
 |
 |-
 | Metadata
 | [[Java Edition protocol/Entity_metadata#Entity Metadata Format|Entity Metadata]]
 |
 |}

==== Link Entities ====

This packet is sent when an entity has been [[Lead|leashed]] to another entity.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="2"| ''protocol:''<br/><code>0x5D</code><br/><br/>''resource:''<br/><code>set_entity_link</code>
 | rowspan="2"| Play
 | rowspan="2"| Client
 | Attached Entity ID
 | {{Type|Int}}
 | Attached entity's EID.
 |-
 | Holding Entity ID
 | {{Type|Int}}
 | ID of the entity holding the lead. Set to -1 to detach.
 |}

==== Set Entity Velocity ====

Velocity is in units of 1/8000 of a block per server tick (50ms); for example, -1343 would move (-1343 / 8000) = −0.167875 blocks per tick (or −3.3575 blocks per second).

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="4"| ''protocol:''<br/><code>0x5E</code><br/><br/>''resource:''<br/><code>set_entity_motion</code>
 | rowspan="4"| Play
 | rowspan="4"| Client
 | Entity ID
 | {{Type|VarInt}}
 |
 |-
 | Velocity X
 | {{Type|Short}}
 | Velocity on the X axis.
 |-
 | Velocity Y
 | {{Type|Short}}
 | Velocity on the Y axis.
 |-
 | Velocity Z
 | {{Type|Short}}
 | Velocity on the Z axis.
 |}

==== Set Equipment ====

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! colspan="2"| Field Name
 ! colspan="2"| Field Type
 ! colspan="2"| Notes
 |-
 | rowspan="3"| ''protocol:''<br/><code>0x5F</code><br/><br/>''resource:''<br/><code>set_equipment</code>
 | rowspan="3"| Play
 | rowspan="3"| Client
 | colspan="2"| Entity ID
 | colspan="2"| {{Type|VarInt}}
 | colspan="2"| Entity's ID.
 |-
 | rowspan="2"| Equipment
 | Slot
 | rowspan="2"| {{Type|Array}}
 | {{Type|Byte}} {{Type|Enum}}
 | rowspan="2"| The length of the array is unknown, it must be read until the most significant bit is 1 ((Slot >>> 7 & 1) == 1)
 | Equipment slot (see below).  Also has the top bit set if another entry follows, and otherwise unset if this is the last item in the array.
 |-
 | Item
 | {{Type|Slot}}
 |
 |}

Equipment slot can be one of the following:

{| class="wikitable"
 ! ID
 ! Equipment slot
 |-
 | 0
 | Main hand
 |-
 | 1
 | Off hand
 |-
 | 2
 | Boots
 |-
 | 3
 | Leggings
 |-
 | 4
 | Chestplate
 |-
 | 5
 | Helmet
 |-
 | 6
 | Body
 |}

==== Set Experience ====

Sent by the server when the client should change experience levels.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="3"| ''protocol:''<br/><code>0x60</code><br/><br/>''resource:''<br/><code>set_experience</code>
 | rowspan="3"| Play
 | rowspan="3"| Client
 | Experience bar
 | {{Type|Float}}
 | Between 0 and 1.
 |-
 | Level
 | {{Type|VarInt}}
 |
 |-
 | Total Experience
 | {{Type|VarInt}}
 | See [[Experience#Leveling up]] on the Minecraft Wiki for Total Experience to Level conversion.
 |}

==== Set Health ====

Sent by the server to set the health of the player it is sent to.

Food [[Food#Hunger and saturation|saturation]] acts as a food “overcharge”. Food values will not decrease while the saturation is over zero. New players logging in or respawning automatically get a saturation of 5.0. Eating food increases the saturation as well as the food bar.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="3"| ''protocol:''<br/><code>0x61</code><br/><br/>''resource:''<br/><code>set_health</code>
 | rowspan="3"| Play
 | rowspan="3"| Client
 | Health
 | {{Type|Float}}
 | 0 or less = dead, 20 = full HP.
 |-
 | Food
 | {{Type|VarInt}}
 | 0–20.
 |-
 | Food Saturation
 | {{Type|Float}}
 | Seems to vary from 0.0 to 5.0 in integer increments.
 |}

==== Set Held Item (clientbound) ====

Sent to change the player's slot selection.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | ''protocol:''<br/><code>0x62</code><br/><br/>''resource:''<br/><code>set_held_slot</code>
 | Play
 | Client
 | Slot
 | {{Type|VarInt}}
 | The slot which the player has selected (0–8).
 |}

==== Update Objectives ====

This is sent to the client when it should create a new [[scoreboard]] objective or remove one.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! colspan="2"| Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="10"| ''protocol:''<br/><code>0x63</code><br/><br/>''resource:''<br/><code>set_objective</code>
 | rowspan="10"| Play
 | rowspan="10"| Client
 | colspan="2"| Objective Name
 | {{Type|String}} (32767)
 | A unique name for the objective.
 |-
 | colspan="2"| Mode
 | {{Type|Byte}}
 | 0 to create the scoreboard. 1 to remove the scoreboard. 2 to update the display text.
 |-
 | colspan="2"| Objective Value
 | {{Type|Optional}} {{Type|Text Component}}
 | Only if mode is 0 or 2.The text to be displayed for the score.
 |-
 | colspan="2"| Type
 | {{Type|Optional}} {{Type|VarInt}} {{Type|Enum}}
 | Only if mode is 0 or 2. 0 = "integer", 1 = "hearts".
 |-
 | colspan="2"| Has Number Format
 | {{Type|Optional}} {{Type|Boolean}}
 | Only if mode is 0 or 2. Whether this objective has a set number format for the scores.
 |-
 | colspan="2"| Number Format
 | {{Type|Optional}} {{Type|VarInt}} {{Type|Enum}}
 | Only if mode is 0 or 2 and the previous boolean is true. Determines how the score number should be formatted.
 |-
 ! Number Format
 ! Field Name
 !
 !
 |-
 | 0: blank
 | colspan="2"| ''no fields''
 | Show nothing.
 |-
 | 1: styled
 | Styling
 | [[Minecraft Wiki:Projects/wiki.vg merge/NBT#Specification:compound_tag|Compound Tag]]
 | The styling to be used when formatting the score number. Contains the [[Minecraft Wiki:Projects/wiki.vg merge/Text formatting#Styling fields|text component styling fields]].
 |-
 | 2: fixed
 | Content
 | {{Type|Text Component}}
 | The text to be used as placeholder.
 |}

==== Set Passengers ====

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="2"| ''protocol:''<br/><code>0x64</code><br/><br/>''resource:''<br/><code>set_passengers</code>
 | rowspan="2"| Play
 | rowspan="2"| Client
 | Entity ID
 | {{Type|VarInt}}
 | Vehicle's EID.
 |-
 | Passengers
 | {{Type|Prefixed Array}} of {{Type|VarInt}}
 | EIDs of entity's passengers.
 |}

==== Set Player Inventory Slot ====

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="2"| ''protocol:''<br/><code>0x65</code><br/><br/>''resource:''<br/><code>set_player_inventory</code>
 | rowspan="2"| Play
 | rowspan="2"| Client
 | Slot
 | {{Type|VarInt}}
 |
 |-
 | Slot Data
 | {{Type|Slot}}
 |
 |}

==== Update Teams ====

Creates and updates teams.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! colspan="2"| Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="20"| ''protocol:''<br/><code>0x66</code><br/><br/>''resource:''<br/><code>set_player_team</code>
 | rowspan="20"| Play
 | rowspan="20"| Client
 | colspan="2"| Team Name
 | {{Type|String}} (32767)
 | A unique name for the team. (Shared with scoreboard).
 |-
 | colspan="2"| Method
 | {{Type|Byte}}
 | Determines the layout of the remaining packet.
 |-
 | rowspan="8"| 0: create team
 | Team Display Name
 | {{Type|Text Component}}
 |
 |-
 | Friendly Flags
 | {{Type|Byte}}
 | Bit mask. 0x01: Allow friendly fire, 0x02: can see invisible players on same team.
 |-
 | Name Tag Visibility
 | {{Type|String}} {{Type|Enum}} (40)
 | <code>always</code>, <code>hideForOtherTeams</code>, <code>hideForOwnTeam</code>, <code>never</code>.
 |-
 | Collision Rule
 | {{Type|String}} {{Type|Enum}} (40)
 | <code>always</code>, <code>pushOtherTeams</code>, <code>pushOwnTeam</code>, <code>never</code>.
 |-
 | Team Color
 | {{Type|VarInt}} {{Type|Enum}}
 | Used to color the name of players on the team; see below.
 |-
 | Team Prefix
 | {{Type|Text Component}}
 | Displayed before the names of players that are part of this team.
 |-
 | Team Suffix
 | {{Type|Text Component}}
 | Displayed after the names of players that are part of this team.
 |-
 | Entities
 | {{Type|Prefixed Array}} of {{Type|String}} (32767)
 | Identifiers for the entities in this team.  For players, this is their username; for other entities, it is their UUID.
 |-
 | 1: remove team
 | ''no fields''
 | ''no fields''
 |
 |-
 | rowspan="7"| 2: update team info
 | Team Display Name
 | {{Type|Text Component}}
 |
 |-
 | Friendly Flags
 | {{Type|Byte}}
 | Bit mask. 0x01: Allow friendly fire, 0x02: can see invisible entities on same team.
 |-
 | Name Tag Visibility
 | {{Type|String}} {{Type|Enum}} (40)
 | <code>always</code>, <code>hideForOtherTeams</code>, <code>hideForOwnTeam</code>, <code>never</code>
 |-
 | Collision Rule
 | {{Type|String}} {{Type|Enum}} (40)
 | <code>always</code>, <code>pushOtherTeams</code>, <code>pushOwnTeam</code>, <code>never</code>
 |-
 | Team Color
 | {{Type|VarInt}} {{Type|Enum}}
 | Used to color the name of players on the team; see below.
 |-
 | Team Prefix
 | {{Type|Text Component}}
 | Displayed before the names of players that are part of this team.
 |-
 | Team Suffix
 | {{Type|Text Component}}
 | Displayed after the names of players that are part of this team.
 |-
 | 3: add entities to team
 | Entities
 | {{Type|Prefixed Array}} of {{Type|String}} (32767)
 | Identifiers for the added entities.  For players, this is their username; for other entities, it is their UUID.
 |-
 | 4: remove entities from team
 | Entities
 | {{Type|Prefixed Array}} of {{Type|String}} (32767)
 | Identifiers for the removed entities.  For players, this is their username; for other entities, it is their UUID.
 |}

Team Color: The color of a team defines how the names of the team members are visualized; any formatting code can be used. The following table lists all the possible values.

{| class="wikitable"
 ! ID
 ! Formatting
 |-
 | 0-15
 | Color formatting, same values as in [[Minecraft Wiki:Projects/wiki.vg merge/Text formatting#Colors|Text formatting#Colors]].
 |-
 | 16
 | Obfuscated
 |-
 | 17
 | Bold
 |-
 | 18
 | Strikethrough
 |-
 | 19
 | Underlined
 |-
 | 20
 | Italic
 |-
 | 21
 | Reset
 |}

==== Update Score ====

This is sent to the client when it should update a scoreboard item.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! colspan="2"| Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="9"| ''protocol:''<br/><code>0x67</code><br/><br/>''resource:''<br/><code>set_score</code>
 | rowspan="9"| Play
 | rowspan="9"| Client
 | colspan="2"| Entity Name
 | {{Type|String}} (32767)
 | The entity whose score this is. For players, this is their username; for other entities, it is their UUID.
 |-
 | colspan="2"| Objective Name
 | {{Type|String}} (32767)
 | The name of the objective the score belongs to.
 |-
 | colspan="2"| Value
 | {{Type|VarInt}}
 | The score to be displayed next to the entry.
 |-
 | colspan="2"| Display Name
 | {{Type|Prefixed Optional}} {{Type|Text Component}}
 | The custom display name.
 |-
 | colspan="2"| Number Format
 | {{Type|Prefixed Optional}} {{Type|VarInt}} {{Type|Enum}}
 | Determines how the score number should be formatted.
 |-
 ! Number Format
 ! Field Name
 !
 !
 |-
 | 0: blank
 | colspan="2"| ''no fields''
 | Show nothing.
 |-
 | 1: styled
 | Styling
 | [[Minecraft Wiki:Projects/wiki.vg merge/NBT#Specification:compound_tag|Compound Tag]]
 | The styling to be used when formatting the score number. Contains the [[Minecraft Wiki:Projects/wiki.vg merge/Text formatting#Styling fields|text component styling fields]].
 |-
 | 2: fixed
 | Content
 | {{Type|Text Component}}
 | The text to be used as placeholder.
 |}

==== Set Simulation Distance ====

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | ''protocol:''<br/><code>0x68</code><br/><br/>''resource:''<br/><code>set_simulation_distance</code>
 | Play
 | Client
 | Simulation Distance
 | {{Type|VarInt}}
 | The distance that the client will process specific things, such as entities.
 |}

==== Set Subtitle Text ====

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="1"| ''protocol:''<br/><code>0x69</code><br/><br/>''resource:''<br/><code>set_subtitle_text</code>
 | rowspan="1"| Play
 | rowspan="1"| Client
 | Subtitle Text
 | {{Type|Text Component}}
 |
 |}

==== Update Time ====

Time is based on ticks, where 20 ticks happen every second. There are 24000 ticks in a day, making Minecraft days exactly 20 minutes long.

The time of day is based on the timestamp modulo 24000. 0 is sunrise, 6000 is noon, 12000 is sunset, and 18000 is midnight.

The default SMP server increments the time by <code>20</code> every second.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="3"| ''protocol:''<br/><code>0x6A</code><br/><br/>''resource:''<br/><code>set_time</code>
 | rowspan="3"| Play
 | rowspan="3"| Client
 | World Age
 | {{Type|Long}}
 | In ticks; not changed by server commands.
 |-
 | Time of day
 | {{Type|Long}}
 | The world (or region) time, in ticks.
 |-
 | Time of day increasing
 | {{Type|Boolean}}
 | If true, the client should automatically advance the time of day according to its ticking rate.
 |}

==== Set Title Text ====

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="1"| ''protocol:''<br/><code>0x6B</code><br/><br/>''resource:''<br/><code>set_title_text</code>
 | rowspan="1"| Play
 | rowspan="1"| Client
 | Title Text
 | {{Type|Text Component}}
 |
 |}

==== Set Title Animation Times ====

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="3"| ''protocol:''<br/><code>0x6C</code><br/><br/>''resource:''<br/><code>set_titles_animation</code>
 | rowspan="3"| Play
 | rowspan="3"| Client
 | Fade In
 | {{Type|Int}}
 | Ticks to spend fading in.
 |-
 | Stay
 | {{Type|Int}}
 | Ticks to keep the title displayed.
 |-
 | Fade Out
 | {{Type|Int}}
 | Ticks to spend fading out, not when to start fading out.
 |}

==== Entity Sound Effect ====

Plays a sound effect from an entity, either by hardcoded ID or Identifier. Sound IDs and names can be found [https://pokechu22.github.io/Burger/1.21.html#sounds here].

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="6"| ''protocol:''<br/><code>0x6D</code><br/><br/>''resource:''<br/><code>sound_entity</code>
 | rowspan="6"| Play
 | rowspan="6"| Client
 | Sound Event
 | {{Type|ID or}} {{Type|Sound Event}}
 | ID in the <code>minecraft:sound_event</code> registry, or an inline definition.
 |-
 | Sound Category
 | {{Type|VarInt}} {{Type|Enum}}
 | The category that this sound will be played from ([https://gist.github.com/konwboj/7c0c380d3923443e9d55 current categories]).
 |-
 | Entity ID
 | {{Type|VarInt}}
 |
 |-
 | Volume
 | {{Type|Float}}
 | 1.0 is 100%, capped between 0.0 and 1.0 by vanilla clients.
 |-
 | Pitch
 | {{Type|Float}}
 | Float between 0.5 and 2.0 by vanilla clients.
 |-
 | Seed
 | {{Type|Long}}
 | Seed used to pick sound variant.
 |}

==== Sound Effect ====

Plays a sound effect at the given location, either by hardcoded ID or Identifier. Sound IDs and names can be found [https://pokechu22.github.io/Burger/1.21.html#sounds here].

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="8"| ''protocol:''<br/><code>0x6E</code><br/><br/>''resource:''<br/><code>sound</code>
 | rowspan="8"| Play
 | rowspan="8"| Client
 | Sound Event
 | {{Type|ID or}} {{Type|Sound Event}}
 | ID in the <code>minecraft:sound_event</code> registry, or an inline definition.
 |-
 | Sound Category
 | {{Type|VarInt}} {{Type|Enum}}
 | The category that this sound will be played from ([https://gist.github.com/konwboj/7c0c380d3923443e9d55 current categories]).
 |-
 | Effect Position X
 | {{Type|Int}}
 | Effect X multiplied by 8 ([[Minecraft Wiki:Projects/wiki.vg merge/Data types#Fixed-point numbers|fixed-point number]] with only 3 bits dedicated to the fractional part).
 |-
 | Effect Position Y
 | {{Type|Int}}
 | Effect Y multiplied by 8 ([[Minecraft Wiki:Projects/wiki.vg merge/Data types#Fixed-point numbers|fixed-point number]] with only 3 bits dedicated to the fractional part).
 |-
 | Effect Position Z
 | {{Type|Int}}
 | Effect Z multiplied by 8 ([[Minecraft Wiki:Projects/wiki.vg merge/Data types#Fixed-point numbers|fixed-point number]] with only 3 bits dedicated to the fractional part).
 |-
 | Volume
 | {{Type|Float}}
 | 1.0 is 100%, capped between 0.0 and 1.0 by vanilla clients.
 |-
 | Pitch
 | {{Type|Float}}
 | Float between 0.5 and 2.0 by vanilla clients.
 |-
 | Seed
 | {{Type|Long}}
 | Seed used to pick sound variant.
 |}

==== Start Configuration ====

Sent during gameplay in order to redo the configuration process. The client must respond with [[#Acknowledge Configuration|Acknowledge Configuration]] for the process to start.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="1"| ''protocol:''<br/><code>0x6F</code><br/><br/>''resource:''<br/><code>start_configuration</code>
 | rowspan="1"| Play
 | rowspan="1"| Client
 | colspan="3"| ''no fields''
 |}

This packet switches the connection state to [[#Configuration|configuration]].

==== Stop Sound ====

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="3"| ''protocol:''<br/><code>0x70</code><br/><br/>''resource:''<br/><code>stop_sound</code>
 | rowspan="3"| Play
 | rowspan="3"| Client
 | Flags
 | {{Type|Byte}}
 | Controls which fields are present.
 |-
 | Source
 | {{Type|Optional}} {{Type|VarInt}} {{Type|Enum}}
 | Only if flags is 3 or 1 (bit mask 0x1). See below. If not present, then sounds from all sources are cleared.
 |-
 | Sound
 | {{Type|Optional}} {{Type|Identifier}}
 | Only if flags is 2 or 3 (bit mask 0x2).  A sound effect name, see [[#Custom Sound Effect|Custom Sound Effect]]. If not present, then all sounds are cleared.
 |}

Categories:

{| class="wikitable"
 ! Name !! Value
 |-
 | master || 0
 |-
 | music || 1
 |-
 | record || 2
 |-
 | weather || 3
 |-
 | block || 4
 |-
 | hostile || 5
 |-
 | neutral || 6
 |-
 | player || 7
 |-
 | ambient || 8
 |-
 | voice || 9
 |}

==== Store Cookie (play) ====

Stores some arbitrary data on the client, which persists between server transfers. The vanilla client only accepts cookies of up to 5 kiB in size.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="2"| ''protocol:''<br/><code>0x71</code><br/><br/>''resource:''<br/><code>store_cookie</code>
 | rowspan="2"| Play
 | rowspan="2"| Client
 | Key
 | {{Type|Identifier}}
 | The identifier of the cookie.
 |-
 | Payload
 | {{Type|Prefixed Array}} (5120) of {{Type|Byte}}
 | The data of the cookie.
 |}

==== System Chat Message ====

{{Main|Minecraft_Wiki:Projects/wiki.vg_merge/Chat}}

Sends the client a raw system message.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="2"| ''protocol:''<br/><code>0x72</code><br/><br/>''resource:''<br/><code>system_chat</code>
 | rowspan="2"| Play
 | rowspan="2"| Client
 | Content
 | {{Type|Text Component}}
 | Limited to 262144 bytes.
 |-
 | Overlay
 | {{Type|Boolean}}
 | Whether the message is an actionbar or chat message. See also [[#Set Action Bar Text]].
 |}

==== Set Tab List Header And Footer ====

This packet may be used by custom servers to display additional information above/below the player list. It is never sent by the vanilla server.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="2"| ''protocol:''<br/><code>0x73</code><br/><br/>''resource:''<br/><code>tab_list</code>
 | rowspan="2"| Play
 | rowspan="2"| Client
 | Header
 | {{Type|Text Component}}
 | To remove the header, send a empty text component: <code>{"text":""}</code>.
 |-
 | Footer
 | {{Type|Text Component}}
 | To remove the footer, send a empty text component: <code>{"text":""}</code>.
 |}

==== Tag Query Response ====

Sent in response to [[#Query Block Entity Tag|Query Block Entity Tag]] or [[#Query Entity Tag|Query Entity Tag]].

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="2"| ''protocol:''<br/><code>0x74</code><br/><br/>''resource:''<br/><code>tag_query</code>
 | rowspan="2"| Play
 | rowspan="2"| Client
 | Transaction ID
 | {{Type|VarInt}}
 | Can be compared to the one sent in the original query packet.
 |-
 | NBT
 | {{Type|NBT}}
 | The NBT of the block or entity.  May be a TAG_END (0) in which case no NBT is present.
 |}

==== Pickup Item ====

Sent by the server when someone picks up an item lying on the ground — its sole purpose appears to be the animation of the item flying towards you. It doesn't destroy the entity in the client memory, and it doesn't add it to your inventory. The server only checks for items to be picked up after each [[#Set Player Position|Set Player Position]] (and [[#Set Player Position And Rotation|Set Player Position And Rotation]]) packet sent by the client. The collector entity can be any entity; it does not have to be a player. The collected entity also can be any entity, but the vanilla server only uses this for items, experience orbs, and the different varieties of arrows.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="3"| ''protocol:''<br/><code>0x75</code><br/><br/>''resource:''<br/><code>take_item_entity</code>
 | rowspan="3"| Play
 | rowspan="3"| Client
 | Collected Entity ID
 | {{Type|VarInt}}
 |
 |-
 | Collector Entity ID
 | {{Type|VarInt}}
 |
 |-
 | Pickup Item Count
 | {{Type|VarInt}}
 | Seems to be 1 for XP orbs, otherwise the number of items in the stack.
 |}

==== Synchronize Vehicle Position ====

Teleports the entity on the client without changing the reference point of movement deltas in future [[#Update Entity Position|Update Entity Position]] packets. Seems to be used to make relative adjustments to vehicle positions; more information needed.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="11"| ''protocol:''<br/><code>0x76</code><br/><br/>''resource:''<br/><code>teleport_entity</code>
 | rowspan="11"| Play
 | rowspan="11"| Client
 | Entity ID
 | {{Type|VarInt}}
 |
 |-
 | X
 | {{Type|Double}}
 |
 |-
 | Y
 | {{Type|Double}}
 |
 |-
 | Z
 | {{Type|Double}}
 |
 |-
 | Velocity X
 | {{Type|Double}}
 |
 |-
 | Velocity Y
 | {{Type|Double}}
 |
 |-
 | Velocity Z
 | {{Type|Double}}
 |
 |-
 | Yaw
 | {{Type|Float}}
 | Rotation on the Y axis, in degrees. 
 |-
 | Pitch
 | {{Type|Float}}
 | Rotation on the Y axis, in degrees.
 |-
 | Flags
 | {{Type|Teleport Flags}}
 |
 |-
 | On Ground
 | {{Type|Boolean}}
 |
 |}

==== Test Instance Block Status ====

Updates the status of the currently open [[Test Instance Block]] screen, if any.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="5"| ''protocol:''<br/><code>0x77</code><br/><br/>''resource:''<br/><code>test_instance_block_status</code>
 | rowspan="5"| Play
 | rowspan="5"| Client
 | Status
 | {{Type|Text Component}}
 |
 |-
 | Has Size
 | {{Type|Boolean}}
 |
 |-
 | Size X
 | {{Type|Optional}} {{Type|Double}}
 | Only present if Has Size is true.
 |-
 | Size Y
 | {{Type|Optional}} {{Type|Double}}
 | Only present if Has Size is true.
 |-
 | Size Z
 | {{Type|Optional}} {{Type|Double}}
 | Only present if Has Size is true.
 |}

==== Set Ticking State ====

Used to adjust the ticking rate of the client, and whether it's frozen.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="2" | ''protocol:''<br/><code>0x78</code><br/><br/>''resource:''<br/><code>ticking_state</code>
 | rowspan="2" | Play
 | rowspan="2" | Client
 | Tick rate
 | {{Type|Float}}
 |
 |-
 | Is frozen
 | {{Type|Boolean}}
 |
 |}

==== Step Tick ====

Advances the client processing by the specified number of ticks. Has no effect unless client ticking is frozen.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | ''protocol:''<br/><code>0x79</code><br/><br/>''resource:''<br/><code>ticking_step</code>
 | Play
 | Client
 | Tick steps
 | {{Type|VarInt}}
 |
 |}

==== Transfer (play) ====

Notifies the client that it should transfer to the given server. Cookies previously stored are preserved between server transfers.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! colspan="2"| Field Name
 ! colspan="2"| Field Type
 ! Notes
 |-
 | rowspan="2"| ''protocol:''<br/><code>0x7A</code><br/><br/>''resource:''<br/><code>transfer</code>
 | rowspan="2"| Play
 | rowspan="2"| Client
 | colspan="2"| Host
 | colspan="2"| {{Type|String}}
 | The hostname or IP of the server.
 |-
 | colspan="2"| Port
 | colspan="2"| {{Type|VarInt}}
 | The port of the server.
 |}

==== Update Advancements ====

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! colspan="2"| Field Name
 ! colspan="2"| Field Type
 ! Notes
 |-
 | rowspan="7"| ''protocol:''<br/><code>0x7B</code><br/><br/>''resource:''<br/><code>update_advancements</code>
 | rowspan="7"| Play
 | rowspan="7"| Client
 | colspan="2"| Reset/Clear
 | colspan="2"| {{Type|Boolean}}
 | Whether to reset/clear the current advancements.
 |-
 | rowspan="2"| Advancement mapping
 | Key
 | rowspan="2"| {{Type|Prefixed Array}}
 | {{Type|Identifier}}
 | The identifier of the advancement.
 |-
 | Value
 | Advancement
 | See below
 |-
 | colspan="2"| Identifiers
 | colspan="2"| {{Type|Prefixed Array}} of {{Type|Identifier}}
 | The identifiers of the advancements that should be removed.
 |-
 | rowspan="2"| Progress mapping
 | Key
 | rowspan="2"| {{Type|Prefixed Array}}
 | {{Type|Identifier}}
 | The identifier of the advancement.
 |-
 | Value
 | Advancement progress
 | See below.
 |-
 | colspan="2"| Show advancements
 | colspan="2"| {{Type|Boolean}}
 |
 |}

Advancement structure:

{| class="wikitable"
 ! colspan="2"| Field Name
 ! colspan="2"| Field Type
 ! Notes
 |-
 | colspan="2"| Parent id
 | colspan="2"| {{Type|Prefixed Optional}} {{Type|Identifier}}
 | The identifier of the parent advancement.
 |-
 | colspan="2"| Display data
 | colspan="2"| {{Type|Prefixed Optional}} Advancement display
 | See below.
 |-
 | colspan="2"| Nested requirements
 | {{Type|Prefixed Array}}
 | {{Type|Prefixed Array}} of {{Type|String}} (32767)
 | Array with a sub-array of criteria. To check if the requirements are met, each sub-array must be tested and mapped with the OR operator, resulting in a boolean array.
These booleans must be mapped with the AND operator to get the result.
 |-
 | colspan="2"| Sends telemetry data
 | colspan="2"| {{Type|Boolean}}
 | Whether the client should include this achievement in the telemetry data when it's completed.
The vanilla client only sends data for advancements on the <code>minecraft</code> namespace.
 |}

Advancement display:

{| class="wikitable"
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | Title
 | {{Type|Text Component}}
 |
 |-
 | Description
 | {{Type|Text Component}}
 |
 |-
 | Icon
 | {{Type|Slot}}
 |
 |-
 | Frame type
 | {{Type|VarInt}} {{Type|Enum}}
 | 0 = <code>task</code>, 1 = <code>challenge</code>, 2 = <code>goal</code>.
 |-
 | Flags
 | {{Type|Int}}
 | 0x01: has background texture; 0x02: <code>show_toast</code>; 0x04: <code>hidden</code>.
 |-
 | Background texture
 | {{Type|Optional}} {{Type|Identifier}}
 | Background texture location.  Only if flags indicates it.
 |-
 | X coord
 | {{Type|Float}}
 |
 |-
 | Y coord
 | {{Type|Float}}
 |
 |}

Advancement progress:

{| class="wikitable"
 ! colspan="2"| Field Name
 ! colspan="2"| Field Type
 ! Notes
 |-
 | rowspan="2"| Criteria
 | Criterion identifier
 | rowspan="2"| {{Type|Prefixed Array}}
 | {{Type|Identifier}}
 | The identifier of the criterion.
 |-
 | Date of achieving
 | {{Type|Prefixed Optional}} {{Type|Long}}
 | Present if achieved. As returned by [https://docs.oracle.com/javase/6/docs/api/java/util/Date.html#getTime() <code>Date.getTime</code>].
 |}

==== Update Attributes ====

Sets [[Attribute|attributes]] on the given entity.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! colspan="2"| Field Name
 ! colspan="2"| Field Type
 ! Notes
 |-
 | rowspan="4"| ''protocol:''<br/><code>0x7C</code><br/><br/>''resource:''<br/><code>update_attributes</code>
 | rowspan="4"| Play
 | rowspan="4"| Client
 | colspan="2"| Entity ID
 | colspan="2"| {{Type|VarInt}}
 |
 |-
 | rowspan="3"| Property
 | Id
 | rowspan="3"| {{Type|Prefixed Array}}
 | {{Type|VarInt}}
 | ID in the <code>minecraft:attribute</code> registry. See also [[Attribute#Attributes]].
 |-
 | Value
 | {{Type|Double}}
 | See below.
 |-
 | Modifiers
 | {{Type|Prefixed Array}} of Modifier Data
 | See [[Attribute#Modifiers]]. Modifier Data defined below.
 |}

''Modifier Data'' structure:

{| class="wikitable"
 |-
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | Id
 | {{Type|Identifier}}
 |
 |-
 | Amount
 | {{Type|Double}}
 | May be positive or negative.
 |-
 | Operation
 | {{Type|Byte}}
 | See below.
 |}

The operation controls how the base value of the modifier is changed.

* 0: Add/subtract amount
* 1: Add/subtract amount percent of the current value
* 2: Multiply by amount percent

All of the 0's are applied first, and then the 1's, and then the 2's.

==== Entity Effect ====

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="5"| ''protocol:''<br/><code>0x7D</code><br/><br/>''resource:''<br/><code>update_mob_effect</code>
 | rowspan="5"| Play
 | rowspan="5"| Client
 | Entity ID
 | {{Type|VarInt}}
 |
 |-
 | Effect ID
 | {{Type|VarInt}}
 | See [[Status effect#Effect list|this table]].
 |-
 | Amplifier
 | {{Type|VarInt}}
 | Vanilla client displays effect level as Amplifier + 1.
 |-
 | Duration
 | {{Type|VarInt}}
 | Duration in ticks. (-1 for infinite)
 |-
 | Flags
 | {{Type|Byte}}
 | Bit field, see below.
 |}

{{missing info|section|What exact effect does the blend bit flag have on the client? What happens if it is used on effects besides DARKNESS?}}

Within flags:

* 0x01: Is ambient - was the effect spawned from a beacon?  All beacon-generated effects are ambient.  Ambient effects use a different icon in the HUD (blue border rather than gray).  If all effects on an entity are ambient, the [[Java Edition protocol/Entity_metadata#Living Entity|"Is potion effect ambient" living metadata field]] should be set to true.  Usually should not be enabled.
* 0x02: Show particles - should all particles from this effect be hidden?  Effects with particles hidden are not included in the calculation of the effect color, and are not rendered on the HUD (but are still rendered within the inventory).  Usually should be enabled.
* 0x04: Show icon - should the icon be displayed on the client?  Usually should be enabled.
* 0x08: Blend - should the effect's hard-coded blending be applied?  Currently only used in the DARKNESS effect to apply extra void fog and adjust the gamma value for lighting.

==== Update Recipes ====

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! colspan="2"| Field Name
 ! colspan="2"| Field Type
 ! Notes
 |-
 | rowspan="4"| ''protocol:''<br/><code>0x7E</code><br/><br/>''resource:''<br/><code>update_recipes</code>
 | rowspan="4"| Play
 | rowspan="4"| Client
 | rowspan="2"| Property Sets
 | Property Set ID
 | rowspan="2"| {{Type|Prefixed Array}}
 | {{Type|Identifier}}
 |
 |-
 | Items
 | {{Type|Prefixed Array}} of {{Type|VarInt}}
 | IDs in the <code>minecraft:item</code> registry.
 |-
 | rowspan="2"| Stonecutter Recipes
 | Ingredients
 | rowspan="2"| {{Type|Prefixed Array}}
 | {{Type|ID Set}}
 |
 |-
 | Slot Display
 | {{Type|Slot Display}}
 |
 |}

==== Update Tags (play) ====

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! colspan="2"| Field Name
 ! colspan="2"| Field Type
 ! Notes
 |-
 | rowspan="2"| ''protocol:''<br/><code>0x7F</code><br/><br/>''resource:''<br/><code>update_tags</code>
 | rowspan="2"| Play
 | rowspan="2"| Client
 | rowspan="2"| Registry to tags map
 | Registry
 | rowspan="2"| {{Type|Prefixed Array}}
 | {{Type|Identifier}}
 | Registry identifier (Vanilla expects tags for the registries <code>minecraft:block</code>, <code>minecraft:item</code>, <code>minecraft:fluid</code>, <code>minecraft:entity_type</code>, and <code>minecraft:game_event</code>)
 |-
 | Tags
 | {{Type|Prefixed Array}} of Tag (See below)
 |
 |}

A tag looks like this:

{| class="wikitable"
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | Tag name
 | {{Type|Identifier}}
 |
 |-
 | Entries
 | {{Type|Prefixed Array}} of {{Type|VarInt}}
 | Numeric IDs of the given type (block, item, etc.). This list replaces the previous list of IDs for the given tag. If some preexisting tags are left unmentioned, a warning is printed.
 |}

See [[Tag]] on the Minecraft Wiki for more information, including a list of vanilla tags.

==== Projectile Power ====

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="2"| ''protocol:''<br/><code>0x80</code><br/><br/>''resource:''<br/><code>projectile_power</code>
 | rowspan="2"| Play
 | rowspan="2"| Client
 | Entity ID
 | {{Type|VarInt}}
 |
 |-
 | Power
 | {{Type|Double}}
 |
 |}

==== Custom Report Details ====

Contains a list of key-value text entries that are included in any crash or disconnection report generated during connection to the server.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! colspan="2"| Field Name
 ! colspan="2"| Field Type
 ! Notes
 |-
 | rowspan="2"| ''protocol:''<br/><code>0x81</code><br/><br/>''resource:''<br/><code>custom_report_details</code>
 | rowspan="2"| Play
 | rowspan="2"| Client
 | rowspan="2"| Details
 | Title
 | rowspan="2"| {{Type|Prefixed Array}} (32)
 | {{Type|String}} (128)
 |
 |-
 | Description
 | {{Type|String}} (4096)
 |
|}

==== Server Links ====

This packet contains a list of links that the vanilla client will display in the menu available from the pause menu. Link labels can be built-in or custom (i.e., any text).

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! colspan="2"| Field Name
 ! colspan="2"| Field Type
 ! Notes
 |-
 | rowspan="3"| ''protocol:''<br/><code>0x82</code><br/><br/>''resource:''<br/><code>server_links</code>
 | rowspan="3"| Play
 | rowspan="3"| Client
 | rowspan="3"| Links
 | Is built-in
 | rowspan="3"| {{Type|Prefixed Array}}
 | {{Type|Boolean}}
 | Determines if the following label is built-in (from enum) or custom (text component).
 |-
 | Label
 | {{Type|VarInt}} {{Type|Enum}} / {{Type|Text Component}}
 | See below.
 |-
 | URL
 | {{Type|String}}
 | Valid URL.
|}

{| class="wikitable"
 ! ID
 ! Name
 ! Notes
 |-
 | 0
 | Bug Report
 | Displayed on connection error screen; included as a comment in the disconnection report.
 |-
 | 1
 | Community Guidelines
 | 
 |-
 | 2
 | Support
 | 
 |-
 | 3
 | Status
 | 
 |-
 | 4
 | Feedback
 | 
 |-
 | 5
 | Community
 | 
 |-
 | 6
 | Website
 | 
 |-
 | 7
 | Forums
 | 
 |-
 | 8
 | News
 | 
 |-
 | 9
 | Announcements
 | 
 |-
 |}

=== Serverbound ===

==== Confirm Teleportation ====

Sent by client as confirmation of [[#Synchronize Player Position|Synchronize Player Position]].

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | ''protocol:''<br/><code>0x00</code><br/><br/>''resource:''<br/><code>accept_teleportation</code>
 | Play
 | Server
 | Teleport ID
 | {{Type|VarInt}}
 | The ID given by the [[#Synchronize Player Position|Synchronize Player Position]] packet.
 |}

==== Query Block Entity Tag ====

Used when <kbd>F3</kbd>+<kbd>I</kbd> is pressed while looking at a block.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="2"| ''protocol:''<br/><code>0x01</code><br/><br/>''resource:''<br/><code>block_entity_tag_query</code>
 | rowspan="2"| Play
 | rowspan="2"| Server
 | Transaction ID
 | {{Type|VarInt}}
 | An incremental ID so that the client can verify that the response matches.
 |-
 | Location
 | {{Type|Position}}
 | The location of the block to check.
 |}

==== Bundle Item Selected ====

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="2"| ''protocol:''<br/><code>0x02</code><br/><br/>''resource:''<br/><code>bundle_item_selected</code>
 | rowspan="2"| Play
 | rowspan="2"| Server
 | Slot of Bundle
 | {{Type|VarInt}}
 |
 |-
 | Slot in Bundle
 | {{Type|VarInt}}
 |
 |}

==== Change Difficulty ====

Must have at least op level 2 to use.  Appears to only be used on singleplayer; the difficulty buttons are still disabled in multiplayer.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="1"| ''protocol:''<br/><code>0x03</code><br/><br/>''resource:''<br/><code>change_difficulty</code>
 | rowspan="1"| Play
 | rowspan="1"| Server
 | New difficulty
 | {{Type|Unsigned Byte}} {{Type|Enum}}
 | 0: peaceful, 1: easy, 2: normal, 3: hard.
 |}

==== Acknowledge Message ====

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="1"| ''protocol:''<br/><code>0x04</code><br/><br/>''resource:''<br/><code>chat_ack</code>
 | rowspan="1"| Play
 | rowspan="1"| Server
 | Message Count
 | {{Type|VarInt}}
 | 
 |}

==== Chat Command ====

{{Main|Minecraft_Wiki:Projects/wiki.vg_merge/Chat}}

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! colspan="2"| Field Name
 ! colspan="2"| Field Type
 ! Notes
 |-
 | rowspan="1"| ''protocol:''<br/><code>0x05</code><br/><br/>''resource:''<br/><code>chat_command</code>
 | rowspan="1"| Play
 | rowspan="1"| Server
 | colspan="2"| Command
 | colspan="2"| {{Type|String}} (32767)
 | colspan="2"| The command typed by the client.
 |}

==== Signed Chat Command ====

{{Main|Minecraft_Wiki:Projects/wiki.vg_merge/Chat}}

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! colspan="2"| Field Name
 ! colspan="2"| Field Type
 ! Notes
 |-
 | rowspan="8"| ''protocol:''<br/><code>0x06</code><br/><br/>''resource:''<br/><code>chat_command_signed</code>
 | rowspan="8"| Play
 | rowspan="8"| Server
 | colspan="2"| Command
 | colspan="2"| {{Type|String}} (32767)
 | colspan="2"| The command typed by the client.
 |-
 | colspan="2"| Timestamp
 | colspan="2"| {{Type|Long}}
 | colspan="2"| The timestamp that the command was executed.
 |-
 | colspan="2"| Salt
 | colspan="2"| {{Type|Long}}
 | colspan="2"| The salt for the following argument signatures.
 |-
 | rowspan="2"| Array of argument signatures
 | Argument name
 | rowspan="2"| {{Type|Prefixed Array}} (8)
 | {{Type|String}} (16)
 | The name of the argument that is signed by the following signature.
 |-
 | Signature
 | {{Type|Byte Array}} (256)
 | The signature that verifies the argument. Always 256 bytes and is not length-prefixed.
 |-
 | colspan="2"| Message Count
 | colspan="2"| {{Type|VarInt}}
 | colspan="2"|
 |-
 | colspan="2"| Acknowledged
 | colspan="2"| {{Type|Fixed BitSet}} (20)
 | colspan="2"|
 |-
 | colspan="2"| Checksum
 | colspan="2"| {{Type|Byte}}
 | colspan="2"|
 |}

==== Chat Message ====

{{Main|Minecraft_Wiki:Projects/wiki.vg_merge/Chat}}

Used to send a chat message to the server.  The message may not be longer than 256 characters or else the server will kick the client.

The server will broadcast a [[#Player Chat Message|Player Chat Message]] packet with Chat Type <code>minecraft:chat</code> to all players that haven't disabled chat (including the player that sent the message). See [[Minecraft Wiki:Projects/wiki.vg merge/Chat#Processing chat|Chat#Processing chat]] for more information.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="7"| ''protocol:''<br/><code>0x07</code><br/><br/>''resource:''<br/><code>chat</code>
 | rowspan="7"| Play
 | rowspan="7"| Server
 | Message
 | {{Type|String}} (256)
 |Content of the message 
|-
 | Timestamp
 | {{Type|Long}}
 |Number of milliseconds since the epoch (1 Jan 1970, midnight, UTC) 
|-
 | Salt
 | {{Type|Long}}
 | The salt used to verify the signature hash. Randomly generated by the client
 |-
 | Signature
 | {{Type|Prefixed Optional}} {{Type|Byte Array}} (256)
 
| The signature used to verify the chat message's authentication. When present, always 256 bytes and not length-prefixed.
This is a SHA256 with RSA digital signature computed over the following:

* The number 1 as a 4-byte int. Always 00 00 00 01.
* The player's 16 byte UUID.
* The chat session (a 16 byte UUID generated randomly generated by the client).
* The index of the message within this chat session as a 4-byte int. First message is 0, next message is 1, etc. Incremented each time the client sends a chat message.
* The salt (from above) as a 8-byte long.
* The timestamp (from above) converted from millisecods to seconds, so divide by 1000, as a 8-byte long.
* The length of the message in bytes (from above) as a 4-byte int.
* The message bytes.
* The number of messages in the last seen set, as a 4-byte int. Always in the range [0,20].
* For each message in the last seen set, from oldest to newest, the 256 byte signature of that message.


The client's chat private key is used for the message signature.
|-
 | Message Count
 | {{Type|VarInt}}
 |Number of signed clientbound chat messages the client has seen from the server since the last serverbound chat message from this client. The server will use this to update its last seen list for the client. 
|-
 | Acknowledged
 | {{Type|Fixed BitSet}} (20)
 | 
Bitmask of which message signatures from the last seen set were used to sign this message. The most recent is the highest bit. If there are less than 20 messages in the last seen set, the lower bits will be zeros.
|-
 | Checksum
 | {{Type|Byte}}
 | 
 |}

==== Player Session ====

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! colspan="2"| Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="4"| ''protocol:''<br/><code>0x08</code><br/><br/>''resource:''<br/><code>chat_session_update</code>
 | rowspan="4"| Play
 | rowspan="4"| Server
 | colspan="2"| Session Id
 | {{Type|UUID}}
 |
 |-
 | rowspan="3"| Public Key
 | Expires At
 | {{Type|Long}}
 | The time the play session key expires in [https://en.wikipedia.org/wiki/Unix_time epoch] milliseconds.
 |-
 | Public Key
 | {{Type|Prefixed Array}} (512) of {{Type|Byte}}
 | A byte array of an X.509-encoded public key.
 |-
 | Key Signature
 | {{Type|Prefixed Array}} (4096) of {{Type|Byte}}
 | The signature consists of the player UUID, the key expiration timestamp, and the public key data. These values are hashed using [https://en.wikipedia.org/wiki/SHA-1 SHA-1] and signed using Mojang's private [https://en.wikipedia.org/wiki/RSA_(cryptosystem) RSA] key.
 |}

==== Chunk Batch Received ====

Notifies the server that the chunk batch has been received by the client. The server uses the value sent in this packet to adjust the number of chunks to be sent in a batch.

The vanilla server will stop sending further chunk data until the client acknowledges the sent chunk batch. After the first acknowledgement, the server adjusts this number to allow up to 10 unacknowledged batches.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="1"| ''protocol:''<br/><code>0x09</code><br/><br/>''resource:''<br/><code>chunk_batch_received</code>
 | rowspan="1"| Play
 | rowspan="1"| Server
 | Chunks per tick
 | {{Type|Float}}
 | Desired chunks per tick.
 |}

==== Client Status ====

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="1"| ''protocol:''<br/><code>0x0A</code><br/><br/>''resource:''<br/><code>client_command</code>
 | rowspan="1"| Play
 | rowspan="1"| Server
 | Action ID
 | {{Type|VarInt}} {{Type|Enum}}
 | See below
 |}

''Action ID'' values:

{| class="wikitable"
 |-
 ! Action ID
 ! Action
 ! Notes
 |-
 | 0
 | Perform respawn
 | Sent when the client is ready to respawn after death.
 |-
 | 1
 | Request stats
 | Sent when the client opens the Statistics menu.
 |}

==== Client Tick End ====

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | ''protocol:''<br/><code>0x0B</code><br/><br/>''resource:''<br/><code>client_tick_end</code>
 | Play
 | Server
 | colspan="3"| ''no fields''
 |}

==== Client Information (play) ====

Sent when the player connects, or when settings are changed.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="9"| ''protocol:''<br/><code>0x0C</code><br/><br/>''resource:''<br/><code>client_information</code>
 | rowspan="9"| Play
 | rowspan="9"| Server
 | Locale
 | {{Type|String}} (16)
 | e.g. <code>en_GB</code>.
 |-
 | View Distance
 | {{Type|Byte}}
 | Client-side render distance, in chunks.
 |-
 | Chat Mode
 | {{Type|VarInt}} {{Type|Enum}}
 | 0: enabled, 1: commands only, 2: hidden.  See [[Minecraft Wiki:Projects/wiki.vg merge/Chat#Client chat mode|Chat#Client chat mode]] for more information.
 |-
 | Chat Colors
 | {{Type|Boolean}}
 | “Colors” multiplayer setting. The vanilla server stores this value but does nothing with it (see [https://bugs.mojang.com/browse/MC-64867 MC-64867]). Third-party servers such as Hypixel disable all coloring in chat and system messages when it is false.
 |-
 | Displayed Skin Parts
 | {{Type|Unsigned Byte}}
 | Bit mask, see below.
 |-
 | Main Hand
 | {{Type|VarInt}} {{Type|Enum}}
 | 0: Left, 1: Right.
 |-
 | Enable text filtering
 | {{Type|Boolean}}
 | Enables filtering of text on signs and written book titles. The vanilla client sets this according to the <code>profanityFilterPreferences.profanityFilterOn</code> account attribute indicated by the [[Minecraft Wiki:Projects/wiki.vg merge/Mojang API#Player Attributes|<code>/player/attributes</code> Mojang API endpoint]]. In offline mode it is always false.
 |-
 | Allow server listings
 | {{Type|Boolean}}
 | Servers usually list online players, this option should let you not show up in that list.
 |-
 | Particle Status
 | {{Type|VarInt}} {{Type|Enum}}
 | 0: all, 1: decreased, 2: minimal
 |}

''Displayed Skin Parts'' flags:

* Bit 0 (0x01): Cape enabled
* Bit 1 (0x02): Jacket enabled
* Bit 2 (0x04): Left Sleeve enabled
* Bit 3 (0x08): Right Sleeve enabled
* Bit 4 (0x10): Left Pants Leg enabled
* Bit 5 (0x20): Right Pants Leg enabled
* Bit 6 (0x40): Hat enabled

The most significant bit (bit 7, 0x80) appears to be unused.

==== Command Suggestions Request ====

Sent when the client needs to tab-complete a <code>minecraft:ask_server</code> suggestion type.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="2"| ''protocol:''<br/><code>0x0D</code><br/><br/>''resource:''<br/><code>command_suggestion</code>
 | rowspan="2"| Play
 | rowspan="2"| Server
 | Transaction Id
 | {{Type|VarInt}}
 | The id of the transaction that the server will send back to the client in the response of this packet. Client generates this and increments it each time it sends another tab completion that doesn't get a response.
 |-
 | Text
 | {{Type|String}} (32500)
 | All text behind the cursor without the <code>/</code> (e.g. to the left of the cursor in left-to-right languages like English).
 |}

==== Acknowledge Configuration ====

Sent by the client upon receiving a [[#Start Configuration|Start Configuration]] packet from the server.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="1"| ''protocol:''<br/><code>0x0E</code><br/><br/>''resource:''<br/><code>configuration_acknowledged</code>
 | rowspan="1"| Play
 | rowspan="1"| Server
 | colspan="3"| ''no fields''
 |}

This packet switches the connection state to [[#Configuration|configuration]].

==== Click Container Button ====

Used when clicking on window buttons. Until 1.14, this was only used by enchantment tables.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="2"| ''protocol:''<br/><code>0x0F</code><br/><br/>''resource:''<br/><code>container_button_click</code>
 | rowspan="2"| Play
 | rowspan="2"| Server
 | Window ID
 | {{Type|VarInt}}
 | The ID of the window sent by [[#Open Screen|Open Screen]].
 |-
 | Button ID
 | {{Type|VarInt}}
 | Meaning depends on window type; see below.
 |}

{| class="wikitable"
 ! Window type
 ! ID
 ! Meaning
 |-
 | rowspan="3"| Enchantment Table
 | 0 || Topmost enchantment.
 |-
 | 1 || Middle enchantment.
 |-
 | 2 || Bottom enchantment.
 |-
 | rowspan="4"| Lectern
 | 1 || Previous page (which does give a redstone output).
 |-
 | 2 || Next page.
 |-
 | 3 || Take Book.
 |-
 | 100+page || Opened page number - 100 + number.
 |-
 | Stonecutter
 | colspan="2"| Recipe button number - 4*row + col.  Depends on the item.
 |-
 | Loom
 | colspan="2"| Recipe button number - 4*row + col.  Depends on the item.
 |}

==== Click Container ====

This packet is sent by the client when the player clicks on a slot in a window.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! colspan="2"| Field Name
 ! colspan="2"| Field Type
 ! Notes
 |-
 | rowspan="8"| ''protocol:''<br/><code>0x10</code><br/><br/>''resource:''<br/><code>container_click</code>
 | rowspan="8"| Play
 | rowspan="8"| Server
 | colspan="2"| Window ID
 | colspan="2"| {{Type|VarInt}}
 | The ID of the window which was clicked. 0 for player inventory. The server ignores any packets targeting a Window ID other than the current one, including ignoring 0 when any other window is open.
 |-
 | colspan="2"| State ID
 | colspan="2"| {{Type|VarInt}}
 | The last received State ID from either a [[#Set Container Slot|Set Container Slot]] or a [[#Set Container Content|Set Container Content]] packet.
 |-
 | colspan="2"| Slot
 | colspan="2"| {{Type|Short}}
 | The clicked slot number, see below.
 |-
 | colspan="2"| Button
 | colspan="2"| {{Type|Byte}}
 | The button used in the click, see below.
 |-
 | colspan="2"| Mode
 | colspan="2"| {{Type|VarInt}} {{Type|Enum}}
 | Inventory operation mode, see below.
 |-
 | rowspan="2"| Array of changed slots
 | Slot number
 | rowspan="2"| {{Type|Prefixed Array}} (128)
 | {{Type|Short}}
 |
 |-
 | Slot data
 | {{Type|Hashed Slot}}
 | New data for this slot, in the client's opinion; see below.
 |-
 | colspan="2"| Carried item
 | colspan="2"| {{Type|Hashed Slot}}
 | Item carried by the cursor. Has to be empty (item ID = -1) for drop mode, otherwise nothing will happen.
 |}

See [[Minecraft Wiki:Projects/wiki.vg merge/Inventory|Inventory]] for further information about how slots are indexed.

After performing the action, the server compares the results to the slot change information included in the packet, as applied on top of the server's view of the container's state prior to the action. For any slots that do not match, it sends [[#Set Container Slot|Set Container Slot]] packets containing the correct results. If State ID does not match the last ID sent by the server, it will instead send a full [[#Set Container Content|Set Container Content]] to resynchronize the client.

When right-clicking on a stack of items, half the stack will be picked up and half left in the slot. If the stack is an odd number, the half left in the slot will be smaller of the amounts.

The distinct type of click performed by the client is determined by the combination of the Mode and Button fields.

{| class="wikitable"
 ! Mode
 ! Button
 ! Slot
 ! Trigger
 |-
 ! rowspan="4"| 0
 | 0
 | Normal
 | Left mouse click
 |-
 | 1
 | Normal
 | Right mouse click
 |-
 | 0
 | -999
 | Left click outside inventory (drop cursor stack)
 |-
 | 1
 | -999
 | Right click outside inventory (drop cursor single item)
 |-
 ! rowspan="2"| 1
 | 0
 | Normal
 | Shift + left mouse click
 |-
 | 1
 | Normal
 | Shift + right mouse click ''(identical behavior)''
 |-
 ! rowspan="7"| 2
 | 0
 | Normal
 | Number key 1
 |-
 | 1
 | Normal
 | Number key 2
 |-
 | 2
 | Normal
 | Number key 3
 |-
 | ⋮
 | ⋮
 | ⋮
 |-
 | 8
 | Normal
 | Number key 9
 |-
 | ⋮
 | ⋮
 | Button is used as the slot index (impossible in vanilla clients)
 |-
 | 40
 | Normal
 | Offhand swap key F
 |-
 ! 3
 | 2
 | Normal
 | Middle click, only defined for creative players in non-player inventories.
 |-
 ! rowspan="2"| 4
 | 0
 | Normal
 | Drop key (Q)
 |-
 | 1
 | Normal
 | Control + Drop key (Q)
 |-
 ! rowspan="9"| 5
 | 0
 | -999
 | Starting left mouse drag
 |-
 | 4
 | -999
 | Starting right mouse drag
 |-
 | 8
 | -999
 | Starting middle mouse drag, only defined for creative players in non-player inventories.
 |-
 | 1
 | Normal
 | Add slot for left-mouse drag
 |-
 | 5
 | Normal
 | Add slot for right-mouse drag
 |-
 | 9
 | Normal
 | Add slot for middle-mouse drag, only defined for creative players in non-player inventories.
 |-
 | 2
 | -999
 | Ending left mouse drag
 |-
 | 6
 | -999
 | Ending right mouse drag
 |-
 | 10
 | -999
 | Ending middle mouse drag, only defined for creative players in non-player inventories.
 |-
 ! rowspan="2"| 6
 | 0
 | Normal
 | Double click
 |-
 | 1
 | Normal
 | Pickup all but check items in reverse order (impossible in vanilla clients)
 |}

Starting from version 1.5, “painting mode” is available for use in inventory windows. It is done by picking up stack of something (more than 1 item), then holding mouse button (left, right or middle) and dragging held stack over empty (or same type in case of right button) slots. In that case client sends the following to server after mouse button release (omitting first pickup packet which is sent as usual):

# packet with mode 5, slot -999, button (0 for left | 4 for right);
# packet for every slot painted on, mode is still 5, button (1 | 5);
# packet with mode 5, slot -999, button (2 | 6);

If any of the painting packets other than the “progress” ones are sent out of order (for example, a start, some slots, then another start; or a left-click in the middle) the painting status will be reset.

==== Close Container ====

This packet is sent by the client when closing a window.

vanilla clients send a Close Window packet with Window ID 0 to close their inventory even though there is never an [[#Open Screen|Open Screen]] packet for the inventory.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="1"| ''protocol:''<br/><code>0x11</code><br/><br/>''resource:''<br/><code>container_close</code>
 | rowspan="1"| Play
 | rowspan="1"| Server
 | Window ID
 | {{Type|VarInt}}
 | This is the ID of the window that was closed. 0 for player inventory.
 |}

==== Change Container Slot State ====

This packet is sent by the client when toggling the state of a Crafter.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="3"| ''protocol:''<br/><code>0x12</code><br/><br/>''resource:''<br/><code>container_slot_state_changed</code>
 | rowspan="3"| Play
 | rowspan="3"| Server
 | Slot ID
 | {{Type|VarInt}}
 | This is the ID of the slot that was changed.
 |-
 | Window ID
 | {{Type|VarInt}}
 | This is the ID of the window that was changed.
 |-
 | State
 | {{Type|Boolean}}
 | The new state of the slot. True for enabled, false for disabled.
 |}

==== Cookie Response (play) ====

Response to a [[#Cookie_Request_(play)|Cookie Request (play)]] from the server. The vanilla server only accepts responses of up to 5 kiB in size.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="2"| ''protocol:''<br/><code>0x13</code><br/><br/>''resource:''<br/><code>cookie_response</code>
 | rowspan="2"| Play
 | rowspan="2"| Server
 | Key
 | {{Type|Identifier}}
 | The identifier of the cookie.
 |-
 | Payload
 | {{Type|Prefixed Optional}} {{Type|Prefixed Array}} (5120) of {{Type|Byte}}
 | The data of the cookie.
 |}

==== Serverbound Plugin Message (play) ====

{{Main|Minecraft Wiki:Projects/wiki.vg merge/Plugin channels}}

Mods and plugins can use this to send their data. Minecraft itself uses some [[Minecraft Wiki:Projects/wiki.vg merge/Plugin channels|plugin channels]]. These internal channels are in the <code>minecraft</code> namespace.

More documentation on this: [https://dinnerbone.com/blog/2012/01/13/minecraft-plugin-channels-messaging/ https://dinnerbone.com/blog/2012/01/13/minecraft-plugin-channels-messaging/]

Note that the length of Data is known only from the packet length, since the packet has no length field of any kind.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="2"| ''protocol:''<br/><code>0x14</code><br/><br/>''resource:''<br/><code>custom_payload</code>
 | rowspan="2"| Play
 | rowspan="2"| Server
 | Channel
 | {{Type|Identifier}}
 | Name of the [[Minecraft Wiki:Projects/wiki.vg merge/Plugin channels|plugin channel]] used to send the data.
 |-
 | Data
 | {{Type|Byte Array}} (32767)
 | Any data, depending on the channel. <code>minecraft:</code> channels are documented [[Minecraft Wiki:Projects/wiki.vg merge/Plugin channels|here]]. The length of this array must be inferred from the packet length.
 |}

In vanilla servers, the maximum data length is 32767 bytes.

==== Debug Sample Subscription ====

Subscribes to the specified type of debug sample data, which is then sent periodically to the client via [[#Debug_Sample|Debug Sample]].

The subscription is retained for 10 seconds (the vanilla server checks that both 10.001 real-time seconds and 201 ticks have elapsed), after which the client is automatically unsubscribed. The vanilla client resends this packet every 5 seconds to keep up the subscription.

The vanilla server only allows subscriptions from players that are server operators.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="1"| ''protocol:''<br/><code>0x15</code><br/><br/>''resource:''<br/><code>debug_sample_subscription</code>
 | rowspan="1"| Play
 | rowspan="1"| Server
 | Sample Type
 | {{Type|VarInt}} {{Type|Enum}}
 | The type of debug sample to subscribe to. Can be one of the following:
* 0 - Tick time
 |}

==== Edit Book ====

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="3"| ''protocol:''<br/><code>0x16</code><br/><br/>''resource:''<br/><code>edit_book</code>
 | rowspan="3"| Play
 | rowspan="3"| Server
 | Slot
 | {{Type|VarInt}}
 | The hotbar slot where the written book is located
 |-
 | Entries
 | {{Type|Prefixed Array}} (100) of {{Type|String}} (1024)
 | Text from each page. Maximum string length is 1024 chars.
 |-
 | Title
 | {{Type|Prefixed Optional}} {{Type|String}} (32)
 | Title of book. Present if book is being signed, not present if book is being edited.
 |}

==== Query Entity Tag ====

Used when <kbd>F3</kbd>+<kbd>I</kbd> is pressed while looking at an entity.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="2"| ''protocol:''<br/><code>0x17</code><br/><br/>''resource:''<br/><code>entity_tag_query</code>
 | rowspan="2"| Play
 | rowspan="2"| Server
 | Transaction ID
 | {{Type|VarInt}}
 | An incremental ID so that the client can verify that the response matches.
 |-
 | Entity ID
 | {{Type|VarInt}}
 | The ID of the entity to query.
 |}

==== Interact ====

This packet is sent from the client to the server when the client attacks or right-clicks another entity (a player, minecart, etc).

A vanilla server only accepts this packet if the entity being attacked/used is visible without obstruction and within a 4-unit radius of the player's position.

The target X, Y, and Z fields represent the difference between the vector location of the cursor at the time of the packet and the entity's position.

Note that middle-click in creative mode is interpreted by the client and sent as a [[#Set Creative Mode Slot|Set Creative Mode Slot]] packet instead.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="7"| ''protocol:''<br/><code>0x18</code><br/><br/>''resource:''<br/><code>interact</code>
 | rowspan="7"| Play
 | rowspan="7"| Server
 | Entity ID
 | {{Type|VarInt}}
 | The ID of the entity to interact. Note the special case described below.
 |-
 | Type
 | {{Type|VarInt}} {{Type|Enum}}
 | 0: interact, 1: attack, 2: interact at.
 |-
 | Target X
 | {{Type|Optional}} {{Type|Float}}
 | Only if Type is interact at.
 |-
 | Target Y
 | {{Type|Optional}} {{Type|Float}}
 | Only if Type is interact at.
 |-
 | Target Z
 | {{Type|Optional}} {{Type|Float}}
 | Only if Type is interact at.
 |-
 | Hand
 | {{Type|Optional}} {{Type|VarInt}} {{Type|Enum}}
 | Only if Type is interact or interact at; 0: main hand, 1: off hand.
 |-
 | Sneak Key Pressed
 | {{Type|Boolean}}
 | If the client is pressing the sneak key. Has the same effect as a Player Command Press/Release sneak key preceding the interaction, and the state is permanently changed.
 |}

Interaction with the ender dragon is an odd special case characteristic of release deadline&ndash;driven design. 8 consecutive entity IDs following the dragon's ID (<var>id</var> + 1, <var>id</var> + 2, ..., <var>id</var> + 8) are reserved for the 8 hitboxes that make up the dragon:

{| class="wikitable"
 ! ID offset
 ! Description
 |-
 | 0
 | The dragon itself (never used in this packet)
 |-
 | 1
 | Head
 |-
 | 2
 | Neck
 |-
 | 3
 | Body
 |-
 | 4
 | Tail 1
 |-
 | 5
 | Tail 2
 |-
 | 6
 | Tail 3
 |-
 | 7
 | Wing 1
 |-
 | 8
 | Wing 2
 |}

==== Jigsaw Generate ====

Sent when Generate is pressed on the [[Jigsaw Block]] interface.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="3"| ''protocol:''<br/><code>0x19</code><br/><br/>''resource:''<br/><code>jigsaw_generate</code>
 | rowspan="3"| Play
 | rowspan="3"| Server
 | Location
 | {{Type|Position}}
 | Block entity location.
 |-
 | Levels
 | {{Type|VarInt}}
 | Value of the levels slider/max depth to generate.
 |-
 | Keep Jigsaws
 | {{Type|Boolean}}
 |
 |}

==== Serverbound Keep Alive (play) ====

The server will frequently send out a keep-alive (see [[#Clientbound Keep Alive (play)|Clientbound Keep Alive]]), each containing a random ID. The client must respond with the same packet.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="1"| ''protocol:''<br/><code>0x1A</code><br/><br/>''resource:''<br/><code>keep_alive</code>
 | rowspan="1"| Play
 | rowspan="1"| Server
 | Keep Alive ID
 | {{Type|Long}}
 |
 |}

==== Lock Difficulty ====

Must have at least op level 2 to use.  Appears to only be used on singleplayer; the difficulty buttons are still disabled in multiplayer.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="1"| ''protocol:''<br/><code>0x1B</code><br/><br/>''resource:''<br/><code>lock_difficulty</code>
 | rowspan="1"| Play
 | rowspan="1"| Server
 | Locked
 | {{Type|Boolean}}
 |
 |}

==== Set Player Position ====

Updates the player's XYZ position on the server.

If the player is in a vehicle, the position is ignored (but in case of [[#Set Player Position and Rotation|Set Player Position and Rotation]], the rotation is still used as normal). No validation steps other than value range clamping are performed in this case.

If the player is sleeping, the position (or rotation) is not changed, and a [[#Synchronize Player Position|Synchronize Player Position]] is sent if the received position deviated from the server's view by more than a meter.

The vanilla server silently clamps the x and z coordinates between -30,000,000 and 30,000,000, and the y coordinate between -20,000,000 and 20,000,000. A similar condition has historically caused a kick for "Illegal position"; this is no longer the case. However, infinite or NaN coordinates (or angles) still result in a kick for <code>multiplayer.disconnect.invalid_player_movement</code>.

As of 1.20.6, checking for moving too fast is achieved like this (sic):

* Each server tick, the player's current position is stored.
* When the player moves, the offset from the stored position to the requested position is computed (&Delta;x, &Delta;y, &Delta;z).
* The requested movement distance squared is computed as &Delta;x&sup2; + &Delta;y&sup2; + &Delta;z&sup2;.
* The baseline expected movement distance squared is computed based on  the player's server-side velocity as Vx&sup2; + Vy&sup2; + Vz&sup2;. The player's server-side velocity is a somewhat ill-defined quantity that includes among other things gravity, jump velocity and knockback, but ''not'' regular horizontal movement. A proper description would bring much of Minecraft's physics engine with it. It is accessible as the <code>Motion</code> NBT tag on the player entity.
* The maximum permitted movement distance squared is computed as 100 (300 if the player is using an elytra), multiplied by the number of movement packets received since the last tick, including this one, unless that value is greater than 5, in which case no multiplier is applied.
* If the requested movement distance squared minus the baseline distance squared is more than the maximum squared, the player is moving too fast.

If the player is moving too fast, it is logged that "<player> moved too quickly! " followed by the change in x, y, and z, and the player is teleported back to their current (before this packet) server-side position.

Checking for block collisions is achieved like this:

* A temporary collision-checked move of the player is attempted from its current position to the requested one.
* The offset from the resulting position to the requested position is computed. If the absolute value of the offset on the y axis is less than 0.5, it (only the y component) is rounded down to 0.
* If the magnitude of the offset is greater than 0.25 and the player isn't in creative or spectator mode, it is logged that "<player> moved wrongly!", and the player is teleported back to their current (before this packet) server-side position.
* In addition, if the player's hitbox stationary at the requested position would intersect with a block, and they aren't in spectator mode, they are teleported back without a log message.

Checking for illegal flight is achieved like this:

* When a movement packet is received, a flag indicating whether or not the player is floating mid-air is updated. The flag is set if the move test described above detected no collision below the player ''and'' the y component of the offset from the player's current position to the requested one is greater than -0.5, unless any of various conditions permitting flight (creative mode, elytra, levitation effect, etc., but not jumping) are met.
* Each server tick, it is checked if the flag has been set for more than 80 consecutive ticks. If so, and the player isn't currently sleeping, dead or riding a vehicle, they are kicked for <code>multiplayer.disconnect.flying</code>.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="4"| ''protocol:''<br/><code>0x1C</code><br/><br/>''resource:''<br/><code>move_player_pos</code>
 | rowspan="4"| Play
 | rowspan="4"| Server
 | X
 | {{Type|Double}}
 | Absolute position.
 |-
 | Feet Y
 | {{Type|Double}}
 | Absolute feet position, normally Head Y - 1.62.
 |-
 | Z
 | {{Type|Double}}
 | Absolute position.
 |-
 | Flags
 | {{Type|Byte}}
 | Bit field: 0x01: on ground, 0x02: pushing against wall.
 |}

==== Set Player Position and Rotation ====

A combination of [[#Set Player Rotation|Move Player Rotation]] and [[#Set Player Position|Move Player Position]].

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="6"| ''protocol:''<br/><code>0x1D</code><br/><br/>''resource:''<br/><code>move_player_pos_rot</code>
 | rowspan="6"| Play
 | rowspan="6"| Server
 | X
 | {{Type|Double}}
 | Absolute position.
 |-
 | Feet Y
 | {{Type|Double}}
 | Absolute feet position, normally Head Y - 1.62.
 |-
 | Z
 | {{Type|Double}}
 | Absolute position.
 |-
 | Yaw
 | {{Type|Float}}
 | Absolute rotation on the X Axis, in degrees.
 |-
 | Pitch
 | {{Type|Float}}
 | Absolute rotation on the Y Axis, in degrees.
 |-
 | Flags
 | {{Type|Byte}}
 | Bit field: 0x01: on ground, 0x02: pushing against wall.
 |}

==== Set Player Rotation ====

[[File:Minecraft-trig-yaw.png|thumb|The unit circle for yaw]]
[[File:Yaw.png|thumb|The unit circle of yaw, redrawn]]

Updates the direction the player is looking in.

Yaw is measured in degrees, and does not follow classical trigonometry rules. The unit circle of yaw on the XZ-plane starts at (0, 1) and turns counterclockwise, with 90 at (-1, 0), 180 at (0,-1) and 270 at (1, 0). Additionally, yaw is not clamped to between 0 and 360 degrees; any number is valid, including negative numbers and numbers greater than 360.

Pitch is measured in degrees, where 0 is looking straight ahead, -90 is looking straight up, and 90 is looking straight down.

The yaw and pitch of player (in degrees), standing at point (x0, y0, z0) and looking towards point (x, y, z) can be calculated with:

 dx = x-x0
 dy = y-y0
 dz = z-z0
 r = sqrt( dx*dx + dy*dy + dz*dz )
 yaw = -atan2(dx,dz)/PI*180
 if yaw < 0 then
     yaw = 360 + yaw
 pitch = -arcsin(dy/r)/PI*180

You can get a unit vector from a given yaw/pitch via:

 x = -cos(pitch) * sin(yaw)
 y = -sin(pitch)
 z =  cos(pitch) * cos(yaw)

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="3"| ''protocol:''<br/><code>0x1E</code><br/><br/>''resource:''<br/><code>move_player_rot</code>
 | rowspan="3"| Play
 | rowspan="3"| Server
 | Yaw
 | {{Type|Float}}
 | Absolute rotation on the X Axis, in degrees.
 |-
 | Pitch
 | {{Type|Float}}
 | Absolute rotation on the Y Axis, in degrees.
 |-
 | Flags
 | {{Type|Byte}}
 | Bit field: 0x01: on ground, 0x02: pushing against wall.
 |}

==== Set Player Movement Flags ====

This packet as well as [[#Set Player Position|Set Player Position]], [[#Set Player Rotation|Set Player Rotation]], and [[#Set Player Position and Rotation|Set Player Position and Rotation]] are called the “serverbound movement packets”. Vanilla clients will send Move Player Position once every 20 ticks even for a stationary player.

This packet is used to indicate whether the player is on ground (walking/swimming), or airborne (jumping/falling).

When dropping from sufficient height, fall damage is applied when this state goes from false to true. The amount of damage applied is based on the point where it last changed from true to false. Note that there are several movement related packets containing this state.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="1"| ''protocol:''<br/><code>0x1F</code><br/><br/>''resource:''<br/><code>move_player_status_only</code>
 | rowspan="1"| Play
 | rowspan="1"| Server
 | Flags
 | {{Type|Byte}}
 | Bit field: 0x01: on ground, 0x02: pushing against wall.
 |}

==== Move Vehicle ====

Sent when a player moves in a vehicle. Fields are the same as in [[#Set Player Position and Rotation|Set Player Position and Rotation]]. Note that all fields use absolute positioning and do not allow for relative positioning.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="6"| ''protocol:''<br/><code>0x20</code><br/><br/>''resource:''<br/><code>move_vehicle</code>
 | rowspan="6"| Play
 | rowspan="6"| Server
 | X
 | {{Type|Double}}
 | Absolute position (X coordinate).
 |-
 | Y
 | {{Type|Double}}
 | Absolute position (Y coordinate).
 |-
 | Z
 | {{Type|Double}}
 | Absolute position (Z coordinate).
 |-
 | Yaw
 | {{Type|Float}}
 | Absolute rotation on the vertical axis, in degrees.
 |-
 | Pitch
 | {{Type|Float}}
 | Absolute rotation on the horizontal axis, in degrees.
 |-
 | On Ground
 | {{Type|Boolean}}
 | ''(This value does not seem to exist)''
 |}

==== Paddle Boat ====

Used to ''visually'' update whether boat paddles are turning.  The server will update the [[Java Edition protocol/Entity_metadata#Boat|Boat entity metadata]] to match the values here.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="2"| ''protocol:''<br/><code>0x21</code><br/><br/>''resource:''<br/><code>paddle_boat</code>
 | rowspan="2"| Play
 | rowspan="2"| Server
 | Left paddle turning
 | {{Type|Boolean}}
 |
 |-
 | Right paddle turning
 | {{Type|Boolean}}
 |
 |}

Right paddle turning is set to true when the left button or forward button is held, left paddle turning is set to true when the right button or forward button is held.

==== Pick Item From Block ====

Used for pick block functionality (middle click) on blocks to retrieve items from the inventory in survival or creative mode or create them in creative mode.  See [[Controls#Pick_Block]] for more information.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="2"| ''protocol:''<br/><code>0x22</code><br/><br/>''resource:''<br/><code>pick_item_from_block</code>
 | rowspan="2"| Play
 | rowspan="2"| Server
 | Location
 | {{Type|Position}}
 | The location of the block.
 |-
 | Include Data
 | {{Type|Boolean}}
 | Used to tell the server to include block data in the new stack, works only if in creative mode.
 |}

==== Pick Item From Entity ====

Used for pick block functionality (middle click) on entities to retrieve items from the inventory in survival or creative mode or create them in creative mode.  See [[Controls#Pick_Block]] for more information.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="2"| ''protocol:''<br/><code>0x23</code><br/><br/>''resource:''<br/><code>pick_item_from_entity</code>
 | rowspan="2"| Play
 | rowspan="2"| Server
 | Entity ID
 | {{Type|VarInt}}
 | The ID of the entity to pick.
 |-
 | Include Data
 | {{Type|Boolean}}
 | Unused by the vanilla server.
 |}

==== Ping Request (play) ====

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="1"| ''protocol:''<br/><code>0x24</code><br/><br/>''resource:''<br/><code>ping_request</code>
 | rowspan="1"| Play
 | rowspan="1"| Server
 | Payload
 | {{Type|Long}}
 | May be any number. vanilla clients use a system-dependent time value which is counted in milliseconds.
 |}

==== Place Recipe ====

This packet is sent when a player clicks a recipe in the crafting book that is craftable (white border).

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="3"| ''protocol:''<br/><code>0x25</code><br/><br/>''resource:''<br/><code>place_recipe</code>
 | rowspan="3"| Play
 | rowspan="3"| Server
 | Window ID
 | {{Type|VarInt}}
 |
 |-
 | Recipe ID
 | {{Type|VarInt}}
 | ID of recipe previously defined in [[#Recipe Book Add|Recipe Book Add]].
 |-
 | Make all
 | {{Type|Boolean}}
 | Affects the amount of items processed; true if shift is down when clicked.
 |}

==== Player Abilities (serverbound) ====

The vanilla client sends this packet when the player starts/stops flying with the Flags parameter changed accordingly.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="1"| ''protocol:''<br/><code>0x26</code><br/><br/>''resource:''<br/><code>player_abilities</code>
 | rowspan="1"| Play
 | rowspan="1"| Server
 | Flags
 | {{Type|Byte}}
 | Bit mask. 0x02: is flying.
 |}

==== Player Action ====

Sent when the player mines a block. A vanilla server only accepts digging packets with coordinates within a 6-unit radius between the center of the block and the player's eyes.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="4"| ''protocol:''<br/><code>0x27</code><br/><br/>''resource:''<br/><code>player_action</code>
 | rowspan="4"| Play
 | rowspan="4"| Server
 | Status
 | {{Type|VarInt}} {{Type|Enum}}
 | The action the player is taking against the block (see below).
 |-
 | Location
 | {{Type|Position}}
 | Block position.
 |-
 | Face
 | {{Type|Byte}} {{Type|Enum}}
 | The face being hit (see below).
 |-
 | Sequence
 | {{Type|VarInt}}
 | Block change sequence number (see [[#Acknowledge Block Change]]).
 |}

Status can be one of seven values:

{| class="wikitable"
 ! Value
 ! Meaning
 ! Notes
 |-
 | 0
 | Started digging
 | Sent when the player starts digging a block. If the block was instamined or the player is in creative mode, the client will ''not'' send Status = Finished digging, and will assume the server completed the destruction. To detect this, it is necessary to [[Breaking#Speed|calculate the block destruction speed]] server-side.
 |-
 | 1
 | Cancelled digging
 | Sent when the player lets go of the Mine Block key (default: left click). Face is always set to -Y.
 |-
 | 2
 | Finished digging
 | Sent when the client thinks it is finished.
 |-
 | 3
 | Drop item stack
 | Triggered by using the Drop Item key (default: Q) with the modifier to drop the entire selected stack (default: Control or Command, depending on OS). Location is always set to 0/0/0, Face is always set to -Y. Sequence is always set to 0.
 |-
 | 4
 | Drop item
 | Triggered by using the Drop Item key (default: Q). Location is always set to 0/0/0, Face is always set to -Y. Sequence is always set to 0.
 |-
 | 5
 | Shoot arrow / finish eating
 | Indicates that the currently held item should have its state updated such as eating food, pulling back bows, using buckets, etc. Location is always set to 0/0/0, Face is always set to -Y. Sequence is always set to 0.
 |-
 | 6
 | Swap item in hand
 | Used to swap or assign an item to the second hand. Location is always set to 0/0/0, Face is always set to -Y. Sequence is always set to 0.
 |}

The Face field can be one of the following values, representing the face being hit:

{| class="wikitable"
 |-
 ! Value
 ! Offset
 ! Face
 |-
 | 0
 | -Y
 | Bottom
 |-
 | 1
 | +Y
 | Top
 |-
 | 2
 | -Z
 | North
 |-
 | 3
 | +Z
 | South
 |-
 | 4
 | -X
 | West
 |-
 | 5
 | +X
 | East
 |}

==== Player Command ====

Sent by the client to indicate that it has performed certain actions: sneaking (crouching), sprinting, exiting a bed, jumping with a horse, and opening a horse's inventory while riding it.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="3"| ''protocol:''<br/><code>0x28</code><br/><br/>''resource:''<br/><code>player_command</code>
 | rowspan="3"| Play
 | rowspan="3"| Server
 | Entity ID
 | {{Type|VarInt}}
 | Player ID (ignored by the vanilla server)
 |-
 | Action ID
 | {{Type|VarInt}} {{Type|Enum}}
 | The ID of the action, see below.
 |-
 | Jump Boost
 | {{Type|VarInt}}
 | Only used by the “start jump with horse” action, in which case it ranges from 0 to 100. In all other cases it is 0.
 |}

Action ID can be one of the following values:

{| class="wikitable"
 ! ID
 ! Action
 |-
 | 0
 | Press sneak key
 |-
 | 1
 | Release sneak key
 |-
 | 2
 | Leave bed
 |-
 | 3
 | Start sprinting
 |-
 | 4
 | Stop sprinting
 |-
 | 5
 | Start jump with horse
 |-
 | 6
 | Stop jump with horse
 |-
 | 7
 | Open vehicle inventory
 |-
 | 8
 | Start flying with elytra
 |}

Leave bed is only sent when the “Leave Bed” button is clicked on the sleep GUI, not when waking up in the morning.

Open vehicle inventory is only sent when pressing the inventory key (default: E) while on a horse or chest boat — all other methods of opening such an inventory (involving right-clicking or shift-right-clicking it) do not use this packet.

==== Player Input ====

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="1"| ''protocol:''<br/><code>0x29</code><br/><br/>''resource:''<br/><code>player_input</code>
 | rowspan="1"| Play
 | rowspan="1"| Server
 | Flags
 | {{Type|Unsigned Byte}}
 | Bit mask; see below
 |}

The flags are as follows:

{| class="wikitable"
 |-
 ! Hex Mask
 ! Field
 |-
 | 0x01
 | Forward
 |-
 | 0x02
 | Backward
 |-
 | 0x04
 | Left
 |-
 | 0x08
 | Right
 |-
 | 0x10
 | Jump
 |-
 | 0x20
 | Sneak
 |-
 | 0x40
 | Sprint
 |}

==== Player Loaded ====

Sent by the client after the server starts sending chunks and the player's chunk has loaded.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="1"| ''protocol:''<br/><code>0x2A</code><br/><br/>''resource:''<br/><code>player_loaded</code>
 | rowspan="1"| Play
 | rowspan="1"| Server
 | colspan="3"| ''no fields''
 |}

==== Pong (play) ====

Response to the clientbound packet ([[#Ping (play)|Ping]]) with the same id.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="1"| ''protocol:''<br/><code>0x2B</code><br/><br/>''resource:''<br/><code>pong</code>
 | rowspan="1"| Play
 | rowspan="1"| Server
 | ID
 | {{Type|Int}}
 | id is the same as the ping packet
 |}

==== Change Recipe Book Settings ====

Replaces Recipe Book Data, type 1.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="3"| ''protocol:''<br/><code>0x2C</code><br/><br/>''resource:''<br/><code>recipe_book_change_settings</code>
 | rowspan="3"| Play
 | rowspan="3"| Server
 | Book ID
 | {{Type|VarInt}} {{Type|Enum}}
 | 0: crafting, 1: furnace, 2: blast furnace, 3: smoker.
 |-
 | Book Open
 | {{Type|Boolean}}
 |
 |-
 | Filter Active
 | {{Type|Boolean}}
 |
 |}

==== Set Seen Recipe ====

Sent when recipe is first seen in recipe book. Replaces Recipe Book Data, type 0.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="1"| ''protocol:''<br/><code>0x2D</code><br/><br/>''resource:''<br/><code>recipe_book_seen_recipe</code>
 | rowspan="1"| Play
 | rowspan="1"| Server
 | Recipe ID
 | {{Type|VarInt}}
 | ID of recipe previously defined in Recipe Book Add. 
 |}

==== Rename Item ====

Sent as a player is renaming an item in an anvil (each keypress in the anvil UI sends a new Rename Item packet). If the new name is empty, then the item loses its custom name (this is different from setting the custom name to the normal name of the item). The item name may be no longer than 50 characters long, and if it is longer than that, then the rename is silently ignored.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="1"| ''protocol:''<br/><code>0x2E</code><br/><br/>''resource:''<br/><code>rename_item</code>
 | rowspan="1"| Play
 | rowspan="1"| Server
 | Item name
 | {{Type|String}} (32767)
 | The new name of the item.
 |}

==== Resource Pack Response (play) ====

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="2"| ''protocol:''<br/><code>0x2F</code><br/><br/>''resource:''<br/><code>resource_pack</code>
 | rowspan="2"| Play
 | rowspan="2"| Server
 | UUID
 | {{Type|UUID}}
 | The unique identifier of the resource pack received in the [[#Add_Resource_Pack_(play)|Add Resource Pack (play)]] request.
 |-
 | Result
 | {{Type|VarInt}} {{Type|Enum}}
 | Result ID (see below).
 |}

Result can be one of the following values:


{| class="wikitable"
 ! ID
 ! Result
 |-
 | 0
 | Successfully downloaded
 |-
 | 1
 | Declined
 |-
 | 2
 | Failed to download
 |-
 | 3
 | Accepted
 |-
 | 4
 | Downloaded
 |-
 | 5
 | Invalid URL
 |-
 | 6
 | Failed to reload
 |-
 | 7
 | Discarded
 |}

==== Seen Advancements ====

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="2"| ''protocol:''<br/><code>0x30</code><br/><br/>''resource:''<br/><code>seen_advancements</code>
 | rowspan="2"| Play
 | rowspan="2"| Server
 | Action
 | {{Type|VarInt}} {{Type|Enum}}
 | 0: Opened tab, 1: Closed screen.
 |-
 | Tab ID
 | {{Type|Optional}} {{Type|Identifier}}
 | Only present if action is Opened tab.
 |}

==== Select Trade ====

When a player selects a specific trade offered by a villager NPC.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="1"| ''protocol:''<br/><code>0x31</code><br/><br/>''resource:''<br/><code>select_trade</code>
 | rowspan="1"| Play
 | rowspan="1"| Server
 | Selected slot
 | {{Type|VarInt}}
 | The selected slot in the players current (trading) inventory.
 |}

==== Set Beacon Effect ====

Changes the effect of the current beacon.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="2"| ''protocol:''<br/><code>0x32</code><br/><br/>''resource:''<br/><code>set_beacon</code>
 | rowspan="2"| Play
 | rowspan="2"| Server
 | Primary Effect
 | {{Type|Prefixed Optional}} {{Type|VarInt}}
 | A [https://minecraft.wiki/w/Potion#ID Potion ID].
 |-
 | Secondary Effect
 | {{Type|Prefixed Optional}} {{Type|VarInt}}
 | A [https://minecraft.wiki/w/Potion#ID Potion ID].
 |}

==== Set Held Item (serverbound) ====

Sent when the player changes the slot selection.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="1"| ''protocol:''<br/><code>0x33</code><br/><br/>''resource:''<br/><code>set_carried_item</code>
 | rowspan="1"| Play
 | rowspan="1"| Server
 | Slot
 | {{Type|Short}}
 | The slot which the player has selected (0–8).
 |}

==== Program Command Block ====

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="4"| ''protocol:''<br/><code>0x34</code><br/><br/>''resource:''<br/><code>set_command_block</code>
 | rowspan="4"| Play
 | rowspan="4"| Server
 | Location
 | {{Type|Position}}
 |
 |-
 | Command
 | {{Type|String}} (32767)
 |
 |-
 | Mode || {{Type|VarInt}} {{Type|Enum}} || 0: chain, 1: repeating, 2: impulse.
 |-
 | Flags
 | {{Type|Byte}}
 | 0x01: Track Output (if false, the output of the previous command will not be stored within the command block); 0x02: Is conditional; 0x04: Automatic.
 |}

==== Program Command Block Minecart ====

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="3"| ''protocol:''<br/><code>0x35</code><br/><br/>''resource:''<br/><code>set_command_minecart</code>
 | rowspan="3"| Play
 | rowspan="3"| Server
 | Entity ID
 | {{Type|VarInt}}
 |
 |-
 | Command
 | {{Type|String}} (32767)
 |
 |-
 | Track Output
 | {{Type|Boolean}}
 | If false, the output of the previous command will not be stored within the command block.
 |}

==== Set Creative Mode Slot ====

While the user is in the standard inventory (i.e., not a crafting bench) in Creative mode, the player will send this packet.

Clicking in the creative inventory menu is quite different from non-creative inventory management. Picking up an item with the mouse actually deletes the item from the server, and placing an item into a slot or dropping it out of the inventory actually tells the server to create the item from scratch. (This can be verified by clicking an item that you don't mind deleting, then severing the connection to the server; the item will be nowhere to be found when you log back in.) As a result of this implementation strategy, the "Destroy Item" slot is just a client-side implementation detail that means "I don't intend to recreate this item.". Additionally, the long listings of items (by category, etc.) are a client-side interface for choosing which item to create. Picking up an item from such listings sends no packets to the server; only when you put it somewhere does it tell the server to create the item in that location.

This action can be described as "set inventory slot". Picking up an item sets the slot to item ID -1. Placing an item into an inventory slot sets the slot to the specified item. Dropping an item (by clicking outside the window) effectively sets slot -1 to the specified item, which causes the server to spawn the item entity, etc.. All other inventory slots are numbered the same as the non-creative inventory (including slots for the 2x2 crafting menu, even though they aren't visible in the vanilla client).

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="2"| ''protocol:''<br/><code>0x36</code><br/><br/>''resource:''<br/><code>set_creative_mode_slot</code>
 | rowspan="2"| Play
 | rowspan="2"| Server
 | Slot
 | {{Type|Short}}
 | Inventory slot.
 |-
 | Clicked Item
 | {{Type|Slot}}
 |
 |}

==== Program Jigsaw Block ====

Sent when Done is pressed on the [[Jigsaw Block]] interface.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="8"| ''protocol:''<br/><code>0x37</code><br/><br/>''resource:''<br/><code>set_jigsaw_block</code>
 | rowspan="8"| Play
 | rowspan="8"| Server
 | Location
 | {{Type|Position}}
 | Block entity location
 |-
 | Name
 | {{Type|Identifier}}
 |
 |-
 | Target
 | {{Type|Identifier}}
 |
 |-
 | Pool
 | {{Type|Identifier}}
 |
 |-
 | Final state
 | {{Type|String}} (32767)
 | "Turns into" on the GUI, <code>final_state</code> in NBT.
 |-
 | Joint type
 | {{Type|String}} (32767)
 | <code>rollable</code> if the attached piece can be rotated, else <code>aligned</code>.
 |-
 | Selection priority
 | {{Type|VarInt}}
 |
 |-
 | Placement priority
 | {{Type|VarInt}}
 |
 |}

==== Program Structure Block ====

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="17"| ''protocol:''<br/><code>0x38</code><br/><br/>''resource:''<br/><code>set_structure_block</code>
 | rowspan="17"| Play
 | rowspan="17"| Server
 |-
 | Location
 | {{Type|Position}}
 | Block entity location.
 |-
 | Action
 | {{Type|VarInt}} {{Type|Enum}}
 | An additional action to perform beyond simply saving the given data; see below.
 |-
 | Mode
 | {{Type|VarInt}} {{Type|Enum}}
 | One of SAVE (0), LOAD (1), CORNER (2), DATA (3).
 |-
 | Name
 | {{Type|String}} (32767)
 |
 |-
 | Offset X || {{Type|Byte}}
 | Between -48 and 48.
 |-
 | Offset Y || {{Type|Byte}}
 | Between -48 and 48.
 |-
 | Offset Z || {{Type|Byte}}
 | Between -48 and 48.
 |-
 | Size X || {{Type|Byte}}
 | Between 0 and 48.
 |-
 | Size Y || {{Type|Byte}}
 | Between 0 and 48.
 |-
 | Size Z || {{Type|Byte}}
 | Between 0 and 48.
 |-
 | Mirror
 | {{Type|VarInt}} {{Type|Enum}}
 | One of NONE (0), LEFT_RIGHT (1), FRONT_BACK (2).
 |-
 | Rotation
 | {{Type|VarInt}} {{Type|Enum}}
 | One of NONE (0), CLOCKWISE_90 (1), CLOCKWISE_180 (2), COUNTERCLOCKWISE_90 (3).
 |-
 | Metadata
 | {{Type|String}} (128)
 |
 |-
 | Integrity
 | {{Type|Float}}
 | Between 0 and 1.
 |-
 |Seed
 |{{Type|VarLong}}
 |
 |-
 | Flags
 | {{Type|Byte}}
 | 0x01: Ignore entities; 0x02: Show air; 0x04: Show bounding box; 0x08: Strict placement.
 |}

Possible actions:

* 0 - Update data
* 1 - Save the structure
* 2 - Load the structure
* 3 - Detect size

The vanilla client uses update data to indicate no special action should be taken (i.e. the done button).

==== Set Test Block ====

Updates the value of the [[Test Block]] at the given position.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="3"| ''protocol:''<br/><code>0x39</code><br/><br/>''resource:''<br/><code>set_test_block</code>
 | rowspan="3"| Play
 | rowspan="3"| Server
 | Position
 | {{Type|Position}}
 |
 |-
 | Mode
 | {{Type|VarInt}} {{Type|Enum}}
 | 0: start, 1: log, 2: fail, 3: accept
 |-
 | Message
 | {{Type|String}}
 |
 |}

==== Update Sign ====

This message is sent from the client to the server when the “Done” button is pushed after placing a sign.

The server only accepts this packet after [[#Open Sign Editor|Open Sign Editor]], otherwise this packet is silently ignored.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="6"| ''protocol:''<br/><code>0x3A</code><br/><br/>''resource:''<br/><code>sign_update</code>
 | rowspan="6"| Play
 | rowspan="6"| Server
 | Location
 | {{Type|Position}}
 | Block Coordinates.
 |-
 | Is Front Text
 | {{Type|Boolean}}
 | Whether the updated text is in front or on the back of the sign
 |-
 | Line 1
 | {{Type|String}} (384)
 | First line of text in the sign.
 |-
 | Line 2
 | {{Type|String}} (384)
 | Second line of text in the sign.
 |-
 | Line 3
 | {{Type|String}} (384)
 | Third line of text in the sign.
 |-
 | Line 4
 | {{Type|String}} (384)
 | Fourth line of text in the sign.
 |}

==== Swing Arm ====

Sent when the player's arm swings.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="1"| ''protocol:''<br/><code>0x3B</code><br/><br/>''resource:''<br/><code>swing</code>
 | rowspan="1"| Play
 | rowspan="1"| Server
 | Hand
 | {{Type|VarInt}} {{Type|Enum}}
 | Hand used for the animation. 0: main hand, 1: off hand.
 |}

==== Teleport To Entity ====

Teleports the player to the given entity.  The player must be in spectator mode.

The vanilla client only uses this to teleport to players, but it appears to accept any type of entity.  The entity does not need to be in the same dimension as the player; if necessary, the player will be respawned in the right world.  If the given entity cannot be found (or isn't loaded), this packet will be ignored.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="1"| ''protocol:''<br/><code>0x3C</code><br/><br/>''resource:''<br/><code>teleport_to_entity</code>
 | rowspan="1"| Play
 | rowspan="1"| Server
 | Target Player
 | {{Type|UUID}}
 | UUID of the player to teleport to (can also be an entity UUID).
 |}

==== Test Instance Block Action ====

Tries to perform an action the [[Test Instance Block]] at the given position.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="10"| ''protocol:''<br/><code>0x3D</code><br/><br/>''resource:''<br/><code>test_instance_block_action</code>
 | rowspan="10"| Play
 | rowspan="10"| Server
 | Position
 | {{Type|Position}}
 |
 |-
 | Action
 | {{Type|VarInt}} {{Type|Enum}}
 | 0: init, 1: query, 2: set, 3: reset, 4: save, 5: export, 6: run.
 |-
 | Test
 | {{Type|Prefixed Optional}} {{Type|VarInt}}
 | ID in the <code>minecraft:test_instance_kind</code> registry.
 |-
 | Size X
 | {{Type|VarInt}}
 |
 |-
 | Size Y
 | {{Type|VarInt}}
 |
 |-
 | Size Z
 | {{Type|VarInt}}
 |
 |-
 | Rotation
 | {{Type|VarInt}} {{Type|Enum}}
 | 0: none, 1: clockwise 90&deg;, 2: clockwise 180&deg;, 3: counter-clockwise 90&deg;.
 |-
 | Ignore Entities
 | {{Type|Boolean}}
 |
 |-
 | Status
 | {{Type|VarInt}} {{Type|Enum}}
 | 0: cleared, 1: running, 2: finished.
 |
 |-
 | Error Message
 | {{Type|Prefixed Optional}} {{Type|Text Component}}
 |
 |}

==== Use Item On ====

{| class="wikitable"
 |-
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="9"| ''protocol:''<br/><code>0x3E</code><br/><br/>''resource:''<br/><code>use_item_on</code>
 | rowspan="9"| Play
 | rowspan="9"| Server
 | Hand
 | {{Type|VarInt}} {{Type|Enum}}
 | The hand from which the block is placed; 0: main hand, 1: off hand.
 |-
 | Location
 | {{Type|Position}}
 | Block position.
 |-
 | Face
 | {{Type|VarInt}} {{Type|Enum}}
 | The face on which the block is placed (as documented at [[#Player Action|Player Action]]).
 |-
 | Cursor Position X
 | {{Type|Float}}
 | The position of the crosshair on the block, from 0 to 1 increasing from west to east.
 |-
 | Cursor Position Y
 | {{Type|Float}}
 | The position of the crosshair on the block, from 0 to 1 increasing from bottom to top.
 |-
 | Cursor Position Z
 | {{Type|Float}}
 | The position of the crosshair on the block, from 0 to 1 increasing from north to south.
 |-
 | Inside block
 | {{Type|Boolean}}
 | True when the player's head is inside of a block.
 |-
 | World Border Hit
 | {{Type|Boolean}}
 | Seems to always be false, even when interacting with blocks around or outside the world border, or while the player is outside the border.
 |-
 | Sequence
 | {{Type|VarInt}}
 | Block change sequence number (see [[#Acknowledge Block Change]]).
 |}

Upon placing a block, this packet is sent once.

The Cursor Position X/Y/Z fields (also known as in-block coordinates) are calculated using raytracing. The unit corresponds to sixteen pixels in the default resource pack. For example, let's say a slab is being placed against the south face of a full block. The Cursor Position X will be higher if the player was pointing near the right (east) edge of the face, lower if pointing near the left. The Cursor Position Y will be used to determine whether it will appear as a bottom slab (values 0.0–0.5) or as a top slab (values 0.5-1.0). The Cursor Position Z should be 1.0 since the player was looking at the southernmost part of the block.

Inside block is true when a player's head (specifically eyes) are inside of a block's collision. In 1.13 and later versions, collision is rather complicated and individual blocks can have multiple collision boxes. For instance, a ring of vines has a non-colliding hole in the middle. This value is only true when the player is directly in the box. In practice, though, this value is only used by scaffolding to place in front of the player when sneaking inside of it (other blocks will place behind when you intersect with them -- try with glass for instance).

==== Use Item ====

Sent when pressing the Use Item key (default: right click) with an item in hand.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="4"| ''protocol:''<br/><code>0x3F</code><br/><br/>''resource:''<br/><code>use_item</code>
 | rowspan="4"| Play
 | rowspan="4"| Server
 | Hand
 | {{Type|VarInt}} {{Type|Enum}}
 | Hand used for the animation. 0: main hand, 1: off hand.
 |-
 | Sequence
 | {{Type|VarInt}}
 | Block change sequence number (see [[#Acknowledge Block Change]]).
 |-
 | Yaw
 | {{Type|Float}}
 | Player head rotation along the Y-Axis.
 |-
 | Pitch
 | {{Type|Float}}
 | Player head rotation along the X-Axis.
 |}

The player's rotation is permanently updated according to the Yaw and Pitch fields before performing the action, unless there is no item in the specified hand.

== Navigation ==
{{Navbox Java Edition technical|General}}

[[Category:Protocol Details]]
[[Category:Java Edition protocol]]
{{license wiki.vg}}
