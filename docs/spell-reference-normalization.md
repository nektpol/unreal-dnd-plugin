# Retained spell-reference normalization

The JSON adapter can explicitly reconcile one retained `5etools` spell token with a loaded catalog. It returns a copied reference with the actual stored target ID on success. It performs no file/network access, catalog replacement, automatic import repair, or gameplay execution.

The native [FDnd5eJsonSpellReferenceNormalizer](../Source/Dnd5eJson/Public/Readers/Dnd5eJsonSpellReferenceNormalizer.h) lives in Dnd5eJson. The [UDnd5eJsonSpellCatalogLibrary](../Source/Dnd5eJson/Public/Readers/Dnd5eJsonSpellCatalogLibrary.h) exposes NormalizeSpellReference to Blueprint. Parsing source syntax belongs to this adapter; the core [selector matcher](spell-catalog.md#spell-selector-matching) remains format-independent.

## Native API

```cpp
static EDnd5eJsonSpellReferenceNormalizeStatus NormalizeReference(
    const FDnd5eSpellCatalog& Catalog, const FString& DatasetNamespace,
    const FDnd5eReferenceContent& Reference, FDnd5eReferenceContent& NormalizedReference,
    TArray<FDnd5eDiagnostic>& Diagnostics);
```

NormalizeSpellReference on the Blueprint library has the same namespace/reference/output arguments and return enum, with a UDnd5eSpellCatalogObject pointer instead of a native catalog reference. It is a static BlueprintCallable operation with execution pins. Null/invalid catalog objects return InvalidCatalog before parsing.

## Accepted input and matching

SourceFormat must equal `5etools` exactly. SourceReference must start with lowercase `{@spell ` and end with `}`, with no surrounding text. The token may contain at most 65,536 TCHARs, inclusive. The body accepts one to three pipe-separated components: name, optional publication, optional display. Empty fields are retained during splitting. Extended forms with extra pipe characters are unsupported, including nested display markup containing such characters. Name/source components containing braces are unsupported. Display is opaque and the operation preserves the existing Display array rather than rebuilding it from the token.

Name and source are trimmed at the adapter boundary; the namespace is passed unchanged. Matching then follows the catalog's exact namespace, whole case-insensitive canonical name/primary-source comparisons, and explicit missing-source/ambiguity policy. Aliases, contributing publications, preferred editions, and default sources are excluded. The ID returned by matching is copied unchanged; no ID is synthesized from normalized casing.

For example, `{@spell invented beacon|syn-b}` can match a loaded Invented Beacon published as SYN-B while returning its original opaque record key. An omitted source remains MissingSource even if only one record has that name.

## Return values and outputs

| Status | Meaning | Diagnostic |
| --- | --- | --- |
| NotAttempted | Default enum value; never returned by the operation | — |
| Normalized | Exactly one explicit-source candidate; copied output receives its target flag and canonical ID | None |
| UnsupportedReference | Unsupported source format/envelope/field count/braces or token exceeds the size limit | Warning, Dnd5e.Json.SpellReference.Unsupported |
| MissingSource | Source omitted/empty after trimming; zero, one, or many name candidates | Matcher Warning, Dnd5e.Catalog.Selector.MissingSource |
| InvalidSelector | Unusable namespace/name/source text | Matcher Error, Dnd5e.Catalog.Selector.Invalid |
| NoMatch | No loaded candidate for explicit namespace/name/source | Matcher Warning, Dnd5e.Catalog.Selector.NoMatch |
| Ambiguous | Several distinct loaded IDs match the explicit selector | Matcher Warning, Dnd5e.Catalog.Selector.Ambiguous |
| InvalidCatalog | Blueprint wrapper received an invalid catalog object | Error, Dnd5e.Json.InvalidCatalogObject |

An unexpected future matcher status is guarded by an Error with code Dnd5e.Json.SpellReference.UnexpectedMatchStatus and returns InvalidSelector. Current matcher paths do not produce it.

Diagnostics is `TArray<FDnd5eDiagnostic>` and is reset on every call. Failures return one diagnostic; success clears previous diagnostics. RelativeFilePath/JsonPointer remain empty because this operation receives no source-location context. Diagnostics use controlled messages rather than quoting source passages.

On every outcome, NormalizedReference begins as an independent value copy of the input. Only Normalized changes bHasTargetRecordId and TargetRecordId. Display, SourceFormat, and SourceReference remain intact. Failures preserve any previously declared target, including an inactive target value. Consumers must check the status before treating the output as newly reconciled. Success may deliberately replace an existing declared ID using the retained token as authority. Native input/output aliasing is supported.

## Consumption and limits

For a combined per-spell diagnostic view, use the separate [reference report API](spell-reference-report.md). It keeps original and normalized lookup outcomes separate rather than automatically replacing the original reference.

The core [spell Content collector](spell-content-references.md) can first obtain typed references and model locations from both description trees. Collection and normalization remain separate explicit operations.

After Normalized, pass the output to [ResolveSpellReference](spell-catalog.md#spell-reference-resolution) on the same catalog to retrieve the actual spell. Normalization does not return or mutate the spell itself. Catalog replacement can change later normalization results; synchronize shared catalog access as for other catalog operations.

This operation handles one reference, not a recursive spell traversal. It does not normalize relationship/reprint strings, infer omitted publications from an owning spell, choose an ambiguous candidate, follow reference chains, or modify reader/build diagnostics. Existing exact-ID/query behavior and automatic reader identity construction remain unchanged.

Verification is recorded in [Automated tests](testing.md). Test fixtures are synthetic; external audits remain opt-in and their supplied data is kept outside the plugin repository.
