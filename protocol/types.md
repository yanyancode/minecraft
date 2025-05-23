<noinclude>{{Exclusive|Java}}
This article defines the '''data types''' used in the [[Java Edition protocol]]. </noinclude>All data sent over the network (except for VarInt and VarLong) is [[wikipedia:Endianness#Big-endian|big-endian]], that is the bytes are sent from most significant byte to least significant byte. The majority of everyday computers are little-endian, therefore it may be necessary to change the endianness before sending data over the network.<includeonly>
{{#ifeq: {{PAGEID}}|{{PAGEID:Java_Edition_Protocol/Packets}}| {{#ifeq: {{REVISIONID}}|{{REVISIONID:Java_Edition_Protocol/Packets}}||{{warning|NOTE: This information is from the '''most recent''' version of the [[Java Edition protocol/Data types|Data types]] article and may not be correct for this protocol version.  Check that article's history for any differences that happened since this version.}}}}}}
</includeonly><noinclude>

== Definitions ==
</noinclude>

{| class="wikitable"
 |-
 ! Name
 ! Size (bytes)
 ! Encodes
 ! Notes
 |-
 ! id=Type:Boolean | {{Type|Boolean}}
 | 1
 | Either false or true
 | True is encoded as <code>0x01</code>, false as <code>0x00</code>.
 |-
 ! id=Type:Byte | {{Type|Byte}}
 | 1
 | An integer between -128 and 127
 | Signed 8-bit integer, [[wikipedia:Two's complement|two's complement]]
 |-
 ! id=Type:Unsigned_Byte | {{Type|Unsigned Byte}}
 | 1
 | An integer between 0 and 255
 | Unsigned 8-bit integer
 |-
 ! id=Type:Short | {{Type|Short}}
 | 2
 | An integer between -32768 and 32767
 | Signed 16-bit integer, two's complement
 |-
 ! id=Type:Unsigned_Short | {{Type|Unsigned Short}}
 | 2
 | An integer between 0 and 65535
 | Unsigned 16-bit integer
 |-
 ! id=Type:Int | {{Type|Int}}
 | 4
 | An integer between -2147483648 and 2147483647
 | Signed 32-bit integer, two's complement
 |-
 ! id=Type:Long | {{Type|Long}}
 | 8
 | An integer between -9223372036854775808 and 9223372036854775807
 | Signed 64-bit integer, two's complement
 |-
 ! id=Type:Float | {{Type|Float}}
 | 4
 | A [[wikipedia:Single-precision floating-point format|single-precision 32-bit IEEE 754 floating point number]]
 | 
 |-
 ! id=Type:Double | {{Type|Double}}
 | 8
 | A [[wikipedia:Double-precision floating-point format|double-precision 64-bit IEEE 754 floating point number]]
 | 
 |-
 ! id=Type:String | {{Type|String}} (n)
 | ≥ 1 <br />≤ (n&times;3) + 3
 | A sequence of [[wikipedia:Unicode|Unicode]] [http://unicode.org/glossary/#unicode_scalar_value scalar values]
 | [[wikipedia:UTF-8|UTF-8]] string prefixed with its size in bytes as a VarInt.  Maximum length of <code>n</code> characters, which varies by context.  The encoding used on the wire is regular UTF-8, ''not'' [https://docs.oracle.com/en/java/javase/18/docs/api/java.base/java/io/DataInput.html#modified-utf-8 Java's "slight modification"].  However, the length of the string for purposes of the length limit is its number of [[wikipedia:UTF-16|UTF-16]] code units, that is, scalar values > U+FFFF are counted as two. Up to <code>n &times; 3</code> bytes can be used to encode a UTF-8 string comprising <code>n</code> code units when converted to UTF-16, and both of those limits are checked.  Maximum <code>n</code> value is 32767.  The + 3 is due to the max size of a valid length VarInt.
 |-
 ! id=Type:Text_Component | {{Type|Text Component}}
 | Varies
 | See [[Text component format]]
 | Encoded as a [[Minecraft Wiki:Projects/wiki.vg merge/NBT|NBT Tag]], with the type of tag used depending on the case:
* As a [[Minecraft Wiki:Projects/wiki.vg merge/NBT#Specification:string_tag|String Tag]]: For components only containing text (no styling, no events etc.).
* As a [[Minecraft Wiki:Projects/wiki.vg merge/NBT#Specification:compound_tag|Compound Tag]]: Every other case.
 |-
 ! id=Type:JSON_Text_Component | {{Type|JSON Text Component}}
 | ≥ 1 <br />≤ (262144&times;3) + 3
 | See [[Text component format]]
 | The maximum permitted length when decoding is 262144, but the vanilla server since 1.20.3 refuses to encode longer than 32767. This may be a bug.
 |-
 ! id=Type:Identifier | {{Type|Identifier}}
 | ≥ 1 <br />≤ (32767&times;3) + 3
 | See [[#Identifier|Identifier]] below
 | Encoded as a String with max length of 32767.
 |- 
 ! id=Type:VarInt | {{Type|VarInt}}
 | ≥ 1 <br />≤ 5
 | An integer between -2147483648 and 2147483647
 | Variable-length data encoding a two's complement signed 32-bit integer; more info in [[#VarInt and VarLong|their section]]
 |-
 ! id=Type:VarLong | {{Type|VarLong}}
 | ≥ 1 <br />≤ 10
 | An integer between -9223372036854775808 and 9223372036854775807
 | Variable-length data encoding a two's complement signed 64-bit integer; more info in [[#VarInt and VarLong|their section]]
 |-
 ! id=Type:Entity_Metadata | {{Type|Entity Metadata}}
 | Varies
 | Miscellaneous information about an entity
 | See [[Java Edition protocol/Entity metadata#Entity Metadata Format|Entity metadata#Entity Metadata Format]]
 |- 
 ! id=Type:Slot | {{Type|Slot}}
 | Varies
 | An item stack in an inventory or container
 | See [[Java Edition protocol/Slot Data|Slot Data]]
 |- 
 ! id=Type:Hashed_Slot | {{Type|Hashed Slot}}
 | Varies
 | Similar to Slot, but with the data component values being sent as a hash instead of their actual contents
 | See [[Java Edition protocol/Slot Data#Hashed Format|Slot Data#Hashed Format]]
 |-
 ! id=Type:NBT | {{Type|NBT}}
 | Varies
 | Depends on context
 | See [[Minecraft Wiki:Projects/wiki.vg merge/NBT|NBT]]
 |-
 ! id=Type:Position | {{Type|Position}}
 | 8
 | An integer/block position: x (-33554432 to 33554431), z (-33554432 to 33554431), y (-2048 to 2047)
 | x as a 26-bit integer, followed by z as a 26-bit integer, followed by y as a 12-bit integer (all signed, two's complement). See also [[#Position|the section below]].
 |-
 ! id=Type:Angle | {{Type|Angle}}
 | 1
 | A rotation angle in steps of 1/256 of a full turn
 | Whether or not this is signed does not matter, since the resulting angles are the same.
 |-
 ! id=Type:UUID | {{Type|UUID}}
 | 16
 | A [[wikipedia:Universally_unique_identifier|UUID]]
 | Encoded as an unsigned 128-bit integer (or two unsigned 64-bit integers: the most significant 64 bits and then the least significant 64 bits)
 |-
 ! id=Type:BitSet | {{Type|BitSet}}
 | Varies
 | See [[#BitSet]] below
 | A length-prefixed bit set.
 |-
 ! id=Type:Fixed_BitSet | {{Type|Fixed BitSet}} (n)
 | ceil(n / 8)
 | See [[#Fixed BitSet]] below
 | A bit set with a fixed length of <var>n</var> bits.
 |-
 ! id=Type:Optional | {{Type|Optional}} X
 | 0 or size of X
 | A field of type X, or nothing
 | Whether or not the field is present must be known from the context.
 |-
 ! id=Type:Prefixed_Optional | {{Type|Prefixed Optional}} X
 | size of {{Type|Boolean}} + (is present [[wikipedia:Ternary conditional operator|?]] Size of X : 0)
 | A boolean and if present, a field of type X
 | The boolean is true if the field is present.
 |-
 ! id=Type:Array | {{Type|Array}} of X
 | length times size of X
 | Zero or more fields of type X
 | The length must be known from the context.
 |-
 ! id=Type:Prefixed_Array | {{Type|Prefixed Array}} of X
 | size of {{Type|VarInt}} + size of X * length
 | See [[#Prefixed Array]] below
 | A length-prefixed array.
 |-
 ! id=Type:Enum | X {{Type|Enum}}
 | size of X
 | A specific value from a given list
 | The list of possible values and how each is encoded as an X must be known from the context. An invalid value sent by either side will usually result in the client being disconnected with an error or even crashing.
 |-
 ! id=Type:EnumSet | {{Type|EnumSet}} (n)
 | ceil(n / 8)
 | id=Type:Fixed_BitSet | {{Type|Fixed BitSet}} (n)
 | A bitset associated to an enum where each bit corresponds to an enum variant. The number of enum variants <var>n</var> must be known from the context.
 |-
 ! id=Type:Byte_Array | {{Type|Byte Array}}
 | Varies
 | Depends on context
 | This is just a sequence of zero or more bytes, its meaning should be explained somewhere else, e.g. in the packet description. The length must also be known from the context.
 |-
 ! id=Type:ID_or | {{Type|ID or}} X
 | size of {{Type|VarInt}} + (size of X or 0)
 | See [[#ID or X]] below
 | Either a registry ID or an inline data definition of type X.
 |-
 ! id=Type:ID_Set | {{Type|ID Set}}
 | Varies
 | See [[#ID Set]] below
 | Set of registry IDs specified either inline or as a reference to a tag.
 |-
 ! id=Type:Sound_Event | {{Type|Sound Event}}
 | Varies
 | See [[#Sound Event]] below
 | Parameters for a sound event.
 |-
 ! id=Type:Chat_Type | {{Type|Chat Type}}
 | Varies
 | See [[#Chat Type]] below
 | Parameters for a direct chat type.
 |-
 ! id=Type:Teleport_Flags | {{Type|Teleport Flags}}
 | 4
 | See [[#Teleport Flags]] below
 | Bit field specifying how a teleportation is to be applied on each axis.
 |-
 ! id=Type:Recipe_Display | {{Type|Recipe Display}}
 | Varies
 | See [[Minecraft Wiki:Projects/wiki.vg merge/Recipes#Recipe Display structure|Recipes#Recipe Display structure]]
 | Description of a recipe for use for use by the client.
 |-
 ! id=Type:Slot_Display | {{Type|Slot Display}}
 | Varies
 | See [[Minecraft Wiki:Projects/wiki.vg merge/Recipes#Slot Display structure|Recipes#Slot Display structure]]
 | Description of a recipe ingredient slot for use for use by the client.
 |-
 ! id=Type:Chunk_Data | {{Type|Chunk Data}}
 | Varies
 | See [[#Chunk Data]] below
 | 
 |-
 ! id=Type:Light_Data | {{Type|Light Data}}
 | Varies
 | See [[#Light Data]] below
 | 
 |}

<noinclude>

== Identifier ==
</noinclude><includeonly>=== Identifier ===</includeonly>

Identifiers are a namespaced location, in the form of <code>minecraft:thing</code>.  If the namespace is not provided, it defaults to <code>minecraft</code> (i.e. <code>thing</code> is <code>minecraft:thing</code>).  Custom content should always be in its own namespace, not the default one.  Both the namespace and value can use all lowercase alphanumeric characters (a-z and 0-9), dot (<code>.</code>), dash (<code>-</code>), and underscore (<code>_</code>). In addition, values can use slash (<code>/</code>). The naming convention is <code>lower_case_with_underscores</code>.  [https://minecraft.net/en-us/article/minecraft-snapshot-17w43a More information].  
For ease of determining whether a namespace or value is valid, here are regular expressions for each:
* Namespace: <code>[a-z0-9.-_]</code>
* Value: <code>[a-z0-9.-_/]</code>

<noinclude>

== VarInt and VarLong ==
</noinclude><includeonly>=== VarInt and VarLong ===</includeonly>

{{:Minecraft Wiki:Projects/wiki.vg merge/VarInt_And_VarLong}}

<noinclude>

== Position ==
</noinclude><includeonly>=== Position ===</includeonly>

<b>Note:</b> What you are seeing here is the latest version of the [[Minecraft Wiki:Projects/wiki.vg merge/Data types|Data types]] article, but the position type was [https://wiki.vg/index.php?title=Data_types&oldid=14345#Position different before 1.14].

64-bit value split into three '''signed''' integer parts:

* x: 26 MSBs
* z: 26 middle bits
* y: 12 LSBs

For example, a 64-bit position can be broken down as follows:

Example value (big endian): <code><span style="outline: solid 2px rgb(255, 0, 0)">01000110000001110110001100</span> <span style="outline: solid 2px rgb(0, 0, 255)">10110000010101101101001000</span> <span style="outline: solid 2px rgb(0, 255, 0)">001100111111</span></code><br>
* The red value is the X coordinate, which is <code>18357644</code> in this example.<br>
* The blue value is the Z coordinate, which is <code>-20882616</code> in this example.<br>
* The green value is the Y coordinate, which is <code>831</code> in this example.<br>

Encoded as follows:

 ((x & 0x3FFFFFF) << 38) | ((z & 0x3FFFFFF) << 12) | (y & 0xFFF)

And decoded as:

 val = read_long();
 x = val >> 38;
 y = val << 52 >> 52;
 z = val << 26 >> 38;

Note: The above assumes that the right shift operator sign extends the value (this is called an [https://en.wikipedia.org/wiki/Arithmetic_shift arithmetic shift]), so that the signedness of the coordinates is preserved. In many languages, this requires the integer type of <code>val</code> to be signed. In the absence of such an operator, the following may be useful:

 if x >= 1 << 25 { x -= 1 << 26 }
 if y >= 1 << 11 { y -= 1 << 12 }
 if z >= 1 << 25 { z -= 1 << 26 }

<noinclude>

== Fixed-point numbers ==
</noinclude><includeonly>=== Fixed-point numbers ===</includeonly>

Some fields may be stored as [https://en.wikipedia.org/wiki/Fixed-point_arithmetic fixed-point numbers], where a certain number of bits represent the signed integer part (number to the left of the decimal point) and the rest represent the fractional part (to the right). Floating point numbers (float and double), in contrast, keep the number itself (mantissa) in one chunk, while the location of the decimal point (exponent) is stored beside it. Essentially, while fixed-point numbers have lower range than floating point numbers, their fractional precision is greater for higher values.

Prior to version 1.9 a fixed-point format with 5 fraction bits and 27 integer bits was used to send entity positions to the client. Some uses of fixed point remain in modern versions, but they differ from that format.

Most programming languages lack support for fractional integers directly, but you can represent them as integers. The following C or Java-like pseudocode converts a double to a fixed-point integer with <var>n</var> fraction bits:

  x_fixed = (int)(x_double * (1 << n));

And back again:

  x_double = (double)x_fixed / (1 << n);

<noinclude>

== Arrays ==
</noinclude><includeonly>=== Arrays ===</includeonly>

The types {{Type|Array}} and {{Type|Prefixed Array}} represent a collection of X in a specified order.

<noinclude>

=== Array ===
</noinclude><includeonly>==== Array ====</includeonly>

Represents a list where the length is not encoded. The length must be known from the context. If the array is empty nothing will be encoded.

A {{Type|String}} Array with the values ["Hello", "World!"] has the following data when encoded:

{| class="wikitable"
 ! Field Name
 ! Field Type
 ! Value
 |-
 | First element
 | {{Type|String}}
 | Hello
 |-
 | Second element
 | {{Type|String}}
 | World!
 |}

<noinclude>

=== Prefixed Array ===
</noinclude><includeonly>==== Prefixed Array ====</includeonly>

Represents an array prefixed by its length. If the array is empty the length will still be encoded.

{| class="wikitable"
 ! Field Name
 ! Field Type
 |-
 | Length
 | {{Type|VarInt}}
 |-
 | Data
 | {{Type|Array}} of X
 |}

<noinclude>

== Bit sets ==
</noinclude><includeonly>=== Bit sets ===</includeonly>

The types {{Type|BitSet}} and {{Type|Fixed BitSet}} represent packed lists of bits. The vanilla implementation uses Java's [https://docs.oracle.com/javase/8/docs/api/java/util/BitSet.html <code>BitSet</code>] class.

<noinclude>

=== BitSet ===
</noinclude><includeonly>==== BitSet ====</includeonly>

Bit sets of type BitSet are prefixed by their length in longs.

{| class="wikitable"
 ! Field Name
 ! Field Type
 ! Meaning
 |-
 | Length
 | {{Type|VarInt}}
 | Number of longs in the following array.  May be 0 (if no bits are set).
 |-
 | Data
 | {{Type|Array}} of {{Type|Long}}
 | A packed representation of the bit set as created by [https://docs.oracle.com/javase/8/docs/api/java/util/BitSet.html#toLongArray-- <code>BitSet.toLongArray</code>].
 |}

The <var>i</var>th bit is set when <code>(Data[i / 64] & (1 << (i % 64))) != 0</code>, where <var>i</var> starts at 0.

<noinclude>

=== Fixed BitSet ===
</noinclude><includeonly>==== Fixed BitSet ====</includeonly>

Bit sets of type Fixed BitSet (n) have a fixed length of <var>n</var> bits, encoded as <code>ceil(n / 8)</code> bytes. Note that this is different from BitSet, which uses longs.

{| class="wikitable"
 ! Field Name
 ! Field Type
 ! Meaning
 |-
 | Data
 | {{Type|Byte Array}} (n)
 | A packed representation of the bit set as created by [https://docs.oracle.com/javase/8/docs/api/java/util/BitSet.html#toByteArray-- <code>BitSet.toByteArray</code>], padded with zeroes at the end to fit the specified length.
 |}

The <var>i</var>th bit is set when <code>(Data[i / 8] & (1 << (i % 8))) != 0</code>, where <var>i</var> starts at 0. This encoding is ''not'' equivalent to the long array in BitSet.

<noinclude>

== Registry references ==
</noinclude><includeonly>=== Registry references ===</includeonly>

<noinclude>

=== ID or X ===
</noinclude><includeonly>==== ID or X ====</includeonly>

Represents a data record of type X, either inline, or by reference to a registry implied by context.

{| class="wikitable"
 ! Field Name
 ! Field Type
 ! Meaning
 |-
 | ID
 | {{Type|VarInt}}
 | 0 if value of type X is given inline; otherwise registry ID + 1.
 |-
 | Value
 | {{Type|Optional}} X
 | Only present if ID is 0.
 |}

<noinclude>

=== ID Set ===
</noinclude><includeonly>==== ID Set ====</includeonly>

Represents a set of IDs in a certain registry (implied by context), either directly (enumerated IDs) or indirectly (tag name).

{| class="wikitable"
 ! Field Name
 ! Field Type
 ! Meaning
 |-
 | Type
 | {{Type|VarInt}}
 | Value used to determine the data that follows. It can be either:
* 0 - Represents a named set of IDs defined by a tag.
* Anything else - Represents an ad-hoc set of IDs enumerated inline.
 |-
 | Tag Name
 | {{Type|Optional}} {{Type|Identifier}}
 | The registry tag defining the ID set. Only present if Type is 0.
 |-
 | IDs
 | {{Type|Optional}} {{Type|Array}} of {{Type|VarInt}}
 | An array of registry IDs. Only present if Type is not 0.<br>The size of the array is equal to <code>Type - 1</code>.
 |}

<noinclude>

== Registry data ==
</noinclude><includeonly>=== Registry data ===</includeonly>

These types are commonly used in conjuction with {{Type|ID or}} X to specify custom data inline.

<noinclude>

=== Sound Event ===
</noinclude><includeonly>==== Sound Event ====</includeonly>

Describes a sound that can be played.

{| class="wikitable"
 ! Name
 ! Type
 ! Description
 |-
 | Sound Name
 | {{Type|Identifier}}
 |
 |-
 | Has Fixed Range
 | {{Type|Boolean}}
 | Whether this sound has a fixed range, as opposed to a variable volume based on distance.
 |-
 | Fixed Range
 | {{Type|Optional}} {{Type|Float}}
 | The maximum range of the sound. Only present if Has Fixed Range is true.
 |}

<noinclude>

=== Chat Type ===
</noinclude><includeonly>==== Chat Type ====</includeonly>

Describes a direct chat type that a message can be sent with.

{| class="wikitable"
 ! Name
 ! Type
 ! Description
 |-
 | Chat
 | (See below)
 |
 |-
 | Narration
 | (See below)
 |
 |}

The chat type decorations look like:

{| class="wikitable"
 ! Name
 ! Type
 ! Description
 |-
 | Translation Key
 | {{Type|String}}
 |
 |-
 | Parameters
 | {{Type|Prefixed Array}} of {{Type|VarInt}} {{Type|Enum}}
 | 0: sender, 1: target, 2: content
 |-
 | Style
 | {{Type|NBT}}
 |
 |}

<noinclude>

== Teleport Flags ==
</noinclude><includeonly>=== Teleport Flags ===</includeonly>

A bit field represented as an {{Type|Int}}, specifying how a teleportation is to be applied on each axis.

In the lower 8 bits of the bit field, a set bit means the teleportation on the corresponding axis is relative, and an unset bit that it is absolute.

{| class="wikitable"
 |-
 ! Hex Mask
 ! Field
 |-
 | 0x0001
 | Relative X
 |-
 | 0x0002
 | Relative Y
 |-
 | 0x0004
 | Relative Z
 |-
 | 0x0008
 | Relative Yaw
 |-
 | 0x0010
 | Relative Pitch
 |-
 | 0x0020
 | Relative Velocity X
 |-
 | 0x0040
 | Relative Velocity Y
 |-
 | 0x0080
 | Relative Velocity Z
 |-
 | 0x0100
 | Rotate velocity according to the change in rotation, ''before'' applying the velocity change in this packet. Combining this with absolute rotation works as expected&mdash;the difference in rotation is still used.
 |}

<noinclude>

== Chunk Data ==
</noinclude><includeonly>=== Chunk Data ===</includeonly>

{| class="wikitable"
 ! colspan="2"| Field Name
 ! colspan="2"| Field Type
 ! Notes
 |-
 | colspan="2"| Heightmaps
 | colspan="2"| {{Type|Prefixed Array}} of Heightmap
 | See [[Minecraft Wiki:Projects/wiki.vg merge/Chunk Format#Heightmaps structure|Chunk Format#Heightmaps structure]]
 |-
 | colspan="2"| Data
 | colspan="2"| {{Type|Prefixed Array}} of {{Type|Byte}}
 | See [[Minecraft Wiki:Projects/wiki.vg merge/Chunk Format#Data structure|Chunk Format#Data structure]]
 |-
 | rowspan="4"| Block Entities
 | Packed XZ
 | rowspan="4"| {{Type|Prefixed Array}}
 | {{Type|Unsigned Byte}}
 | The packed section coordinates are relative to the chunk they are in. Values 0-15 are valid. <pre>packed_xz = ((blockX & 15) << 4) | (blockZ & 15) // encode
x = packed_xz >> 4, z = packed_xz & 15 // decode</pre>
 |-
 | Y
 | {{Type|Short}}
 | The height relative to the world
 |-
 | Type
 | {{Type|VarInt}}
 | The type of block entity
 |-
 | Data
 | [[Minecraft Wiki:Projects/wiki.vg merge/NBT|NBT]]
 | The block entity's data, without the X, Y, and Z values
 |}

<noinclude>

== Light Data ==
</noinclude><includeonly>=== Light Data ===</includeonly>

{| class="wikitable"
 ! colspan="2"| Field Name
 ! colspan="2"| Field Type
 ! Notes
 |-
 | colspan="2"| Sky Light Mask
 | colspan="2"| {{Type|BitSet}}
 | BitSet containing bits for each section in the world + 2.  Each set bit indicates that the corresponding 16×16×16 chunk section has data in the Sky Light array below.  The least significant bit is for blocks 16 blocks to 1 block below the min world height (one section below the world), while the most significant bit covers blocks 1 to 16 blocks above the max world height (one section above the world).
 |-
 | colspan="2"| Block Light Mask
 | colspan="2"| {{Type|BitSet}}
 | BitSet containing bits for each section in the world + 2.  Each set bit indicates that the corresponding 16×16×16 chunk section has data in the Block Light array below.  The order of bits is the same as in Sky Light Mask.
 |-
 | colspan="2"| Empty Sky Light Mask
 | colspan="2"| {{Type|BitSet}}
 | BitSet containing bits for each section in the world + 2.  Each set bit indicates that the corresponding 16×16×16 chunk section has all zeros for its Sky Light data.  The order of bits is the same as in Sky Light Mask.
 |-
 | colspan="2"| Empty Block Light Mask
 | colspan="2"| {{Type|BitSet}}
 | BitSet containing bits for each section in the world + 2.  Each set bit indicates that the corresponding 16×16×16 chunk section has all zeros for its Block Light data.  The order of bits is the same as in Sky Light Mask.
 |-
 | Sky Light arrays
 | Sky Light array
 | {{Type|Prefixed Array}}
 | {{Type|Prefixed Array}} (2048) of {{Type|Byte}}
 | The length of any inner array is always 2048; There is 1 array for each bit set to true in the sky light mask, starting with the lowest value.  Half a byte per light value.
 |-
 | Block Light arrays
 | Block Light array
 | {{Type|Prefixed Array}}
 | {{Type|Prefixed Array}} (2048) of {{Type|Byte}}
 | The length of any inner array is always 2048; There is 1 array for each bit set to true in the block light mask, starting with the lowest value.  Half a byte per light value.
 |}


<noinclude>

== Navigation ==
{{Navbox Java Edition technical|General}}
[[Category:Protocol Details]]
[[Category:Java Edition protocol]]
{{license wiki.vg}}
</noinclude>
