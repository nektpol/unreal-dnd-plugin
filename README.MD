# Dnd5e

An Unreal Engine plugin being developed to represent user-supplied fifth-edition reference content as structured data for C++ and Blueprint consumers.

## Status

Early development. The runtime module contains shared identity, provenance, measurements, costs, dice-expression, media, spell, and structured Content values with read-only extraction helpers and an in-memory spell catalog. A Blueprint-visible UObject owns each catalog and exposes read-only lookup/filtering and validated replacement. The JSON adapter builds catalogs from supplied documents through native and Blueprint APIs, optionally joins a supplied availability lookup, and retains unmodeled Content with warnings. General reference resolution, navigation, execution, media loading, and complete rule semantics remain pending.

The spell reader maps material requirements/copper costs/consumption, point and area ranges, all four duration kinds, combat classifications, typed character-level dice scaling, sections/lists/tables/quotes/insets, formatting, dice annotations, selected references, and HTTP(S) links. The catalog supports exact-ID lookup and filters for name/aliases, source, level, schools, traits, and availability providers. Unknown rule annotations, future Content forms, and unsupported presentation details remain explicit retained data.

The standard editor automation suite has 329 passing cases on 2026-10-30. The combined opt-in run passed 349 cases, converted all 969 records across the supplied snapshot's 18 spell files, joined 14,448 availability associations, and exercised runtime/editor loading, the spell browser, and normalized-reference lookup. See [Automated tests](docs/testing.md) for evidence and limits.

The [JSON spell reader](docs/json-spell-reader.md) accepts supplied text and an explicit spell-array index. It constructs identity/provenance and typed values, retains supported metadata and fallbacks, and returns diagnostics. Unknown required top-level values fail with a default output spell. The runtime reader performs no file or network access.

For Blueprint use, construct and retain a [spell catalog object](docs/spell-catalog.md#blueprint-ownership-and-operations), supply document text to BuildSpellCatalog or explicit filenames to [LoadSpellCatalog](docs/json-spell-file-loader.md), query IDs, and retrieve copied structs with TryGetSpell. [ResolveSpellReference](docs/spell-catalog.md#spell-reference-resolution) follows a supplied normalized target ID and distinguishes missing/invalid IDs, unsupported categories, and absent targets. It does not infer omitted sources. Each object owns an independent transient catalog; failed replacement preserves its previous records. The [editor tools](docs/editor-dataset-loading.md) add file pickers, remembered local selection, load diagnostics, and a searchable spell list with read-only structured details.

The development host project uses Unreal Engine 5.8. Compatibility with other versions has not been established.

Unsupported block/inline payloads retain whole JSON nodes or inline token fragments when typed mapping is incomplete. Successful source acceptance with fallback warnings does not establish full semantic rule support.

Annotation infrastructure is implemented: `@dice` and `@damage` become typed DiceRoll/DamageRoll annotations containing unevaluated dice expressions, display content, and their original tokens. Other annotation kinds currently remain preserved fallbacks. See [current annotation coverage](docs/json-spell-reader.md#current-annotation-coverage); extending typed representation does not require implementing gameplay execution.

## Planned capabilities

- Read local, user-supplied datasets in a documented 5etools-compatible JSON format.
- Represent classes, subclasses, species, spells, items, creatures, books, adventures, and other reference content.
- Distinguish 2014 and 2024 rules content and preserve source information.
- Expose typed records, structured document content, and read-only queries to C++ and Blueprints.
- Work toward semantic representation of all rules in the supported fifth-edition content, including their conditions, dependencies, and exceptions. Current document payloads and source-fragment preservation do not establish this coverage.
- Answer reference queries such as what a class gains at a given level or which level corresponds to an XP total.

The consuming project manages character state, resolves choices, and applies gameplay changes. This plugin does not implement a leveling system, combat system, or automated rules engine.

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
- [JSON spell reader](docs/json-spell-reader.md): implemented text-to-spell API, accepted subset, identity, preservation, limits, and diagnostics.
- [JSON spell-file loader](docs/json-spell-file-loader.md): explicit external selections, bounded reads, native/Blueprint calls, provenance, and atomic failure behavior.
- [Editor dataset loading](docs/editor-dataset-loading.md): file selection, local preferences, Tools commands, Message Log results, and searchable read-only spell browsing.
- [Spell availability and catalog](docs/spell-catalog.md): supplied-document builds, availability joins, atomic replacement, exact-ID/reference lookup, and read-only filters.
- [Source-format compatibility](docs/source-format-compatibility.md): verified source mappings, normalization requirements, and known gaps for the audited dataset snapshot.
- [Shared models](docs/shared-models.md): reusable metadata, measurements, money, components, and structured content.
- [Spell model](docs/spell-model.md): implemented spell fields, classification roles, relationships, and spell-specific area/duration context.

## License

See [LICENSE](LICENSE) for the repository license. Third-party datasets retain their own applicable terms; the repository license does not grant rights to those datasets.
