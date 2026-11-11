# Bestiary model

`FDnd5eCreature` is a Blueprint-visible value in the `Dnd5e` module. The creature header uses the shared record identity and provenance model. Size, creature type, and alignment use the existing size enum and Gameplay Tag vocabulary.

| Field | C++ type | Meaning |
| --- | --- | --- |
| Header | `FDnd5eRecordHeader` | Display name, record identity, and provenance. |
| Sizes | `TArray<EDnd5eSize>` | Ordered size choices. |
| CreatureType and CreatureSubtypes | `FGameplayTag`, `TArray<FDnd5eCreatureSubtype>` | Single creature type and ordered subtype values with name, optional prefix, and a hidden-prefix flag. Read `Name` for the original subtype term. |
| CreatureTypeChoices | `TArray<FGameplayTag>` | Ordered alternatives. The single type is unset for a choice. |
| Alignment | `FGameplayTagContainer` | Tags for an explicit simple alignment. |
| bHasAlignment and AlignmentOptions | `bool`, `TArray<FDnd5eCreatureAlignmentOption>` | Authored field presence and ordered options. Each option retains its `TArray<FString> Codes`, optional chance, note, and special text. |
| ArmorClasses | `TArray<FDnd5eCreatureArmorClass>` | Ordered numeric or special AC entries. Each holds `bool bHasValue`, `int32 Value`, `TArray<FDnd5eContentBlock> Sources`, `TArray<FDnd5eInlineContent>` for each of `Condition` and `Special`, and `bool bWrapInParentheses`. |
| ArmorClassPresence | `EDnd5eValuePresence` | Distinguishes omitted, null, and present AC arrays. |
| HitPoints | `FDnd5eCreatureHitPoints` | Field presence, printed average and dice formula, or special text in `TArray<FDnd5eInlineContent> Special`. Check `bHasAverage` before reading the numeric value. |
| WalkSpeed | `FDnd5eDistance` | Convenience projection of the top-level walking distance, including authored zero. |
| Movement | `FDnd5eCreatureMovement` | Presence, standard and alternate speeds with conditions, hover, and ordered choice modes with a shared distance and note. |
| AbilityScores | `FDnd5eCreatureAbilityScores` | Six named `int32` ability scores. |
| Skills | `TArray<FDnd5eCreatureSkillBonus>` | Skill name and signed integer bonus. |
| Senses and Languages | `TArray<FString>` each | Authored senses and languages. |
| PassivePerception | `int32` | Printed numeric passive Perception. |
| PassivePerceptionPresence and PassivePerceptionFormula | `EDnd5eValuePresence`, `FString` | Distinguish omitted, null, numeric, and authored formula values. |
| ChallengeRating | `FDnd5eCreatureChallengeRating` | Presence, authored base, lair and coven CR strings, and optional base and lair XP overrides. Use `ChallengeRating.Base` to read the base string. |
| Traits, Actions, BonusActions | `TArray<FDnd5eCreatureEntry>` each | Ordered named entries. Each entry holds a name and `TArray<FDnd5eContentBlock> Entries`. |

Integer members default to zero. Arrays, strings, and tag containers default to empty. The model currently provides the initial stat-block representation. The native `FDnd5eJsonCreatureReader::ReadCreature` reads a selected direct row from a supplied `monster` document by exact authored name and source. It maps all 14 native creature types from a string or an object with a string type and optional plain or prefixed subtypes. The initial stat-block conversion supports numeric, sourced, conditional, and special AC entries, signed skill bonuses, and separate trait, action, and bonus action groups. Failure returns diagnostics and an empty creature. Selected copy rows require a future assembly step. Catalog lookup, supplementary lore and media, and editor browsing are pending.

See [Shared models](shared-models.md) for record, measurement, dice, and Content definitions.

### Document audit

`FDnd5eJsonCreatureReader::AuditDocument` parses one supplied document and visits every row. The result separates document diagnostics from row status, diagnostics, and raw field shapes. Field shapes include JSON type and exact pointer, with array indices grouped under a shared path. The inventory captures top-level fields and descends up to four levels below each field. Successful row conversion can still leave fields or inline content outside the initial reader scope.
