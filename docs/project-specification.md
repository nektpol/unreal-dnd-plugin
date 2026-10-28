# Unreal D&D Reference Plugin — Project Specification

Status: Initial specification for review  
Date: 2026-10-28

## 1. Purpose

Provide an Unreal Engine plugin that reads user-supplied reference datasets, represents their content through Unreal-compatible types, and exposes stateless queries to C++ and Blueprint consumers.

The consuming project owns character state, gameplay decisions, choice resolution, and the application of rules. The plugin supplies information needed by those systems.

The developer implements the plugin. Design assistance covers specifications, architecture, documentation, guidance, and reviews; implementation changes require an explicit request.

## 2. Agreed scope

The plugin must:

- Accept a locally supplied dataset using a documented, supported 5etools-compatible JSON format.
- Require no connection to 5etools or any other content service.
- Keep supplied datasets outside the plugin and source repository.
- Represent both 2014 and 2024 rules content without conflating versions.
- Expose structured records and relationships suitable for other developers to use.
- Provide deterministic reference queries and calculations without changing gameplay state.
- Ultimately support every content category in the supported dataset versions, including books and adventures.
- Distribute only plugin code, documentation, and examples or fixtures whose distribution is permitted.

Complete category coverage is a long-term objective, not a claim about the first release. Supported dataset versions and category coverage must be documented explicitly.

## 3. Out of scope

- Character creation, character sheets, or persistent character management.
- Applying level gains, granting abilities, resolving player selections, or modifying XP.
- Combat execution, spell effects, AI, encounters, or an automated rules engine.
- Downloading, scraping, synchronizing, or distributing third-party datasets.
- Recreating the 5etools website or its complete user interface.
- Automatically granting permission to use, export, or redistribute imported material.

An editor inspection interface is in scope as a development aid; a player-facing encyclopedia is not required.

## 4. Content and distribution boundaries

The goal is lawful development and distribution. Separating files is an engineering control, not a determination of rights in a dataset. Access to a file, ownership of a book, and permission to redistribute its contents must not be treated as interchangeable.

### Required boundaries

1. The plugin works with no dataset installed and reports an empty or unavailable catalog clearly.
2. The user explicitly selects an external dataset directory. No dataset is acquired automatically.
3. Dataset paths belong in untracked, per-user configuration, never shared project defaults.
4. Source data, extracted text, images, caches, and generated content assets stay outside the plugin and repository.
5. Default build and packaging operations must not copy external data into a distributable.
6. Logs and diagnostics must avoid reproducing source passages or complete records by default.
7. Committed tests and examples use original synthetic content or content with documented redistribution permission and required attribution.
8. Any future export feature requires an explicit destination and action; it must explain that exported records contain source content.
9. Schema compatibility must not be presented as permission to use the underlying content.

Use independently authored adapter code. Any proposed reuse of upstream code, schemas, assets, or documentation requires checking its applicable license first.

### Development arrangement

```text
C:\Unreal Projects\unreal-dnd-plugin\    Documentation and implementation
C:\LocalDatasets\ReferenceData\         User-supplied source files
C:\LocalDatasets\ReferenceCache\        Optional disposable generated data
```

These are illustrative paths, not defaults to hard-code. No external dataset has been acquired or authorized by this specification. Git exclusions and release checks provide additional safeguards but do not replace external storage.

### Permitted fixtures

Prefer original synthetic records for parser and query tests. If SRD content is used, record the precise source, version, applicable license, and required attribution. Do not assume that all content sharing an SRD source label is licensed correctly without verification.

Official references:

- [System Reference Document and licensing guidance](https://www.dndbeyond.com/srd/)
- [Wizards Creator FAQ](https://www.dndbeyond.com/creator-faq)

These references distinguish SRD material from other published content. This document does not certify legal compliance; distribution decisions involving third-party material may require qualified legal review.

## 5. Content model requirements

### Identity and provenance

Every record must have a stable identity within a documented dataset namespace, a category, source identity, and available provenance. Display names must not serve as unique identifiers.

Retain source-provided identifiers and establish a deterministic identity policy for records that lack them. Identity must distinguish same-name records, different sources, and revisions where necessary.

Track rules edition separately from source publication and dataset format version. Permit unknown or shared applicability; do not force every record into either 2014 or 2024. Represent reprints and replacements as relationships rather than silently overwriting records.

### Structured fields and content trees

Expose typed fields for established concepts and a structured content tree for paragraphs, lists, tables, sections, inline references, and media references. Books and adventures require ordered hierarchical documents, not just flat table rows.

The adapter must account for source references, inherited records, and formatting constructs as they are confirmed during dataset analysis. Unsupported fields or constructs must produce diagnostics and remain recoverable where practical; they must not disappear silently.

Preserving original fragments in memory or an external cache is a proposed fallback, subject to memory and storage design. It does not mean committing those fragments to the repository.

### Long-term category coverage

- Classes, subclasses, class features, and progression information.
- Species/races, sub-options, backgrounds, feats, and other character options.
- Spells, equipment, magic items, and related properties.
- Creatures, vehicles, objects, and other stat blocks.
- Rules, conditions, actions, tables, and reference entries.
- Books, adventures, chapters, and associated media references.

This list is an initial grouping. A versioned inventory of the actual external format must establish the exhaustive category list before claiming full coverage. Community extensions and homebrew support remain a separate scope decision.

## 6. Query contract

Queries are read-only and deterministic for a selected catalog snapshot. They return structured outcomes distinguishing success, invalid input, missing data, unresolved references, ambiguous identity, and unsupported content.

Names below describe intended behavior, not final C++ signatures.

| Query | Required context | Expected result |
| --- | --- | --- |
| Get record | Stable record identity | Typed record or explicit failure |
| Find records | Category and optional source/edition filters | Matching identities and summaries |
| Get class level gains | Class identity, level, optional subclass identity | Fixed grants, choice requirements, progression values, and completeness diagnostics |
| Get level for XP | XP amount and progression-table identity | Level determined by that table or explicit failure |
| Get document section | Document identity and section identity | Ordered structured content and references |

### Class level gains

- A request describes gains at the requested level; cumulative gains are a separate operation.
- Distinguish fixed grants, unresolved choices, values at the level, and changes from the preceding level.
- Return references to relevant features and their source context.
- Do not select a subclass, feat, spell, or other option for the consumer.
- If subclass context is necessary and absent, report incomplete context rather than presenting the result as complete.
- Do not imply that class-level progression alone resolves multiclass character progression.

### XP lookup

- Read thresholds from a selected reference table; do not embed restricted dataset content in code.
- Define exact-boundary behavior: XP equal to a threshold returns that threshold's level.
- For a valid table, XP above the final threshold returns the highest represented level.
- Reject negative XP and malformed tables; missing progression data is not a successful level-one result.
- Do not modify XP or apply a level transition.

## 7. Proposed architecture

```text
External local files
        |
        v
Format adapter -> Validation and reference resolution
        |
        v
Normalized in-memory catalog -> Read-only query API -> C++ / Blueprints
        ^
        |
Editor configuration and diagnostics
```

Proposed responsibilities:

- Core model: identities, provenance, structured content, typed records, and query results.
- Format adapter: file discovery, parsing, format compatibility, and source-specific normalization.
- Catalog: record storage, indices, reference resolution, and a consistent snapshot for queries.
- Query layer: retrieval, filtering, and deterministic reference calculations.
- Editor integration: external-path configuration, loading controls, and diagnostics.

These are logical boundaries, not yet a committed Unreal module layout. The core model must not depend on editor APIs or the external format's field names.

Initial recommendation: use an in-memory catalog. Data Asset generation, persistent caches, hot reload, and packaged-game external loading require later design decisions. Runtime query access is required; runtime parsing and file access are not yet committed.

## 8. Quality requirements

- C++ and Blueprint consumers receive equivalent query meaning and explicit failure information.
- Repeated imports of the same input produce equivalent identities and normalized results.
- Invalid files and unresolved references produce actionable diagnostics without silently presenting incomplete records as complete.
- A failed load must not leave a partially replaced active catalog.
- Queries perform no network access or gameplay-state mutations.
- Large documents and media must not require eagerly loading all media bytes.
- Local input is treated as untrusted: constrain paths, detect reference cycles, and establish parsing resource limits.
- Performance targets will be measured against a permitted representative dataset before numerical budgets are set.

## 9. Delivery milestones

### A. Architecture and format inventory

Document module boundaries, data ownership, identity, edition policy, supported format versions, and category coverage. Analyze only a separately supplied dataset that the developer is authorized to use. Keep the inventory descriptive rather than copying source entries.

### B. First complete vertical slice

Use synthetic class/progression fixtures to demonstrate external loading, normalization, class-level gains, and XP lookup through C++ and Blueprints. Include missing-data and invalid-input behavior. This milestone follows the project's central use cases.

### C. Character reference content

Expand classes and subclasses, species, backgrounds, feats, and related choices. Validate edition separation, same-name records, and cross-references.

### D. Spells, items, and bestiary

Expand typed fields and common structured descriptions. Document category-specific limitations and preserve unsupported constructs.

### E. Documents and remaining categories

Support book/adventure hierarchy, tables, internal links, and external media references. Complete the format inventory and report gaps explicitly.

### F. Distribution readiness

Verify an empty-data installation, build/package exclusions, fixture provenance, attribution, documentation, and the supported Unreal/platform matrix.

## 10. First milestone acceptance criteria

- A fresh installation contains no supplied dataset or generated content and operates without network access.
- A developer can select external synthetic fixtures without saving their absolute path in shared configuration.
- Class-level queries distinguish fixed grants, choices, and progression values.
- XP lookup handles thresholds, negative inputs, missing tables, and the final threshold correctly.
- Same-name records and rules editions remain distinguishable.
- Diagnostics identify invalid records and unresolved references without dumping source content.
- Failed loads leave the previous catalog usable.
- Repository and release review finds only permitted fixtures and no external data or caches.

## 11. Decisions still required

- Public plugin name and C++ naming prefix.
- Minimum Unreal version and supported platforms.
- Editor-only import versus packaged-runtime external loading.
- Exact format versions and the first category support matrix.
- Blueprint representation of flexible content and category-specific records.
- Dataset namespace, duplicate precedence, and multi-dataset loading policy.
- Whether optional generated assets or persistent caches are needed.
- Plugin code license and policy for accepting contributions and fixtures.

Resolve these in the architecture document and record consequential choices in architecture decision records. No implementation is required to approve this specification.
