Registries are repositories of data that contain entries pertaining to certain aspects of the game, such as the world, the player, among others.

The ability for the server to send customized registries to the client was introduced in 1.16.3, which allows for a great deal of customization over certain features of the game.

== Overview ==

The server sends these registries to the client via [[Minecraft Wiki:Projects/wiki.vg merge/Protocol#Registry_Data|Registry Data]] packet during the configuration phase.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! colspan="2"| Field Name
 ! colspan="2"| Field Type
 ! Notes
 |-
 | rowspan="3"| [[Minecraft Wiki:Projects/wiki.vg merge/Protocol#Registry_Data_2|Varies]]
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


The structure of the entries' data depends on the registry specified in the packet. The structure for each [[#Available_Registries|available registry]] is defined below.

Throughout the configuration phase, the server will send multiple [[Minecraft Wiki:Projects/wiki.vg merge/Protocol#Registry_Data|Registry Data]] packets, each one pertaining to a different registry.

=== Client/Server Exchange ===

In order to save bandwidth, the server omits the data for entries pertaining to a data pack that is mutually supported by both the client and server. The exchange is as follows:

# '''S'''→'''C''': [[Minecraft Wiki:Projects/wiki.vg merge/Protocol#Clientbound_Known_Packs|Clientbound Known Packs]]
# '''C'''→'''S''': [[Minecraft Wiki:Projects/wiki.vg merge/Protocol#Serverbound_Known_Packs|Serverbound Known Packs]]
# ''Server computes the mutually supported data packs''
# '''S'''→'''C''': Multiple [[Minecraft Wiki:Projects/wiki.vg merge/Protocol#Registry_Data|Registry Data]] (excluding mutually supported data)

{{Warning|The ordering in which the entries of a registry are sent defines the numeric ID that they will be assigned to. It is essential to maintain consistency between server and client, since many parts of the protocol reference these entries by their ID. The client will disconnect upon receiving a reference to a non-existing entry.}}

== Available Registries ==

The current release specification allows for nine different registries to be sent to the client. A brief explanation of each of them is presented below, along with the format of their entries' element field.

'''Fields marked in <span style="border: solid 1px black; background: #d4ecfc; color: #d4ecfc;">__</span> blue represent data for server-side exclusive operations, and thus have no visible impact on the client.'''

=== Armor Trim Material ===

The <code>minecraft:trim_material</code> registry. It defines various visual properties of trim materials in armors. 

{| class="wikitable"
 ! colspan="3"|Name
 ! colspan="2"|Type
 !style="width: 250px;" colspan="2"| Meaning
 ! colspan="2"|Values
 |-
 | colspan="3"| asset_name
 | colspan="2"| [[Minecraft Wiki:Projects/wiki.vg merge/NBT#Specification:string_tag|String Tag]]
 | colspan="2"| The trim color model to be rendered on top of the armor.
The Notchian client uses the corresponding asset located at <code>trims/color_palettes</code>.
 | colspan="2"| Example: "minecraft:amethyst".
 |-
 | colspan="3"| ingredient
 | colspan="2"| [[Minecraft Wiki:Projects/wiki.vg merge/NBT#Specification:string_tag|String Tag]]
 | colspan="2"| The ingredient used.
This has the visual effect of showing the trimmed armor model on the Smithing Table when the correct item is placed.
 | colspan="2"| Example: "minecraft:copper_ingot".
 |-
 | colspan="3"| item_model_index
 | colspan="2"| [[Minecraft Wiki:Projects/wiki.vg merge/NBT#Specification:float_tag|Float Tag]]
 | colspan="2"| Color index of the trim on the armor item when in the inventory.
 | colspan="2"| Default values vary between 0.1 and 1.0.
 |-
 | colspan="3"| override_armor_materials
 | colspan="2"| Optional [[Minecraft Wiki:Projects/wiki.vg merge/NBT#Specification:compound_tag|Compound Tag]]
 | colspan="2"| Asset for different types of armor materials, which overrides the value specified in the asset_name field.
The Notchian client uses this to give a darker color shade when a trim material is applied to armor of the same material, such as iron applied to iron armor.
 | colspan="2"| The key can be either:
* <code>leather</code>
* <code>chainmail</code>
* <code>iron</code>
* <code>gold</code>
* <code>diamond</code>
* <code>turtle</code>
* <code>netherite</code>
The value accepts the same values as asset_name.
 |-
 | colspan="3"| description
 | colspan="2"| [[Minecraft Wiki:Projects/wiki.vg merge/NBT#Specification:compound_tag|Compound Tag]] or [[Minecraft Wiki:Projects/wiki.vg merge/NBT#Specification:string_tag|String Tag]]
 | colspan="2"| The name of the trim material to be displayed on the armor tool-tip.
Any styling used in this component is also applied to the trim pattern description.
 | colspan="2"| See [[Minecraft Wiki:Projects/wiki.vg merge/Text formatting|Text formatting]].
 |}

=== Armor Trim Pattern ===

The <code>minecraft:trim_pattern</code> registry. It defines various visual properties of trim patterns in armors.

{{missing info|section|The <code>decal</code> field seems to exist to prevent overlapping between the armor model and the trim model. What exact effect does it have?}}

{| class="wikitable"
 ! colspan="3"|Name
 ! colspan="2"|Type
 !style="width: 250px;" colspan="2"| Meaning
 ! colspan="2"|Values
 |-
 | colspan="3"| asset_id
 | colspan="2"| [[Minecraft Wiki:Projects/wiki.vg merge/NBT#Specification:string_tag|String Tag]]
 | colspan="2"| The trim pattern model to be rendered on top of the armor.
The Notchian client uses the corresponding asset located at <code>trims/models/armor</code>.
 | colspan="2"| Example: "minecraft:coast".
 |-
 | colspan="3"| template_item
 | colspan="2"| [[Minecraft Wiki:Projects/wiki.vg merge/NBT#Specification:string_tag|String Tag]]
 | colspan="2"| The template item used for this trim.
This has the visual effect of showing the trimmed armor model on the Smithing Table when the correct item is placed.
 | colspan="2"| Example: "minecraft:coast_armor_trim_smithing_template".
 |-
 | colspan="3"| description
 | colspan="2"| [[Minecraft Wiki:Projects/wiki.vg merge/NBT#Specification:compound_tag|Compound Tag]] or [[Minecraft Wiki:Projects/wiki.vg merge/NBT#Specification:string_tag|String Tag]]
 | colspan="2"| The name of the trim pattern to be displayed on the armor tool-tip.
 | colspan="2"| See [[Minecraft Wiki:Projects/wiki.vg merge/Text formatting|Text formatting]].
 |-
 | colspan="3"| decal
 | colspan="2"| [[Minecraft Wiki:Projects/wiki.vg merge/NBT#Specification:byte_tag|Byte Tag]]
 | colspan="2"| Whether this trim is a decal.
 | colspan="2"| 1: true, 0: false.
 |}

=== Banner Pattern ===

The <code>minecraft:banner_pattern</code> registry. It defines the textures for different banner patterns. 

{| class="wikitable"
 ! Name
 ! Type
 ! Meaning
 ! Values
 |-
 | asset_id
 | [[Minecraft Wiki:Projects/wiki.vg merge/NBT#Specification:string_tag|String Tag]]
 | The texture of the pattern.
The Notchian client uses the corresponding asset located at <code>textures/entity/banner</code> or <code>textures/entity/shield</code>, depending on the case.
 | Example: "minecraft:diagonal_left".
 |-
 | translation_key
 | [[Minecraft Wiki:Projects/wiki.vg merge/NBT#Specification:string_tag|String Tag]]
 | The translation key representing the banner pattern, shown in the item's tooltip.
It is appended to the banner color when used, resulting in <code><translation_key>.<color></code>.
 | Example: "block.minecraft.banner.diagonal_left.blue", which translates to "Blue Per Bend Sinister".
 |}

=== Biome ===

The <code>minecraft:worldgen/biome</code> registry. It defines several aesthetic characteristics of the biomes present in the game.

Biome entries are referenced in the [[Minecraft Wiki:Projects/wiki.vg merge/Protocol#Chunk_Biomes|Chunk Biomes]] and [[Minecraft Wiki:Projects/wiki.vg merge/Protocol#Chunk_Data_and_Update_Light|Chunk Data and Update Light]] packets.

{| class="wikitable"
 ! colspan="3"|Name
 ! colspan="2"|Type
 !style="width: 250px;" colspan="2"| Meaning
 ! colspan="2"|Values
 |-
 | colspan="3"| has_precipitation
 | colspan="2"| [[Minecraft Wiki:Projects/wiki.vg merge/NBT#Specification:byte_tag|Byte Tag]]
 | colspan="2"| Determines whether or not the biome has precipitation.
 | colspan="2"| 1: true, 0: false. 
 |-
 | colspan="3"| temperature
 | colspan="2"| [[Minecraft Wiki:Projects/wiki.vg merge/NBT#Specification:float_tag|Float Tag]]
 | colspan="2"| The temperature factor of the biome.
Affects foliage and grass color if they are not explicitly set.
 | colspan="2"| The default values vary between -0.5 and 2.0.
 |-
 | colspan="3"| temperature_modifier
 | colspan="2"| Optional [[Minecraft Wiki:Projects/wiki.vg merge/NBT#Specification:string_tag|String Tag]]
 | colspan="2"| Modifier that affects the resulting temperature.
 | colspan="2"| Can be either:
* <code>none</code>, for a static temperature throughout the biome (aside from variations depending on the height).
* <code>frozen</code>, for pockets of warm temperature (0.2) to be randomly distributed throughout the biome. This is used on frozen ocean variants in the Notchian client to simulate spots of unfrozen water, where it always rains instead of snowing.
 |-
 | colspan="3"| downfall
 | colspan="2"| [[Minecraft Wiki:Projects/wiki.vg merge/NBT#Specification:float_tag|Float Tag]]
 | colspan="2"| The downfall factor of the biome.
Affects foliage and grass color if they are not explicitly set.
 | colspan="2"| The default values vary between 0.0 and 1.0.
 |-
 | colspan="3"| effects
 | colspan="2"| [[Minecraft Wiki:Projects/wiki.vg merge/NBT#Specification:compound_tag|Compound Tag]]
 | colspan="2"| Biome special effects.
 | colspan="2"| See [[#Effects|Effects]].
 |}

==== Effects ====

{| class="wikitable"
 ! colspan="3"|Name
 ! colspan="2"|Type
 !style="width: 250px;" colspan="2"| Meaning
 ! colspan="2"|Values
 |-
 | colspan="3"| fog_color
 | colspan="2"| [[Minecraft Wiki:Projects/wiki.vg merge/NBT#Specification:int_tag|Int Tag]]
 | colspan="2"| The color of the fog effect when looking past the view distance.
 | colspan="2"| Example: 8364543, which is #7FA1FF in RGB.
 |-
 | colspan="3"| water_color
 | colspan="2"| [[Minecraft Wiki:Projects/wiki.vg merge/NBT#Specification:int_tag|Int Tag]]
 | colspan="2"| The tint color of the water blocks.
 | colspan="2"| Example: 8364543, which is #7FA1FF in RGB.
 |-
 | colspan="3"| water_fog_color
 | colspan="2"| [[Minecraft Wiki:Projects/wiki.vg merge/NBT#Specification:int_tag|Int Tag]]
 | colspan="2"| The color of the fog effect when looking past the view distance when underwater.
 | colspan="2"| Example: 8364543, which is #7FA1FF in RGB.
 |-
 | colspan="3"| sky_color
 | colspan="2"| [[Minecraft Wiki:Projects/wiki.vg merge/NBT#Specification:int_tag|Int Tag]]
 | colspan="2"| The color of the sky.
 | colspan="2"| Example: 8364543, which is #7FA1FF in RGB.
 |-
 | colspan="3"| foliage_color
 | colspan="2"| Optional [[Minecraft Wiki:Projects/wiki.vg merge/NBT#Specification:int_tag|Int Tag]]
 | colspan="2"| The tint color of leaves.
If not specified, the foliage color is calculated based on biome <code>temperature</code> and <code>downfall</code>.
 | colspan="2"| Example: 8364543, which is #7FA1FF in RGB.
 |-
 | colspan="3"| grass_color
 | colspan="2"| Optional [[Minecraft Wiki:Projects/wiki.vg merge/NBT#Specification:int_tag|Int Tag]]
 | colspan="2"| The tint color of the grass.
If not specified, the grass color is calculated based on biome <code>temperature</code> and <code>downfall</code>.
 | colspan="2"| Example: 8364543, which is #7FA1FF in RGB.
 |-
 | colspan="3"| grass_color_modifier
 | colspan="2"| Optional [[Minecraft Wiki:Projects/wiki.vg merge/NBT#Specification:string_tag|String Tag]]
 | colspan="2"| Modifier that affects the resulting grass color. 
 | colspan="2"| Can be either:
* <code>none</code>, for a static grass color throughout the biome.
* <code>dark_forest</code>, for a darker, and less saturated shade of the color.
* <code>swamp</code>, for overriding it with two fixed colors (#4C763C and #6A7039), randomly distributed throughout the biome.
 |-
 | colspan="3"| particle
 | colspan="2"| Optional [[Minecraft Wiki:Projects/wiki.vg merge/NBT#Specification:compound_tag|Compound Tag]]
 | colspan="2"| Ambient visual particles.
 | colspan="2"| See [[#Particle|Particle]].
 |-
 | colspan="3"| ambient_sound
 | colspan="2"| Optional [[Minecraft Wiki:Projects/wiki.vg merge/NBT#Specification:string_tag|String Tag]] or [[Minecraft Wiki:Projects/wiki.vg merge/NBT#Specification:compound_tag|Compound Tag]]
 | colspan="2"| Ambient soundtrack that starts playing when entering the biome, and fades out when exiting it.
 | colspan="2"| Can be either:
* as a [[Minecraft Wiki:Projects/wiki.vg merge/NBT#Specification:string_tag|String Tag]]: the ID of a soundtrack, such as "minecraft:ambient.basalt_deltas.loop".
* as a [[Minecraft Wiki:Projects/wiki.vg merge/NBT#Specification:compound_tag|Compound Tag]]: see [[#Ambient sound|Ambient sound]].
 |-
 | colspan="3"| mood_sound
 | colspan="2"| Optional [[Minecraft Wiki:Projects/wiki.vg merge/NBT#Specification:compound_tag|Compound Tag]]
 | colspan="2"| Additional ambient sound that plays in moody situations. Moodiness increases when blocks around the player are at both sky and block light level zero, and decreases otherwise.
The moodiness calculation happens once per tick, and after reaching a certain value, the ambient mood sound is played.
 | colspan="2"| See [[#Mood sound|Mood sound]].
 |-
 | colspan="3"| additions_sound
 | colspan="2"| Optional [[Minecraft Wiki:Projects/wiki.vg merge/NBT#Specification:compound_tag|Compound Tag]]
 | colspan="2"| Additional ambient sound that has a chance of playing randomly every tick.
 | colspan="2"| See [[#Additions sound|Additions sound]].
 |-
 | colspan="3"| music
 | colspan="2"| Optional [[Minecraft Wiki:Projects/wiki.vg merge/NBT#Specification:compound_tag|Compound Tag]]
 | colspan="2"| Music properties for the biome.
 | colspan="2"| See [[#Music|Music]].
 |}

==== Particle ====

{| class="wikitable"
 ! colspan="3"|Name
 ! colspan="2"|Type
 !style="width: 250px;" colspan="2"| Meaning
 ! colspan="2"|Values
 |-
 | colspan="3"| options
 | colspan="2"| [[Minecraft Wiki:Projects/wiki.vg merge/NBT#Specification:compound_tag|Compound Tag]]
 | colspan="2"| Particle type and related options.
 | colspan="2"| See [[#Particle options|Particle options]].
 |-
 | colspan="3"| probability
 | colspan="2"| [[Minecraft Wiki:Projects/wiki.vg merge/NBT#Specification:float_tag|Float Tag]]
 | colspan="2"| The chance for the particle to be spawned. Ambient particles are attempted to be spawned multiple times every tick.
 | colspan="2"| The default values vary between 0.0 and 1.0.
 |}

=== Particle options ===

{{missing info|section|The extra data specified in the [[Minecraft Wiki:Projects/wiki.vg merge/Protocol#Particle|Particle]] definitions is missing information to allow the particle to be successfully serialized as NBT.<br>Is it here the best place to define them, or somewhere else?}}

{| class="wikitable"
 ! colspan="3"|Name
 ! colspan="2"|Type
 !style="width: 250px;" colspan="2"| Meaning
 ! colspan="2"|Values
 |-
 | colspan="3"| type
 | colspan="2"| [[Minecraft Wiki:Projects/wiki.vg merge/NBT#Specification:string_tag|String Tag]]
 | colspan="2"| The name of the particle.
 | colspan="2"| See [[Minecraft Wiki:Projects/wiki.vg merge/Protocol#Particle|protocol particle data]].
 |-
 | colspan="3"| value
 | colspan="2"| Optional Varies
 | colspan="2"| Any necessary extra data to fully define the particle.
 | colspan="2"| See [[Minecraft Wiki:Projects/wiki.vg merge/Protocol#Particle|protocol particle data]].
 |}

==== Ambient sound ====

{| class="wikitable"
 ! colspan="3"|Name
 ! colspan="2"|Type
 !style="width: 250px;" colspan="2"| Meaning
 ! colspan="2"|Values
 |-
 | colspan="3"| sound_id
 | colspan="2"| [[Minecraft Wiki:Projects/wiki.vg merge/NBT#Specification:string_tag|String Tag]]
 | colspan="2"| The ID of a soundtrack
 | colspan="2"| Example: "minecraft:ambient.basalt_deltas.loop"
 |-
 | colspan="3"| range
 | colspan="2"| Optional [[Minecraft Wiki:Projects/wiki.vg merge/NBT#Specification:float_tag|Float Tag]]
 | colspan="2"| The range of the sound. If not specified, the volume is used to calculate the effective range.
 | colspan="2"|
 |}

==== Mood sound ====

{| class="wikitable"
 ! colspan="3"|Name
 ! colspan="2"|Type
 !style="width: 250px;" colspan="2"| Meaning
 ! colspan="2"|Values
 |-
 | colspan="3"| sound
 | colspan="2"| [[Minecraft Wiki:Projects/wiki.vg merge/NBT#Specification:string_tag|String Tag]]
 | colspan="2"| The ID of a soundtrack.
 | colspan="2"| Example: "minecraft:ambient.basalt_deltas.mood"
 |-
 | colspan="3"| tick_delay
 | colspan="2"| [[Minecraft Wiki:Projects/wiki.vg merge/NBT#Specification:int_tag|Int Tag]]
 | colspan="2"| Defines the rate at which the moodiness increase, and also the minimum time between plays.
 | colspan="2"| The default value is always 6000.
 |-
 | colspan="3"| block_search_extent
 | colspan="2"| [[Minecraft Wiki:Projects/wiki.vg merge/NBT#Specification:int_tag|Int Tag]]
 | colspan="2"| The radius used for the block search around the player during moodiness calculation.
 | colspan="2"| The default value is always 8.
 |-
 | colspan="3"| offset
 | colspan="2"| [[Minecraft Wiki:Projects/wiki.vg merge/NBT#Specification:double_tag|Double Tag]]
 | colspan="2"| The distance offset from the player when playing the sound.
The sound plays in the direction of the selected block during moodiness calculation, and is magnified by the offset.
 | colspan="2"| The default value is always 2.0.
 |}

==== Additions sound ====

{| class="wikitable"
 ! colspan="3"|Name
 ! colspan="2"|Type
 !style="width: 250px;" colspan="2"| Meaning
 ! colspan="2"|Values
 |-
 | colspan="3"| sound
 | colspan="2"| [[Minecraft Wiki:Projects/wiki.vg merge/NBT#Specification:string_tag|String Tag]]
 | colspan="2"| The ID of a soundtrack.
 | colspan="2"| Example: "minecraft:ambient.basalt_deltas.additions"
 |-
 | colspan="3"| tick_chance
 | colspan="2"| [[Minecraft Wiki:Projects/wiki.vg merge/NBT#Specification:double_tag|Double Tag]]
 | colspan="2"| The chance of the sound playing during the tick.
 | colspan="2"| The default value is always 0.0111.
 |}

==== Music ====

{| class="wikitable"
 ! colspan="3"|Name
 ! colspan="2"|Type
 !style="width: 250px;" colspan="2"| Meaning
 ! colspan="2"|Values
 |-
 | colspan="3"| sound
 | colspan="2"| [[Minecraft Wiki:Projects/wiki.vg merge/NBT#Specification:string_tag|String Tag]]
 | colspan="2"| The ID of a soundtrack.
 | colspan="2"| Example: "minecraft:music.nether.basalt_deltas"
 |-
 | colspan="3"| min_delay
 | colspan="2"| [[Minecraft Wiki:Projects/wiki.vg merge/NBT#Specification:int_tag|Int Tag]]
 | colspan="2"| The minimum time in ticks since the last music finished for this music to be able to play.
 | colspan="2"| The default value is always 12000.
 |-
 | colspan="3"| max_delay
 | colspan="2"| [[Minecraft Wiki:Projects/wiki.vg merge/NBT#Specification:int_tag|Int Tag]]
 | colspan="2"| The maximum time in ticks since the last music finished for this music to be able to play.
 | colspan="2"| The default value is always 24000.
 |-
 | colspan="3"| replace_current_music
 | colspan="2"| [[Minecraft Wiki:Projects/wiki.vg merge/NBT#Specification:byte_tag|Byte Tag]]
 | colspan="2"| Whether this music can replace the current one.
 | colspan="2"| 1: true, 0: false. 
 |}

=== Chat Type ===

The <code>minecraft:chat_type</code> registry. It defines the different types of in-game chat and how they're formatted.

Chat type entries are referenced in the [[Minecraft Wiki:Projects/wiki.vg merge/Protocol#Disguised_Chat_Message|Disguised Chat Message]] and [[Minecraft Wiki:Projects/wiki.vg merge/Protocol#Player_Chat_Message|Player Chat Message]] packets.

{| class="wikitable"
 ! colspan="3"|Name
 ! colspan="2"|Type
 !style="width: 250px;" colspan="2"| Meaning
 ! colspan="2"|Values
 |-
 | colspan="3"| chat
 | colspan="2"| [[Minecraft Wiki:Projects/wiki.vg merge/NBT#Specification:compound_tag|Compound Tag]]
 | colspan="2"| The chat decoration.
 | colspan="2"| See [[#Decoration|Decoration]].
 |-
 | colspan="3"| narration
 | colspan="2"| [[Minecraft Wiki:Projects/wiki.vg merge/NBT#Specification:compound_tag|Compound Tag]]
 | colspan="2"| The narration decoration.
 | colspan="2"| See [[#Decoration|Decoration]].
 |}

==== Decoration ====

{| class="wikitable"
 ! colspan="3"|Name
 ! colspan="2"|Type
 !style="width: 250px;" colspan="2"| Meaning
 ! colspan="2"|Values
 |-
 | colspan="3"| translation_key
 | colspan="2"| [[Minecraft Wiki:Projects/wiki.vg merge/NBT#Specification:string_tag|String Tag]]
 | colspan="2"| The translation key representing the chat format. It can also be a formatting string directly.
 | colspan="2"| Example: "chat.type.text", which translates to "<%s> %s".
 |-
 | colspan="3"| style
 | colspan="2"| Optional [[Minecraft Wiki:Projects/wiki.vg merge/NBT#Specification:compound_tag|Compound Tag]]
 | colspan="2"| Optional styling to be applied on the final message.
Not present in the narration decoration.
 | colspan="2"| See [[Minecraft Wiki:Projects/wiki.vg merge/Text formatting#Styling fields|Text formatting#Styling fields]].
 |-
 | colspan="3"| parameters
 | colspan="2"| [[Minecraft Wiki:Projects/wiki.vg merge/NBT#Specification:list_tag|List Tag]] of [[Minecraft Wiki:Projects/wiki.vg merge/NBT#Specification:string_tag|String Tag]]
 | colspan="2"| Placeholders used when formatting the string given by the translation_key field.
 | colspan="2"| Can be either:
* <code>sender</code>, for the name of the player sending the message.
* <code>target</code>, for the name of the player receiving the message, which may be empty.
* <code>content</code>, for the actual message.
 |}

=== Damage Type ===

The <code>minecraft:damage_type</code> registry. It defines the different types of damage an entity can sustain.

Damage type entries are referenced in the [[Minecraft Wiki:Projects/wiki.vg merge/Protocol#Damage_Event|Damage Event]] packet.

{| class="wikitable"
 ! Name
 ! Type
 !style="width: 40%;" | Meaning
 ! Values
 |- style="background: #d4ecfc;"
 | message_id
 | [[Minecraft Wiki:Projects/wiki.vg merge/NBT#Specification:string_tag|String Tag]]
 | Id of the death message. The full message is displayed as <code>death.attack.<message_id></code>.
 | Example: "onFire".
 |- style="background: #d4ecfc;"
 | scaling
 | [[Minecraft Wiki:Projects/wiki.vg merge/NBT#Specification:string_tag|String Tag]]
 | Whether the damage taken scales with the difficulty.
 | Can be either:
* <code>never</code>
* <code>when_caused_by_living_non_player</code>
* <code>always</code>
 |- style="background: #d4ecfc;"
 | exhaustion
 | [[Minecraft Wiki:Projects/wiki.vg merge/NBT#Specification:float_tag|Float Tag]]
 | The amount of exhaustion caused when suffering this type of damage.
 | Default values are either 0.0 or 0.1.
 |-
 | effects
 | Optional [[Minecraft Wiki:Projects/wiki.vg merge/NBT#Specification:string_tag|String Tag]]
 | Effect played when the player suffers this damage, including the sound that is played.
 | Can be either:
* <code>hurt</code>
* <code>thorns</code>
* <code>drowning</code>
* <code>burning</code>
* <code>poking</code>
* <code>freezing</code>
 |- style="background: #d4ecfc;"
 | death_message_type
 | Optional [[Minecraft Wiki:Projects/wiki.vg merge/NBT#Specification:string_tag|String Tag]]
 | Defines how the death message is constructed.
 | Can be either:
* <code>default</code>, for the message to be built normally.
* <code>fall_variants</code>, for the most significant fall damage to be considered.
* <code>intentional_game_design</code>, for [https://bugs.mojang.com/browse/MCPE-28723 MCPE-28723] to be considered as an argument when translating the message.
 |}

=== Dimension Type ===

The <code>minecraft:dimension_type</code> registry. It defines the types of dimension that can be attributed to a world, along with all their characteristics.

Dimension type entries are referenced in the [[Minecraft Wiki:Projects/wiki.vg merge/Protocol#Login_(play)|Login (play)]] and [[Minecraft Wiki:Projects/wiki.vg merge/Protocol#Respawn|Respawn]] packets.

{| class="wikitable"
 ! Name
 ! Type
 !style="width: 40%;" | Meaning
 ! Values
 |- style="background: #d4ecfc;"
 | fixed_time
 | Optional [[Minecraft Wiki:Projects/wiki.vg merge/NBT#Specification:long_tag|Long Tag]]
 | If set, the time of the day fixed to the specified value.
 | Allowed values vary between 0 and 24000.
 |-
 | has_skylight
 | [[Minecraft Wiki:Projects/wiki.vg merge/NBT#Specification:byte_tag|Byte Tag]]
 | Whether the dimension has skylight access or not.
 | 1: true, 0: false.
 |- style="background: #d4ecfc;"
 | has_ceiling
 | [[Minecraft Wiki:Projects/wiki.vg merge/NBT#Specification:byte_tag|Byte Tag]]
 | Whether the dimension has a bedrock ceiling or not. When true, causes lava to spread faster.
 | 1: true, 0: false.
 |- style="background: #d4ecfc;"
 | ultrawarm
 | [[Minecraft Wiki:Projects/wiki.vg merge/NBT#Specification:byte_tag|Byte Tag]]
 | Whether the dimensions behaves like the nether (water evaporates and sponges dry) or not. Also causes lava to spread thinner.
 | 1: true, 0: false.
 |-
 | natural
 | [[Minecraft Wiki:Projects/wiki.vg merge/NBT#Specification:byte_tag|Byte Tag]]
 | When false, compasses spin randomly. When true, nether portals can spawn zombified piglins.
 | 1: true, 0: false.
 |- style="background: #d4ecfc;"
 | coordinate_scale
 | [[Minecraft Wiki:Projects/wiki.vg merge/NBT#Specification:double_tag|Double Tag]]
 | The multiplier applied to coordinates when traveling to the dimension.
 | Allowed values vary between 1e-5 (0.00001) and 3e7 (30000000).
 |- style="background: #d4ecfc;"
 | bed_works
 | [[Minecraft Wiki:Projects/wiki.vg merge/NBT#Specification:byte_tag|Byte Tag]]
 | Whether players can use a bed to sleep.
 | 1: true, 0: false.
 |- style="background: #d4ecfc;"
 | respawn_anchor_works
 | [[Minecraft Wiki:Projects/wiki.vg merge/NBT#Specification:byte_tag|Byte Tag]]
 | Whether players can charge and use respawn anchors.
 | 1: true, 0: false.
 |-
 | min_y
 | [[Minecraft Wiki:Projects/wiki.vg merge/NBT#Specification:int_tag|Int Tag]]
 | The minimum Y level.
 | Allowed values vary between -2032 and 2031, and must also be a multiple of 16.
{{warning|min_y + height cannot exceed 2032.}}
 |-
 | height
 | [[Minecraft Wiki:Projects/wiki.vg merge/NBT#Specification:int_tag|Int Tag]]
 | The maximum height.
 | Allowed values vary between 16 and 4064, and must also be a multiple of 16.
{{warning|min_y + height cannot exceed 2032.}}
 |- style="background: #d4ecfc;"
 | logical_height
 | [[Minecraft Wiki:Projects/wiki.vg merge/NBT#Specification:int_tag|Int Tag]]
 | The maximum height to which chorus fruits and nether portals can bring players within this dimension. (Must be lower than height)
 | Allowed values vary between 0 and 4064, and must also be a multiple of 16.
{{warning|logical_height cannot exceed the height.}}
 |- style="background: #d4ecfc;"
 | infiniburn
 | [[Minecraft Wiki:Projects/wiki.vg merge/NBT#Specification:string_tag|String Tag]]
 | A resource location defining what block tag to use for infiniburn.
 | "#" or minecraft resource "#minecraft:...".
 |-
 | effects
 | [[Minecraft Wiki:Projects/wiki.vg merge/NBT#Specification:string_tag|String Tag]]
 | Defines special dimensional effects, which includes:
* Cloud level: Height at which clouds appear, if at all.
* Sky type: Whether it's the normal sky with sun and moon; the low-visibility, foggy sky of the nether; or the static sky of the end.
* Forced light map: Whether a bright light map is forced, siimilar to the night vision effect.
* Constant ambient light: Whether blocks have shade on their faces.
 | Can be either:
* <code>minecraft:overworld</code>, for clouds at 192, normal sky type, normal light map and normal ambient light.
* <code>minecraft:the_nether</code>, for '''no clouds''', nether sky type, normal light map and '''constant''' ambient light.
* <code>minecraft:the_end</code>, for '''no clouds''', end sky type, '''forced''' light map and normal ambient light.
 |-
 | ambient_light
 | [[Minecraft Wiki:Projects/wiki.vg merge/NBT#Specification:float_tag|Float Tag]]
 | How much light the dimension has. Used as interpolation factor when calculating the brightness generated from sky light.
 | The default values are 0.0 and 0.1, 0.1 for the nether and 0.0 for the other dimensions. 
 |-
 | piglin_safe
 | [[Minecraft Wiki:Projects/wiki.vg merge/NBT#Specification:byte_tag|Byte Tag]]
 | Whether piglins shake and transform to zombified piglins.
 | 1: true, 0: false.
 |- style="background: #d4ecfc;"
 | has_raids
 | [[Minecraft Wiki:Projects/wiki.vg merge/NBT#Specification:byte_tag|Byte Tag]]
 | Whether players with the Bad Omen effect can cause a raid.
 | 1: true, 0: false.
 |- style="background: #d4ecfc;"
 | monster_spawn_light_level
 | [[Minecraft Wiki:Projects/wiki.vg merge/NBT#Specification:int_tag|Int Tag]] or [[Minecraft Wiki:Projects/wiki.vg merge/NBT#Specification:compound_tag|Compound Tag]]
 | During a monster spawn attempt, this is the maximum allowed light level for it to succeed. It can be either a fixed value, or one of several types of distributions.
 | Can be either:
* as a [[Minecraft Wiki:Projects/wiki.vg merge/NBT#Specification:int_tag|Int Tag]]: Allowed values vary between 0 and 15.
* as a [[Minecraft Wiki:Projects/wiki.vg merge/NBT#Specification:compound_tag|Compound Tag]]: See [https://minecraft.wiki/w/Dimension_type here].
 |- style="background: #d4ecfc;"
 | monster_spawn_block_light_limit
 | [[Minecraft Wiki:Projects/wiki.vg merge/NBT#Specification:int_tag|Int Tag]]
 | Maximum allowed block light level monster spawn attempts to happen.
 | Allowed values vary between 0 and 15.
The default values are 0 and 15, 15 for the nether (where monsters can spawn anywhere) and 0 for other dimensions (where monsters can only spawn naturally in complete darkness).
 |}

=== Wolf Variant ===

The <code>minecraft:wolf_variant</code> registry. It defines the textures for different wolf variants. 

{| class="wikitable"
 ! Name
 ! Type
 ! Meaning
 ! Values
 |-
 | wild_texture
 | [[Minecraft Wiki:Projects/wiki.vg merge/NBT#Specification:string_tag|String Tag]]
 | The texture for the wild version of this wolf.
The Notchian client uses the corresponding asset located at <code>textures</code>.
 | Example: "minecraft:entity/wolf/wolf_ashen".
 |-
 | tame_texture
 | [[Minecraft Wiki:Projects/wiki.vg merge/NBT#Specification:string_tag|String Tag]]
 | The texture for the tamed version of this wolf.
The Notchian client uses the corresponding asset located at <code>textures</code>.
 | Example: "minecraft:entity/wolf/wolf_ashen_tame".
 |-
 | angry_texture
 | [[Minecraft Wiki:Projects/wiki.vg merge/NBT#Specification:string_tag|String Tag]]
 | The texture for the angry version of this wolf.
The Notchian client uses the corresponding asset located at <code>textures</code>.
 | Example: "minecraft:entity/wolf/wolf_ashen_angry".
 |- style="background: #d4ecfc;"
 | biomes
 | [[Minecraft Wiki:Projects/wiki.vg merge/NBT#Specification:string_tag|String Tag]] or [[Minecraft Wiki:Projects/wiki.vg merge/NBT#Specification:list_tag|List Tag]]
 | Biomes in which this wolf can spawn in.
 | See [https://minecraft.wiki/w/Wolf#Wolf_variants here].
 |}

=== Painting Variant ===

The <code>minecraft:painting_variant</code> registry. It defines the textures and dimensions for the game's paintings. 

{| class="wikitable"
 ! Name
 ! Type
 ! Meaning
 ! Values
 |-
 | asset_id
 | [[Minecraft Wiki:Projects/wiki.vg merge/NBT#Specification:string_tag|String Tag]]
 | The texture for the painting.
The Notchian client uses the corresponding asset located at 
<code>textures/painting</code>.
 | Example: "minecraft:backyard".
 |-
 | height
 | [[Minecraft Wiki:Projects/wiki.vg merge/NBT#Specification:int_tag|Int Tag]]
 | The height of the painting, in blocks.
 | Example: <code>2</code>
 |-
 | width
 | [[Minecraft Wiki:Projects/wiki.vg merge/NBT#Specification:int_tag|Int Tag]]
 | The width of the painting, in blocks.
 |
 |-
 | title
 | [[Minecraft Wiki:Projects/wiki.vg merge/NBT#Specification:compound_tag|Compound Tag]] or [[Minecraft Wiki:Projects/wiki.vg merge/NBT#Specification:string_tag|String Tag]]
 | The displayed title of the painting. See [[Minecraft Wiki:Projects/wiki.vg merge/Text formatting|Text formatting]].
 | Example: <code>{"color": "gray", "translate": "painting.minecraft.skeleton.title"}</code>
 |-
 | author
 | [[Minecraft Wiki:Projects/wiki.vg merge/NBT#Specification:compound_tag|Compound Tag]] or [[Minecraft Wiki:Projects/wiki.vg merge/NBT#Specification:string_tag|String Tag]]
 | The displayed author of the painting. See [[Minecraft Wiki:Projects/wiki.vg merge/Text formatting|Text formatting]].
 | Example: <code>{"color": "gray", "translate": "painting.minecraft.skeleton.author"}</code>
 |}

== Default Registries ==

The default content of the registries is available in the JSON format for the following versions:
* [https://gist.github.com/Mansitoh/e6c5cf8bbf17e9faf4e4e75bb3f4789d 1.21]
* [https://gist.github.com/WinX64/ab8c7a8df797c273b32d3a3b66522906 1.20.6]
* [https://gist.github.com/WinX64/3675ffee90360e9fc1e45074e49f6ede 1.20.2]
* [https://gist.github.com/WinX64/2d257d3df3c7ab9c4b02dc90be881ab2 1.20.1]
* [https://gist.github.com/nikes/aff59b758a807858da131a1881525b14 1.19.2]
* [https://gist.github.com/rj00a/f2970a8ce4d09477ec8f16003b9dce86 1.19]

[[Category:Protocol Details]]
[[Category:Java Edition protocol]]

{{license wiki.vg}}
