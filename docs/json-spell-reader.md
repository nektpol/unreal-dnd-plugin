# JSON spell reader

`FDnd5eJsonSpellReader::ReadSpell` converts one explicitly selected entry from supplied JSON text into [FDnd5eSpell](spell-model.md). It maps the inspected 5etools spell fields and preserves unmodeled Content explicitly. Source acceptance, typed mapping, and complete rule interpretation are separate claims. It does not load files, discover datasets, fetch URLs, resolve a catalog, or execute rules.

The API lives in Dnd5eJson: [reader](../Source/Dnd5eJson/Public/Readers/Dnd5eJsonSpellReader.h), [request](../Source/Dnd5eJson/Public/Readers/Dnd5eJsonSpellReadRequest.h), and [result](../Source/Dnd5eJson/Public/Readers/Dnd5eJsonSpellReadResult.h). A consuming module needs Dnd5eJson as a dependency. Json and GameplayTags are private implementation dependencies; public dependencies are Core, CoreUObject, Dnd5e, and Engine. Engine supplies the public base class of the module's separate Blueprint catalog-build library.

For multiple supplied documents and supplementary availability, use the separate [catalog builder and query API](spell-catalog.md). ReadSpell itself remains a one-record converter and leaves availability unknown. Shared JSON status is declared in [Dnd5eJsonReadStatus.h](../Source/Dnd5eJson/Public/Readers/Dnd5eJsonReadStatus.h).

## Calling the reader

After reading, the separate [retained-reference normalizer](spell-reference-normalization.md) can explicitly reconcile a supported spell token with a loaded catalog's canonical ID. It does not change ReadSpell's identity construction, missing-source policy, or import diagnostics.

```cpp
#include "Readers/Dnd5eJsonSpellReader.h"

FDnd5eJsonSpellReadRequest Request;
Request.JsonText = SuppliedJsonText;
Request.Context.DatasetNamespace = TEXT("my-collection");
Request.Context.RelativeFilePath = TEXT("spells/local.json");
Request.SpellIndex = SelectedIndex;
FDnd5eJsonSpellReadResult Result = FDnd5eJsonSpellReader::ReadSpell(Request);
```

SuppliedJsonText and SelectedIndex come from the caller. The text must have an object root with an exact `spell` key holding an array. SpellIndex selects one object in that array; its default INDEX_NONE selects nothing. A bare spell object, root array, filename, or URL is not inferred as another request format.

DatasetNamespace is required, case-preserving, and independent of the dataset's location. It cannot contain edge whitespace, control characters, or invalid UTF-16. RelativeFilePath may be empty; otherwise it is a portable relative source label using forward slashes, with no absolute roots, backslashes, colons, control/invalid Unicode characters, empty segments, `.` segments, or `..` traversal. The reader never opens this label.

Every call returns Success or Failure. Success may include warnings; Failure includes an Error and a fully default spell, even if earlier fields converted successfully. NotAttempted remains the result value's construction default. Inputs are unchanged. Diagnostics are returned quietly; callers choose their logging and presentation.

## Accepted spell subset

Field names and discriminator strings are case-sensitive. Missing required fields, present nulls, wrong types, unsupported values, and unknown selected-record properties fail explicitly. Optional fields are validated when present.

| Source | Implemented conversion |
| --- | --- |
| Required `name`, `source` | Nonempty identity strings without edge whitespace, control characters, or invalid UTF-16; original display/source spelling retained |
| Optional `page` | Positive int32 numeric token or nonempty text, retaining the page discriminant |
| Required `level` | Integer token from 0 to 9 |
| Required `school` | A/C/D/E/V/I/N/T/P → existing Abjuration/Conjuration/Divination/Enchantment/Evocation/Illusion/Necromancy/Transmutation/Psionic tags |
| Required nonempty `time` array | Ordered action/bonus/reaction/round/minute/hour/special units; positive int32 number except that special may omit it; optional string condition/note |
| Required `range` | Measured point, symbolic point, special, or supported area shape; see geometry policy below |
| Required `components` | Boolean v/s/r, material Boolean/string/detail object, exact source-copper costs, and consumption policy |
| Required nonempty `duration` array | Ordered instant/timed/permanent/special; timed amount/unit/upTo/concentration; permanent ending-condition tags |
| Required nonempty `entries` array | Ordered text/numbers/sections/lists/tables/quotes/insets, with explicit JSON fallback for unmodeled structures |
| Optional `entriesHigherLevel` | Same conversion; an empty array is valid |
| Optional `alias` | Ordered string aliases |
| Optional `meta.ritual` | Boolean spell ritual capability |
| Optional `miscTags`, `areaTags` | String arrays retained as classification evidence, without invented tags/geometry |
| Optional `srd`, `srd52` | Boolean or string retained as metadata |
| Optional `basicRules`, `basicRules2024`, `hasFluff`, `hasFluffImages` | Boolean metadata |
| Optional `otherSources`, `additionalSources` | Arrays of objects with required nonempty string source and optional page, retained as metadata |
| Optional `referenceSources` | Array of nonempty source-ID strings, retained as metadata |
| Optional `reprintedAs` | Ordered nonempty strings retained as directed relationships; supported explicit `name|source` selectors receive target IDs, other nonempty selector spellings remain unresolved with warnings |
| Optional damage/condition/ability/attack/creature indexes | Known strings mapped to their separate role-specific tag containers; unknown strings fail |
| Optional `scalingLevelDice` | One object or ordered object array → typed labeled level/dice groups, with numeric level ordering |

Control fields such as spell level, casting quantity, page numbers, list indices/counts, and column spans require checked integer tokens in their field's range. FDnd5eNumber measurements, costs, and numeric Content retain their exact SourceLiteral: suitable int32 tokens use Integer, finite decimals/exponents use Decimal, and larger integers or unsuitable conversions use Literal. Underflowing nonzero decimals stay Literal instead of being claimed as zero. Negative Content/finite roll integers are supported; measurements/costs reject negative tokens and timed duration must be positive. No number string is accepted as a numeric JSON value.

The reader checks allowed keys inside every normalized rule/value object. `_copy` and unhandled selected-record fields fail with UnsupportedRequiredValue. Unmodeled Content types or extra Content fields preserve the complete node as JSON fallback with a warning, rather than dropping a potential rule field into generic metadata. Known malformed structures still fail. Class availability and supplemental fluff are in separate source files and are not inferred from their absence here.

No areas are inferred from measured point ranges or areaTags. Concentration stays on the corresponding duration Requirements; Spell.Traits also carries its derived summary. Ritual is a spell trait, not an extra casting-time entry. Source publication names do not imply declared edition, license permission, or a default source.

### Materials, geometry, durations, and classifications

Material `m=true` adds a requirement tag without invented details; false adds no requirement. A supplied string or detail object creates material details. Detail text becomes Content, optional cost becomes one Copper FDnd5eMoney entry with its original numeric token, and consumption distinguishes omitted/true/false/`optional`. Explicit zero cost keeps bHasCost=true. Royalty uses its independent tag and is not merged into material cost.

Measured point units are inches/feet/yards/miles. Symbolic point values are self/touch/sight/unlimited/plane/special. Area shapes map to self range/origin, following the source parser: cone/line → Length, cube → EdgeLength, sphere/hemisphere/radius/cylinder → Radius, emanation → Extent. Only supplied dimensions are added. Cylinders may supply a separate height via both typeSecondary and amountSecondary; one without the other fails. Area tags alone never produce geometry.

Permanent duration endings map dispel/trigger/discharge to shared tags. Special/instant do not invent a time. Scaling groups preserve order and original expression strings, sort positive level keys numerically, and do not invent missing levels or execute expressions. The exact storage contract is in [Spell model](spell-model.md#level-based-dice-scaling).

Source indexes map independently: damageInflict/Resist/Immune/Vulnerable, conditionInflict/Immune, savingThrow, abilityCheck, spellAttack M/R, and affectsCreatureType. They remain classifications, not full effect definitions, amounts, save outcomes, or target predicates.

## Identity and provenance

RecordId contains the exact supplied DatasetNamespace, Category::Spell, and this opaque adapter key:

```text
5etools:spell:v1:<escaped-name>|<escaped-source>
```

Each component is encoded as UTF-8: ASCII letters/digits and `-._~` remain literal; all other bytes use uppercase `%HH`. Case, internal spaces, and Unicode code points are preserved before escaping; no Unicode normalization or source default is applied. For example, the invented name Clockwork Spark/source SYNTH produces `5etools:spell:v1:Clockwork%20Spark|SYNTH`.

Paths, array indices, and dataset release names do not alter SourceKey. Changes to source/name case do alter this version's key. The core catalog supports [exact-ID lookup](spell-catalog.md#core-catalog). This is a storage identity, not the source's default lowercase lookup UID; omitted-source reference resolution and ambiguity handling still need dedicated policies.

Header.Provenance has one primary entry with original SourceId, optional Page, supplied RelativeFilePath, and `/spell/<index>` JsonPointer. IdentityFieldsJson retains original name/source. ClassificationEvidenceJson retains school, supplied meta, miscTags, and areaTags. AdditionalMetadataJson retains only the documented metadata fields. Reference-source arrays are not expanded into additional provenance origins in this batch.

Retained JSON preserves value types and numeric token spelling; property order and formatting may change. It is not a second whole-spell snapshot. Aliases remain typed header values. Reprint relationships retain source syntax `5etools` and their complete original selector. Exactly two pipe-separated components with usable explicit name/source normalize to the existing spell ID in the caller's dataset namespace. Component edge whitespace is trimmed for the ID while raw SourceReference stays unchanged. Headers, inline spell references, and reprints share the same private ID constructor; the v1 format and existing IDs are unchanged.

Missing/blank components, extra pipe-separated fields, or unusable identity text leave bHasTargetRecord=false and a default target ID, with one located UnresolvedReference warning. The outer field must still be an array of nonempty strings; object/non-string entries and empty strings retain their existing structural failures. No default source, case-folded identity, source inheritance, redirect, or preferred edition is selected.

A normalized reprint sets bHasTargetRecord=true without requiring a loaded target or emitting the old unconditional reprint warning. Call [ResolveSpellRelationship](spell-catalog.md#spell-reference-resolution) separately to follow the declared ID in a selected catalog. A target absent from a subset returns TargetNotFound; reading does not fetch missing files, merge reprints, or recursively follow chains.

## Content, inline tokens, and fallback behavior

A source string becomes a Paragraph containing ordered inline fragments. Plain text, including whitespace and empty strings, is retained. Supported tokens are bold/italic and their aliases, dice/damage annotations, selected record references, and explicit HTTP(S) external links. Nested supported formatting is recursive. Dice annotations retain the complete token and unevaluated expression; links/references retain their original token. Spell references with an explicit source get a versioned target ID without claiming a loaded target. Callers can separately use the [catalog resolver](spell-catalog.md#spell-reference-resolution) to look up that exact declared ID. Omitted-source or non-spell references stay unresolved with warnings; no source/category is guessed.

Unmodeled inline tokens, including rule annotations without an agreed semantic model, remain Unsupported inline fragments with the complete original token and warnings; their surrounding text stays typed. An unterminated token retains the complete remaining fragment. DisplayText stays empty instead of claiming an interpreted label. This preserves source meaning for later work without asserting typed semantics.

### Current annotation coverage

Annotation support is implemented at three levels: the shared FDnd5eAnnotationContent payload, typed extraction helpers, and source-token conversion. The reader currently converts these annotation kinds:

| Source example | Annotation.Kind | Annotation.Value |
| --- | --- | --- |
| `{@dice 2d6}` | Content::Annotation::DiceRoll | `FDnd5eDiceExpression` with Expression=`2d6` and Syntax=`5etools` |
| `{@damage 2d6}` | Content::Annotation::DamageRoll | `FDnd5eDiceExpression` with Expression=`2d6` and Syntax=`5etools` |

Both produce an Inline::Annotation wrapper, retain the complete original token in SourceAnnotation, set SourceFormat=`5etools`, and retain display content separately. An explicit nonempty second argument supplies Display; otherwise the expression supplies it. Remaining arguments are retained in the original token without invented typed fields. Expressions are stored without validating/evaluating their formula grammar or rolling dice; that is sufficient for this reference-data representation. DamageRoll alone does not specify a damage type, target, or effect outcome.

Other annotation kinds, including attack modifiers, difficulty classes, recharge, chance, and ability/check annotations, currently use the Unsupported inline path with a warning. This is incomplete annotation coverage, not an absence of annotation infrastructure. The [shared annotation contract and extractors](shared-models.md#annotation-content) define the canonical payload. Expanding typed rule representation remains separate from executing gameplay rules.

Sections/entries recursively populate headings and blocks. Lists populate ordered items/labels, marker styles, optional start (including zero), and column counts. Unmodeled layout styles remain metadata with warnings. Directly nested/unmodeled item forms preserve the complete list instead of inventing an extra marker. Tables populate captions, headers, body rows/cells, spans, relative widths/alignment, optional intro/footnotes/outro, and source flags. Unknown column styles are retained with warnings. Quotes/insets preserve their text, attribution/headings, flags, source/page overrides, and ReadAloud classification.

Roll cells keep numeric matching bounds separate from displayed blocks/overrides. The source's roll-bound zero convention normalizes to matching result 100 while retaining display 0/00; ordinary numeric zero cells remain zero. Finite negative bounds stay finite. Verified positive/negative 10^20 sentinels become unbounded flags before narrowing to int32. Recognized sentinel spellings are the full integer token or signed/unsigned `1e20`/`1e+20` exponent forms. Arbitrary oversized bounds fail. Simultaneous exact and min/max, reversed normalized finite bounds, and malformed cells fail.

Unknown object forms and unsupported extra fields on a known Content object retain the entire JSON node with FragmentKind=Json. Numeric token spelling survives serialization; whitespace/property order need not. They produce PreservedUnsupportedContent. Explicitly retained presentation metadata produces PreservedUnsupportedMetadata. Generated wrappers carry the checked/inherited SourceId, supplied file label, and JSON Pointer; pages are stored only where explicitly supplied. Known sections with missing entries or invalid child values still fail.

Success certifies acceptance under this storage subset. Retained prose/markup does not certify semantic interpretation of its rules.

## Limits and diagnostics

The reader accepts at most 16,777,216 TCHAR input code units, 64 simultaneously open JSON containers, and 1,000,000 input value tokens. Scalars and container starts each count as one value. Conversion also limits recursive Content/inline depth to 64, converted entries/inline fragments to 100,000 per call, and table expansion to 4,096 columns. Child readers share the conversion budget. Limits cover unselected input/root metadata where relevant. Excess input/output fails; it is never silently truncated.

A token preflight rejects repeated property names and case-colliding names within each object before Unreal's case-insensitive object map can discard data. Different objects may use the same names. The subsequent DOM parse preserves numeric tokens. Unreal 5.8 requires an object/array root; scalar roots follow its InvalidSyntax path, while parsed array roots produce InvalidRoot.

Other root properties/unselected records are parsed and preflighted but not converted into the chosen spell. Unknown keys on the chosen spell and its converted objects are rejected in sorted case-sensitive order. Later processing follows fixed field order and source-array order. The first Error stops dependent work; earlier warnings remain.

Codes are fixed FNames with prefix `Dnd5e.Json.`:

| Suffix | Severity | Meaning |
| --- | --- | --- |
| InvalidContext | Error | Invalid namespace or source label |
| NoSpellSelection | Error | Negative selection index |
| EmptyInput | Error | Empty/whitespace-only document |
| ResourceLimitExceeded | Error | Input/depth/value limit exceeded |
| InvalidSyntax | Error | Token/DOM parse failure |
| DuplicateProperty | Error | Repeated or case-colliding object keys |
| InvalidRoot | Error | Parsed root is not an object |
| MissingSpellArray / InvalidSpellArray | Error | Missing exact spell key or non-array value |
| SpellIndexOutOfBounds / InvalidSpellObject | Error | Selected index/entry is invalid |
| MissingRequiredField / InvalidFieldValue | Error | Required field absent, or supplied type/value invalid |
| UnsupportedRequiredValue | Error | Source property/value/structure is outside the supported subset |
| SerializationFailure | Error | Could not serialize retained metadata |
| PreservedUnsupportedContent | Warning | Complete unmodeled inline token or JSON Content node retained as Unsupported |
| PreservedUnsupportedMetadata | Warning | Presentation metadata retained without complete typed layout support |
| UnresolvedReference | Warning | Omitted-source/unsupported-category inline reference, or unsupported nonempty reprint selector, retained without a normalized target |

Every diagnostic includes severity, code, message, supplied RelativeFilePath, and the most specific known JsonPointer. Request/whole-document/preflight errors may use an empty pointer. Pointer property segments escape `~` as `~0` and `/` as `~1`. The reader does not turn arbitrary property names into FNames or register new tags from input.

## Verification and remaining work

See [Automated tests](testing.md) for synthetic reader behavior, boundary/rejection coverage, and the optional caller-selected external-file smoke test. All retained source records/media and real-data reports stay outside the plugin repository.

This API remains a one-record text converter. The separate [catalog builder](spell-catalog.md) converts multiple supplied documents and joins the optional availability lookup, and the core resolver can follow a supplied normalized spell-reference ID. The [editor tools](editor-dataset-loading.md) select/load files and browse records. More typed inline/rule semantics, edition policy, omitted-source/ambiguous selector normalization, other-domain/general reference resolution, and complete Content/media coverage remain later capabilities. Acceptance of every record in an inspected snapshot under explicit fallback policy does not establish full rule interpretation or compatibility with every future/custom source form.
