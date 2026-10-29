# Shared models

Status: current shared declarations and remaining logical contracts. The [252-case standard suite](testing.md) covers all 59 public model schemas/defaults and 264 reflected fields, populated copy/serialization, Content helpers, the expanded [JSON spell reader](json-spell-reader.md), and [spell availability/catalog operations](spell-catalog.md). Opt-in source-file auditing is recorded separately. Earlier batch notes retain their historical verification dates. Complete semantic rule interpretation, general reference resolution, and editor dataset loading remain pending.

This document is the canonical definition of reusable value and content types. Domain documents link here and define only their own composition and context. Keep one definition per shared concept rather than copying it into each domain document or creating a separate document for every struct.

The record-header contract below defines the identity, classification, and provenance concepts used by these models. The [Spell model](spell-model.md) explains how spells compose these shared types. Native names are declared in [Dnd5eTags.h](../Source/Dnd5e/Public/Dnd5eTags.h) and defined in [Dnd5eTags.cpp](../Source/Dnd5e/Private/Dnd5eTags.cpp); shared ownership and role-specific use are described below.

The implemented Content and supporting value types are a normalized model, not the source JSON schema. [Source-format compatibility](source-format-compatibility.md) records verified mappings and known gaps from 5etools 2.39.0. Structure and field-type compatibility do not establish an implemented importer or full dataset coverage.

## Contents

- [Record identity](#record-identity)
    - [Record header](#record-header)
    - [Record ID](#record-id)
    - [Record relationships](#record-relationships)
- [Diagnostics](#diagnostics)
- [Reusable values](#reusable-values)
    - [Dice expression](#dice-expression)
    - [Numeric value](#numeric-value)
    - [Media reference](#media-reference)
- [Measurements and timing](#measurements-and-timing)
    - [Distance](#distance)
    - [Range](#range)
    - [Area](#area)
        - [Named dimensions](#named-dimensions)
    - [Time measurement](#time-measurement)
    - [Duration](#duration)
    - [Casting time](#casting-time)
- [Costs and components](#costs-and-components)
    - [Money](#money)
    - [Components](#components)
        - [Material component](#material-component)
    - [Supporting value tags](#supporting-value-tags)
- [Shared structured content](#shared-structured-content)
    - [Semantic coverage](#semantic-coverage)
    - [Structure and storage](#structure-and-storage)
        - [Blocks and inline content](#blocks-and-inline-content)
        - [Block payload storage](#block-payload-storage)
        - [Typed extraction API](#typed-extraction-api)
        - [Inline payload representation](#inline-payload-representation)
        - [Ordered content tree](#ordered-content-tree)
    - [Content metadata](#content-metadata)
        - [Page reference](#page-reference)
        - [FDnd5eContentMetadata](#fdnd5econtentmetadata)
    - [Block content](#block-content)
        - [Typed block payloads](#typed-block-payloads)
        - [Text, paragraphs, and sections](#text-paragraphs-and-sections)
        - [List items](#list-items)
        - [List container](#list-container)
        - [Tables](#tables)
            - [Table-cell base fields](#table-cell-base-fields)
            - [Table field contract](#table-field-contract)
                - [FDnd5eTableRollRange](#fdnd5etablerollrange)
                - [FDnd5eTableCellContent additions](#fdnd5etablecellcontent-additions)
                - [FDnd5eTableRowContent](#fdnd5etablerowcontent)
                - [FDnd5eTableColumnContent](#fdnd5etablecolumncontent)
                - [FDnd5eTableContent](#fdnd5etablecontent)
        - [Quotes](#quotes)
        - [Insets](#insets)
    - [Media content](#media-content)
        - [Images](#images)
        - [Galleries](#galleries)
    - [Inline content](#inline-content)
        - [Formatted inline content](#formatted-inline-content)
        - [Annotation content](#annotation-content)
        - [Numeric content](#numeric-content)
    - [References and links](#references-and-links)
        - [Inline record references](#inline-record-references)
        - [External link content](#external-link-content)
        - [Document-local anchor link content](#document-local-anchor-link-content)
    - [Unsupported-content fallback](#unsupported-content-fallback)
        - [Fallback payload](#fallback-payload)
        - [Fallback tags and extraction](#fallback-tags-and-extraction)
        - [Fragment preservation and display](#fragment-preservation-and-display)
        - [Validation and loading boundaries](#validation-and-loading-boundaries)

## Record identity

Identity and shared record-header contracts for independently identifiable domain records.

### Record header

Implemented: exported, Blueprint-visible `FDnd5eRecordProvenance` and `FDnd5eRecordHeader` are declared in [Dnd5eRecordHeader.h](../Source/Dnd5e/Public/Records/Dnd5eRecordHeader.h). Primary domain records compose this header; embedded values need a header only when independently identifiable. The spell reader populates identity/name/provenance and retained metadata; the catalog checks its indexing requirements. The current automation suite covers storage/copy and those operations. General cross-domain semantic validation remains pending.

FDnd5eRecordHeader fields:

| Field | C++ type and default | Meaning |
| --- | --- | --- |
| RecordId | `FDnd5eRecordId`, default-constructed | Shared namespace/category/source-key identity |
| DisplayName | `FString`, empty | Human-readable record name |
| Aliases | `TArray<FString>`, empty | Additional source-provided names |
| Classification | `FGameplayTagContainer`, empty | Supported record classifications, keeping distinct roles in their own tag branches |
| Provenance | `TArray<FDnd5eRecordProvenance>`, empty | Primary origin followed by any contributing origins |

Identity remains dataset namespace, category, and an opaque category-specific source key. Display names, aliases, file paths, and dataset release versions do not replace identity. Category is stored in RecordId.Category. The spell adapter defines its source-key construction, the caller assigns the namespace, and the core implements exact equality/hashing below. Cross-dataset reference resolution remains pending; the header itself owns no catalog.

Classification tags must distinguish declared edition, source generation, and source-provided SRD/Basic Rules membership. Missing edition does not imply both editions; publication generation must not be substituted for an explicit declared edition. Preserve original values and the basis of documented derivations. Unknown imported strings must not automatically become native tags.

FDnd5eRecordProvenance fields:

| Field | C++ type and default | Meaning |
| --- | --- | --- |
| SourceId | `FString`, empty | Original publication/source identifier |
| Page | `FDnd5ePageReference`, default-constructed | Numeric/textual page reference when supplied |
| RelativeFilePath | `FString`, empty | File relative to the selected external dataset root |
| JsonPointer | `FString`, empty | Source record locator in that file |
| IdentityFieldsJson | `FString`, empty | Optional JSON object retaining original category-specific identity fields |
| ClassificationEvidenceJson | `FString`, empty | Optional JSON object retaining original classification values and the basis of any derived tags |
| AdditionalMetadataJson | `FString`, empty | Optional JSON object retaining other record metadata not otherwise represented |

When available, Provenance[0] is the primary source record; subsequent entries retain contributing origins in deterministic order. Ordering does not itself define inheritance or override precedence. The adapter must define assembly rules. Per-record provenance complements per-node Content Metadata.

The three JSON strings optionally retain valid JSON objects containing metadata, identity inputs, and classification evidence. Empty means no retained object. They are not a second copy of a spell body or a replacement for typed rule relationships. Absolute dataset roots and release configuration belong outside these values in untracked local configuration. Source identifiers and membership metadata do not establish licensing permission.

### Record ID

Implemented: `FDnd5eRecordId` is an exported, Blueprint-visible value in [Dnd5eRecordId.h](../Source/Dnd5e/Public/Records/Dnd5eRecordId.h), with equality/hash definitions in [Dnd5eRecordId.cpp](../Source/Dnd5e/Private/Records/Dnd5eRecordId.cpp). This type is shared by domain relationships, queries, and content references. The [spell catalog](spell-catalog.md) uses it as an exact index key.

| Field | C++ type and default | Meaning |
| --- | --- | --- |
| DatasetNamespace | `FString`, empty | Logical dataset identity, independent of its local directory |
| Category | `FGameplayTag`, invalid | Record category, such as spell or class feature |
| SourceKey | `FString`, empty | Opaque category-specific identity produced by the adapter |

A complete ID requires a nonempty namespace, a recognized record-category tag, and a nonempty source key. An empty default value represents no ID. Strings preserve spelling and case; do not apply universal trimming, lowercasing, or name-plus-source concatenation in this core type. The adapter defines category-specific canonicalization before producing IDs. C++ ==/!= and reflected struct equality compare the full namespace/source-key code units case-sensitively and the category tag exactly. GetTypeHash follows the same equality contract, including string length; its process-local value is not a persisted identifier or cross-platform stable hash. There is no wildcard/ancestor matching in ID equality. The [spell key policy](json-spell-reader.md#identity-and-provenance) does not define identity for other domains.

Native categories are Dnd5e::Category::{Spell, Class, Subclass, Species, Background, Feat, OptionalFeature, CharacterOption, Reward}. They classify record/provider domains; Dnd5e.Content.Inline.Reference identifies a content payload. The eight non-spell categories support source availability selectors and do not establish implemented domain records, importers, or their normalized ID policies.

### Record relationships

Implemented: exported, Blueprint-visible `FDnd5eRecordRelationship` is declared in [Dnd5eRecordRelationship.h](../Source/Dnd5e/Public/Records/Dnd5eRecordRelationship.h). This is a directed record link, separate from inline Content references with display text and spell availability associations. The spell reader retains reprintedAs strings as unresolved relationships. Automatic target normalization/resolution and general relationship consistency validation remain pending.

| Field | C++ type and default | Meaning |
| --- | --- | --- |
| Kind | `FGameplayTag`, invalid | Directed relationship classification; initially ReprintedAs |
| bHasTargetRecord | `bool = false` | Whether a complete normalized identity has been produced |
| TargetRecordId | `FDnd5eRecordId`, default-constructed | Target identity when the flag is true |
| SourceFormat | `FString`, empty | Syntax identifier for SourceReference; initially 5etools |
| SourceReference | `FString`, empty | Original source reference string, without rewriting its spelling or arguments |

The containing record is the origin. Kind currently uses Dnd5e::Record::Relationship::ReprintedAs, with string Dnd5e.Record.Relationship.ReprintedAs. It means the containing record has a reprint at the target; it does not assert identical rules, bidirectional identity, automatic replacement, or a preferred edition.

Despite its shortened name, bHasTargetRecord means a complete normalized target ID has been produced, not that a target record is loaded or resolved. False leaves TargetRecordId default-constructed; true requires a namespace, category, and source key under the shared ID contract. A retained source reference can be meaningful before normalization succeeds. A usable relationship needs a recognized Kind and either a complete target ID or a source reference with a declared syntax.

A supplied SourceReference requires SourceFormat; the initial syntax identifier is 5etools. Preserve the original reference even after normalizing its target ID. A programmatically authored relationship with a complete target ID can omit both source fields. Do not universally split or rewrite source references, infer category-specific source defaults, or invent a name-plus-source ID format in this value. Those remain adapter/catalog responsibilities. Editor EditCondition metadata controls authoring visibility only.

## Diagnostics

Implemented: EDnd5eDiagnosticSeverity and exported, Blueprint-visible FDnd5eDiagnostic are declared in [Dnd5eDiagnostic.h](../Source/Dnd5e/Public/Diagnostics/Dnd5eDiagnostic.h). The core owns this shared value so validation/catalog code need not depend on the JSON adapter. Its five fields are reflected with EditAnywhere/BlueprintReadWrite; the implemented property category is Dnd5eDiagnostic. Declarations and the expanded 141-test suite passed on 2026-10-29.

EDnd5eDiagnosticSeverity is a Blueprint-visible uint8 enum: Unspecified, Info, Warning, Error. Unspecified is the empty/default state, not an emitted severity. A producer should supply a real severity, controlled non-None code, and nonempty message for an emitted diagnostic. These requirements are not enforced by the value's constructor.

| Field | C++ type and default | Meaning |
| --- | --- | --- |
| Severity | `EDnd5eDiagnosticSeverity = EDnd5eDiagnosticSeverity::Unspecified` | Severity of an emitted diagnostic |
| Code | `FName`, None | Stable program-defined identifier, independent of message wording |
| Message | `FString`, empty | Human-readable explanation |
| RelativeFilePath | `FString`, empty | Logical source file relative to the selected external dataset root |
| JsonPointer | `FString`, empty | JSON source location when known |

Code is a controlled program identifier, not a Gameplay Tag or arbitrary source string to intern. Callers/tests should compare Code/Severity rather than English message wording. The default Code is NAME_None. Diagnostic creation, categorization, and logging policies belong to the producer/caller, not this value declaration.

RelativeFilePath is source context, not a file to open. JsonPointer is opaque location text in the core, matching existing provenance conventions; no JSON parser dependency is introduced. An empty pointer identifies no narrower known location, including a whole-document problem. Do not invent a record ID when input fails before identity can be established. No diagnostic emitter, source validation, logging side effect, or UI is implemented by this struct.

The JSON adapter composes arrays of this type into [FDnd5eJsonSpellReadResult](../Source/Dnd5eJson/Public/Readers/Dnd5eJsonSpellReadResult.h) and its [availability/catalog results](spell-catalog.md). Core catalog operations also return these diagnostics without depending on Dnd5eJson.

## Reusable values

Values that domain models and content payloads can share without requiring a document wrapper.

### Dice expression

Implemented: exported, Blueprint-visible `FDnd5eDiceExpression` is declared in `Source/Dnd5e/Public/Values/Dnd5eDiceExpression.h`. This shared value can be used by descriptions and future spell, weapon, or creature models without depending on a content wrapper. Its storage declaration was source-reviewed and compiled in the host editor target on 2026-10-28; expression parsing and evaluation are not implemented.

| Field | C++ type and default | Meaning |
| --- | --- | --- |
| Expression | `FString`, empty | Expression in the declared syntax, excluding an enclosing inline tag and its display arguments |
| Syntax | `FString`, empty | Expression-language identifier; initially `5etools` for expressions retained from the inspected format |

Both fields must be nonempty when an expression is supplied. This is syntax-preserving storage, not a parsed syntax tree or an assertion that an expression is executable. Do not universally split expressions into count, die size, and modifier: the source also supports arithmetic, alternatives, variables, prompts, and scaling forms. Do not interpret an unknown syntax as the initial syntax. Validation of a supported grammar and any future normalized expression representation require separate contracts.

An expression does not carry its display label, damage classification, source token, or UI action. Those belong to its containing model. No dice rolling, random-number generation, prompt execution, or character-state evaluation is included.

### Numeric value

Implemented: `EDnd5eNumberKind : uint8` and exported, Blueprint-visible `FDnd5eNumber` are declared in `Source/Dnd5e/Public/Values/Dnd5eNumber.h`. The enum members are Unspecified, Integer, Decimal, and Literal. This value represents an actual number, separately from a string that happens to contain digits. Its storage declaration was source-reviewed and compiled in the host editor target on 2026-10-28; source-token conversion and semantic validation are not implemented.

| Field | C++ type and default | Meaning |
| --- | --- | --- |
| Kind | `EDnd5eNumberKind = Unspecified` | Selects the meaningful representation |
| Integer | `int32 = 0` | Signed integral value when Kind is Integer |
| Decimal | `double = 0.0` | Finite binary floating-point value when Kind is Decimal |
| SourceLiteral | `FString`, empty | Original JSON number token, when available; required when Kind is Literal |

Unspecified means no supplied numeric value; zero is a valid number. Literal means the original valid JSON number is retained without claiming an int32 or double conversion. Use it when an exact integer exceeds int32 or a decimal cannot be converted to a suitable finite value. SourceLiteral is a numeric token such as `12`, `0.125`, or `1e3`, without JSON string quotes; it is not an arbitrary expression or display label. It must follow JSON number grammar when supplied. JSON does not permit NaN or infinity.

The supplied snapshot's 816 primitive integer table cells range from 0 to 133, so int32 is sufficient for the observed integral values; larger imported integers still require checked conversion or Literal preservation. The generic Decimal representation uses double: JavaScript Number and Unreal's FJsonValueNumber use double precision, and narrowing to float can discard significant digits even for small magnitudes. A field with a separate, limited precision requirement, such as a table's relative width weight, can still use float.

For imported numbers, preserve SourceLiteral before any potentially lossy conversion. A double may approximate a decimal value; retaining the literal preserves source precision and spelling without claiming exact decimal arithmetic. If a future JSON reader has already discarded the original token, it must report any inability to preserve it rather than invent an original literal from a rounded double. Manually constructed numbers may omit SourceLiteral for Integer or Decimal.

Only the selected numeric field is meaningful; keep unused fields at their defaults. The future adapter must check range, finiteness, and conversion suitability before selecting a representation. Typed content extraction does not perform these checks. This value does not replace every integer field in the project: counts, enum discriminants, and existing validated measurements retain their own contracts.

### Media reference

Implemented: `EDnd5eMediaReferenceKind : uint8` and exported, Blueprint-visible `FDnd5eMediaReference` are declared in `Source/Dnd5e/Public/Values/Dnd5eMediaReference.h`. The enum members are Unspecified, DatasetRelative, and ExternalUrl. The declaration was source-reviewed and compiled in the host editor target on 2026-10-28. This shared value describes a resource location without loading a texture, creating an asset, or downloading content.

| Field | C++ type and default | Meaning |
| --- | --- | --- |
| Kind | `EDnd5eMediaReferenceKind = Unspecified` | Selects the meaningful resource-location field |
| RelativePath | `FString`, empty | Resource path relative to the user-selected media root for the containing model |
| Url | `FString`, empty | External resource URI when Kind is ExternalUrl |

DatasetRelative requires a nonempty relative path; ExternalUrl requires a nonempty URI. Keep the unused field empty. Unspecified represents no supplied reference. A matching content payload can still contain an incomplete reference; future semantic validation is separate from typed extraction.

For an image, the resolver context supplies the selected image root. The source's internal href.path is relative to its image base, not necessarily to the JSON file or dataset data directory. Do not prepend a local drive, plugin directory, website hostname, or img/ segment to the stored relative path by assumption. Root selection and any verified path mapping belong to untracked user configuration and the future adapter/resolver. A different media family may supply a different root context while reusing this value type.

Relative paths must remain relative and resolve within the selected root; do not interpret absolute paths or parent traversal as valid dataset-relative resource locations. Path normalization and containment checking are future resolver responsibilities. Preserve source strings when normalization cannot be justified rather than silently opening an unrelated local file. ExternalUrl stores a destination only; it does not authorize or implement a network request.

The inspected renderer supports both internal and external media hrefs. All image hrefs observed in the supplied snapshot were internal. Declaring ExternalUrl support in this value does not establish an external-image loader. Binary media, local absolute roots, and generated textures remain outside the plugin repository under the external-content policy.

## Measurements and timing

Reusable measurements, geometry, and timing values. All declarations below are exported and Blueprint-visible. Source-reviewed and compiled in the host editor target on 2026-10-29 using the temporary `-DisablePlugin=Fab` override. Storage declarations are implemented; semantic validation and source conversion remain unimplemented. Tests were not run.

### Distance

`EDnd5eDistanceUnit : uint8` and `FDnd5eDistance` are in `Values/Dnd5eDistance.h` under the runtime public directory. Enum members: Unspecified, Inches, Feet, Yards, Miles.

| Field | C++ type and default | Meaning |
| --- | --- | --- |
| Amount | `FDnd5eNumber`, default-constructed | Numeric magnitude, separate from unit |
| Unit | `EDnd5eDistanceUnit = EDnd5eDistanceUnit::Unspecified` | Supplied measurement unit |

Amount uses the existing numeric value: select Integer for the observed integral spell measurements; preserve decimal/literal representations under the numeric contract when needed. An unspecified amount is not zero. Supplied distances require a finite nonnegative amount and a known unit; context may impose stricter constraints. Preserve units rather than converting to Unreal centimeters. Self, Touch, Sight, and Unlimited remain range classifications, not measurement units.

### Range

`FDnd5eRange` is in `Values/Dnd5eRange.h`. Kind uses the Range::Kind tags in [Supporting value tags](#supporting-value-tags).

| Field | C++ type and default | Meaning |
| --- | --- | --- |
| Kind | `FGameplayTag`, invalid | Distance, Self, Touch, Sight, Unlimited, UnlimitedSamePlane, or Special |
| bHasDistance | `bool = false` | Whether Distance is supplied |
| Distance | `FDnd5eDistance`, default-constructed | Measurement when the kind requires it |
| Description | `TArray<FDnd5eContentBlock>`, empty | Optional source explanation or special range details |

Distance kind requires bHasDistance and a meaningful Distance. Non-measured kinds leave the flag false and the nested value default-constructed. Description preserves special explanations. Area geometry is separate: a source-defined self-originating cone has Self range plus an area measurement, not a targeting range equal to the cone length. No targeting execution is introduced.

### Area

`FDnd5eArea` and its dimension value are in `Values/Dnd5eArea.h`. Shape uses Area::Shape tags.

| Field | C++ type and default | Meaning |
| --- | --- | --- |
| Shape | `FGameplayTag`, invalid | Supported shape/area-form classification |
| Dimensions | `TArray<FDnd5eAreaDimension>`, empty | Named dimensions explicitly available from source |
| Description | `TArray<FDnd5eContentBlock>`, empty | Remaining source explanation |

Preserve partial geometry when measurements are not supplied. Radius and Emanation are distinct source area forms, not instructions to infer a collision shape or merge them into Sphere. Origin belongs to the owning domain context, such as FDnd5eSpellArea. The values contain no actors, collision components, or affected-target calculations.

#### Named dimensions

`EDnd5eAreaDimensionKind : uint8` has members Unspecified, Radius, Diameter, Length, Width, Height, EdgeLength, Extent. `FDnd5eAreaDimension` uses this enum to interpret its measurement.

| Field | C++ type and default | Meaning |
| --- | --- | --- |
| Kind | `EDnd5eAreaDimensionKind = EDnd5eAreaDimensionKind::Unspecified` | Measurement role |
| Measurement | `FDnd5eDistance`, default-constructed | Value and its original unit |

An absent dimension has no array entry. Extent retains a source-defined outward measure without asserting a radius from a point. Duplicate dimension kinds and incompatible shape/dimension combinations require future validation. Do not invent line widths, cylinder heights, or zero dimensions to fill absent source values.

### Time measurement

`EDnd5eTimeUnit : uint8` and `FDnd5eTimeSpan` are in `Values/Dnd5eTimeSpan.h`. Enum members: Unspecified, Turn, Round, Minute, Hour, Day, Week, Month, Year.

| Field | C++ type and default | Meaning |
| --- | --- | --- |
| Amount | `FDnd5eNumber`, default-constructed | Number of time units |
| Unit | `EDnd5eTimeUnit = EDnd5eTimeUnit::Unspecified` | Measurement unit |

A supplied duration measurement requires a meaningful positive amount and known unit. Preserve Turn versus Round; do not convert them into seconds. This value is reference data, not FTimespan or a running timer. Casting time has a separate contract because it includes action-economy units and casting conditions.

### Duration

`EDnd5eDurationKind : uint8` and `FDnd5eDuration` are in `Values/Dnd5eDuration.h`. Enum members: Unspecified, Instantaneous, Timed, Permanent, Special.

| Field | C++ type and default | Meaning |
| --- | --- | --- |
| Kind | `EDnd5eDurationKind = EDnd5eDurationKind::Unspecified` | Duration representation |
| bHasTime | `bool = false` | Whether Time is supplied |
| Time | `FDnd5eTimeSpan`, default-constructed | Meaningful for a timed duration |
| bIsMaximum | `bool = false` | Source says up to the supplied time |
| EndingConditions | `FGameplayTagContainer`, empty | Explicit source endings such as dispelled, triggered, or discharged |
| Description | `TArray<FDnd5eContentBlock>`, empty | Remaining source duration explanation |

Timed requires bHasTime and meaningful Time; other kinds retain no fabricated time. bIsMaximum qualifies a supplied timed measurement. EndingConditions uses Duration::Ending tags for explicit source endings. Permanent is not an enormous timer, and absent endings do not imply an invented ending rule. The shared Trait::Concentration tag retains its source association on FDnd5eSpellDuration.Requirements when the requirement belongs to that entry. It is not owned by Duration or Spell in the tag hierarchy. Validation and effect-ending execution are not implemented.

### Casting time

`FDnd5eCastingTime` is in `Values/Dnd5eCastingTime.h`. Its name remains appropriate for the spell context; Unit now consumes shared Activation::Time tags. A future non-spell activation value can reuse those identities without being called casting time. Casting time describes performing the casting, while duration describes the resulting effect.

| Field | C++ type and default | Meaning |
| --- | --- | --- |
| Quantity | `int32 = 0` | Number of units; supplied casting times require a positive quantity |
| Unit | `FGameplayTag`, invalid | Shared activation timing designation; numeric forms use a quantity |
| Condition | `TArray<FDnd5eContentBlock>`, empty | Source condition, including any supported markup |
| Note | `TArray<FDnd5eContentBlock>`, empty | Separate source note |

Quantity must be positive for a supplied numeric timing form; zero is its empty authoring default. Activation::Time::Special does not imply a fabricated quantity. Special/free-form import and a possible generalized activation value require later contracts. Condition and Note are separate structured documents, each represented by Content blocks. They do not evaluate triggers. A spell can hold multiple casting-time entries. Renderer-generated ritual display entries are not imported as source casting-time units.

## Costs and components

Reusable monetary and component requirements. The declarations are exported and Blueprint-visible and compiled on 2026-10-29 with the temporary Fab exclusion. Storage does not implement inventory, payment, substitution, or consumption.

### Money

`EDnd5eCurrency : uint8` and `FDnd5eMoney` are in `Values/Dnd5eMoney.h`. Enum members: Unspecified, Copper, Silver, Electrum, Gold, Platinum.

| Field | C++ type and default | Meaning |
| --- | --- | --- |
| Amount | `FDnd5eNumber`, default-constructed | Nonnegative monetary amount when supplied |
| Currency | `EDnd5eCurrency = EDnd5eCurrency::Unspecified` | Denomination of that amount |

Supplied amounts must be nonnegative and have a known denomination. Zero is an explicit amount; Unspecified is different. Costs hold arrays of these values so mixed denominations remain separate, such as five gold and one copper. Preserve denominations without automatic combining or conversion. The inspected spell material-cost field maps to one Copper entry. This is not a wallet.

### Components

`FDnd5eComponents` is in `Values/Dnd5eComponents.h`. Types uses Component::Type tags.

| Field | C++ type and default | Meaning |
| --- | --- | --- |
| Types | `FGameplayTagContainer`, empty | Required component classifications |
| bHasMaterial | `bool = false` | Whether material details are supplied |
| Material | `FDnd5eMaterialComponent`, default-constructed | Optional material details |
| Description | `TArray<FDnd5eContentBlock>`, empty | Additional component requirements/explanation |

Types identifies requirements; bHasMaterial identifies whether detail data is supplied. The source-supported m=true form can require Material with no detail object. A supplied material string/object supplies details and sets bHasMaterial. Present details require the Material classification. Description preserves additional requirements. Royalty is a separate source component flag; retain its classification without calculating its level-based display fee or adding it to material cost. Full rule mapping remains future work.

#### Material component

`EDnd5eMaterialConsumption : uint8` has members Unspecified, Consumed, Optional, NotConsumed. It and `FDnd5eMaterialComponent` are declared before FDnd5eComponents in the same header.

| Field | C++ type and default | Meaning |
| --- | --- | --- |
| Description | `TArray<FDnd5eContentBlock>`, empty | Material requirement text with supported structure preserved |
| bHasCost | `bool = false` | Whether a cost is supplied |
| Cost | `TArray<FDnd5eMoney>`, empty | One or more amounts/denominations when present |
| Consumption | `EDnd5eMaterialConsumption = EDnd5eMaterialConsumption::Unspecified` | Supplied consumption classification |

bHasCost=false means no supplied cost and an empty Cost array; true requires at least one valid money entry. An absent cost does not mean zero or free materials. Missing consumption remains Unspecified. Source true maps to Consumed, optional to Optional, and supported explicit false to NotConsumed; omission must not be treated as false. Retain the full description for requirements beyond these fields. Consistency validation and consumption execution are unimplemented.

### Supporting value tags

The supporting subset below now contains 33 native tags after adding shared Round/Special timing designations. The complete 121-tag runtime registry is declared in [Dnd5eTags.h](../Source/Dnd5e/Public/Dnd5eTags.h) and defined in [Dnd5eTags.cpp](../Source/Dnd5e/Private/Dnd5eTags.cpp). Early serialized names require host-side redirects; the current namespaces below are authoritative for these shared values. Namespaces mirror the tag hierarchy; every symbol appends its exact spelling to the listed prefix. Area-dimension kind, duration kind, and consumption use enums because they select fixed structural interpretations. Tags retain extensible rule classifications. A field's number of selections alone does not determine its representation.

| C++ namespace under Dnd5e | Tag-string prefix | Symbols |
| --- | --- | --- |
| Activation::Time | `Dnd5e.Activation.Time.` | Action, BonusAction, Reaction, Round, Minute, Hour, Special |
| Range::Kind | `Dnd5e.Range.Kind.` | Distance, Self, Touch, Sight, Unlimited, UnlimitedSamePlane, Special |
| Area::Shape | `Dnd5e.Area.Shape.` | Cone, Line, Cube, Radius, Sphere, Hemisphere, Cylinder, Emanation |
| Area::Origin | `Dnd5e.Area.Origin.` | Self, SelectedPoint, Target |
| Duration::Ending | `Dnd5e.Duration.Ending.` | Dispelled, Triggered, Discharged |
| Component::Type | `Dnd5e.Component.Type.` | Verbal, Somatic, Material, Royalty |
| Trait | `Dnd5e.Trait.` | Concentration |

For example, Dnd5e::Activation::Time::Action corresponds to Dnd5e.Activation.Time.Action. Range::Kind::Self and Area::Origin::Self classify different fields. Unknown source strings require explicit adapter preservation and diagnostics, not automatic native-tag registration. The shared concentration trait is applied to the context carrying the requirement, including each applicable spell-duration entry. A spell-level trait index can summarize those requirements, but must not become an independent conflicting source of truth.

## Shared structured content

Decision: use a shared structured-content model across domains, including spell descriptions, features, books, and adventures, rather than reducing descriptions to a single plain string.

Preserve paragraphs and inline formatting, named sections and their order, lists, tables, and references to other records. Unsupported source constructs must remain recoverable where practical and produce diagnostics. Invalid selected JSON, duplicate normalized record IDs, or unresolved required relationships must fail catalog loading; unresolved optional descriptive references may be retained with warnings. A failed replacement load preserves the previous active catalog. These are intended validation requirements; catalog loading is not implemented. Do not interpret descriptive references as executable rules.

The model should allow consumers to render content in Unreal UI, extract plain text, or follow references. This is a data representation requirement, not a commitment to ship a complete rendering UI. Core content wrappers, metadata storage, the text, paragraph, section, formatted, reference, external-link, anchor-link, annotation, number, list, table, quote, inset, image, and gallery payloads, and shared unsupported block/inline storage are implemented. DiceRoll and DamageRoll have typed expression extraction; other annotation/media families, link/reference parsing and resolution, numeric/source conversion, media loading, metadata population/resolution, and automatic fallback capture/diagnostics are incomplete.

### Semantic coverage

The final target is semantic representation of all fifth-edition D&D rules within the supported 2014/2024 content scope. This includes rule concepts, prerequisites, choices, conditions, triggers, formulas, outcomes, dependencies, exceptions, and their source/version context. It applies across domains and descriptive content, not only to the source's JSON fields or inline annotation tags. This is a long-term requirement, not a claim of current implementation.

The document-content model preserves how information is organized and displayed. Complete semantic coverage also requires rule-data models and relationships beyond the currently implemented document payloads. Those models must describe what a rule means and the context it requires so consumers can inspect and use it. Their exact declarations and cross-domain composition require further design. Difficulty classes and other examples illustrate the requirement; no example defines its limits or automatically sets implementation priority.

The consuming project still owns character state and gameplay execution. The plugin supplies rule definitions, relationships, and reference data without choosing player actions or applying effects. Source descriptions may contain prose rather than explicit machine-readable fields; importing the JSON alone does not establish mechanical interpretation of every sentence. Full coverage therefore requires explicit semantic modeling and verified mappings, potentially including curated interpretation where the source lacks structured semantics. Preserving prose alone does not fulfill that requirement.

An initial release may have incomplete coverage. Unsupported fallbacks preserve compatibility while coverage grows; they do not remove known rules from the roadmap. Coverage claims must distinguish typed semantics and verified mappings from source-fragment preservation across all rule families. A comprehensive rule-coverage inventory is needed to track this goal; an inventory of source syntax alone is insufficient. Fallback extraction must never report that an unmodeled rule is fully understood.

The current [source vocabulary inventory](source-format-compatibility.md#source-vocabulary-inventory) classifies the inspected renderer forms and literal tag names against storage availability. Its structural counts and candidate mappings do not establish semantic coverage of every rule or implemented source importing. Shared contextual-value and rule-data contracts remain further design work.

### Structure and storage

The block/inline distinction, typed storage, and owned-tree representation.

#### Blocks and inline content

Decision: separate shared content into two logical layers:

| Layer | Responsibility |
| --- | --- |
| Blocks | Paragraphs, named sections, lists, tables, quotes, and insets |
| Inline content | Text, emphasis, record references, and annotations within text-bearing blocks or table cells |

Preserve ordering and nesting at both layers where the source provides them. This division describes content semantics, not a specific widget or rendering format. A paragraph or table cell can contain ordinary text and record references in sequence. The initial wrappers and payload declarations are implemented; the remaining source-node vocabulary and its rule contracts remain pending.

#### Block payload storage

Decision: the shared block representation contains a kind Gameplay Tag and an `FInstancedStruct` holding its concrete payload. Paragraphs, tables, sections, and other block kinds retain distinct payload structs.

The implemented wrappers FDnd5eContentBlock and FDnd5eInlineContent have the same three reflected fields:

| Field | C++ type and default | Meaning |
| --- | --- | --- |
| Kind | `FGameplayTag`, invalid | Exact payload discriminator |
| Payload | `FInstancedStruct`, empty | Concrete payload storage |
| Metadata | `FDnd5eContentMetadata`, default-constructed | Source-local metadata and provenance |

Consumers retrieve the expected concrete type before accessing fields. The implemented helpers explicitly reject mismatched kinds, wrong payload types, or absent payloads. Full recursive validation is separate.

Unreal provides Blueprint operations for creating and retrieving instanced struct values; see [Epic's Blueprint instanced struct library](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Engine/UBlueprintInstancedStructLibrary). Typed helpers for section, paragraph, text, formatted content, reference, external link, anchor link, annotation, number, list, table, quote, inset, image, gallery, and unsupported blocks/inline items are implemented, along with the inner dice-expression annotation extractor. Their wrapper checks do not recursively validate payload contents or prove source-format coverage. The remaining payload vocabulary and helper APIs are still to be completed.

#### Typed extraction API

The exported UDnd5eContentLibrary in [Dnd5eContentLibrary.h](../Source/Dnd5e/Public/Content/Dnd5eContentLibrary.h) exposes these 18 BlueprintCallable/BlueprintPure static functions. Each returns bool, takes a const input reference, and fills an output reference of the listed type.

| Function | Input type | Output type |
| --- | --- | --- |
| TryGetSection | `FDnd5eContentBlock` | `FDnd5eSectionContent` |
| TryGetParagraph | `FDnd5eContentBlock` | `FDnd5eParagraphContent` |
| TryGetInlineText | `FDnd5eInlineContent` | `FDnd5eTextContent` |
| TryGetFormatted | `FDnd5eInlineContent` | `FDnd5eFormattedContent` |
| TryGetList | `FDnd5eContentBlock` | `FDnd5eListContent` |
| TryGetTable | `FDnd5eContentBlock` | `FDnd5eTableContent` |
| TryGetQuote | `FDnd5eContentBlock` | `FDnd5eQuoteContent` |
| TryGetInset | `FDnd5eContentBlock` | `FDnd5eInsetContent` |
| TryGetReference | `FDnd5eInlineContent` | `FDnd5eReferenceContent` |
| TryGetAnnotation | `FDnd5eInlineContent` | `FDnd5eAnnotationContent` |
| TryGetNumber | `FDnd5eInlineContent` | `FDnd5eNumberContent` |
| TryGetAnnotationDiceExpression | `FDnd5eAnnotationContent` | `FDnd5eDiceExpression` |
| TryGetExternalLink | `FDnd5eInlineContent` | `FDnd5eExternalLinkContent` |
| TryGetAnchorLink | `FDnd5eInlineContent` | `FDnd5eAnchorLinkContent` |
| TryGetImage | `FDnd5eContentBlock` | `FDnd5eImageContent` |
| TryGetGallery | `FDnd5eContentBlock` | `FDnd5eGalleryContent` |
| TryGetUnsupportedBlock | `FDnd5eContentBlock` | `FDnd5eUnsupportedContent` |
| TryGetUnsupportedInline | `FDnd5eInlineContent` | `FDnd5eUnsupportedContent` |

Wrapper helpers check the exact native Kind, valid FInstancedStruct storage, and exact UScriptStruct payload type. Success copies the payload into the output and returns true; failure resets the output to its default and returns false. They do not mutate the input, traverse children, log expected extraction failures, resolve references, or validate a payload's rule semantics. TryGetAnnotationDiceExpression instead checks the inner annotation Kind for DiceRoll or DamageRoll and extracts an exact FDnd5eDiceExpression from Annotation.Value with the same copy/reset contract.

#### Inline payload representation

Decision: inline items use the same kind-Gameplay-Tag and `FInstancedStruct` payload pattern as blocks, with distinct inline payload types:

| Kind | Logical payload |
| --- | --- |
| Text | Literal text |
| Formatted span | Formatting tags and ordered inline children |
| Record reference | Display content and target reference, following the inline-reference policy |
| External link | Display content and destination URI |
| Document-local anchor link | Display content and target anchor ID in the owning document |
| Annotation | Structured information such as a dice expression, preserved without execution |
| Number | Typed numeric value and its optional display label |
| Unsupported | Retained source fragment and optional plain display text, without semantic support |

Formatted spans may contain record references so formatting does not erase link semantics. Preserve inline ordering and nesting. Apply kind/payload consistency validation to inline items as well as blocks. Text, formatted, reference, external-link, anchor-link, annotation, number, and unsupported payloads/accessors are implemented; additional annotation value families remain pending. Typed extraction does not implement recursive validation, source capture, or target resolution.

#### Ordered content tree

Decision: organize content as an ordered tree. A document contains an ordered array of blocks. Sections, list items, and insets contain child blocks; paragraphs contain ordered inline items; formatted spans contain child inline items. Children use the agreed kind-tag and instanced-payload representations.

Ordinary blocks and inline items do not require catalog record IDs. Links to independently identifiable content continue to use the record-reference model. Source-provided anchors are stored in Metadata.AnchorId; document scoping and resolution remain adapter/catalog responsibilities.

The reflected declarations implement an owned ordered tree rather than a graph of shared catalog records. The spell reader checks supported source shapes and enforces Content/inline depth 64 and converted-value budgets during conversion. This does not supply a general validator for arbitrarily authored reflected trees; complete recursive child-kind/semantic validation remains pending.

### Content metadata

Decision: compose shared `FDnd5eContentMetadata` into both `FDnd5eContentBlock` and `FDnd5eInlineContent`, alongside Kind and Payload. It preserves source-local identity and provenance without turning ordinary content nodes into catalog records. The metadata/page types are implemented; the spell reader populates checked/inherited source identifiers, explicit pages/anchors, file labels, and pointers under its [documented policy](json-spell-reader.md#content-inline-tokens-and-fallback-behavior).

Supporting source nodes that do not have a wrapper can use the same metadata type when needed. In particular, source list items can carry id/source/page fields, and `FDnd5eListItemContent` now also contains a Metadata member. This is a source-driven follow-up to the wrapper decision, not a requirement to attach metadata to every numeric value struct.

#### Page reference

Use a reusable `FDnd5ePageReference`, with a Blueprint-visible `EDnd5ePageReferenceKind : uint8` enum: Unspecified, Number, Text. This preserves whether the source page field was numeric or textual.

| Field | C++ type and default | Meaning |
| --- | --- | --- |
| Kind | `EDnd5ePageReferenceKind = EDnd5ePageReferenceKind::Unspecified` | Which page representation is present |
| Number | `int32 = 0` | Meaningful when Kind is Number |
| Text | `FString` | Meaningful when Kind is Text |

Do not infer absence from a zero number or an empty string alone. The discriminant establishes presence and representation. Number and Text do not require simultaneous population. Validate numeric conversion before narrowing source numbers to int32.

#### FDnd5eContentMetadata

| Field | C++ type | Meaning |
| --- | --- | --- |
| SourceType | `FString` | Original explicit source type discriminator; empty when none was supplied |
| AnchorId | `FString` | Original source-local id; empty when absent |
| SourceId | `FString` | Explicit publication/source override on this node |
| Page | `FDnd5ePageReference` | Explicit page reference on this node |
| RelativeFilePath | `FString` | Diagnostic path relative to the selected dataset root |
| JsonPointer | `FString` | Source locator within that file, following JSON Pointer escaping |
| Aliases | `TArray<FString>` | Original aliases in order, when supplied |
| AdditionalMetadataJson | `FString` | Optional JSON object containing retained metadata not otherwise represented |

Store explicitly supplied source/page overrides here. Effective inherited source/page context is resolved separately from parent or owning-record context; do not fabricate an explicit override on every child. Anchor IDs are scoped to the relevant document/dataset context, not assumed globally unique. They are not the same as record IDs or diagnostic JSON Pointers.

Absolute dataset roots remain catalog/per-user configuration, not repeated node metadata. Keep source metadata as data, not automatically registered Gameplay Tags. AdditionalMetadataJson contains metadata properties only, such as unhandled data/ruleType fields, not another copy of recognized entries/items/rows/heading content or the whole subtree. Preserve valid JSON values inside that object; do not execute or automatically log them. Empty string means no retained additional metadata. This field is a preservation mechanism, not semantic support for its contents.

The declared native Content kind remains the normalized category. SourceType records the original external form, so entries and section can normalize to a Section payload without erasing their origin. Synthetic inline nodes produced from source strings may share the source string's locator; do not invent external IDs for them.

### Block content

Payloads for document blocks and their supporting list/table values.

#### Typed block payloads

Decision: use distinct supporting types for block content rather than one oversized struct containing every possible block field.

| Block kind | Logical payload |
| --- | --- |
| Paragraph | Ordered inline content |
| Section | Heading and ordered child blocks |
| List | Ordered list items, each containing blocks |
| Table | Column information, rows, and structured cells |
| Quote | Content and optional attribution |
| Inset | Optional heading and child blocks |

A shared block representation identifies its kind with a Gameplay Tag and associates it with the appropriate typed payload. The typed payload fields and extraction helpers below are implemented and compiled. Source style/metadata normalization, full recursive validation, and remaining presentation vocabulary are pending. Exact kind/type checks do not validate the contents of a successfully extracted payload.

#### Text, paragraphs, and sections

| Implemented type | Field | C++ type | Meaning |
| --- | --- | --- | --- |
| FDnd5eTextContent | Text | `FString` | Literal text run |
| FDnd5eParagraphContent | Inlines | `TArray<FDnd5eInlineContent>` | Inline content in reading order |
| FDnd5eSectionContent | Heading | `TArray<FDnd5eInlineContent>` | Structured heading in reading order |
| FDnd5eSectionContent | Blocks | `TArray<FDnd5eContentBlock>` | Ordered section body |

Source strings can contain inline markup. Recognized markup must be normalized into inline nodes rather than treated as ordinary literal text. The section heading declaration uses structured inline content, consistent with list/inset headings; the importer still needs to populate it. The C++ host and existing test fixtures have been migrated to the array representation, and the host editor target builds. Test execution and Blueprint graph migration were not verified by that compilation.

#### List items

Decision: use `FDnd5eListItemContent` as a supporting value owned by a list.

| Field | Type | Meaning |
| --- | --- | --- |
| Label | `TArray<FDnd5eInlineContent>` | Optional label expressed as ordered inline content; an empty array means no separate label |
| Blocks | `TArray<FDnd5eContentBlock>` | Ordered body blocks, including paragraphs, tables, or nested lists |
| Metadata | `FDnd5eContentMetadata` | Source-local item anchor, explicit provenance, and retained metadata |

Labels can preserve formatting and references through the shared inline model. A list item does not need its own catalog record ID or wrapper-kind tag. Its containing list owns it as a typed value. The list container's presentation and numbering fields are specified separately.

Label, Blocks, and Metadata are implemented. The adapter must populate Metadata from the source item rather than silently dropping item-level anchors or overrides.

#### List container

Decision: represent a list block with `FDnd5eListContent`.

| Field | Representation | Meaning |
| --- | --- | --- |
| Heading | `TArray<FDnd5eInlineContent>` | Optional list heading; an empty array means no heading |
| Items | `TArray<FDnd5eListItemContent>` | List items in reading order |
| MarkerStyle | EDnd5eListMarkerStyle, default Unspecified | Marker representation, separate from layout tags |
| Presentation | `FGameplayTagContainer` | Additional layout classifications, such as hanging labels; does not duplicate marker style |
| StartIndex | int32, default 1 | Starting displayed number for a numbered list, not an array index |
| bHasColumnCount | bool, default false | Whether a preferred column count was supplied |
| ColumnCount | int32, default 1 | Preferred column count; meaningful only when bHasColumnCount is true |

Preserve numbering and presentation metadata independently of the ordered item data. Column count is a layout preference, not a change to reading order. When supplied it must be positive; otherwise consumers ignore ColumnCount. Editor edit-condition metadata does not replace this semantic rule.

`EDnd5eListMarkerStyle` defines Unspecified, Bullet, Decimal, LowerRoman, UpperRoman, and None. Unspecified means no marker style has been established; None explicitly requests no markers. Marker Gameplay Tags are not part of the vocabulary. Unknown imported styles must remain recoverable and produce diagnostics rather than silently selecting a known enum member. The enum is a formatting value, not a catalog identity or general rule for how all single-valued properties are represented.

The container is a block payload; its wrapper uses `Dnd5e.Content.Block.List`, exposed as the native `Dnd5e::Content::Block::List` tag. `UDnd5eContentLibrary::TryGetList` takes a const block input and an `FDnd5eListContent` output. It follows the existing extraction contract: exact kind and payload type checks, read-only copy-out on success, and reset output on failure. Exact native presentation tags remain to be finalized.

#### Tables

Decision: decompose table content into distinct supporting types:

| Type | Responsibility |
| --- | --- |
| `FDnd5eTableCellContent` | Cell content, column span, and cell metadata |
| `FDnd5eTableRowContent` | Ordered cells and row presentation |
| `FDnd5eTableColumnContent` | Column presentation and sizing hints |
| `FDnd5eTableContent` | Caption, columns, header rows, body rows, and surrounding content |

Cells contain ordered blocks, allowing paragraphs, lists, and other supported content. Inline formatting remains within text-bearing blocks. Header rows and body rows remain distinct, and multiple header rows must be representable.

The inspected source's cell width represents a column span, not a pixel width. Preserve optional exact-result and result-range metadata for rollable cells without implementing dice rolling. The supporting types, table kind, and accessor described below are implemented and source-reviewed; source-format mappings, presentation vocabulary, and full content validation remain separate work. The table declarations and accessor are included in the successful standard editor build on 2026-10-29. Automated test execution for this batch remains deferred.

##### Table-cell base fields

The base `FDnd5eTableCellContent` contains ordered `Blocks` (`TArray<FDnd5eContentBlock>`), `ColumnSpan` (`int32`, default 1), and `Presentation` (`FGameplayTagContainer`). ColumnSpan must be positive. The row owns the cell as a typed value; it has no independent record identity or wrapper-kind tag. Its additional alignment and roll fields are defined below.

##### Table field contract

The following describes the implemented table representation. All types are shared across domains rather than spell-specific.

`EDnd5eContentAlignment` is a Blueprint-visible uint8 enum with Unspecified, Left, Center, Right, and Justify. Unspecified means no alignment override and is distinct from explicit Left. Alignment is not duplicated through Gameplay Tags.

###### FDnd5eTableRollRange

| Field | Type and default | Meaning |
| --- | --- | --- |
| Minimum | `int32 = 0` | Inclusive finite lower bound |
| Maximum | `int32 = 0` | Inclusive finite upper bound |
| bMinimumUnbounded | `bool = false` | Ignore Minimum when the source explicitly specifies no finite lower bound |
| bMaximumUnbounded | `bool = false` | Ignore Maximum when the source explicitly specifies no finite upper bound |
| bHasDisplayMinimum | `bool = false` | Whether DisplayMinimum is meaningful |
| DisplayMinimum | `int32 = 0` | Display-only lower number |
| bHasDisplayMaximum | `bool = false` | Whether DisplayMaximum is meaningful |
| DisplayMaximum | `int32 = 0` | Display-only upper number |
| PadToDigits | `int32 = 0` | Minimum displayed digit count; zero means no padding |

Equal finite bounds represent an exact result. Finite intervals require Minimum <= Maximum. Missing or malformed bounds must not silently become unbounded; the flags represent explicit source semantics. PadToDigits must be nonnegative. Preserve numerical matching bounds separately from display overrides and the displayed cell content. Percentile conventions require verified source mapping; zero must not globally be converted to 100. This type does not execute rolls.

###### FDnd5eTableCellContent additions

| Field | Type and default | Meaning |
| --- | --- | --- |
| Alignment | `EDnd5eContentAlignment = Unspecified` | Cell-specific alignment override |
| bHasRollRange | `bool = false` | Whether the cell carries roll-result metadata |
| RollRange | `FDnd5eTableRollRange` | Meaningful only when bHasRollRange is true |

Retain Blocks, ColumnSpan, and Presentation. Defaults inside RollRange do not imply that a cell is rollable. A cell's explicit alignment overrides its column preference; spanning-cell inheritance requires a documented consumer policy.

###### FDnd5eTableRowContent

| Field | Type | Meaning |
| --- | --- | --- |
| Cells | `TArray<FDnd5eTableCellContent>` | Cells in reading order |
| Presentation | `FGameplayTagContainer` | Row-specific style classifications |

Header and body rows share this type; their containing arrays establish their roles.

###### FDnd5eTableColumnContent

| Field | Type and default | Meaning |
| --- | --- | --- |
| Alignment | `EDnd5eContentAlignment = Unspecified` | Column alignment preference |
| Presentation | `FGameplayTagContainer` | Other supported column styles |
| bHasWidthWeight | `bool = false` | Whether WidthWeight is meaningful |
| WidthWeight | `float = 1.0f` | Positive finite relative width, when supplied |
| RollExpression | `FString` | Optional explicitly identified dice expression; empty means unspecified |

WidthWeight is proportional, not a physical distance or pixel count. Only supported source sizing syntax maps to it. Column headings are represented through HeaderRows, supporting grouped headings and column spans. RollExpression remains descriptive data; it is not inferred from arbitrary prose or evaluated by the content model.

5etools does not expose a numeric WidthWeight property: its colStyles values contain CSS-class strings. The inspected styles include fractional grid widths: col-2 maps to weight 2.0, col-2-1 to 2.1, and col-2-3 to 2.3. The latter is a decimal grid width, not the fraction two-thirds. A float preserves these fractions, whereas an integer would lose them. The adapter derives the normalized weight only from recognized source classes. Observed malformed or unrecognized styles must remain recoverable and diagnostic; do not map every col-prefixed token blindly.

###### FDnd5eTableContent

| Field | Type and default | Meaning |
| --- | --- | --- |
| Caption | `TArray<FDnd5eInlineContent>` | Ordered inline items forming one optional structured caption |
| Columns | `TArray<FDnd5eTableColumnContent>` | Logical column definitions |
| HeaderRows | `TArray<FDnd5eTableRowContent>` | Zero or more header rows |
| Rows | `TArray<FDnd5eTableRowContent>` | Body rows |
| Intro | `TArray<FDnd5eContentBlock>` | Preceding content |
| Footnotes | `TArray<FDnd5eContentBlock>` | Attached footnotes |
| Outro | `TArray<FDnd5eContentBlock>` | Following content |
| Presentation | `FGameplayTagContainer` | Other supported table styles |
| bStripedRows | `bool = true` | Alternating-row display preference |
| bIsNameGenerator | `bool = false` | Source-provided name-generator hint, without implementing generation |

The wrapper kind is `Dnd5e.Content.Block.Table`, exposed as `Dnd5e::Content::Block::Table`. `TryGetTable` takes a const block input and an `FDnd5eTableContent` output, following the same exact-kind/type checks, read-only extraction, and failure reset as existing accessors. Supporting rows, columns, cells, and ranges need no wrapper-kind tags.

The adapter must consider spans when establishing the logical grid and preserve multiple header rows, intentional row shapes, and unknown source metadata. A table schema is not complete source-format support: source mappings and full table validation remain separate implementation work.

Source normalization precedes validation of finite roll intervals. The renderer supports percentile zero display conventions and code-generated infinity sentinels that cannot be cast directly to int32. See the [compatibility mapping](source-format-compatibility.md#numeric-and-enum-checks). The numeric-content storage below supports retaining raw numeric cells as Number inline payloads inside paragraph blocks; rendering a number as Text alone does not preserve its original JSON type. Adapter mapping is not implemented.

#### Quotes

`FDnd5eQuoteContent` is implemented with these fields:

| Field | C++ type and default | Meaning |
| --- | --- | --- |
| Blocks | `TArray<FDnd5eContentBlock>` | Ordered quoted content |
| Attribution | `TArray<FDnd5eContentBlock>` | Optional speaker/author lines |
| Source | `TArray<FDnd5eInlineContent>` | Optional work/source title |
| Presentation | `FGameplayTagContainer` | Additional quote presentation classifications |
| bShowQuotationMarks | `bool = true` | Quotation-mark preference |
| bItalicize | `bool = true` | Italic-presentation preference |

The native wrapper kind is `Dnd5e.Content.Block.Quote`; `TryGetQuote` uses the standard typed extraction contract. Source entries map to Blocks; by maps to Attribution; from maps to Source. Source skipMarks and skipItalics invert the corresponding positive booleans. Omitted skip flags leave the defaults true. Preserve supported style and inline semantics without implementing rendering in the model.

#### Insets

`FDnd5eInsetContent` is implemented with these fields:

| Field | C++ type | Meaning |
| --- | --- | --- |
| Heading | `TArray<FDnd5eInlineContent>` | Optional structured heading |
| Blocks | `TArray<FDnd5eContentBlock>` | Ordered inset body |
| Presentation | `FGameplayTagContainer` | Additional subtype and presentation classifications |

The native wrapper kind is `Dnd5e.Content.Block.Inset`; `TryGetInset` uses the standard typed extraction contract. Both inset and insetReadaloud source forms share this payload, preserving their distinction through Presentation. The implemented native classification is `Dnd5e::Content::Presentation::ReadAloud`, with tag `Dnd5e.Content.Presentation.ReadAloud`. Source IDs, page/source overrides, and other nested metadata belong to the containing wrapper's implemented Metadata field; adapter population remains pending.

### Media content

The image/gallery block payloads, six native tags, and their two extractors are implemented, source-reviewed, and compiled in the host editor target on 2026-10-28. Their locations and descriptive metadata remain usable independently of any image-loading or UI subsystem. No automated tests were added or run for this batch. Source parsing, path resolution, resource loading, and typed map interpretation are not implemented.

#### Images

Exported, Blueprint-visible `FDnd5eImageContent` is declared in `Dnd5eContentType.h`, after both wrapper definitions, using the shared media-reference header.

| Field | C++ type and default | Meaning |
| --- | --- | --- |
| Resource | `FDnd5eMediaReference`, default-constructed | Main image location |
| bHasThumbnail | `bool = false` | Whether Thumbnail was supplied |
| Thumbnail | `FDnd5eMediaReference`, default-constructed | Optional separate thumbnail location |
| Title | `TArray<FDnd5eInlineContent>`, empty | Source title in reading order, preserving supported inline markup |
| Credit | `TArray<FDnd5eInlineContent>`, empty | Source credit in reading order |
| AltText | `FString`, empty | Supplied plain alternative text; no automatic title substitution in storage |
| bHasWidth | `bool = false` | Whether Width is supplied |
| Width | `int32 = 0` | Positive source pixel width, meaningful when bHasWidth is true |
| bHasHeight | `bool = false` | Whether Height is supplied |
| Height | `int32 = 0` | Positive source pixel height, meaningful when bHasHeight is true |
| Classification | `FGameplayTagContainer`, empty | Image roles, initially map/player-map classifications |
| Presentation | `FGameplayTagContainer`, empty | Supported styling expectations, initially light/dark background hints |

The implemented native wrapper kind is `Dnd5e::Content::Block::Image`, backed by `Dnd5e.Content.Block.Image`. The extractor is `TryGetImage(const FDnd5eContentBlock& ContentBlock, FDnd5eImageContent& Image)` using the existing exact-kind, exact-type, copy-out, and reset contract. It neither verifies resource existence nor loads pixels.

Use independent presence flags for width and height; an omitted dimension must not become an explicitly supplied zero. Supplied dimensions must be positive and checked before narrowing to int32. The snapshot's observed widths are 28–11,873 pixels and heights 79–9,000 pixels, all integers. These are intrinsic source dimensions, not Unreal world units, texture handles, or layout maxima. Do not infer a missing dimension or aspect ratio in the stored value.

When bHasThumbnail is false, keep Thumbnail default-constructed. When true, require a meaningful media reference. Thumbnail and Resource remain independent; a future consumer chooses whether/how to fall back to the main image. Title and Credit may contain text, formatted spans, references, and other semantically allowed inline content; existing link-label nesting constraints still apply within any nested links.

Implemented native classification tags are `Dnd5e::Content::ImageRole::Map` (`Dnd5e.Content.ImageRole.Map`) and `PlayerMap` (`Dnd5e.Content.ImageRole.Map.Player`). The future adapter maps source imageType map to Map and mapPlayer to PlayerMap. The child tag supports parent-category queries without requiring a second explicit Map entry. Missing imageType does not imply a map or a player-facing variant. Preserve unrecognized values with diagnostics; do not automatically register them as tags.

Implemented native presentation tags are `Dnd5e::Content::Presentation::ExpectsLightBackground` and `ExpectsDarkBackground`, backed by the matching `Dnd5e.Content.Presentation.*` strings. The future adapter assigns a hint only from a supplied true flag. Do not infer one from the absence of the other. Contradictory true flags require diagnosis rather than silently discarding one. These hints do not make the plugin render an image or change lighting.

Node IDs, source/page overrides, and diagnostic locators belong to the block wrapper's Metadata. Preserve original imageType values and unrepresented maxWidth/maxHeight/unit fields, style, grid, mapParent, mapRegions, hash, and other source properties through Metadata.AdditionalMetadataJson as applicable. Do not copy the recognized image body into that field. Map coordinates, grid interpretation, dynamic-map controls, inline image tags, browser-specific lazy-loading behavior, and full presentation normalization need separate contracts or fallback. Preserving these fields does not expose them as typed map APIs.

#### Galleries

Exported, Blueprint-visible `FDnd5eGalleryContent` is declared in `Dnd5eContentType.h`, after the block and inline wrappers.

| Field | C++ type and default | Meaning |
| --- | --- | --- |
| Heading | `TArray<FDnd5eInlineContent>`, empty | Optional source gallery name |
| Images | `TArray<FDnd5eContentBlock>`, empty | Ordered image wrappers, preserving each child's own metadata |
| Presentation | `FGameplayTagContainer`, empty | Supported gallery layout hints; initial vocabulary remains pending |

The implemented native kind is `Dnd5e::Content::Block::Gallery`, backed by `Dnd5e.Content.Block.Gallery`. The extractor is `TryGetGallery(const FDnd5eContentBlock& ContentBlock, FDnd5eGalleryContent& Gallery)` with the standard typed extraction contract. It does not walk or load its images.

The Images array uses wrappers rather than bare image payloads so source IDs, provenance, and the future unsupported-block fallback can remain attached at the original position. Initial valid children are Image blocks; the later fallback policy may admit explicit unsupported placeholders. Do not silently accept paragraphs or nested galleries as images. Unknown/malformed children require diagnosis and preservation or load failure according to the supported field's contract. No image-specific wrapper or per-gallery copy of metadata is introduced.

The source renderer copies gallery children, adds title/credit padding, and deletes imageType for display. The normalized model preserves the original role and metadata; renderer-only padding and role suppression are consumer presentation decisions. Gallery membership must not erase Map/PlayerMap classification or invent missing titles/credits. A gallery preserves ordering, not a commitment to a particular widget layout.

### Inline content

Formatting, annotations, and numeric values within text-bearing content.

#### Formatted inline content

Decision: represent a formatted span with `FDnd5eFormattedContent`.

| Field | Type | Meaning |
| --- | --- | --- |
| Formatting | `FGameplayTagContainer` | Formatting classifications, initially bold and italic; multiple classifications may apply together |
| Inlines | `TArray<FDnd5eInlineContent>` | Ordered inline children within the formatted span |

Store this payload in an inline wrapper whose kind identifies formatted content. The wrapper kind identifies the payload category; the Formatting tags describe presentation of its children. Children can contain literal text, nested formatting, record references, or other supported inline content.

The plugin preserves formatting information; a consuming UI determines its visual rendering. Do not flatten formatted spans into strings or discard embedded reference semantics. The implemented wrapper kind is `Dnd5e.Content.Inline.Formatted`; native formatting tags are `Dnd5e.Content.Formatting.Bold` and `Dnd5e.Content.Formatting.Italic`. `TryGetFormatted` follows the typed extraction contract. Additional source formatting and detailed nesting validation remain incomplete.

#### Annotation content

Implemented: exported, Blueprint-visible `FDnd5eAnnotationContent` is declared in `Dnd5eContentType.h`, after the inline wrapper. An annotation adds semantic information to its display content. Its kind selects a typed value, allowing different annotation families without adding unrelated optional fields to one struct. Storage, native tags, typed extraction, copy/reset behavior, serialization, and dice/damage token conversion are covered by the current [automation suite](testing.md). The [reader coverage](json-spell-reader.md#current-annotation-coverage) distinguishes the two implemented annotation kinds from remaining families.

| Field | C++ type and default | Meaning |
| --- | --- | --- |
| Kind | `FGameplayTag`, invalid | Semantic annotation kind, distinct from the enclosing inline kind |
| Display | `TArray<FDnd5eInlineContent>`, empty | Readable content in order, independent of the stored value |
| Value | `FInstancedStruct`, empty | Typed semantic value required by Kind |
| SourceFormat | `FString`, empty | Identifier for interpreting SourceAnnotation; initially `5etools` |
| SourceAnnotation | `FString`, empty | Complete original annotation token, retaining all supplied arguments |

The implemented enclosing kind is `Dnd5e::Content::Inline::Annotation`, backed by `Dnd5e.Content.Inline.Annotation`. The initial semantic kinds are `Dnd5e::Content::Annotation::DiceRoll` and `DamageRoll`, backed by `Dnd5e.Content.Annotation.DiceRoll` and `Dnd5e.Content.Annotation.DamageRoll`. Both require Value to contain exactly `FDnd5eDiceExpression`.

The two tags describe different levels: the inline kind chooses the content payload, while the annotation kind describes the meaning of its value. Additional annotation families can introduce their own typed values later. Do not encode their fields in JSON strings as a replacement for a future supported type. Unmodeled constructs use the implemented unsupported-content policy rather than pretending to be a supported DiceRoll annotation.

The implemented reader maps `@dice` to DiceRoll and `@damage` to DamageRoll. It stores the first argument as FDnd5eDiceExpression.Expression with Syntax=`5etools`, retains the complete token, and reads the explicit nonempty display argument or the expression into Display. Formula grammar validation and evaluation are not implemented; storing the typed unevaluated expression fulfills its reference-data role. Additional arguments remain in SourceAnnotation until their normalized contracts exist. `@autodice`, `@hit`, `@d20`, `@dc`, `@dcYourSpellSave`, chance, recharge, scaling, ability/check annotations, and legacy dice objects remain planned typed coverage. Each needs an appropriate contract and verified mapping; the reader currently preserves unmodeled tokens as Unsupported with warnings. Never infer a damage type from the DamageRoll kind alone.

The implemented APIs on `UDnd5eContentLibrary` are:

- `TryGetAnnotation(const FDnd5eInlineContent& InlineContent, FDnd5eAnnotationContent& Annotation)`: check the enclosing Annotation kind and exact payload type, copy out on success, otherwise reset and return false.
- `TryGetAnnotationDiceExpression(const FDnd5eAnnotationContent& Annotation, FDnd5eDiceExpression& Expression)`: accept exactly the DiceRoll or DamageRoll semantic kind and an exact FDnd5eDiceExpression Value, copy out on success, otherwise reset and return false.

The outer extractor does not recursively validate the semantic value. The inner extractor does not parse or evaluate the expression. Neither helper requires nonempty Display or performs source normalization. A future validator checks required fields and nesting. Imported annotations retain SourceFormat and the complete SourceAnnotation; manually authored annotations need not invent a source token. A supplied source token requires its format identifier. Provenance remains on the enclosing inline wrapper.

#### Numeric content

Implemented: exported, Blueprint-visible `FDnd5eNumberContent` is declared in `Dnd5eContentType.h`, after the inline wrapper, using the shared numeric-value header. Storage, the native tag, and its extraction helper were source-reviewed and compiled in the host editor target on 2026-10-28. Source conversion, token preservation, and recursive validation remain future work.

| Field | C++ type and default | Meaning |
| --- | --- | --- |
| Value | `FDnd5eNumber`, default-constructed | Numeric meaning, preserving its distinction from text |
| Display | `TArray<FDnd5eInlineContent>`, empty | Optional explicit display override; empty means render the numeric value |

The implemented native kind is `Dnd5e::Content::Inline::Number`, backed by `Dnd5e.Content.Inline.Number`. The API is `TryGetNumber(const FDnd5eInlineContent& InlineContent, FDnd5eNumberContent& Number)` with the existing exact-kind, exact-type, copy-out, and reset contract. It extracts a typed payload without converting or validating the contained numeric value.

A raw JSON numeric table cell maps to a cell containing a paragraph block with one Number inline. A JSON string containing digits remains Text unless it has separately recognized markup; do not guess that it was a number. A number can have a human-friendly Display without losing its numeric Value. If Display is empty, a consumer may use SourceLiteral when present or format the selected typed value; Unspecified is not a valid value to render as zero.

For both annotation and numeric display arrays, initially permit text and formatted spans containing text. Do not nest another annotation, number, record reference, or navigable link in these labels. Surrounding formatted spans can contain annotation/number nodes. A future recursive validator enforces these rules separately from extraction. No formatting UI or number-to-string helper is included in this batch.

### References and links

Catalog references, external destinations, and document-local anchors have separate contracts.

#### Inline record references

Decision: preserve display content, the target record identity when resolvable, and the original source reference in an inline record reference. The source reference must remain available for diagnosis or later resolution when the target is unavailable. Missing targets must not prevent retaining usable display content.

An unresolved descriptive link produces a warning rather than failing the entire catalog load. Required structural relationships still follow the stricter catalog-loading policy; a descriptive mention is not equivalent to a required parent or feature relationship. This supports readable content when only part of a dataset is imported.

Implemented: `FDnd5eReferenceContent` is an exported, Blueprint-visible payload in `Dnd5eContentType.h`, declared after `FDnd5eInlineContent`. Its fields, native tag, and accessor were source-reviewed and compiled successfully in the host editor target on 2026-10-28. No new automated tests were added or run for this batch.

| Field | C++ type and default | Meaning |
| --- | --- | --- |
| Display | `TArray<FDnd5eInlineContent>`, empty | Label in reading order, preserving formatting |
| SourceFormat | `FString`, empty | Identifier for interpreting the original reference; use `5etools` for the inspected format |
| SourceReference | `FString`, empty | Complete original reference token, including its tag name and supplied arguments |
| bHasTargetRecordId | `bool = false` | Whether TargetRecordId contains a normalized ID |
| TargetRecordId | `FDnd5eRecordId`, empty | Normalized record identity, meaningful only when the flag is true |

The implemented wrapper kind is `Dnd5e::Content::Inline::Reference`, backed by `Dnd5e.Content.Inline.Reference`. The extraction API is `TryGetReference(const FDnd5eInlineContent& InlineContent, FDnd5eReferenceContent& Reference)` on `UDnd5eContentLibrary`, following the existing exact-kind, exact-type, copy-out, and reset-on-failure contract.

The target flag means a normalized identity is available; it does not assert that a record exists in the active catalog. Target lookup is a separate operation. A reference whose source token cannot yet be normalized can retain Display and SourceReference with the flag false. If the flag is false, keep TargetRecordId default-constructed; if true, require a complete ID. Changing or replacing a catalog does not turn this value into a live object pointer.

For imported references, retain the full original token and its SourceFormat even after normalization succeeds. The adapter derives usable Display content from an explicit label or the source's supported label rules; it must not require a successful catalog lookup to retain a readable label. Manually authored references need not invent a source token. A supplied SourceReference requires a nonempty SourceFormat. Interpretation of omitted arguments still requires the owning record/document context; a token alone does not settle its target.

Display may contain text and formatted spans. Nested record references or other navigable links inside the label are invalid; a future recursive validator enforces this separately from typed extraction. Keep node provenance on the existing inline wrapper's Metadata. Document-local anchor links and external URLs follow the separate contracts below and are not encoded as fake record IDs. No parser, resolver, lookup status enum, or catalog mutation is introduced by this storage/accessor batch.

#### External link content

Implemented: exported, Blueprint-visible `FDnd5eExternalLinkContent` is declared in `Dnd5eContentType.h`, after the inline wrapper. Storage, the native kind, and its extractor were source-reviewed and compiled in the host editor target on 2026-10-28. It stores a destination URI and readable label as data, distinct from a catalog record reference and a document-local anchor link. Source normalization and navigation are not implemented.

| Field | C++ type and default | Meaning |
| --- | --- | --- |
| Display | `TArray<FDnd5eInlineContent>`, empty | Link label in reading order |
| Url | `FString`, empty | Destination URI, including any query or fragment |
| SourceFormat | `FString`, empty | Identifier for interpreting SourceReference; initially `5etools` |
| SourceReference | `FString`, empty | Complete original inline link token, when one exists |

The implemented native kind is `Dnd5e::Content::Inline::ExternalLink`, backed by `Dnd5e.Content.Inline.ExternalLink`. The API is `TryGetExternalLink(const FDnd5eInlineContent& InlineContent, FDnd5eExternalLinkContent& Link)` using the existing exact-kind, exact-type, copy-out, and reset-on-failure contract.

A usable link requires a nonempty destination. Imported labels are retained independently of whether a consumer can open the destination. Preserve supplied schemes, query strings, and fragments; do not rewrite them into catalog IDs, local file paths, or Unreal asset paths. An external URI fragment remains part of Url, not a local TargetAnchorId. Storage/extraction neither opens a browser nor fetches any content.

Initial adapter contracts cover explicit absolute destinations in `@link` tokens and typed link objects with external hrefs. The source also infers HTTP for scheme-less `@link` destinations; that behavior needs an explicit normalization policy before those forms are declared supported. Do not silently label a relative route or ambiguous scheme-less string as a resolved absolute URI. An unsupported destination may remain recoverable under the future fallback policy; a matching payload alone is not semantic validation.

Retain SourceFormat and the complete SourceReference for an imported inline token. Typed link objects do not invent a token; preserve additional unrepresented object metadata through wrapper Metadata, using the fallback policy when the object cannot be meaningfully normalized. A supplied SourceReference requires a format identifier. Manually authored links need not invent source text.

#### Document-local anchor link content

Implemented: exported, Blueprint-visible `FDnd5eAnchorLinkContent` is declared in `Dnd5eContentType.h`, after the inline wrapper. Storage, the native kind, and its extractor were source-reviewed and compiled in the host editor target on 2026-10-28. This initial contract targets an anchor **within the owning document**. No document ID is stored because document scope comes from the containing document context. Source parsing, anchor indexing, and target lookup are not implemented. No automated tests were added or run for this link batch.

| Field | C++ type and default | Meaning |
| --- | --- | --- |
| Display | `TArray<FDnd5eInlineContent>`, empty | Readable label in order |
| TargetAnchorId | `FString`, empty | Target's source-local anchor ID in the owning document |
| SourceFormat | `FString`, empty | Identifier for interpreting SourceReference; initially `5etools` |
| SourceReference | `FString`, empty | Complete original anchor-reference token, when one exists |

The implemented native kind is `Dnd5e::Content::Inline::AnchorLink`, backed by `Dnd5e.Content.Inline.AnchorLink`. The API is `TryGetAnchorLink(const FDnd5eInlineContent& InlineContent, FDnd5eAnchorLinkContent& Link)` using the existing exact-kind, exact-type, copy-out, and reset-on-failure contract.

TargetAnchorId must be nonempty for a usable target. Preserve its spelling and case; do not universally lowercase it, URL-encode it, derive it from a heading, or replace it with a JSON Pointer. The source adapter owns any verified source-specific transformation. An absent target, missing owning context, or ambiguous anchor does not turn this value into a global search across documents. A future resolver reports the condition and preserves Display; optional descriptive links can remain with warnings. Typed extraction does not perform lookup or require a loaded target.

The target node declares its anchor on its wrapper's Metadata.AnchorId (or list-item Metadata.AnchorId). The link payload stores that ID in TargetAnchorId. The link wrapper's own Metadata.AnchorId, if supplied, identifies the link node itself, not its destination. Source locators and record IDs remain different concepts.

An initial source mapping is `@area`, whose compact label, anchor ID, and flags are distinct arguments. Preserve its full original token and source-defined label behavior without depending on the website's global BookUtil renderer state. The document model and anchor index are not implemented by this batch. Cross-document `@book`/`@adventure` links can contain document IDs, chapter indexes, section names, and occurrences; they require a separate document-location contract and must not be treated as current-document anchor links. General website-internal routes, filters, and other unrecognized link forms likewise require dedicated mapping or fallback.

For both new link labels, initially permit text and formatted spans containing text. Do not nest navigable links or record references within a link label. Formatting may wrap a link. A future recursive validator checks nesting and required fields separately from extraction. Provenance remains on the existing inline wrapper. Neither payload introduces navigation behavior, a target pointer, a cached resolution flag, or a resolver API.

### Unsupported-content fallback

Decision: retain an unsupported-content payload at the original position when the adapter encounters a content construct it cannot interpret. Preserve:

- Original source kind and source fragment.
- Available display text, when supplied or reliably recoverable.
- Source location for diagnostics.

This is a compatibility mechanism for incomplete versions, unknown extensions, and future source changes. Known rules remain planned semantic work under [Semantic coverage](#semantic-coverage), even when their original source and display can already be retained. This applies to every rule family; difficulty classes are only one illustrative example.

Issue a diagnostic instead of silently discarding the construct. Consumers can show available text or an unsupported-content placeholder; preservation does not imply semantic support. Original fragments are opaque source data, not executable content, and follow the external-content storage and distribution boundaries.

Unsupported data that prevents interpretation of a required structural field still fails validation under the catalog-loading policy. The fallback is not a blanket exemption from validation failures. The enum, shared payload, two native tags, and both typed extractors below were implemented on 2026-10-28. The expanded [spell reader](json-spell-reader.md#content-inline-tokens-and-fallback-behavior) now retains unmodeled JSON Content nodes and inline tokens with warnings; malformed known structures fail. Full semantic validation remains pending.

#### Fallback payload

Implemented: `EDnd5eSourceFragmentKind : uint8` is declared in `Dnd5eContentType.h`, with Unspecified, Text, and Json members. This enum identifies the representation of a retained fragment, not a rule category or the enclosing content kind.

Exported, Blueprint-visible `FDnd5eUnsupportedContent` is declared in the same header. One payload serves both block and inline fallbacks because its fields have the same meaning in both contexts. It contains no normalized child wrappers and does not require either wrapper to be declared first.

| Field | C++ type and default | Meaning |
| --- | --- | --- |
| SourceFormat | `FString`, empty | Identifier for interpreting the original fragment; initially `5etools` for that format |
| FragmentKind | `EDnd5eSourceFragmentKind = Unspecified` | Text or Json representation of SourceFragment |
| SourceFragment | `FString`, empty | Complete retained source construct in the selected representation |
| DisplayText | `FString`, empty | Optional plain text supplied or reliably recovered for presentation |

Keep the original external kind in the enclosing wrapper's Metadata.SourceType, and the source locator in Metadata.RelativeFilePath/JsonPointer. Its Metadata may also retain any explicit anchor or source/page information. Do not duplicate these in new SourceKind or SourceLocation fields on the payload. The normalized wrapper Kind is Unsupported even when the original source kind is known but its semantics are not implemented.

An intended usable fallback requires a nonempty SourceFormat, a selected Text or Json FragmentKind, and a nonempty SourceFragment. DisplayText may be empty. SourceType may be empty if the source supplies no identifiable kind; do not invent an original type. The future validator enforces these requirements; default-constructed payloads remain extractable by type but are not automatically semantically valid.

#### Fallback tags and extraction

Implemented native tags:

| C++ symbol | Tag string | Wrapper / exact Payload type |
| --- | --- | --- |
| `Dnd5e::Content::Block::Unsupported` | `Dnd5e.Content.Block.Unsupported` | FDnd5eContentBlock / FDnd5eUnsupportedContent |
| `Dnd5e::Content::Inline::Unsupported` | `Dnd5e.Content.Inline.Unsupported` | FDnd5eInlineContent / FDnd5eUnsupportedContent |

Implemented APIs on UDnd5eContentLibrary:

- `TryGetUnsupportedBlock(const FDnd5eContentBlock& ContentBlock, FDnd5eUnsupportedContent& Unsupported)`.
- `TryGetUnsupportedInline(const FDnd5eInlineContent& InlineContent, FDnd5eUnsupportedContent& Unsupported)`.

Both follow the existing exact-kind, valid-Payload, exact-script-struct, const-read/copy-out contract. On failure, reset the output to FDnd5eUnsupportedContent{} and return false quietly. Sharing the payload does not make the two wrapper tags interchangeable. These helpers do not parse a fragment, recover display text, validate its contents, emit diagnostics, or claim semantic support.

#### Fragment preservation and display

Text stores the original source text directly, such as a complete inline token with its delimiters and arguments. Do not JSON-quote a Text fragment. Json stores a complete JSON value representing the unsupported node; object/array structure, property values, and value types must be retained. Preserve original JSON text when available. A value-preserving reserialization may differ in whitespace or property order; it must not be described as byte-identical original text. Numeric precision must not be silently lost before a fragment is captured. If equivalent preservation cannot be guaranteed, report the limitation or fail according to the load contract.

Unlike Metadata.AdditionalMetadataJson, SourceFragment may hold an entire unsupported source node. Retain that fragment in one fallback at its original position; do not also duplicate it as additional metadata or generate a second normalized copy of its body by guesswork. Additional metadata remains for separate retained properties.

DisplayText is plain optional presentation data, not an interpretation of the rule or a safe-to-execute/render-HTML marker. Recover it only from a supplied label or a documented, reliable extraction rule. If none is available, leave it empty and let a consumer present an unsupported-content placeholder. Do not automatically expose the raw fragment as normal display text or diagnostics. Source fragments remain opaque external data and follow the repository's external-content boundaries.

#### Validation and loading boundaries

A future adapter may create a fallback for an unknown source kind or an unsupported form whose original data can be retained. It must report incomplete semantic coverage separately from successful source preservation. Unknown properties on an otherwise supported node may belong in additional metadata; they do not automatically require replacing the entire node with a fallback.

Malformed selected JSON, invalid required identity fields, and unsupported required structural relationships remain load failures. Fallback must not make corrupt known payloads, missing mandatory references, or invalid required data appear valid. Specific child contracts, such as a gallery admitting an explicit unsupported placeholder, require an intentional rule; a fallback is not permission for arbitrary nesting.

The [spell reader](json-spell-reader.md#limits-and-diagnostics) enforces fixed input/depth/value limits plus shared Content conversion and table-expansion budgets. Broader loading still needs configurable retention/loading policies. Exceeding a limit must not silently truncate a fragment and claim full preservation. The shared storage types themselves add no parser, loader, cache, or limit enforcement.
