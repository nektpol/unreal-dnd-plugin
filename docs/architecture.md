# Architecture

## Module boundaries

Decision: use three modules. Their scaffolds are registered in `Dnd5e.uplugin`.

| Module | Type | Responsibility |
| --- | --- | --- |
| Dnd5e | Runtime | Shared data types, catalog, references, and read-only queries |
| Dnd5eJson | Runtime | External JSON parsing and normalization into core records |
| Dnd5eEditor | Editor | Dataset selection, loading controls, inspection, and diagnostics UI |

Dependencies flow from `Dnd5eEditor` to `Dnd5eJson` and `Dnd5e`, and from `Dnd5eJson` to `Dnd5e`. The core module must not depend on either of the other modules. Editor dependencies must remain outside runtime modules.

Data model, catalog, and queries initially share the core module. JSON loading can be used at runtime; this does not automatically load, download, or package any dataset.

The new modules currently contain empty startup and shutdown hooks. Their responsibilities describe future implementation, not completed functionality. Dependencies are private initially; move a dependency to the public list if future public headers require its types.

## Record identity

Decision: identify a record through three logical parts:

| Part | Responsibility |
| --- | --- |
| Dataset namespace | Identifies the logical collection independently of its local directory |
| Category | Distinguishes classes, spells, class features, books, and other record kinds |
| Source key | Holds the category-specific identity constructed by the format adapter |

The source key is an opaque value to the core catalog and query layer. The adapter owns its construction, normalization, and interpretation of external references. It must preserve all information required to distinguish records of that category; a universal name-plus-source pair is insufficient.

Inspection of [5etools source utilities](https://5e.tools/js/utils.js) established these starting points:

- Generic URL identities for many categories use name and source, with category supplied by context.
- Class-feature UIDs include feature name, class name, class source, level, and feature source, with defaults for omitted source fields.
- Books and adventures use explicit IDs for their URL identities.

These observations are not an exhaustive format contract. Inventory and verify each supported category's reference syntax, default values, escaping, and case normalization before implementation. A source key need not be identical to a website URL fragment.

Display names, local file paths, and dataset release versions are not separate identity components. Identity remains deterministic for unchanged source identity fields; upstream renames may change name-derived identities. Duplicate identities must generate diagnostics rather than silently overwrite records. Dataset namespace assignment and cross-dataset reference resolution still require detailed design.

No identity structs or parsers have been implemented by this decision.

## Next design decision

Choose the first domain to design, then inventory its source shape and define its supporting types.

## Illustrative class-level query (not a committed feature)

Class-level gains and XP lookup were examples of the data API boundary, not required features or the basis for the first milestone. The following is a possible query shape retained for reference, not a committed contract: a query could accept a class record ID, a class level, and an optional subclass record ID and return gains at the requested class level rather than cumulative gains or character-level/multiclass progression.

The result separates:

- Fixed grants: references to features granted at that level.
- Choices: selection requirements to be resolved by the consumer.
- Progression values: values at the requested level, such as spell slots.
- Progression changes: differences from the preceding level, where supported.
- Completeness information: missing context or unsupported details affecting the answer.

If subclass context is needed but absent, return known class information and mark the answer incomplete. A supplied subclass must belong to the supplied class; an incompatible selection is an invalid request, not a successful complete result. Missing records and invalid levels must produce explicit query failures.

The query must not apply grants, select options, or mutate character state. Fixed feature references and structured choice requirements must preserve source context. Precise Unreal types, level validation rules, first-level delta semantics, and result status representation remain to be designed.

## Catalog loading and validation

Decision: construct and validate a temporary catalog before replacing the active catalog. Queries must not observe a partially loaded replacement. A failed load preserves the previous active catalog, if any.

| Condition | Outcome |
| --- | --- |
| Invalid JSON in an input file selected for import | Fail the load |
| Duplicate normalized record IDs | Fail the load |
| Unresolved required reference | Fail the load |
| Unresolved optional reference | Retain the record with a diagnostic |
| Unknown field or classification | Preserve where practical and report a diagnostic |
| Unsupported content category | Report explicitly and allow supported categories to load |

Category contracts determine which references are required. If a supported record requires a target in an unsupported category, the required-reference failure rule still applies. Unknown constructs that prevent a required field from being interpreted must not be treated as harmless unknown metadata.

Every load produces a report identifying loaded, skipped, and incomplete content and diagnostics. A load that succeeds with warnings must remain distinguishable from a fully supported, warning-free import. This policy does not claim full category coverage or authorize silent loss of unsupported data.

## Provenance

Decision: preserve provenance separately from record identity. Each record retains, where available:

- Original publication/source identifier.
- Page number or source-provided page reference.
- Dataset-relative JSON file path.
- Position within that file, using a deterministic locator such as a JSON Pointer (exact locator format remains to be chosen).
- Original identity fields used by the adapter to construct the source key.
- Basis of normalized classifications, distinguishing explicit metadata from documented derivation.

The catalog retains the selected absolute dataset root and dataset version, when known, rather than repeating them on every record. Local paths must not be written into shared configuration or source control. File location is diagnostic information and does not form part of record identity. Source metadata is not proof of licensing permission.

For records assembled through inheritance or multiple inputs, the detailed representation of contributing origins remains to be designed; do not imply that a single file locator accounts for all derived fields.

## Edition metadata and Gameplay Tags

Decision: use Unreal Gameplay Tags extensively for classification and filtering. Preserve three separate concepts rather than collapsing them into one edition value:

- Declared edition: the record's explicit edition metadata, if supplied.
- Source generation: the source publication's classification, including whether it was classified by date or explicit metadata.
- Reference membership: source-provided SRD and Basic Rules flags.

Local inspection of 5etools 2.39.0 found explicit `edition` values `classic` and `one` on classes; source filtering also uses `SourceUtil.isClassicSource`, and SRD/Basic Rules membership uses separate fields. These mechanisms must remain distinguishable in the normalized model. Classification by publication date is not a claim of rules compatibility. Missing metadata does not mean applicability to both editions.

Retain original values and the basis of derived classifications alongside normalized tags. Source-provided membership flags do not constitute independently verified licensing permission.

Proposed tag branches (names pending agreement):

```text
Dnd5e.Edition.Declared.2014
Dnd5e.Edition.Declared.2024
Dnd5e.Source.Generation.2014
Dnd5e.Source.Generation.2024
Dnd5e.Reference.SRD.5_1
Dnd5e.Reference.SRD.5_2
Dnd5e.Reference.BasicRules.2014
Dnd5e.Reference.BasicRules.2024
```

Gameplay Tags classify records; record IDs continue to identify and reference individual records. Do not infer edition from identity strings.

Decision: define the fixed, plugin-owned classification vocabulary as native C++ Gameplay Tags in the core `Dnd5e` module. Category, edition, spell school, and damage type are examples of classification domains. Individual tag names and the complete vocabulary still require review. The adapter maps recognized external values to these tags; it must not automatically register arbitrary imported strings as native tags.

Dataset-specific extensions require a separate mapping and registration policy and are not settled by this decision. No tag definitions or module dependencies have been implemented yet.

### Missing metadata and classification validation

Decision:

- Assign a classification tag only when supported by source data or a documented derivation rule.
- Preserve unrecognized source values and produce diagnostics rather than guessing their meaning.
- Missing declared edition means no declared-edition tag; it does not mean applicability to both editions.
- Independently supported source-generation tags may still be assigned when declared edition is absent.
- Keep diagnostic status separate from content-classification tags.
- Validate mutually exclusive classifications according to each field's cardinality. Membership in multiple reference collections is not inherently a conflict.

The distinction between explicitly declared metadata and derived source classifications must remain visible to consumers. Catalog validation follows the loading policy above.

See [Epic's Gameplay Tags documentation](https://dev.epicgames.com/documentation/unreal-engine/using-gameplay-tags-in-unreal-engine) for dictionary registration and query behavior.

## Main content domains

Decision: organize the core model around the following domains. Proposed type names express the design direction; no corresponding structs have been implemented yet.

| Domain | Proposed primary representation |
| --- | --- |
| Species / Races | FDnd5eSpecies |
| Classes | FDnd5eClass |
| Feats | FDnd5eFeat |
| Options and Features | Separate feature and option record types |
| Backgrounds | FDnd5eBackground |
| Items | FDnd5eItem |
| Spells | FDnd5eSpell |
| Conditions | FDnd5eCondition |
| Bestiary | FDnd5eCreature |
| Adventures | FDnd5eAdventure |
| Books | FDnd5eBook |

All primary records carry shared identity, display name, classification Gameplay Tags, and provenance, plus domain-specific typed data. These are organizational boundaries, not a requirement to fit every website section into exactly one struct or a claim of exhaustive dataset coverage.

Use supporting value structs for embedded information such as range. Independently identifiable content, such as subclasses and features, has its own record identity and is referenced where appropriate. Books and adventures share supporting document-content types. Determine the ownership and reference semantics of individual fields during domain design.

Additional domains may be introduced as the format inventory expands. The API exposes content and reference information; gameplay state and rule execution remain the consuming project's responsibility.

### Shared record header

Decision: use composition for common record metadata. Each primary domain record contains an `FDnd5eRecordHeader` with record identity, display name, classification Gameplay Tags, and provenance. Domain-specific fields remain on the containing record.

Supporting value structs do not require a header unless they represent independently identifiable records. The header provides consistency without requiring a single oversized record type or inheritance between domain structs. Exact field types and reflected declarations remain to be designed; this decision adds no implementation.
