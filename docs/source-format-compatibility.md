# Source-format compatibility

Status: source-structure audit, 2026-10-28. Reference snapshot: 5etools 2.39.0. This document distinguishes implemented storage types from importer support.

## Scope and conclusion

The audit parsed all 519 JSON files under the supplied snapshot's data directory, including generated files, and inspected the relevant renderer, tag handling, and column stylesheet. All files parsed successfully. Structural occurrence counts are not unique content records or compatibility percentages. Homebrew repositories and the separate 2014 application distribution were not independently audited.

The implemented structs are broadly suitable as a normalized Unreal content model. They are not direct JSON equivalents: the source uses short discriminators, strings containing inline markup, CSS-class strings, and several alternate object shapes. A format adapter must translate those representations into the typed tree.

No importer is implemented or exercised by this audit. Complete or lossless source-format coverage is not established. No runtime tests or compilation were performed for this audit. Only structure, numeric characteristics, and code behavior were inspected; source passages were not copied into the documentation.

## Mapping of implemented types

| Unreal representation | Source representation | Assessment |
| --- | --- | --- |
| FDnd5eContentBlock / FDnd5eInlineContent | Source type strings, primitive values, and inline markup | Intentional normalized wrappers; not direct deserialization targets |
| FDnd5eTextContent.Text (`FString`) | Literal text strings | Suitable for text runs; recognized markup must be split into semantic inline nodes |
| FDnd5eFormattedContent | Nested inline formatting, including b/bold and i/italic | Container and ordered children fit; native vocabulary currently covers only bold and italic |
| FDnd5eParagraphContent.Inlines (`TArray<FDnd5eInlineContent>`) | Plain string entries and text-bearing content | Suitable normalization of ordered inline content |
| FDnd5eSectionContent | entries/section objects with name and entries | Body structure and structured Heading fit; C++ call sites are migrated and compiled; importer tokenization remains required |
| FDnd5eListItemContent | String items or item/itemSub/itemSpell objects with name and entry/entries | Label/body split fits; subtype styling, punctuation, direct nested lists, and anchors need explicit handling |
| FDnd5eListContent | name, items, style, start, columns | Suitable normalized structure; styles map to marker enum and presentation tags, not directly into the container |
| FDnd5eTableRollRange | cell.roll.exact or min/max, optional pad; additional bounds/display values produced by renderer normalization | Integer storage fits observed explicit numeric fields; normalize source conventions before validating bounds |
| FDnd5eTableCellContent | String/integer cells, cell objects, and cellHeader objects | Blocks plus ColumnSpan and optional roll metadata fit display structure; Number inline storage is implemented for primitive numeric meaning, while adapter conversion remains pending |
| FDnd5eTableRowContent | Row arrays or row objects containing row and style | Suitable normalization into ordered typed cells |
| FDnd5eTableColumnContent | colStyles and recognized structured header annotations | Derived model; source widths are style strings, not float or integer WidthWeight properties |
| FDnd5eTableContent | caption, colLabels/colLabelRows, rows, colStyles, footnotes, isStriped, isNameGenerator | Main shape fits; standalone table identity/source metadata and unsupported properties still require preservation |
| FDnd5eQuoteContent | entries, by, from, skipMarks, skipItalics, style | Field types and inverse boolean defaults fit the inspected source |
| FDnd5eInsetContent | inset/insetReadaloud with name, entries, style, and optional metadata | Content shape fits; ReadAloud classification is now declared/defined, while adapter population and metadata preservation are incomplete |

All content arrays in these mappings contain the element types declared in [Shared models](shared-models.md). Presentation containers hold normalized classifications. They must not be treated as arbitrary source CSS or as substitutes for missing source metadata.

## Source vocabulary inventory

Status: structural follow-up inventory of the same local 2.39.0 snapshot. All 519 JSON files were scanned again, along with the renderer dispatch and literal tag declarations. The public headers currently expose 29 exported structs and 18 extraction helpers; these counts describe code shapes, not compatibility or semantic-coverage percentages.

The inventory separates three questions: whether storage exists, whether a source mapping is implemented, and whether the rule meaning is represented. Source importing/tokenization, reference resolution, and recursive semantic validation remain unimplemented. Generic fallback storage exists, but automatic capture and diagnostics do not. No row below means a source form is already imported successfully.

Counts include generated files, examples, and non-document data such as Foundry schemas. A dictionary type value can collide with a renderer entry discriminator; the attack count is a concrete example. Lexical inline-tag counts also include examples and nested strings rather than validated markup trees. These are structural evidence for planning, not unique rule records, an exhaustive prose-rule audit, or runtime test results. The separate 2014 application distribution, other releases, and homebrew extensions were not exhaustively audited.

The source inventory determines the compatibility vocabulary. A separate rule-data design must account for all rule meanings in scope, including meanings expressed only in prose. Multiple source forms may share one normalized representation when their semantics and provenance are preserved; one tag does not imply one new struct.

<!-- BEGIN GENERATED COVERAGE TABLES -->

### Renderer object forms

All 39 explicit dispatch cases plus the two special handling forms are listed. Counts are candidate dictionary discriminator occurrences across the data tree, not context-validated entry counts. Existing means a compiled storage shape is available for a documented candidate mapping, not full field coverage. Partial means a reusable portion exists. Normalize? means an explicit loss-preserving normalization decision is still required. Missing means a dedicated semantic or structural contract is absent. **Source parsing/import is unimplemented for every row.**

| Source form | Candidate occurrences | Storage assessment | Remaining contract |
| --- | ---: | --- | --- |
| `entries` | 63,745 | Existing | Section; identity/metadata and default-depth mappings remain adapter work. |
| `options` | 41 | Missing | Choice/selection contract; count occurs on 28 of 41 candidates and must not be erased by treating this as prose only. |
| `list` | 8,742 | Existing | List/item values; nested-list markers, subtype presentation, and punctuation policies remain pending. |
| `table` | 5,147 | Existing | Table/row/cell values; source conversion, spanning-grid validation, and styles remain pending. |
| `tableGroup` | 43 | Missing | Grouped-table ownership, heading/context, and identity need a contract. |
| `inset` | 2,812 | Existing | Inset; source metadata and presentation mapping remain adapter work. |
| `insetReadaloud` | 7,797 | Existing | Inset plus ReadAloud tag; mapping is not implemented. |
| `variant` | 126 | Missing | Variant/revision semantics and composition need an explicit contract. |
| `variantInner` | 6 | Missing | Variant subtype semantics and presentation need an explicit contract. |
| `variantSub` | 4 | Missing | Variant subtype semantics and presentation need an explicit contract. |
| `spellcasting` | 1,331 | Partial | Document blocks can retain prose; spellcasting definitions/relationships need domain models. |
| `quote` | 1,085 | Existing | Quote with attribution/source; source conversion remains pending. |
| `optfeature` | 0 | Missing | Renderer-supported legacy/domain form; zero observed does not establish irrelevance. |
| `patron` | 0 | Missing | Renderer-supported legacy/domain form; domain and nesting decisions remain open. |
| `abilityDc` | 20 | Partial | Expression storage exists; contextual formula/property-reference semantics are missing. |
| `abilityAttackMod` | 18 | Partial | Expression storage exists; contextual formula/property-reference semantics are missing. |
| `abilityGeneric` | 11 | Partial | Text/value storage exists; formula/attribute semantics need a contract. |
| `inline` | 2 | Normalize? | Candidate transparent ordered grouping; wrapper metadata must survive any flattening. |
| `inlineBlock` | 4 | Normalize? | Ordered grouping needs explicit block/inline boundary and metadata rules. |
| `bonus` | 246 | Partial | Number value exists; signed-modifier role and source display semantics are not modeled. |
| `bonusSpeed` | 41 | Partial | Numeric storage exists; measurement/modifier-role semantics need a contract. |
| `dice` | 134 | Partial | Shared expression value exists; legacy shapes, context, prompts, and thresholds need contracts. |
| `link` | 4 | Partial | External/anchor/record values exist; internal routes and alternate href shapes remain incomplete. |
| `actions` | 7 | Partial | Section storage can retain text; action grouping and semantic relationships remain open. |
| `attack` | 118 | Partial | Attack data/roles need domain contracts. Candidate count mixes object contexts; only 7 candidates have attackEntries/hitEntries. |
| `ingredient` | 3,051 | Missing | Ingredient quantities, units, references, and domain context need a contract. |
| `item` | 17,737 | Existing | List-item Label/Blocks/Metadata; adapter handling is pending. |
| `itemSub` | 133 | Partial | List-item storage exists; subtype label styling and punctuation need rules. |
| `itemSpell` | 2 | Partial | List-item storage exists; spell-specific label/reference semantics need rules. |
| `statblockInline` | 7 | Missing | Embedded typed domain record, dependencies, and ownership need an envelope contract. |
| `statblock` | 10,460 | Missing | Block-level record reference/embed contract and category identity mappings are missing. |
| `image` | 17,183 | Existing | Image/media-reference storage; typed map semantics and resource resolution are missing. |
| `gallery` | 1,180 | Existing | Ordered image wrappers; normalization and child validation remain pending. |
| `flowchart` | 17 | Missing | Flow grouping/relationships and preservation rules need a contract. |
| `flowBlock` | 115 | Missing | Flow-node structure/presentation needs a contract. |
| `homebrew` | 5 | Missing | Revision/change presentation and semantics need a contract. |
| `code` | 0 | Missing | Verbatim block representation; no source execution. |
| `hr` | 14 | Missing | Separator representation and presentation need a contract. |
| `wrappedHtml` | 0 | Missing | Opaque source/presentation boundary; raw retention is not normalized semantics or HTML execution. |
| `section` | 9,475 | Existing | Special alias handled outside the dispatch cases; Section storage exists. |
| `wrapper` | 68 | Normalize? | Special wrapped/wrappeds handling outside dispatch; transparent grouping/metadata policy is pending. |

Plain strings, numeric primitives, source objects with omitted type, and alternate field shapes also require contextual handling. Text/Paragraph and Number storage exist, but their parsing, numeric-token preservation, and grouping mappings are unimplemented. Supporting cell/cellHeader/row objects are covered by the existing table contract and remain subject to adapter/validation work.

### Inline tag families

All 121 literal tag names declared in the inspected renderer are assigned to exactly one family below. Counts are lexical tag openings across JSON strings, including nested markup and examples; they do not validate arguments or prove importer support. The four declared names unobserved in this snapshot are cite, help, initiative, and itemEntry. Zero occurrences do not remove them from the source vocabulary. **No tag parser or source mapping is implemented.**

| Family | Source names (each used with @) | Declared names | Observed openings | Storage and semantic gaps |
| --- | --- | ---: | ---: | --- |
| Text formatting | `@b`, `@bold`, `@i`, `@italic`, `@s`, `@strike`, `@s2`, `@strikeDouble`, `@u`, `@underline`, `@u2`, `@underlineDouble`, `@sup`, `@sub`, `@kbd`, `@code` | 16 | 14,714 | Formatted children exist; only Bold/Italic native styles exist. Remaining style vocabulary and verbatim semantics need design. |
| Font/style/comic | `@style`, `@font`, `@color`, `@highlight`, `@comic`, `@comicH1`, `@comicH2`, `@comicH3`, `@comicH4`, `@comicNote` | 10 | 938 | Arbitrary source style/font parameters cannot become native tags automatically; explicit presentation/value mapping is missing. |
| Dice and scaling | `@dice`, `@damage`, `@autodice`, `@scaledice`, `@scaledamage` | 5 | 26,871 | DiceRoll/DamageRoll can hold expression text; auto/scaling/argument semantics and parsed expression structure remain missing. |
| Values and contextual checks | `@dc`, `@dcYourSpellSave`, `@hit`, `@d20`, `@hitYourSpellAttack`, `@ability`, `@savingThrow`, `@skillCheck`, `@initiative` | 9 | 22,417 | Literal/contextual values, property references, modifiers, and check roles need shared rule-value contracts. |
| Chance/recharge | `@chance`, `@recharge`, `@coinflip` | 3 | 1,386 | Threshold, interval, probability, and context semantics need typed definitions; execution remains consumer-owned. |
| Attack/action/outcome labels | `@atk`, `@atkr`, `@h`, `@m`, `@hom`, `@actSave`, `@actSaveSuccess`, `@actSaveFail`, `@actSaveFailBy`, `@actSaveSuccessOrFail`, `@actTrigger`, `@actResponse` | 12 | 12,907 | Labels do not by themselves define triggers/outcomes/effects. Context and semantic-role contracts are missing. |
| Player-option references | `@background`, `@backgroundFluff`, `@charoption`, `@class`, `@classFeature`, `@classFluff`, `@feat`, `@optfeature`, `@race`, `@raceFluff`, `@subclass`, `@subclassFeature`, `@subclassFluff` | 13 | 4,398 | Generic reference storage exists; category-specific identity/default/reference contracts are missing. |
| Rule/classification references | `@action`, `@condition`, `@disease`, `@itemMastery`, `@itemProperty`, `@language`, `@sense`, `@skill`, `@status`, `@variantrule` | 10 | 35,142 | Generic reference storage exists; target categories, identity mappings, and referenced rule definitions remain missing. |
| Entity references | `@creature`, `@creatureFluff`, `@item`, `@itemEntry`, `@itemFluff`, `@object`, `@spell`, `@spellFluff`, `@table`, `@trap`, `@hazard`, `@vehicle`, `@vehupgrade` | 13 | 105,573 | Generic reference storage exists. Only Spell has a native category tag; all category-specific UID mapping and resolution remain unimplemented. |
| Supporting-domain references | `@boon`, `@card`, `@crochet`, `@crochetFluff`, `@cult`, `@deck`, `@deity`, `@facility`, `@legroup`, `@psionic`, `@recipe`, `@recipeFluff`, `@reward` | 13 | 3,034 | Reference envelope exists; category/domain models and identity mappings remain missing. |
| Document locations | `@book`, `@adventure`, `@area`, `@quickref` | 4 | 32,339 | Current-document AnchorLink exists; document ownership, cross-document/chapter/section selectors and resolution are missing. |
| URI/routes/filters | `@link`, `@filter`, `@5etools` | 3 | 1,972 | ExternalLink exists; website routes, filter queries, and scheme/default policies need separate contracts. |
| Inline media | `@5etoolsImg`, `@5etoolsAudio` | 2 | 125 | Block images/gallery and shared media locations exist; inline image/audio content contracts are missing. |
| Source loading links | `@loader` | 1 | 2 | Source-locator/intent contract missing. No automatic external service access is authorized or implemented. |
| Units | `@unit` | 1 | 96 | Typed quantity/unit normalization requires a contract; digit text alone is insufficient. |
| Notes/citations/changes | `@note`, `@footnote`, `@tip`, `@help`, `@cite`, `@homebrew` | 6 | 356 | Note/footnote/citation/change roles and reference ownership need contracts; formatting alone may lose meaning. |

Every observed tag spelling matched a literal renderer declaration in this scan. That is a vocabulary check, not proof of valid usage, complete handling of extensions, or semantic understanding. A generic Reference, Annotation, or Unsupported container does not implement the identities, typed values, or rule definitions needed by every tag placed in its family.

<!-- END GENERATED COVERAGE TABLES -->

## Numeric and enum checks

- List start and columns values are JSON integers. Observed starts include zero; do not validate StartIndex with an unconditional minimum of one. Preserve its meaning separately from array indexing and renderer-specific numbering adjustments.
- Cell and header width values are positive JSON integers representing column spans. The observed body-cell range was 2–6 and the header-cell range was 2–4; an omitted span defaults to one. int32 is suitable.
- Explicit roll exact/min/max values were integers in this snapshot. Negative finite values occur. Do not interpret an arbitrary negative number as infinity.
- The renderer can produce positive/negative infinity sentinels of magnitude 10^20. Convert verified sentinels to the unbounded flags before narrowing to int32. Do not cast them into int32.
- One explicit roll range uses a zero maximum. The renderer has a zero-to-100 conversion for roll-cell bounds, while display remains separate. Supported percentile normalization must happen before ordinary finite-range validation; the source's display value must not be destroyed. Do not apply a universal zero-to-100 rule to all table data.
- Column classes contain fractional grid weights, including col-0-6, col-1-2, and col-2-3. float is suitable for their normalized weights; int32 would lose fractions. Only recognized classes map to a weight.
- The observed column styles also contain a col-0 token with no matching explicit grid-width rule in the inspected column stylesheet. Do not blindly convert it into a valid supplied zero weight. Preserve and diagnose it or use a verified alternative mapping.
- The list-marker enum covers the observed bullet/decimal/Roman/no-marker classifications. Hanging, caption, and subtype layout styles are additional presentation semantics. Missing style information and explicitly no markers remain distinct.
- Alignment classes include text-center, text-right, and text-left. Unspecified/Left/Center/Right fit; Justify is a supported model value whose adapter mapping still needs a recognized source rule. Unrecognized tokens must not be silently mapped to an enum value.

## Exact field mappings

### Lists

| Source field or form | Target / conversion rule |
| --- | --- |
| name | Parse into Heading inline items |
| items string | Text inside a paragraph block in a list item |
| item.name | Parse into the item's Label |
| item.entry / item.entries | Normalize into the item's Blocks |
| columns | bHasColumnCount = true and ColumnCount = the supplied integer |
| start | StartIndex; preserve explicit zero |
| list-decimal / list-lower-roman / list-upper-roman | MarkerStyle Decimal / LowerRoman / UpperRoman |
| list-no-bullets | MarkerStyle None |
| list-hang, list-hang-notitle, list-caption, list-hang-subtrait | Preserve distinct layout semantics through documented tags or retained metadata; no automatic collapse into a single generic style |

The source treats a directly nested list differently from an ordinary list item to avoid inserting an extra marker. The current Items array requires a deliberate normalization or supporting metadata policy for that case. itemSub labels are rendered differently from item labels, and nameDot can suppress punctuation. Preserve these semantics explicitly; do not infer that label/body arrays alone reproduce them.

### Tables

| Source field or form | Target / conversion rule |
| --- | --- |
| caption | Caption, parsed as inline content |
| colLabels | One HeaderRows entry |
| colLabelRows | Multiple HeaderRows entries, preserving grouping |
| cellHeader.width / cell.width | ColumnSpan |
| raw numeric body cell | Paragraph containing a Number inline with FDnd5eNumber; retain the original number token before lossy conversion. A digit-only source string remains Text. Storage is implemented; adapter mapping and token preservation are not |
| row array / row.row | Cells array |
| row.style | Row Presentation |
| colStyles | Column alignment, supported relative width, and remaining presentation metadata |
| rowStyles | Renderer support exists, but the inspected implementation indexes these by body-cell column position; do not mistake this for per-row styling |
| roll.exact | Equal finite Minimum and Maximum |
| roll.min / roll.max | Inclusive bounds after verified normalization |
| roll.pad = true | PadToDigits = 2 |
| roll.displayMin / roll.displayMax | Optional display overrides; code-supported, not observed in the audited explicit JSON cells |
| isStriped | bStripedRows; absent source field follows the renderer's striped default |
| isNameGenerator | bIsNameGenerator; no generation behavior |
| intro / footnotes / outro | Separate ordered block arrays; intro/outro are renderer-supported but were not observed on the surveyed typed tables |

RollExpression is a derived convenience field, not a required source column property. Preserve recognized dice annotations in header content; only populate the field when the association is unambiguous. Source strings can also encode result ranges; implementing the supported parser is separate from having a range struct.

### Quotes and insets

| Source field or form | Target / conversion rule |
| --- | --- |
| quote.entries | Quote Blocks |
| quote.by | Attribution blocks; strings were observed, and the renderer also accepts arrays |
| quote.from | Quote Source inline items |
| quote.skipMarks | bShowQuotationMarks = !skipMarks; absent field implies true |
| quote.skipItalics | bItalicize = !skipItalics; absent field implies true |
| inset.name / inset.entries | Inset Heading / Blocks |
| insetReadaloud | Inset payload plus an explicit ReadAloud classification |
| style | Supported presentation mapping plus preservation/diagnostics for unknown styles |

### Images and galleries

A follow-up structural inspection of the same 519-file snapshot found 17,183 image nodes and 1,180 galleries, including generated occurrences. Gallery arrays contained 2,676 children, all image nodes. All observed main image hrefs and the 1,622 thumbnail hrefs used internal path/type objects. The renderer's getEntryMediaUrl also supports external href URLs; that code support was not observed as an external image href in this snapshot.

The implemented storage contracts are [Media reference](shared-models.md#media-reference), [Images](shared-models.md#images), and [Galleries](shared-models.md#galleries). Their types, six native tags, and two extractors were source-reviewed and compiled in the host editor target on 2026-10-28. The mappings below remain adapter requirements; source parsing, resource resolution/loading, and typed map behavior are not implemented.

| Source | Normalized storage / adapter constraint |
| --- | --- |
| internal href.path | DatasetRelative media reference, resolved later against a user-selected image root rather than the JSON file's directory |
| external href.url | ExternalUrl reference; storage does not fetch it |
| hrefThumbnail | Independent optional media reference |
| width / height | Independent presence flags and checked positive int32 pixel values |
| title / credit | Structured inline arrays |
| altText | Plain supplied alternative text, separate from title |
| imageType map / mapPlayer | Fixed Map / PlayerMap classification tags; original value retained |
| background expectation flags | Explicitly supported Presentation hints; missing does not imply the opposite |
| gallery.images | Ordered Image block wrappers, preserving each child's metadata |
| gallery.name | Optional structured Heading |
| id / source / page | Existing wrapper Metadata |
| layout limits, styles, grid/map fields, and other unrepresented properties | Retained metadata or dedicated future contract/fallback; no claim of typed map support |

Supplied width and height each occurred 17,048 times and were positive integers: widths 28–11,873 and heights 79–9,000. These fit int32. Layout maxima are different fields and must not replace intrinsic dimensions. The gallery renderer's temporary title/credit padding and removal of imageType are display behavior; normalized storage must retain original roles and metadata. The model does not depend on website base URLs, browser state, dynamic viewers, or automatic media downloads.

## Follow-up status and remaining gaps

Long-term support requires [semantic coverage](shared-models.md#semantic-coverage) across all fifth-edition rules in scope, including semantics expressed in prose. The structural audit below covers source forms and storage mappings; it is not an exhaustive inventory or verification of D&D rules. Fallback preservation can support partial releases while semantic models are incomplete, but does not mark the corresponding rules supported. Difficulty-class forms such as `@dc` and `@dcYourSpellSave` are examples of unfinished typed work, with no special priority established by their use as examples.

The implemented [fallback storage contract](shared-models.md#fallback-payload) uses one shared FDnd5eUnsupportedContent payload for distinct block/inline Unsupported kinds. It distinguishes retained Text from Json fragments and leaves source kind/location on wrapper Metadata. The enum, payload, tags, and both extractors were source-reviewed and compiled in the host editor target on 2026-10-28. Source capture, diagnostics, resource limits, and semantic validation remain future work. Retaining a fragment does not count as verified interpretation of its rule semantics.

The implemented link-storage contracts distinguish [external destinations](shared-models.md#external-link-content) from [anchors in the owning document](shared-models.md#document-local-anchor-link-content). Their payloads, native kinds, and extractors are source-reviewed and compiled. Source `@link` and external typed hrefs inform the first mapping; `@area` informs current-document anchors. Source normalization, parsing, lookup, and navigation are unimplemented. Cross-document book/adventure selectors and website-internal routes require separate contracts or fallback.

1. **Rich section headings.** The source-driven change to `TArray<FDnd5eInlineContent>` and the C++ host/test-fixture migration are implemented and compile in the host editor target. Importer tokenization and any affected Blueprint graph migration remain separate work. Existing tests were compiled but not run for this migration.
2. **Anchors and source metadata.** The metadata/page types and their composition in both wrappers and source list items are implemented and compiled, following [Shared models](shared-models.md#content-metadata). The page discriminant preserves numeric versus textual forms. Adapter population, inherited context, and anchor resolution remain future work; the fields alone do not establish import support.
3. **ReadAloud tag.** The native `Dnd5e::Content::Presentation::ReadAloud` tag is now declared and defined as `Dnd5e.Content.Presentation.ReadAloud`. This replaces the earlier proposed Inset namespace spelling; a duplicate is unnecessary. Adapter mapping into an inset's Presentation container remains to be implemented.
4. **Incomplete semantic content vocabulary.** Record-reference, Annotation/Number, external-link, anchor-link, image, gallery, and unsupported block/inline storage/extractors are implemented and compiled, together with their supporting shared values and native tags. Source-token parsing, category-specific identity normalization, numeric conversion, target lookup, media resolution/loading, and automatic fallback capture remain unimplemented. Other annotation/media families, cross-document links, and typed map semantics still need payloads or dedicated mapping contracts. Full books/adventures cannot currently be represented without missing constructs.
5. **Incomplete presentation vocabulary and list normalization.** Storage containers exist, but the presentation vocabulary currently declares bold/italic and ReadAloud. Hanging-list variants, row-indent-first, no-wrap, and other styles still need mapping or preservation rules. [Numeric-content storage](shared-models.md#numeric-content) is implemented; source conversion and original-token preservation remain pending. Directly nested list markers need an explicit normalization decision.
6. **Adapter and validation.** Correct C++ field types do not implement string tokenization, reference resolution, bounds conversion, style parsing, diagnostics, or full structural validation. These are required before a dataset can be declared supported.

The existing extraction helpers were source-reviewed: each checks its expected kind and exact payload type, uses a const input, copies the value on success, and resets output on failure. This is a local accessor contract, not a recursive validity or source-compatibility check.

## Upstream evidence

The checked local snapshot is authoritative for this audit. Relevant upstream source locations are `js/render.js` (entry dispatch, lists, items, tables, quotes, insets, and roll-row conversion), `js/render-dice.js` (infinity sentinel), and `scss/includes/util-column.scss` (fractional widths). The [upstream renderer](https://5e.tools/js/render.js) is a useful reference, but the live site may differ from the audited snapshot.
