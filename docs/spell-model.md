# Spell model

Status: FDnd5eSpell and its supporting values are implemented, including typed level-based dice scaling and supplementary availability associations. The [JSON spell reader](json-spell-reader.md) maps the inspected primary spell fields and structured Content. The [catalog builder and query API](spell-catalog.md) join a supplied availability lookup and index normalized spells. Unmodeled Content is retained with warnings. General reference resolution and complete rule semantics remain pending. See [automated verification](testing.md) for current build/test evidence.

The exported, Blueprint-visible record is declared in [Dnd5eSpell.h](../Source/Dnd5e/Public/Spells/Dnd5eSpell.h). Its area/duration associations and two level-scaling structs are in [Dnd5eSpellTypes.h](../Source/Dnd5e/Public/Spells/Dnd5eSpellTypes.h). These declarations represent reference data; consumers own character state and gameplay execution. [Source-format compatibility](source-format-compatibility.md) distinguishes storage, mapped input, preservation, and semantic support.

## Contents

- [Scope](#scope)
- [Implemented fields](#implemented-fields)
- [Shared definitions](#shared-definitions)
- [Shared tag ownership](#shared-tag-ownership)
- [Spell-area association](#spell-area-association)
- [Spell-duration association](#spell-duration-association)
- [Level-based dice scaling](#level-based-dice-scaling)
- [Spell availability](#spell-availability)
- [Classification and relationship mappings](#classification-and-relationship-mappings)
- [Remaining design work](#remaining-design-work)

## Scope

FDnd5eSpell is a value struct, not a UObject, data asset, running spell instance, or executable effect. It contains supporting values directly and references independently identified records through shared relationship values. It does not generate an ID or select an edition by default.

Local inspection found 969 records in the supplied 5etools 2.39.0 spells-*.json spell arrays. None has an explicit edition field. These are source-record counts, not unique spells or verified importer coverage. Declared edition must not be invented from publication generation; SRD/Basic Rules membership, source generation, and explicit edition are separate metadata roles. No source passages or records are bundled here.

## Implemented fields

All 25 fields are reflected with EditAnywhere/BlueprintReadWrite in category Dnd5e|Spell. Strings, tags, arrays, and nested structs use default construction; Level explicitly defaults to -1 and bHasAvailabilityData to false. The table follows declaration order.

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
| LevelScaling | `TArray<FDnd5eSpellLevelScaling>`, empty | Ordered labeled groups of source-provided character-level dice entries |
| bHasAvailabilityData | `bool = false` | A matching supplementary lookup entry was supplied and converted |
| Availability | `TArray<FDnd5eSpellAvailability>`, empty | Source associations with classes, subclasses, species, and other providers |

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
| Native classifications | [Native tag declarations](../Source/Dnd5e/Public/Dnd5eTags.h) |

## Shared tag ownership

Native tag namespaces describe concepts, not their first consuming model. CastingTimes.Unit uses shared Activation::Time designations; Schools uses Magic::School; AttackTypes uses Attack::Mode::{Melee,Ranged}. The field retains its implemented name AttackTypes, but the stored axis is mode, independent of weapon/spell origin. These indexes are not a full attack definition.

Spell::Trait::Ritual means the spell supports ritual casting. It does not choose a casting mode, add a renderer-generated time entry to CastingTimes, or calculate extra time/cost. Trait::Concentration is shared with other contexts. The spell reader derives its Traits filter summary from represented duration requirements while preserving the detailed association below. Manually edited values have no automatic observer that keeps this summary synchronized.

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

## Level-based dice scaling

These spell-specific values are declared in Dnd5eSpellTypes.h and exported with DND5E_API. They reuse the shared [dice-expression value](shared-models.md#dice-expression); expressions remain unevaluated.

FDnd5eSpellLevelScalingEntry:

| Field | C++ type and default | Meaning |
| --- | --- | --- |
| Level | `int32 = 0` | Supplied character-level threshold; zero is an unspecified authoring default |
| Dice | `FDnd5eDiceExpression`, default-constructed | Original expression with source syntax identifier |

FDnd5eSpellLevelScaling:

| Field | C++ type and default | Meaning |
| --- | --- | --- |
| Label | `TArray<FDnd5eInlineContent>`, empty | Source-supplied label, including supported formatting or retained fallback tokens |
| Entries | `TArray<FDnd5eSpellLevelScalingEntry>`, empty | Positive level keys sorted numerically within this group |

The reader accepts scalingLevelDice as one object or an array of objects, each containing required string label and scaling object. Scaling keys must be positive int32 digit strings; values must be nonempty expression strings. Imported Dice.Syntax is `5etools`. Group order is preserved, level entries are sorted, and missing levels are not invented. An absent level-one entry remains absent. This is structured reference data, not a leveling system, dice evaluator, interpolation rule, or universal representation of every higher-level effect.

## Spell availability

The exported, Blueprint-visible structs below are declared in [Dnd5eSpellAvailability.h](../Source/Dnd5e/Public/Spells/Dnd5eSpellAvailability.h), with EditAnywhere/BlueprintReadWrite fields in Dnd5e|SpellAvailability. They represent source associations, separately from generic directed reprint relationships and from character eligibility or gameplay grants.

FDnd5eSpellAvailabilityProvider:

| Field | C++ type and default | Meaning |
| --- | --- | --- |
| Category | `FGameplayTag`, invalid | Provider category, such as Class or Subclass |
| Name | `FString`, empty | Provider name; full name for a subclass |
| SourceId | `FString`, empty | Provider publication identifier |
| ShortName | `FString`, empty | Subclass short name retained from its lookup key |
| ClassName | `FString`, empty | Parent class name for a subclass |
| ClassSourceId | `FString`, empty | Parent class publication identifier |
| BaseName | `FString`, empty | Base species name when supplied |
| BaseSourceId | `FString`, empty | Base species publication identifier when supplied |

This is a source selector. No normalized class/subclass/species record ID is fabricated before those domain adapters define identity. Empty optional context stays empty; a matching name alone does not prove provider identity across publications or parent classes.

FDnd5eSpellAvailability:

| Field | C++ type and default | Meaning |
| --- | --- | --- |
| Provider | `FDnd5eSpellAvailabilityProvider`, default-constructed | Linked source provider selector |
| bIsVariant | `bool = false` | Association came from the classVariant branch |
| bHasUnqualifiedDefinition | `bool = false` | Source includes a declaration without a definition-source qualifier |
| DefinedInSources | `TArray<FString>`, empty | Explicit definition-publication qualifiers, preserving order |
| Subgroups | `TArray<FString>`, empty | Source subSubclasses qualifiers on this subclass association |
| FeatureTypes | `TArray<FString>`, empty | Raw optional-feature type codes; no arbitrary tag registration |
| Provenance | `FDnd5eRecordProvenance`, default-constructed | Supplementary lookup origin and retained identity evidence |

bHasAvailabilityData=false with an empty Availability array means unknown or not supplied. True with an empty array means the supplied lookup explicitly contains an empty entry. True with associations means known links; it does not assert complete character rules. Nonempty links with a false presence flag are inconsistent and rejected by the catalog. The primary ReadSpell operation leaves availability unknown; the optional catalog join populates it.

Unqualified declarations and explicit DefinedInSources can coexist. Unqualified does not mean defined in every publication. Class variants remain distinct links; subclass full/short names, parent context, and subgroup lists remain associated. Provider.SourceId is a publication selector; Provenance.SourceId stays empty for the generated lookup, which has no single publication. The [catalog reference](spell-catalog.md#availability-source-mapping) defines source shapes, ordering, matching, and failure behavior.

## Classification and relationship mappings

This table records intended source mappings. The [reader's accepted subset](json-spell-reader.md#accepted-spell-subset) identifies which mappings are actually implemented; the full table is not a coverage claim. Native identities are declared in [Native tag declarations](../Source/Dnd5e/Public/Dnd5eTags.h).

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

The retained field spellings ConditionInflicted and bHasTargetRecord follow the implementation. Reprint links do not make records equivalent, automatically choose a newer edition, or implement target lookup. The [reader identity contract](json-spell-reader.md#identity-and-provenance) now defines versioned spell keys from explicit name/source; other categories and omitted-source references need their own policies.

The JSON module supplies [FDnd5eJsonSpellReadRequest](../Source/Dnd5eJson/Public/Readers/Dnd5eJsonSpellReadRequest.h), [FDnd5eJsonSpellReadResult](../Source/Dnd5eJson/Public/Readers/Dnd5eJsonSpellReadResult.h), and the [ReadSpell operation](../Source/Dnd5eJson/Public/Readers/Dnd5eJsonSpellReader.h). The operation enforces its documented Success/Failure contract; manually authored result values are ordinary mutable storage.

## Remaining design work

Storage for the first spell record and the limited text-to-spell reader are complete. Full source importing and spell/rule semantics are not. The reader rejects unhandled required forms; it does not silently fill the corresponding fields with defaults and claim success.

The reader maps scalingLevelDice, known combat classifications, and selected publication/membership metadata. The catalog builder additionally joins the separate generated availability lookup when supplied. Complete areaTags/miscTags meaning and supplemental fluff/media loading remain pending. Do not use header metadata JSON as a replacement for unmodeled rule data, or treat absence of inline class lists as absence of class availability. Unknown attack labels, schools, and other required values fail explicitly rather than producing guessed tags.

Further work includes more typed inline/rule semantics, provider/general reference resolution, additional supplementary files, complete document/media support, explicit edition policy, and comprehensive 2014/2024 rule coverage. Acceptance with retained fallbacks does not establish full semantic interpretation. Reference data and classifications do not implement casting, combat, or character progression.

Annotation work is an expansion of existing support: the reader already creates typed DiceRoll and DamageRoll annotations with unevaluated FDnd5eDiceExpression values. [Other annotation families](json-spell-reader.md#current-annotation-coverage) still need typed contracts/mappings. The goal remains structured reference data; rolling expressions or applying their effects is consumer-owned.
