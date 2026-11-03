# Item definitions

Status: type/property/mastery records and named mastery-reference storage are implemented. Definition-aware identity construction, lookup, JSON conversion, template expansion and copy/variant assembly remain pending. Registering Category.ItemMastery does not add inline-token conversion or reference resolution to the existing spell reader.

All four exported Blueprint-visible structs are declared in [Dnd5eItemDefinitions.h](../Source/Dnd5e/Public/Items/Dnd5eItemDefinitions.h). They compose shared headers/provenance, Content, selectors and directed relationships. They describe reference data without executing a mastery/property rule. See [Automated tests](testing.md) for verified scope.

## Contents

- [Type definition](#type-definition)
- [Property definition](#property-definition)
- [Mastery definition](#mastery-definition)
- [Mastery reference](#mastery-reference)
- [Identity and source boundaries](#identity-and-source-boundaries)
- [Templates and remaining work](#templates-and-remaining-work)

## Type definition

FDnd5eItemTypeDefinition describes an item family separately from an item record.

| Field | C++ type and default | Meaning |
| --- | --- | --- |
| Header | `FDnd5eRecordHeader`, default | Shared display/identity/provenance/membership data |
| Abbreviation | `FString`, empty | Supplied type abbreviation |
| Description | `TArray<FDnd5eContentBlock>`, empty | Definition Content |
| Relationships | `TArray<FDnd5eRecordRelationship>`, empty | Directed links and retained source targets |

It contains no item-instance price, weight or equipment state. Default construction assigns no category or canonical key.

## Property definition

FDnd5eItemPropertyDefinition retains definition Content and an independent unexpanded display template.

| Field | C++ type and default | Meaning |
| --- | --- | --- |
| Header | `FDnd5eRecordHeader`, default | Shared data; DisplayName may remain unrepresented |
| Abbreviation | `FString`, empty | Supplied property abbreviation |
| Description | `TArray<FDnd5eContentBlock>`, empty | Definition Content |
| Relationships | `TArray<FDnd5eRecordRelationship>`, empty | Directed links and retained source targets |
| bHasTemplate | `bool = false` | Whether a separate string template is supplied |
| TemplateSyntax | `FString`, empty | Grammar identifier, active when bHasTemplate is true |
| TemplateText | `FString`, empty | Exact unexpanded string, active when bHasTemplate is true |

A supplied empty template differs from absence. False presence does not clear inactive syntax/text; both have editor EditCondition metadata. Construction and serialization do not parse or expand placeholders. A future adapter can identify the source property grammar as `5etools-item-property`; this is distinct from dice-expression syntax. Synthetic authoring/tests use their own syntax identifier.

## Mastery definition

FDnd5eItemMasteryDefinition represents a named definition, without an abbreviation.

| Field | C++ type and default | Meaning |
| --- | --- | --- |
| Header | `FDnd5eRecordHeader`, default | Shared named-definition data and provenance |
| Description | `TArray<FDnd5eContentBlock>`, empty | Mastery definition Content |
| Relationships | `TArray<FDnd5eRecordRelationship>`, empty | Declared directed links when supplied |

Category.ItemMastery identifies the domain. Individual mastery names are externally supplied records, not native tags. No current links or gameplay effects are fabricated.

## Mastery reference

FDnd5eItemMasteryReference is composed by [FDnd5eItem.Masteries](item-model.md#definition-selection).

| Field | C++ type and default | Meaning |
| --- | --- | --- |
| Selector | `FDnd5eRecordSelector`, default | Named mastery request, using Category.ItemMastery when authored |
| Note | `TArray<FDnd5eInlineContent>`, empty | Independent readable note attached to this reference |

A source UID string can later map to a selector without a note; an object containing uid/note can retain both. These mappings are not implemented yet. Notes remain on the item reference rather than mutating or duplicating the target definition. An omitted publication stays omitted; no target ID or availability rule is inferred.

## Identity and source boundaries

Type and property requests use the shared [abbreviation selector](shared-models.md#item-definition-selector), with distinct ItemType and ItemProperty categories. Masteries use the shared named FDnd5eRecordSelector. The same abbreviation can exist in different domains/publications; it is not a fabricated record name. Dataset scope and publication remain distinct inputs to a future lookup.

Shared [record headers/provenance](shared-models.md#record-header) retain original names, origins and membership evidence. Most inspected property definitions lack a direct source name, so an empty DisplayName does not imply a missing abbreviation identity. Any derived display label needs a separate documented policy. These constructors assign no categories, sources, defaults or canonical IDs. The existing spell identity constructor is not a definition identity constructor.

Relationships reuse the [directed relationship value](shared-models.md#record-relationships). Raw reprint strings can be retained with bHasTargetRecord=false; the existing spell reprint normalizer does not normalize definition targets. Source renderer aliases/default-publication fallbacks are not automatically plugin policy.

## Templates and remaining work

Definition Content is not automatically appended to an item's Description. Property templates and placeholders inside definition Content need explicit application rules, supported substitutions and provenance. Missing item fields cannot be silently replaced with empty text to claim successful expansion. Structured itemEntry.entriesTemplate input is a separate later concern, not automatically represented by a string template field.

The values preserve nested Content, notes, selectors, template storage and relationships through copying/reflected serialization. They add no parser, evaluator, validator, lookup or assembly stage. Type _copy dependencies and additional type-entry records must be handled explicitly before complete imported definitions are claimed. Canonical IDs/lookup, bounded supplied-document conversion, remaining template/assembly contracts and finite source audits remain follow-up work.
