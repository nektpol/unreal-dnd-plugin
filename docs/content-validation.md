# Content validation

`UDnd5eContentValidationLibrary::ValidateSpellContent(const FDnd5eSpell&)` is a static BlueprintCallable Core operation declared in [Dnd5eContentValidationLibrary.h](../Source/Dnd5e/Public/Content/Dnd5eContentValidationLibrary.h). It returns the shared [validation result and issue values](shared-models.md#fdnd5econtentvalidationresult). It reads no JSON/files, resolves no targets, evaluates no expressions, and does not modify the supplied spell.

## Result and locations

bValid is true only after the entire selected Content graph validates. Issues contain copied OwnerRecordId, ContentPath, and Diagnostic. ContentPath is a current-model location such as `/Description/0/Payload/Inlines/1/Payload/Value`. Diagnostic.RelativeFilePath and JsonPointer remain empty; model paths are not original source locations. The first structural/value error stops traversal and returns bValid=false. Opaque leaves with intact evidence are valid but yield an aggregate Warning; validity does not assert their rules are understood.

Description and HigherLevelDescription are always checked. SupplementaryContent.Entries/Images are checked only when bHasSupplementaryContent is true. All roots share a maximum depth of 64 (root wrapper depth zero) and 10,000 visited Content structs. Invalid limits fail rather than truncate. The validator does not validate every non-Content spell field or perform a catalog lookup.

## Checks

| Family | Implemented checks |
| --- | --- |
| Wrappers | Known exact kind and exact nonempty payload type; parent/child tag matching is insufficient |
| Numbers/annotations | Active number kind, finite decimals, nonblank literal/formula evidence, exact role/value types, nonnegative integer DC, d20 sides=20, chance 0–100 and recursive outcomes |
| Scaling | Nonempty base expressions/increment and syntax, supported axis, positive sorted unique inputs; slot levels at most 9, psi inputs not given a universal spell-slot ceiling; base/input arrays individually at most 10,000 |
| References | Active selector category/name/source and class-feature qualifier/level; active target ID completeness; paired retained format/token evidence; no inferred target existence |
| Labels | All Display arrays use ordinary recursive inline checks, preserving nested-label compatibility |
| Lists/tables | Nonnegative list start, positive supplied column count, finite positive supplied weights, spans 1–4096, finite range ordering/nonnegative padding, rows no wider than declared columns; ragged rows accepted |
| Media | Active relative-path or basic HTTP(S)-locator policy, active thumbnail/dimensions; gallery/supplement image arrays contain Image or Unsupported wrappers |
| Destinations | Required document/topic/page identities, nonnegative supplied indices, nonempty ordered query clauses with field/source evidence; DSL values remain unevaluated |
| Fallbacks | Nonblank source format, Text/Json fragment kind, nonempty retained source fragment |

Tables permit at most 4096 columns, 10,000 rows across header/body, and 10,000 cells before recursive checks. Query clauses are bounded at 10,000. Inactive optional fields are ignored. Custom registered selector categories may be represented without a qualifier; class-feature qualifiers have their explicit shared contract. The validator checks stored formula/locator structure, not a complete dice language, network reachability, physical filesystem containment, or all D&D prose semantics.

## Catalog integration

BuildRequest.bValidateContent and LoadRequest.bValidateContent default false. When true, the builder validates candidates after supplementary joins and before atomic replacement. Result.ContentValidationIssues retains dedicated model issues even on a failed build, while ordinary Diagnostics receives a summary Error. Failure preserves the previous catalog; candidate records and unmatched supplementary rows are not exposed as a partial success. A successful result may carry opaque-content warnings.

These options are available through the existing native/Blueprint request operations. Editor settings currently expose primary/availability selection only and do not silently enable validation. See [automated verification](testing.md) for the tested synthetic and supplied-data boundaries.
