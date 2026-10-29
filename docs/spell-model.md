# Spell model

Status: FDnd5eSpell, its shared relationship value, supporting structs, and revised native tags are implemented as in-memory storage. Source review and the standard host editor build passed on 2026-10-29. Tests were not run. JSON importing, catalog loading, normalization, and semantic validation remain unimplemented.

The exported, Blueprint-visible record is declared in [Dnd5eSpell.h](../Source/Dnd5e/Public/Spells/Dnd5eSpell.h). Its two spell-context wrappers are in [Dnd5eSpellTypes.h](../Source/Dnd5e/Public/Spells/Dnd5eSpellTypes.h). These declarations represent reference data; consumers own character state and gameplay execution. [Source-format compatibility](source-format-compatibility.md) distinguishes storage from implemented source support.

## Contents

- [Scope](#scope)
- [Implemented fields](#implemented-fields)
- [Shared definitions](#shared-definitions)
- [Shared tag ownership](#shared-tag-ownership)
- [Spell-area association](#spell-area-association)
- [Spell-duration association](#spell-duration-association)
- [Classification and relationship mappings](#classification-and-relationship-mappings)
- [Remaining design work](#remaining-design-work)

## Scope

FDnd5eSpell is a value struct, not a UObject, data asset, running spell instance, or executable effect. It contains supporting values directly and references independently identified records through shared relationship values. It does not generate an ID or select an edition by default.

Local inspection found 969 records in the supplied 5etools 2.39.0 spells-*.json spell arrays. None has an explicit edition field. These are source-record counts, not unique spells or verified importer coverage. Declared edition must not be invented from publication generation; SRD/Basic Rules membership, source generation, and explicit edition are separate metadata roles. No source passages or records are bundled here.

## Implemented fields

All 22 fields are reflected with EditAnywhere/BlueprintReadWrite in category Dnd5e|Spell. Strings, tags, arrays, and nested structs use default construction; Level explicitly defaults to -1. The table follows declaration order.

| Field | C++ type and default | Meaning |
| --- | --- | --- |
| Header | `FDnd5eRecordHeader`, default-constructed | Identity, name, aliases, classification, and provenance |
| Level | `int32 = -1` | Spell level; -1 is unspecified, zero is a cantrip |
| Schools | `FGameplayTagContainer`, empty | School classifications, with room for multiple schools |
| CastingTimes | `TArray<FDnd5eCastingTime>`, empty | Source-ordered casting-time alternatives |
| Range | `FDnd5eRange`, default-constructed | Targeting range, separately from area measurements |
| Areas | `TArray<FDnd5eSpellArea>`, empty | Supported geometry and origin associations |
| Components | `FDnd5eComponents`, default-constructed | Component classifications and material details |
| Durations | `TArray<FDnd5eSpellDuration>`, empty | Source-ordered duration entries with associated requirements |
| Description | `TArray<FDnd5eContentBlock>`, empty | Main structured description |
| HigherLevelDescription | `TArray<FDnd5eContentBlock>`, empty | Separate higher-level/scaling description |
| Traits | `FGameplayTagContainer`, empty | Additional supported spell classifications; initially Ritual |
| DamageInflicted | `FGameplayTagContainer`, empty | Damage types indexed as inflicted by the source |
| DamageResisted | `FGameplayTagContainer`, empty | Damage types indexed under the source resistance role |
| DamageImmunities | `FGameplayTagContainer`, empty | Damage types indexed under the source immunity role |
| DamageVulnerabilities | `FGameplayTagContainer`, empty | Damage types indexed under the source vulnerability role |
| ConditionInflicted | `FGameplayTagContainer`, empty | Conditions indexed as inflicted by the source |
| ConditionImmunities | `FGameplayTagContainer`, empty | Conditions indexed under the source immunity role |
| SavingThrowAbilities | `FGameplayTagContainer`, empty | Abilities mentioned by source saving-throw classification |
| AbilityCheckAbilities | `FGameplayTagContainer`, empty | Abilities mentioned by source ability-check classification |
| AttackTypes | `FGameplayTagContainer`, empty | Melee/ranged spell-attack classifications |
| AffectedCreatureTypes | `FGameplayTagContainer`, empty | Source-provided affected creature-type classifications |
| Relationships | `TArray<FDnd5eRecordRelationship>`, empty | Directed relationships, initially source reprints |

A valid standard supplied level is 0–9; zero means cantrip, while -1 means unspecified. Default construction creates an incomplete authoring value, not a validated spell. Header.RecordId.Category is supplied as Category::Spell by the author/adapter; there is no constructor that automatically selects it. Header holds the record name and provenance, avoiding duplicate Name/source/page fields.

Schools is a container intentionally. Current inspected records each supply one recognized school code; import that one school without fabricating others. The container can represent multiple schools under a supported extension. CastingTimes and Durations preserve the order and association of their source entries. Description and HigherLevelDescription are separate structured documents; higher-level prose is not a parsed scaling formula.

Empty Areas means that no structured areas are represented, not proof that the source spell has no area. Shape classification alone does not establish missing measurements or origin. Empty classification collections likewise do not establish that a rule or exception cannot occur in descriptive text. The declaration has no conversion, extraction, query, validation, or effect-execution methods.

## Shared definitions

Reusable types have one canonical definition in [Shared models](shared-models.md). This document defines their spell-specific composition and contextual associations without duplicating their fields.

| Spell part | Canonical definition |
| --- | --- |
| Header | [Record header and provenance](shared-models.md#record-header) |
| Identity | [Record ID](shared-models.md#record-id) |
| Relationships | [FDnd5eRecordRelationship](shared-models.md#record-relationships) |
| CastingTimes | [FDnd5eCastingTime](shared-models.md#casting-time) |
| Range | [FDnd5eRange](shared-models.md#range) |
| Area geometry/dimensions | [FDnd5eArea and FDnd5eAreaDimension](shared-models.md#area) |
| Components | [FDnd5eComponents and material details](shared-models.md#components) |
| Material cost entries | [FDnd5eMoney](shared-models.md#money) |
| Duration/time measurement | [FDnd5eDuration](shared-models.md#duration) and [FDnd5eTimeSpan](shared-models.md#time-measurement) |
| Structured descriptions | [Shared Content](shared-models.md#shared-structured-content) |
| Native classifications | [Gameplay Tag vocabulary](gameplay-tags.md) |

## Shared tag ownership

Native tag namespaces describe concepts, not their first consuming model. CastingTimes.Unit uses shared Activation::Time designations; Schools uses Magic::School; AttackTypes uses Attack::Mode::{Melee,Ranged}. The field retains its implemented name AttackTypes, but the stored axis is mode, independent of weapon/spell origin. These indexes are not a full attack definition.

Spell::Trait::Ritual means the spell supports ritual casting. It does not choose a casting mode, add a renderer-generated time entry to CastingTimes, or calculate extra time/cost. Trait::Concentration is shared with other contexts and can appear in Traits as a derived filter summary of represented requirements. Keep the detailed requirement association described below; no automatic summary builder is implemented.

Damage, condition, ability-score, creature, range, area, and relationship concepts also use shared identities. Preserve role through its field rather than flattening all tags into Header.Classification or a generic effect list. Header.Classification is for record-level metadata roles under a supported classification contract. Its complete edition/membership vocabulary and derivation policy are pending.

## Spell-area association

FDnd5eSpell.Areas is a `TArray<FDnd5eSpellArea>`. The exported, Blueprint-visible wrapper has these fields:

| Field | C++ type and default | Meaning |
| --- | --- | --- |
| Geometry | `FDnd5eArea`, default-constructed | Reusable shape and dimensions |
| Origin | `FGameplayTag`, invalid | Explicit/supported Self, SelectedPoint, or Target origin |
| OriginDescription | `TArray<FDnd5eContentBlock>`, empty | Placement constraints/explanation |

Origin uses Dnd5e::Area::Origin::{Self, SelectedPoint, Target}. Set it only from supported source information; absence stays invalid. Measured targeting range alone does not prove area origin. Geometry and placement describe reference content without collision, targeting execution, or affected-actor queries. Radius, Emanation, and other source forms retain their distinctions. Extracting dimensions or placement from insufficient prose/areaTags is not implemented or implied.

## Spell-duration association

FDnd5eSpell.Durations is a `TArray<FDnd5eSpellDuration>`. Its exported, Blueprint-visible wrapper has these fields:

| Field | C++ type and default | Meaning |
| --- | --- | --- |
| Duration | `FDnd5eDuration`, default-constructed | Reusable duration definition |
| Requirements | `FGameplayTagContainer`, empty | Requirements associated with this duration entry; initially shared concentration |

Requirements uses shared Dnd5e::Trait::Concentration, with tag Dnd5e.Trait.Concentration, where source duration[].concentration supplies the requirement. Keep it associated with that entry; an empty container is not a synthesized NoConcentration tag. The reusable Duration.Kind is EDnd5eDurationKind. A spell Traits summary must be derived and kept consistent rather than treated as another independent authority.

Concentration during casting needs its own future contextual rule contract. A Minute/Hour timing designation does not itself implement or fully express those rules. This storage does not maintain concentration or end effects.

## Classification and relationship mappings

This table records intended source mappings, not implemented parser behavior. Native identities are declared in [Gameplay Tag vocabulary](gameplay-tags.md).

| Source field | Stored field/role | Native branch |
| --- | --- | --- |
| school | Schools | Magic::School; codes A/C/D/E/V/I/N/T map to the eight standard schools, P to parser-supported Psionic |
| meta.ritual true | Traits | Spell::Trait::Ritual |
| spellAttack M/R | AttackTypes | Attack::Mode::Melee/Ranged |
| damageInflict | DamageInflicted | Damage::Type |
| damageResist | DamageResisted | Damage::Type |
| damageImmune | DamageImmunities | Damage::Type |
| damageVulnerable | DamageVulnerabilities | Damage::Type |
| conditionInflict | ConditionInflicted | Condition::Type |
| conditionImmune | ConditionImmunities | Condition::Type |
| savingThrow | SavingThrowAbilities | Ability |
| abilityCheck | AbilityCheckAbilities | Ability |
| affectsCreatureType | AffectedCreatureTypes | Creature::Type |
| reprintedAs | Relationships | Record::Relationship::ReprintedAs plus retained source reference and optional normalized target ID |

Damage/condition indexes do not establish an unconditional effect, amount, target, timing, or exception. SavingThrowAbilities does not encode a DC, success/failure outcome, or save sequence. AbilityCheckAbilities does not imply a skill proficiency. Creature-type indexes are not complete eligibility predicates. Preserve full descriptions until the corresponding typed rule models exist.

The retained field spellings ConditionInflicted and bHasTargetRecord follow the implementation. Reprint links do not make records equivalent, automatically choose a newer edition, or implement target lookup. Source-key normalization and category-specific defaults require an adapter contract before IDs can be constructed reliably.

## Remaining design work

Storage for the first spell record is complete; source importing and complete spell/rule semantics are not. The focused reader still needs an accepted input/Content subset, category-specific ID construction, provenance/classification mapping, unknown-field preservation/diagnostics, and validation behavior.

Further contracts are required for scalingLevelDice, complete areaTags/miscTags meaning, publication/membership metadata, supplemental fluff/media loading, and spell-list availability from the separate generated lookup. Do not use header metadata JSON as a generic replacement for unmodeled rule data, or treat absence of inline class lists as absence of class availability. Unknown attack labels, schools, and other values need explicit handling rather than a guessed native tag.

The next slice reads a deliberately limited external source record into this in-memory shape. Expanding it to every source record, 2014/2024 rules, supplementary files, and all semantics expressed in prose remains subsequent work. Reference data and classification storage do not implement casting, combat, or character progression.
