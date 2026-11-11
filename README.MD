# Dnd5e

An Unreal Engine plugin being developed to represent user-supplied fifth-edition reference content as structured data for C++ and Blueprint consumers.

## Status

The [item foundation](docs/item-model.md) now provides a Blueprint-visible item value, abbreviated type/property selectors, and distinct rarity classifications. Shared price, mass, readable attunement and structured attunement indexes and weapon/armor values, type/property/mastery records and named mastery references are implemented. Definition IDs, supplied definition JSON conversion, catalog replacement and Blueprint lookup are available. The base-item reader converts supplied baseitem rows into item values. The reader converts the inspected weapon and armor fields. The selected-row ordinary-item reader converts readable and structured attunement and direct base-item links. Blueprint callers can resolve a link against supplied base records. The copy assembler previews ordinary-item chains and the supported array modifiers from a supplied document, with source-row origins. The typed copy reader converts assembled rows directly and preserves original field and dependency locations. The magic variant matcher resolves standalone and copied templates and finds eligible base rows from two supplied documents using source predicates and an explicit edition policy. The generator previews eligible specific variants with inherited fields, supported entry substitutions and standalone transforms, a stable pair ID and both original source locations. The typed variant reader converts the same pair into an item with its pair ID, original base link and template ancestry. The group reader converts named item families with distinct IDs and ordered member selectors. The reusable-entry reader preserves template recipes, and Content represents their references with typed selectors. Callers can expand one supplied reference into replacement Content blocks. The supplementary reader converts item text and images, including copied fields, and attaches them by canonical name and publication. The shared item reader converts fixed and dice-based charges and recharge declarations. Typed items also expose signed bonuses and attached spell declarations with their authored usage and cast levels. The typed item catalog exposes atomic replacement, exact ID lookup and filtered Blueprint queries. Its JSON builder stages base items, ordinary items, resolved copies and groups from supplied documents before publishing the replacement. The same build can generate explicitly selected variants and attach supplementary content by authored identity. Separate catalog lookups resolve definitions, group members, and attached spells in caller-selected dataset namespaces. The offline item loader returns an item catalog and a definition catalog from explicitly selected local files. The editor can remember an item selection, retain both catalogs, and show searchable item and group rows with read-only details.

Early development. The runtime module contains shared identity, provenance, measurements, costs, dice-expression, media, spell, and structured Content values with read-only extraction helpers and an in-memory spell catalog. A Blueprint-visible UObject owns each catalog and exposes read-only lookup/filtering and validated replacement. The JSON adapter builds catalogs from supplied documents through native and Blueprint APIs, optionally joins supplied availability and supplementary text/image documents, supports explicit edition policy and Content validation, and retains unknown future Content with warnings. General reference resolution, navigation, execution, media loading, and complete rule semantics remain pending.

The spell reader maps material requirements/copper costs/consumption, point and area ranges, all four duration kinds, combat classifications, typed character-level dice scaling, sections/lists/tables/quotes/insets, formatting, dice annotations, typed generic/class-feature selectors, document/query destinations, notes, image definitions, and HTTP(S) links. The catalog supports exact-ID lookup and filters for name/aliases, source, level, schools, traits, and availability providers. Unknown rule annotations, future Content forms, and unsupported presentation details remain explicit retained data.

Verified on 2026-11-11: 884 standard cases plus 36 external audit cases pass (920 total). The item audits cover 102 definitions, 230 base items, 67 ordinary-item copy chains through raw assembly and typed conversion, 119 item groups with 669 member declarations, and 13 reusable entry templates. The bonus and spell audit converts 519 bonuses and 1,196 attached spell references. The charge audit converts 326 capacity fields, 325 recharge triggers and 293 recharge amounts. The supplementary item audit reads 980 rows and resolves 984 typed image blocks. The reference audit reads and expands all 122 authored entry tokens with the arguments from the supplied item and variant files. The variant audit checks all 230 templates under both edition policies and generates 221 first-eligible standalone previews and all five eligible copied previews. It converts the five copied pairs into typed items and checks preview identity and original row locations. The audit converts all 969 primary records, 92 supplementary rows and 91 images, with source/typed counts matching all 27 observed inline token kinds and no opaque payloads in this snapshot. Availability, all 389 reprint targets, runtime/editor behavior and atomic failure remain covered. See [Automated tests](docs/testing.md) for evidence and limits.

The [JSON spell reader](docs/json-spell-reader.md) accepts supplied text and an explicit spell-array index. It constructs identity/provenance and typed values, retains supported metadata and fallbacks, and returns diagnostics. Unknown required top-level values fail with a default output spell. The runtime reader performs no file or network access.

For Blueprint use, construct and retain a [spell catalog object](docs/spell-catalog.md#blueprint-ownership-and-operations), supply document text to BuildSpellCatalog or explicit filenames to [LoadSpellCatalog](docs/json-spell-file-loader.md), query IDs, and retrieve copied structs with TryGetSpell. [ResolveSpellReference](docs/spell-catalog.md#spell-reference-resolution) follows a supplied normalized target ID and distinguishes missing/invalid IDs, unsupported categories, and absent targets. It does not infer omitted sources. Each object owns an independent transient catalog; failed replacement preserves its previous records. The [editor tools](docs/editor-dataset-loading.md) add file pickers, remembered local selection, load diagnostics, and a searchable spell list with read-only structured details.

[MatchSpellSelector](docs/spell-catalog.md#spell-selector-matching) matches loaded canonical names/primary publications case-insensitively within an exact namespace, returning stored IDs. It reports MissingSource even for one name candidate, and Ambiguous for multiple explicit-source candidates. It does not choose a default source or change exact-ID lookup or existing query semantics.

The development host project uses Unreal Engine 5.8. Compatibility with other versions has not been established.

The [editor spell browser](docs/editor-dataset-loading.md#reference-report-inspection) now displays the selected spell's detached reference report in its existing read-only inspector. Its cached summary distinguishes original lookup, normalization, normalized lookup, and collection diagnostics; search and refresh clear the snapshot.

[ReportSpellReferences](docs/spell-reference-report.md) combines located occurrences, original-ID lookup, retained-token normalization, and normalized lookup into a detached report. It preserves independent outcomes and diagnostics, including conflicting valid targets, without rewriting the spell or catalog.

[CollectSpellReferences](docs/spell-content-references.md) walks both spell description trees and returns independent typed reference occurrences with model locations. It enforces structural/size limits and warns when preserved fallback content remains opaque. Collection does not resolve or rewrite references.

[NormalizeSpellReference](docs/spell-reference-normalization.md) explicitly reconciles one retained spell token with a loaded canonical ID. It preserves source/display content and reports omitted sources, ambiguity, unsupported syntax, and absent targets. It does not rewrite the imported catalog automatically.

Unsupported block/inline payloads retain whole JSON nodes or inline token fragments when typed mapping is incomplete. Successful source acceptance with fallback warnings does not establish full semantic rule support.

Annotation infrastructure is implemented: `@dice` and `@damage` become typed DiceRoll/DamageRoll annotations containing unevaluated dice expressions, display content, and their original tokens. Literal DC, attack/generic d20 modifiers, and percentage/outcome annotations are now typed as well; remaining annotation kinds retain explicit fallbacks. See [current annotation coverage](docs/json-spell-reader.md#current-annotation-coverage); extending typed representation does not require implementing gameplay execution.

## Planned capabilities

- Read local, user-supplied datasets in a documented 5etools-compatible JSON format.
- Represent classes, subclasses, species, spells, items, creatures, books, adventures, and other reference content.
- Distinguish 2014 and 2024 rules content and preserve source information.
- Expose typed records, structured document content, and read-only queries to C++ and Blueprints.
- Work toward semantic representation of all rules in the supported fifth-edition content, including their conditions, dependencies, and exceptions. Current document payloads and source-fragment preservation do not establish this coverage.
- Answer reference queries such as what a class gains at a given level or which level corresponds to an XP total.

The consuming project manages character state, resolves choices, and applies gameplay changes. This plugin does not implement a leveling system, combat system, or automated rules engine.

Explicit `name|source` spell reprints now receive normalized target IDs while retaining their original selectors. ResolveSpellRelationship retrieves a chosen loaded target without merging editions, replacing the original record, or following chains. Omitted-source and extended selectors remain separate policy/coverage work.

## External data

No third-party dataset is bundled with this repository. The plugin is intended to operate on files explicitly supplied by the user, without downloading data or connecting to 5etools.

Keep datasets and generated content outside both the plugin directory and Git repository. Store local dataset paths only in untracked, per-user configuration. Do not commit source records, extracted text, images, caches, or generated content assets.

Format compatibility does not grant permission to use or redistribute content. Users must have the necessary rights for their datasets and any resulting exports. Development fixtures should use original synthetic content or appropriately licensed material with required attribution.

## Development setup

1. Create a C++ Unreal Engine project to serve as the development host.
2. Place or clone this repository at `<HostProject>/Plugins/Dnd5e`.
3. Open the host project's `.uproject`, enable Dnd5e if necessary, and build the host project's editor target.

The [file loader reference](docs/json-spell-file-loader.md) documents runtime/Blueprint loading. For editor use, open **Tools → Dnd5e Dataset Settings**, choose a root and spell files, then choose **Tools → Load Dnd5e Dataset**. Open **Tools → Dnd5e Spell Browser** to search loaded names and inspect a selected record. The [editor reference](docs/editor-dataset-loading.md) explains local settings, session ownership, diagnostics, and browsing. No dataset is needed to construct or query synthetic in-memory content types.

## Documentation

- [Automated tests](docs/testing.md): coverage, running the development suite, fixtures, and verification limits.
- [Spell reference reports](docs/spell-reference-report.md): located original/normalized references, independent lookup outcomes, stage diagnostics, and detached snapshots.
- [Spell Content reference collection](docs/spell-content-references.md): bounded traversal, copied occurrences/model locations, failures, and opaque-fallback warnings.
- [Retained spell-reference normalization](docs/spell-reference-normalization.md): explicit token-to-canonical-ID reconciliation against a loaded catalog, statuses, preservation, and limits.
- [JSON spell reader](docs/json-spell-reader.md): implemented text-to-spell API, accepted subset, identity, preservation, limits, and diagnostics.
- [JSON spell-file loader](docs/json-spell-file-loader.md): explicit external selections, bounded reads, native/Blueprint calls, provenance, and atomic failure behavior.
- [Editor dataset loading](docs/editor-dataset-loading.md): file selection, local preferences, Tools commands, Message Log results, and searchable read-only spell browsing.
- [Spell availability and catalog](docs/spell-catalog.md): supplied-document builds, availability joins, atomic replacement, exact-ID/reference lookup, and read-only filters.
- [Source-format compatibility](docs/source-format-compatibility.md): verified source mappings, normalization requirements, and known gaps for the audited dataset snapshot.
- [Supplementary input and metadata](docs/spell-data-inputs.md): supplied prose/images, exact joins, source memberships/hints, and explicit edition assignments.
- [Content validation](docs/content-validation.md): read-only structural/value validation, model issue paths, and opt-in atomic builder integration.
- [Shared models](docs/shared-models.md): reusable metadata, measurements, money, components, and structured content.
- [Item definitions](docs/item-definitions.md): type/property/mastery records, named references, retained templates and identity limits.
- [Item model](docs/item-model.md): implemented foundation fields, selector/rarity roles, shared Content reuse, and remaining item work.
- [Spell model](docs/spell-model.md): implemented spell fields, classification roles, relationships, and spell-specific area/duration context.

## License

See [LICENSE](LICENSE) for the repository license. Third-party datasets retain their own applicable terms; the repository license does not grant rights to those datasets.
