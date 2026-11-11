# Bestiary model

`FDnd5eCreature` is a Blueprint-visible value in the `Dnd5e` module. The creature header uses the shared record identity and provenance model. Size, creature type, and alignment use the existing size enum and Gameplay Tag vocabulary.

| Field | C++ type | Meaning |
| --- | --- | --- |
| Header | `FDnd5eRecordHeader` | Display name, record identity, and provenance. |
| Sizes | `TArray<EDnd5eSize>` | Ordered size choices. |
| CreatureType and CreatureSubtypes | `FGameplayTag`, `TArray<FString>` | Creature type and authored subtype terms. |
| Alignment | `FGameplayTagContainer` | Alignment traits. |
| ArmorClasses | `TArray<FDnd5eCreatureArmorClass>` | Printed AC values and their source entries. Each entry holds an `int32 Value` and `TArray<FDnd5eContentBlock> Sources`. |
| HitPoints | `FDnd5eCreatureHitPoints` | Printed `int32 Average` and `FDnd5eDiceExpression Formula`. |
| WalkSpeed | `FDnd5eDistance` | Walking distance with its unit. |
| AbilityScores | `FDnd5eCreatureAbilityScores` | Six named `int32` ability scores. |
| Skills | `TArray<FDnd5eCreatureSkillBonus>` | Skill name and signed integer bonus. |
| Senses and Languages | `TArray<FString>` each | Authored senses and languages. |
| PassivePerception | `int32` | Printed passive Perception. |
| ChallengeRating | `FString` | Authored challenge rating text, including fractions. |
| Traits, Actions, BonusActions | `TArray<FDnd5eCreatureEntry>` each | Ordered named entries. Each entry holds a name and `TArray<FDnd5eContentBlock> Entries`. |

Integer members default to zero. Arrays, strings, and tag containers default to empty. The model currently provides the initial stat-block representation. JSON import, catalog lookup, supplementary lore and media, and editor browsing are pending.

See [Shared models](shared-models.md) for record, measurement, dice, and Content definitions.
