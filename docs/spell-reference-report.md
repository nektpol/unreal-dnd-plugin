# Spell reference report

The [native reporter](../Source/Dnd5eJson/Public/Readers/Dnd5eJsonSpellReferenceReporter.h) combines [Content reference collection](spell-content-references.md), [exact reference lookup](spell-catalog.md#spell-reference-resolution), and [retained-token normalization](spell-reference-normalization.md) into a read-only report for one spell. It lives in Dnd5eJson and performs no file/network access, catalog replacement, source-default inference, reference rewriting, or gameplay execution.

## API

```cpp
static FDnd5eJsonSpellReferenceReport BuildReport(
    const FDnd5eSpellCatalog& Catalog, const FDnd5eSpell& Spell);
```

[UDnd5eJsonSpellCatalogLibrary](../Source/Dnd5eJson/Public/Readers/Dnd5eJsonSpellCatalogLibrary.h) exposes static BlueprintCallable ReportSpellReferences(CatalogObject, Spell), returning the same report with execution pins. The caller supplies the catalog and spell explicitly. The spell need not itself be installed in that catalog.

## Report models

Both values are exported, Blueprint-visible structs owned by Dnd5eJson. All fields are EditAnywhere/BlueprintReadWrite value storage; the operation itself is read-only.

### FDnd5eJsonSpellReferenceReportEntry

| Field | C++ type and default | Meaning |
| --- | --- | --- |
| Occurrence | `FDnd5eContentReferenceOccurrence`, default struct | Original copied reference and model path |
| OriginalResolutionStatus | `EDnd5eReferenceResolveStatus`, NotAttempted | Exact lookup of the originally declared target |
| OriginalResolutionDiagnostics | `TArray<FDnd5eDiagnostic>`, empty | Diagnostics from original lookup |
| NormalizationStatus | `EDnd5eJsonSpellReferenceNormalizeStatus`, NotAttempted | Independent retained-token normalization outcome |
| NormalizedReference | `FDnd5eReferenceContent`, default struct | Normalizer's output copy, preserving original source/display content |
| NormalizationDiagnostics | `TArray<FDnd5eDiagnostic>`, empty | Diagnostics from normalization/matching |
| NormalizedResolutionStatus | `EDnd5eReferenceResolveStatus`, NotAttempted | Exact lookup attempted only after Normalized |
| NormalizedResolutionDiagnostics | `TArray<FDnd5eDiagnostic>`, empty | Diagnostics from normalized lookup |

### FDnd5eJsonSpellReferenceReport

| Field | C++ type and default | Meaning |
| --- | --- | --- |
| bCollected | `bool`, false | Structural collection completed; does not mean every target resolves |
| Entries | `TArray<FDnd5eJsonSpellReferenceReportEntry>`, empty | One entry for each collected typed occurrence, preserving collection order |
| Diagnostics | `TArray<FDnd5eDiagnostic>`, empty | Collection-level warnings/errors, or invalid-catalog error |

## Processing and stage independence

The collector scans Description followed by HigherLevelDescription, preserving model paths and preorder. It enforces its 64-level, 10,000-struct, and 1,024-occurrence limits. Collection failure returns bCollected=false, no partial entries, and the collector's error. A successful empty spell returns bCollected=true with no entries. Known opaque fallbacks can produce collection warnings even when there are no typed references.

For each occurrence, the reporter resolves the original declared ID, then independently attempts normalization of the retained token. Normalization uses Spell.Header.RecordId.DatasetNamespace as owning-record namespace context. It does not infer a source publication from the owner. Invalid owner namespace is reported at the normalization stage; original exact lookup independently follows the namespace in the reference's declared ID.

Only Normalized triggers the second exact lookup. Otherwise NormalizedResolutionStatus remains NotAttempted and its diagnostics remain empty. NormalizedReference can still contain a pre-existing target on failed normalization because that operation preserves the original copy. Its presence does not imply the second lookup ran.

Both original and normalized routes can resolve to different records. The report retains both outcomes and IDs; it does not choose which is authoritative. Conversely, an unsupported token or omitted source does not erase a successfully resolved original ID. Non-spell references are still collected and receive the existing spell-specific resolver/normalizer outcomes.

Each stage keeps its own diagnostics. The model path belongs to Occurrence.ContentPath and must not be inserted into a diagnostic's JsonPointer as if it were an original source location. Top-level bCollected stays true for per-entry missing/invalid/unsupported/ambiguous targets. Inspect entry statuses and diagnostic arrays rather than interpreting that flag as overall semantic success.

## Ownership, lifetime, and limits

The result is a detached value snapshot with independent original/normalized reference storage. Editing it does not modify the input spell or catalog, and later catalog replacement does not update an old report. There are no retained pointers to catalog records and no recursive traversal into referenced spells or reprint chains. The report stores no full resolved spell copies; use a chosen reference with the exact resolver when a consumer needs the record.

Calls are synchronous. The caller owns synchronization against concurrent input edits/catalog replacement, as for the existing APIs. Reports can copy nested Content through collector and resolver operations; compute them on demand rather than in an automatically reevaluated pure node or every frame. Traversal bounds do not guarantee a fixed byte budget for arbitrarily large value strings/payload copies.

The Blueprint wrapper checks IsValid(CatalogObject) before collection. Invalid objects return bCollected=false, no entries, and one Error with code Dnd5e.Json.InvalidCatalogObject. Native calls receive a required catalog reference. Neither route logs source passages.

## Verification and coverage limits

[Automated tests](testing.md) cover frozen schema/default/copy/serialization contracts, independent stage outcomes including conflicting valid targets, structural failure/limits, catalog changes, original/output ownership, and reflected Blueprint invocation through actual reader-built input. An opt-in supplied-catalog audit exercises all converted spells and reports aggregate typed occurrence and lookup outcomes outside Git.

Reports cover typed references present in the converted Content tree. They do not find references hidden in opaque fallback text or prose, establish complete rule understanding, normalize every record category, provide editor navigation, or authorize bulk catalog rewriting.
