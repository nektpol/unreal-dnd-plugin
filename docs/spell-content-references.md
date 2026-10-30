# Collecting spell Content references

[UDnd5eSpellContentLibrary](../Source/Dnd5e/Public/Spells/Dnd5eSpellContentLibrary.h) exposes one stateless core operation to C++ and Blueprint:

```cpp
static bool CollectSpellReferences(const FDnd5eSpell& Spell,
    TArray<FDnd5eContentReferenceOccurrence>& Occurrences,
    TArray<FDnd5eDiagnostic>& Diagnostics);
```

The function is BlueprintCallable with execution pins. It scans Description followed by HigherLevelDescription and returns independent reference copies and model locations. Its [shared occurrence model](shared-models.md#content-reference-occurrences) contains the copied reference and its ContentPath. It performs no I/O, token parsing, catalog lookup, normalization, mutation, or gameplay execution. Relationships/reprint links outside Content are not occurrences returned by this operation.

## Locations and order

An example ContentPath is `/Description/0/Payload/Inlines/1/Payload`: the reference payload in the second inline of the first description paragraph. It uses reflected field names, array indices, and explicit Payload steps through instanced wrappers. This is an Unreal model location, not a source JSON pointer or persistent identity. Inserting/reordering content or changing the schema can change it.

Collection is depth-first preorder: a reference is recorded before references in its Display children. Description precedes HigherLevelDescription; child fields follow reflected declaration order and elements follow array order. Equal tokens/targets are retained as separate occurrences. A reference is collected regardless of source format, omitted target flag, or target validity. It need not describe a spell target.

## Traversal vocabulary

Every block/inline wrapper must have an exact supported Kind and exact valid payload type. The approved payloads are Section, Paragraph, List, Table, Quote, Inset, Image, Gallery, Text, Formatted, Reference, Annotation, Number, ExternalLink, AnchorLink, and the appropriate Unsupported fallback. ListItem, TableColumn, TableRow, and TableCell are approved structural array elements.

The visitor opens wrapper Payload explicitly, then visits arrays of approved Content structs. This includes section/list/inset/gallery headings; paragraph/formatted inlines; list item labels/blocks; table caption, header/body rows and cell blocks, intro/footnotes/outro; quote blocks/attribution/source; image title/credit; gallery images; and reference/annotation/number/link Display arrays. Table columns are visited but contain no Content children in the current schema.

It excludes metadata, gameplay tag internals, arbitrary struct/annotation Value payloads, raw source fragments, and non-Content spell fields. Known Unsupported payloads are opaque leaves. New payload kinds or child-container shapes require an explicit visitor/coverage update.

The collector checks wrapper structure, not all Content semantics. For example, it mechanically traverses nested reference Display arrays even where the model's semantic label rules restrict that composition. Successful collection does not validate reference targets, label composition, table widths/spans, rule values, media locations, or complete rule understanding.

## Limits and failure behavior

| Budget | Inclusive maximum |
| --- | --- |
| Struct depth | 64, with each root block at depth 0; opening a Payload or visiting a structural array element adds a level |
| Visited Content structs | 10,000, shared across both description roots, including wrappers, payloads, and structural elements |
| Reference occurrences | 1,024 |

Both output arrays reset at the start. A successful empty spell returns true and empty outputs. Unknown/invalid wrapper kinds, empty payloads, or wrong payload types return false, no occurrences, and one Error with code Dnd5e.Content.References.InvalidContent. Exceeded budgets return false, no occurrences, and one Error with code Dnd5e.Content.References.Limit. Earlier discoveries are not returned as partial results.

If an otherwise successful scan encountered known Unsupported payloads, it returns one aggregate Warning with code Dnd5e.Content.References.OpaqueFallback and the count of opaque leaves. Thus true with zero occurrences does not establish that fallback text contains no references. Source-location fields remain empty; model paths must not be presented as original JSON locations.

Traversal keeps internal pointers temporarily and copies reference values only after successful completion. Those pointers never escape the synchronous call. The caller owns input lifetime/synchronization; do not mutate the input while traversing it. Output copies remain independent after the input is changed or destroyed. Limits bound traversal and occurrence counts, not arbitrary source-string byte sizes or every possible aggregate copy cost.

## Using the results

After true, inspect each occurrence's Reference.SourceReference and ContentPath. The separate [retained-reference normalizer](spell-reference-normalization.md) can reconcile a supported spell token against a loaded catalog under its explicit namespace/source policy. The [exact reference resolver](spell-catalog.md#spell-reference-resolution) can retrieve a declared loaded target. Neither operation is invoked automatically by collection.

Tests and the opt-in external audit are described in [Automated tests](testing.md). The external collection audit counts typed references present in converted models; it does not establish complete parsing of references hidden in preserved text.
