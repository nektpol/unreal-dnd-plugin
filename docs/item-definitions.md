# Item definitions

Status: definition records, canonical identity construction, native catalog and Blueprint lookup are implemented. Supplied JSON conversion, template expansion and copy/variant assembly remain follow-up work.

All four exported Blueprint-visible structs are declared in [Dnd5eItemDefinitions.h](../Source/Dnd5e/Public/Items/Dnd5eItemDefinitions.h). They compose shared headers/provenance, Content, selectors and directed relationships. They describe reference data without executing a mastery/property rule. See [Automated tests](testing.md) for verified scope.

## Contents

- [Type definition](#type-definition)
- [Property definition](#property-definition)
- [Mastery definition](#mastery-definition)
- [Mastery reference](#mastery-reference)
- [Identity and source boundaries](#identity-and-source-boundaries)
- [Catalog and Blueprint API](#catalog-and-blueprint-api)
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

Type and property requests use the shared [abbreviation selector](shared-models.md#item-definition-selector), with distinct ItemType and ItemProperty categories. Masteries use the shared named FDnd5eRecordSelector. The same abbreviation can exist in different domains/publications; it is not a fabricated record name. Dataset scope and publication are distinct inputs to catalog lookup.

Shared [record headers/provenance](shared-models.md#record-header) retain original names, origins and membership evidence. Most inspected property definitions lack a direct source name, so an empty DisplayName does not imply a missing abbreviation identity. Any derived display label needs a separate documented policy. Value construction leaves identity fields at their defaults. Call the definition ID factory with the dataset namespace, category, key and publication.

Relationships reuse the [directed relationship value](shared-models.md#record-relationships). Raw reprint strings can be retained with bHasTargetRecord=false; the existing spell reprint normalizer does not normalize definition targets. Source renderer aliases/default-publication fallbacks are not automatically plugin policy.

## Catalog and Blueprint API

Dnd5e::ItemDefinitions::TryMakeId produces a SourceKey of the form `dnd5e:item-definition:v1:<domain>:<key>|<source>`. The domain is type, property or mastery. The key is an abbreviation for types/properties and DisplayName for masteries. Key and publication are trimmed and lowercased. The factory escapes percent, pipe and colon in that order. The trimmed dataset namespace remains case-sensitive. Empty parts, control characters, components over 1,024 characters and categories outside these three domains fail and reset the output. [Identity declaration](../Source/Dnd5e/Public/Items/Dnd5eItemDefinitionIdentity.h)

ReplaceDefinitions stages all three arrays before publishing a replacement. It recomputes each ID using the row's category domain, key and primary provenance publication. A missing or mismatched identity, duplicate or total above 100,000 fails and preserves the active catalog. Empty arrays clear the catalog. IDs retain supplied order within each domain, with types first, then properties, then masteries. The caller coordinates concurrent access. [Native catalog](../Source/Dnd5e/Public/Catalog/Dnd5eItemDefinitionCatalog.h)

| Operation | Result |
| --- | --- |
| TryGetType, TryGetProperty, TryGetMastery | Exact ID lookup in the typed domain. Returns a copy and resets the output on a miss. |
| MatchDefinitionSelector | Matches type/property abbreviation in the requested category and scope. |
| MatchMasterySelector | Matches mastery DisplayName in scope and rejects a populated qualifier. |
| GetDefinitionCount, GetDefinitionIds | Total count and a copy of ordered IDs. |

Explicit publication yields Matched with one ID or NoMatch with an empty array. A valid selector with omitted publication yields MissingSource and zero or more inspection candidates. An inactive SourceId is ignored. Invalid scope, key, active publication, category or mastery qualifier yields InvalidSelector. Both output arrays reset on each match. Branch on Matched before retrieving a record.

Blueprint callers use Try Make Definition Id and a retained Item Definition Catalog Object. The wrapper exposes the same replacement, matching, retrieval and count operations. [Blueprint library](../Source/Dnd5e/Public/Items/Dnd5eItemDefinitionLibrary.h) · [Blueprint catalog object](../Source/Dnd5e/Public/Catalog/Dnd5eItemDefinitionCatalogObject.h)

## Templates and remaining work

Definition Content is not automatically appended to an item's Description. Property templates and placeholders inside definition Content need explicit application rules, supported substitutions and provenance. Missing item fields cannot be silently replaced with empty text to claim successful expansion. Structured itemEntry.entriesTemplate input is a separate later concern, not automatically represented by a string template field.

The values preserve nested Content, notes, selectors, template storage and relationships through copying/reflected serialization. The catalog validates identity and owns copies. Content parsing, template evaluation and assembly belong to later stages. Type _copy dependencies and additional type-entry records must be handled explicitly before complete imported definitions are claimed. Bounded supplied-document conversion, remaining template/assembly contracts and finite source audits remain follow-up work.
