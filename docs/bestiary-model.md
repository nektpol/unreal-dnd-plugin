# Bestiary model

`FDnd5eCreature` is a Blueprint-visible value in the `Dnd5e` module. The creature header uses the shared record identity and provenance model. Size, creature type, and alignment use the existing size enum and Gameplay Tag vocabulary.

| Field | C++ type | Meaning |
| --- | --- | --- |
| Header | `FDnd5eRecordHeader` | Display name, record identity, and provenance. |
| Sizes | `TArray<EDnd5eSize>` | Ordered size choices. |
| CreatureType and CreatureSubtypes | `FGameplayTag`, `TArray<FString>` | Creature type and authored subtype terms. |
| Alignment | `FGameplayTagContainer` | Tags for an explicit simple alignment. |
| bHasAlignment and AlignmentOptions | `bool`, `TArray<FDnd5eCreatureAlignmentOption>` | Authored field presence and ordered options. Each option retains its `TArray<FString> Codes`, optional chance, note, and special text. |
| ArmorClasses | `TArray<FDnd5eCreatureArmorClass>` | Printed AC values and their source entries. Each entry holds an `int32 Value` and `TArray<FDnd5eContentBlock> Sources`. |
| HitPoints | `FDnd5eCreatureHitPoints` | Printed `int32 Average` and `FDnd5eDiceExpression Formula`. |
| WalkSpeed | `FDnd5eDistance` | Walking distance with its unit. |
| AbilityScores | `FDnd5eCreatureAbilityScores` | Six named `int32` ability scores. |
| Skills | `TArray<FDnd5eCreatureSkillBonus>` | Skill name and signed integer bonus. |
| Senses and Languages | `TArray<FString>` each | Authored senses and languages. |
| PassivePerception | `int32` | Printed passive Perception. |
| ChallengeRating | `FString` | Authored challenge rating text, including fractions. |
| Traits, Actions, BonusActions | `TArray<FDnd5eCreatureEntry>` each | Ordered named entries. Each entry holds a name and `TArray<FDnd5eContentBlock> Entries`. |

Integer members default to zero. Arrays, strings, and tag containers default to empty. The model currently provides the initial stat-block representation. The native `FDnd5eJsonCreatureReader::ReadCreature` reads a selected direct row from a supplied `monster` document by exact authored name and source. It maps all 14 native creature types from a string or an object with a string type and optional string subtypes. The initial stat-block conversion supports numeric and sourced AC entries, signed skill bonuses, and separate trait, action, and bonus action groups. Failure returns diagnostics and an empty creature. Selected copy rows require a future assembly step. Catalog lookup, supplementary lore and media, and editor browsing are pending.

See [Shared models](shared-models.md) for record, measurement, dice, and Content definitions.

### Document audit

`FDnd5eJsonCreatureReader::AuditDocument` parses one supplied document and visits every row. The result separates document diagnostics from row status, diagnostics, and raw field shapes. Field shapes include JSON type and exact pointer, with array indices grouped under a shared path. The inventory captures top-level fields and descends up to four levels below each field. Successful row conversion can still leave fields or inline content outside the initial reader scope.
