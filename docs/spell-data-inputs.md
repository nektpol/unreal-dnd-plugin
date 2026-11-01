# Supplementary spell input and source metadata

Primary spells, supplementary prose/images, source membership, explicit edition assignments, and availability associations have separate roles. All input remains caller-supplied. These APIs do not discover/download data, open media resources, choose preferred editions, or execute gameplay rules.

## Supplementary values and supplied-text reader

FDnd5eSpell has bHasSupplementaryContent=false and FDnd5eSupplementaryContent SupplementaryContent. Its [shared value](shared-models.md#fdnd5esupplementarycontent) stores Entries (`TArray<FDnd5eContentBlock>`) and Images (`TArray<FDnd5eContentBlock>`). Images contains typed Image wrappers or explicit Unsupported preserved image forms. Supplementary prose is not appended to mechanical Description. A supplied known-empty matching row sets the presence flag; not supplied leaves it false.

[FDnd5eJsonSpellSupplementReader](../Source/Dnd5eJson/Public/Readers/Spells/Dnd5eJsonSpellSupplementReader.h) exposes:

```cpp
static bool ReadDocument(const FDnd5eJsonDocument& Document,
    TArray<FDnd5eJsonSpellSupplement>& Rows, int32& ConvertedValues,
    TArray<FDnd5eDiagnostic>& Diagnostics);
```

It parses an object containing spellFluff, whose rows require name/source and optionally entries/images. Selected-row unknown keys/types fail. Each returned row retains DatasetNamespace/Name/SourceId, shared Content, and its independent provenance. Rows, ConvertedValues, and diagnostics are reset per call; rows publish only after the whole document succeeds. Failure leaves rows empty. Each row uses the established 100,000 Content-value/depth conversion limits; at most 100,000 rows and 1,000,000 converted values per supplied supplementary document are accepted.

FDnd5eJsonSpellSupplement is exported and Blueprint-visible:

| Field | C++ type and default | Meaning |
| --- | --- | --- |
| DatasetNamespace | `FString`, empty | Exact logical input scope |
| Name | `FString`, empty | Supplement's requested primary name |
| SourceId | `FString`, empty | Explicit primary publication |
| Content | `FDnd5eSupplementaryContent`, default | Separate prose/image blocks |
| Provenance | `FDnd5eRecordProvenance`, default | Supplementary row origin, independent of primary origin |

## Building and loading

The existing [builder](spell-catalog.md#catalog-builder) and [file loader](json-spell-file-loader.md) add these fields:

| Owner/field | C++ type and default | Meaning |
| --- | --- | --- |
| BuildRequest.SpellFluffDocuments | `TArray<FDnd5eJsonDocument>`, empty | Explicit supplied supplementary text/context |
| LoadRequest.SpellFluffFiles | `TArray<FString>`, empty | Explicit portable filenames under the selected dataset root |
| BuildResult.UnmatchedSupplements | `TArray<FDnd5eJsonSpellSupplement>`, empty | Successful-build supplied rows with no exact selected primary match |
| BuildRequest/LoadRequest.EditionPolicyId | `FString`, empty | Caller-supplied versioned classification policy name |
| BuildRequest/LoadRequest.EditionAssignments | `TArray<FDnd5eSourceEditionAssignment>`, empty | Exact namespace/publication-to-edition assignments |
| BuildRequest/LoadRequest.bValidateContent | `bool`, false | Opt-in candidate Content validation before replacement |
| BuildResult.ContentValidationIssues | `TArray<FDnd5eContentValidationIssue>`, empty, BlueprintReadOnly | Model issues, separate from source JSON diagnostics |

Primary and supplementary rows join on exact namespace/name/source using the existing escaped spell identity policy. Duplicate primary/supplementary identities fail. Unmatched supplementary rows remain returned as data with Info diagnostics and attach nowhere. Matched rows append contributing provenance after primary provenance. The previous catalog survives any later parse/join/validation failure.

One budget covers at most 1024 primary, supplementary, and active lookup documents/files combined; builder input is at most 67,108,864 TCHAR units and conversion at most 1,000,000 Content/availability values. The file loader additionally retains 16 MiB per-file and 64 MiB aggregate byte limits, checks all selected paths before reads, rejects duplicates across all three families, and forwards the policy/validation options. An empty supplementary selection is inactive. These options currently have native/Blueprint APIs; editor preferences have no supplementary/policy picker yet.

## Images

The shared Content reader maps image href.type=internal/path into DatasetRelative media, or a supported external HTTP(S) href into ExternalUrl media. Positive supplied width/height have independent presence flags. Title/credit remain recursive inline Content. Unsupported image properties/href kinds preserve the complete node; unsafe known relative paths or invalid supplied dimensions fail.

`Dnd5eMedia::IsDatasetRelativePath` validates portable relative path syntax and UTF-16, excluding roots/drive prefixes, backslashes, traversal/empty segments, controls and reserved/wildcard characters. It performs no I/O and does not establish physical containment through filesystem links. The caller chooses the media base and any later resource loading. Unknown thumbnail/gallery source forms remain explicit fallbacks until separately mapped.

## Membership and availability hints

Primary provenance has Memberships (`TArray<FDnd5eSourceMembership>`). Source fields srd/srd52/basicRules/basicRules2024 map to distinct source-field identifiers. Boolean false is Excluded, true is Included, and supported nonblank SRD string names are IncludedWithAlternateName. Absent fields create no entry. Raw metadata is retained alongside typed values. These are source inclusion facts, not automatic licensing permission or edition inference.

SupplementaryTextHint and SupplementaryImagesHint are EDnd5eDataAvailability values on the spell: Unknown by default, Absent/Present from explicit hasFluff/hasFluffImages false/true. A Present hint does not mean supplementary input was supplied, attached, or its media exists.

## Explicit edition classification

Edition assignments require a nonblank policy ID and at most 1024 distinct exact namespace/source pairs. Each assignment has nonempty tags drawn from Rules.Edition.Original2014/Revised2024. Matching primary headers receive those tags and retain policy ID/tag evidence alongside original classification evidence. Empty assignments are inactive, including an otherwise unused policy ID. A caller may explicitly assign both tags; no publication abbreviation is hardcoded as an edition rule.

Classification does not merge records, follow reprints, infer a newest version, alter exact identity, or supply omitted reference publications. Secondary source roles, miscTags, and areaTags remain validated provenance evidence rather than inferred effects or geometry. [Content reference collection](spell-content-references.md) keeps its explicit mechanical-description roots; supplementary traversal is not silently folded into that API.
