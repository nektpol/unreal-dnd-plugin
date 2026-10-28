# Dnd5e

An Unreal Engine plugin being developed to represent user-supplied fifth-edition reference content as structured data for C++ and Blueprint consumers.

## Status

Early development and architecture planning. The plugin currently contains the initial Unreal module scaffold; the capabilities below describe the intended scope, not implemented features.

The development host project uses Unreal Engine 5.8. Compatibility with other versions has not been established.

## Planned capabilities

- Read local, user-supplied datasets in a documented 5etools-compatible JSON format.
- Represent classes, subclasses, species, spells, items, creatures, books, adventures, and other reference content.
- Distinguish 2014 and 2024 rules content and preserve source information.
- Expose typed records, structured document content, and read-only queries to C++ and Blueprints.
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

Local dataset configuration and import instructions will be documented when those features are implemented. No dataset is needed for the current scaffold.

## Documentation

- [Project specification](docs/project-specification.md): scope, data boundaries, proposed architecture, queries, milestones, and acceptance criteria.

## License

See [LICENSE](LICENSE) for the repository license. Third-party datasets retain their own applicable terms; the repository license does not grant rights to those datasets.
