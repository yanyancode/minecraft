{{Hatnote|This page previously contained documentation on text formatting features, which can now be found at [[Text_component_format]].}}

This article details various aspects of Minecraft's chat system. The packets themselves are documented in [[Java Edition protocol/Packets]].

== Client chat mode ==

The client may use the Chat Mode field of the [[Java Edition protocol/Packets#Client Information (configuration)|Client Information]] packet to indicate that it only wants to receive some types of chat messages.

It is the server's responsibility to not send packets if the client has the given type disabled.  However, it is the client's responsibility to not send incorrect chat packets.

Here's a matrix comparing what packets the server should send to clients based on their chat settings:

{| class="wikitable"
 !rowspan="2" | Clientbound packet
 !colspan="3" | Client setting
 !rowspan="2" | Usage
 |-
 ! Full
 ! Commands only
 ! Hidden
 |-
 | [[Java Edition protocol/Packets#Player Chat Message|Player Chat Message]]
 | {{tc|yes|✔}}
 | {{tc|no|✘}}
 | {{tc|no|✘}}
 | Player-initiated chat messages, including the commands <code>/say</code>, <code>/me</code>, <code>/msg</code>, <code>/tell</code>, <code>/w</code> and <code>/teammsg</code>.
 |-
 | [[Java Edition protocol/Packets#Disguised Chat Message|Disguised Chat Message]]
 | {{tc|yes|✔}}
 | {{tc|no|✘}}
 | {{tc|no|✘}}
 | Messages sent by non-players using the commands <code>/say</code>, <code>/me</code>, <code>/msg</code>, <code>/tell</code>, <code>/w</code> and <code>/teammsg</code>.
 |-
 | [[Java Edition protocol/Packets#System Chat Message|System Chat Message]]
 | {{tc|yes|✔}}
 | {{tc|yes|✔}}
 | {{tc|no|✘}}
 | Feedback from running a command, such as "Your game mode has been updated to creative."
 |-
 | [[Java Edition protocol/Packets#System Chat Message|System Chat Message]] (overlay)
 | {{tc|yes|✔}}
 | {{tc|yes|✔}}
 | {{tc|yes|✔}}
 | Game state information that is displayed above the hot bar, such as "You may not rest now, the bed is too far away".
 |}

Here's a matrix comparing what the client may send based on its chat setting:

{| class="wikitable"
 !rowspan="2" | Serverbound packet
 !colspan="3" | Client setting
 |-
 ! Full
 ! Commands only
 ! Hidden
 |-
 | [[Java Edition protocol/Packets#Chat Message|Chat Message]]
 | {{tc|yes|✔}}
 | {{tc|partial|✘<ref group="note">This behavior varies.  The Notchian server <em>previously</em> rejected chat messages, but now allows them to go through (sending them to all players, but they're invisible on the sender's side).  CraftBukkit and derivatives continue to reject this.  See [https://bugs.mojang.com/browse/MC-116824 MC-116824] for more information.</ref>}}
 | {{tc|no|✘}}
 |-
 | [[Java Edition protocol/Packets#Chat Command|Chat Command]]
 | {{tc|yes|✔}}
 | {{tc|yes|✔}}
 | {{tc|no|✘}}
 |}

If the client attempts to send a chat message and the server rejects it, the Notchian server will send that client a [[Java Edition protocol/Packets#System Chat Message|System Chat Message]] (non-overlay) with a red <code>chat.disabled.options</code> translation component (which becomes "Chat disabled in client options.").

== Social Interactions (blocking) ==

1.16 added a ''social interactions screen'' that lets players block chat from other players. Blocking takes place clientside by detecting whether a message is from a blocked player.

[[Java Edition protocol/Packets#Player Chat Message|Player Chat Message]] packets are blocked based on the included sender UUID.

[[Java Edition protocol/Packets#Disguised Chat Message|Disguised Chat Message]] packets are never blocked.

[[Java Edition protocol/Packets#System Chat Message|System Chat Message]] packets (regardless of the Overlay field!) are blocked based on the first occurrence of <code><''playername''></code> anywhere in the message, including split across multiple text components, where ''playername'' may be any text, including the empty string or whitespace. If ''playername'' is the name of a blocked player (matched case-sensitively), the message is blocked. This only occurs if Hide Matched Names is enabled in Chat Settings (the default).

== Notes ==

<references group="note" />

[[Category:Protocol Details]]
[[Category:Java Edition protocol]]
{{license wiki.vg}}
