# Item model

Status: foundation, shared price/mass values, readable attunement and structured attunement indexes and weapon/armor data, definition records and named mastery-reference storage are implemented. The supplied base-item reader covers identity, selectors, Content, rarity, fixed price, weight, and weapon and armor fields. The ordinary-item reader converts a selected row with readable attunement, structured attunement indexes and direct base-item links. A separate assembler produces previews and field origins for ordinary-item copy chains with supported array modifiers. The typed copy reader converts assembled rows directly with original source locations. Magic variant matching supports standalone and copied templates. Eligible pairs can generate previews with inherited fields, supported entry substitutions and transforms, and original source locations. Typed variant import preserves the generated pair ID, the base link and template ancestry. Item groups read common item fields and ordered member declarations with their own record IDs. Reusable entry templates and their Content references are represented. An item catalog is a later stage. The spell reader/catalog do not import or index these values automatically.

The exported Blueprint-visible [FDnd5eItem](../Source/Dnd5e/Public/Items/Dnd5eItem.h) composes existing shared identity and Content. It is a reference-data value, with no UObject ownership, equipment state, inventory system or gameplay execution. Build/test evidence is recorded in [Automated tests](testing.md).

## Contents

- [Fields](#fields)
- [Base-item references](#base-item-references)
- [Item groups](#item-groups)
- [Reusable item entry templates](#reusable-item-entry-templates)
- [Ordinary-item copy assembly](#ordinary-item-copy-assembly)
  - [Typed copy reading](#typed-copy-reading)
- [Magic variant matching](#magic-variant-matching)
  - [Copied variant templates](#copied-variant-templates)
  - [Specific variant generation](#specific-variant-generation)
  - [Typed variant import](#typed-variant-import)
- [Definition selection](#definition-selection)
- [Rarity](#rarity)
- [Content and presence](#content-and-presence)
- [Price, mass and attunement](#price-mass-and-attunement)
- [Weapon and armor data](#weapon-and-armor-data)
  - [Weapon data](#weapon-data)
  - [Armor data](#armor-data)
- [Remaining work](#remaining-work)

## Fields

| Field | C++ type and default | Meaning |
| --- | --- | --- |
| Header | `FDnd5eRecordHeader`, default | Shared identity/name/aliases/classification/provenance |
| bHasBaseItemId | `bool`, false | Whether BaseItemId represents a supplied link |
| BaseItemId | `FDnd5eRecordId`, default | Canonical target of a direct base-item reference |
| bHasType | `bool`, false | Whether Type represents a supplied type selection |
| Type | `FDnd5eItemDefinitionSelector`, default | Item-type abbreviation/publication; active when bHasType is true |
| Properties | `TArray<FDnd5eItemDefinitionSelector>`, empty | Represented property selectors in order |
| Rarity | `FGameplayTag`, invalid | One represented rarity classification |
| Description | `TArray<FDnd5eContentBlock>`, empty | Item description Content |
| bHasSupplementaryContent | `bool`, false | Whether separate supplementary data is supplied |
| SupplementaryContent | `FDnd5eSupplementaryContent`, default | Shared prose/image arrays, active when supplied |
| Price | `FDnd5ePrice`, default | Fixed-cost presence, denomination array and separate multiplier |
| bHasWeight | `bool = false` | Whether Weight is supplied |
| Weight | `FDnd5eMass`, default | Active when bHasWeight is true |
| bHasWeightMultiplier | `bool = false` | Whether WeightMultiplier is supplied |
| WeightMultiplier | `FDnd5eNumber`, default | Separate multiplier; active when supplied |
| WeightNote | `TArray<FDnd5eInlineContent>`, empty | Readable weight wording |
| Attunement | `FDnd5eAttunementRequirement`, default | Primary declaration |
| bHasAlternativeAttunement | `bool = false` | Whether an alternative is supplied |
| AlternativeAttunement | `FDnd5eAttunementRequirement`, default | Independent alternative declaration, active when supplied |
| bHasWeaponData | `bool = false` | Whether WeaponData is represented |
| WeaponData | `FDnd5eItemWeaponData`, default | Active when its flag is true |
| bHasArmorData | `bool = false` | Whether ArmorData is represented |
| ArmorData | `FDnd5eItemArmorData`, default | Active when its flag is true |
| Masteries | `TArray<FDnd5eItemMasteryReference>`, empty | Ordered named mastery references with independent notes |

Default construction supplies no Category::Item or canonical key. Authors/adapters provide identity deliberately. Empty Properties means no normalized properties have been represented; this declaration does not define whether a missing raw template field inherits or an explicit empty array clears earlier properties. The future assembly stage must retain source presence/override evidence.

## Base-item references

The ordinary-item reader normalizes an exact `baseItem` member containing a nonempty name and explicit source separated by a single pipe. It uses the document dataset namespace and the same canonical item ID rules as the base-item reader. A valid reference sets `bHasBaseItemId` and consumes the source member from additional metadata. The referenced record can be loaded later. Invalid values and self references fail at `/item/<index>/baseItem`. Base-row conversion leaves this typed link inactive.

Call `UDnd5eItemLibrary::TryGetReferencedBaseItem` or **Try Get Referenced Base Item** in Blueprint with the ordinary item and a caller-supplied array of base records. The helper returns a copy only for a unique match of the full record ID, including the dataset namespace. An inactive or invalid link, no match, or duplicate matches returns false and clears the output. The returned base record retains its own identity and provenance. The ordinary record keeps its own fields, including any absent equipment values. [Core lookup](../Source/Dnd5e/Public/Items/Dnd5eItemLibrary.h)

## Definition selection

The shared [item-definition selector](shared-models.md#item-definition-selector) carries an abbreviation rather than pretending it is a canonical record name. Native Category.ItemType and Category.ItemProperty identify different definition domains; Category.Item remains the item-record category. No default publication, dataset scope, target ID or lookup result is inferred. The existing named FDnd5eRecordSelector continues to represent named references inside Content. Named mastery references also use FDnd5eRecordSelector with Category.ItemMastery. [Item definitions](item-definitions.md) documents the separate type/property/mastery records and the reference Note field. Masteries contains references rather than embedded definitions; an empty normalized array does not decide raw inheritance/clearing behavior.

## Rarity

Native `Dnd5e.Item.Rarity` identities are None, Common, Uncommon, Rare, VeryRare, Legendary, Artifact, Unknown, UnknownMagic and Varies. The single tag distinguishes absent/unrepresented rarity (invalid) from the ten explicit classifications. None does not prove that an item is nonmagical. UnknownMagic is a sibling of Unknown, preserving their source distinction rather than introducing an implication through tag hierarchy. No rarity derives price, eligibility or effects.

## Content and presence

Description and supplementary values reuse the [shared Content models](shared-models.md#shared-structured-content), including typed nested references, annotations and images. Supplementary prose is separate from Description; false presence means its inactive storage is not declared supplied. Type and supplementary payloads have their corresponding EditCondition metadata for editor authoring. Ordinary value copies and reflected serialization preserve nested Content and selectors independently.

This batch adds no item-specific extraction/traversal/validation API. ValidateSpellContent remains spell-specific; an item is not validated by constructing a fake spell. A shared traversal entry point can be extracted when the actual second consumer is implemented.

## Price, mass and attunement

The canonical [Price](shared-models.md#price), [Mass](shared-models.md#mass) and [Attunement requirement](shared-models.md#attunement-requirement) definitions own these reusable fields. Item composition keeps fixed-price absence/null/zero distinct, weight separate from its multiplier/note, and primary attunement separate from alternative attunement. The alternative neither overwrites the primary nor automatically relaxes its conditions.

Storage computes no effective cost, weight or eligibility and performs no unit conversion. Each attunement value can also carry an independent [structured index](shared-models.md#attunement-index). Group boundaries, alternative alignment combinations, explicit boolean presence and unsupported fragments are retained. Index completeness describes represented input; it does not establish prose completeness or eligibility. False presence flags leave inactive storage intact; EditCondition metadata guides authoring without enforcing validity.

## Weapon and armor data

Both exported Blueprint-visible values are declared in [Dnd5eItemEquipment.h](../Source/Dnd5e/Public/Items/Dnd5eItemEquipment.h). Their independent flags on FDnd5eItem represent supplied data, not equipped state or a mutually exclusive type assertion. Default construction does not infer an item type or classification.

### Weapon data

EDnd5eWeaponCategory is a Blueprint-visible uint8 enum: Unspecified, Simple, Martial. Ranged/firearm are separate concepts, not members of this category.

| Field | C++ type and default | Meaning |
| --- | --- | --- |
| Category | `EDnd5eWeaponCategory = EDnd5eWeaponCategory::Unspecified` | Represented simple/martial category |
| bHasPrimaryDamage | `bool = false` | Whether PrimaryDamage is supplied |
| PrimaryDamage | `FDnd5eDamageExpression`, default | Active when its flag is true |
| bHasAlternateDamage | `bool = false` | Whether separate AlternateDamage is supplied |
| AlternateDamage | `FDnd5eDamageExpression`, default | Active when its flag is true |
| bHasRange | `bool = false` | Whether attack range is supplied |
| Range | `FDnd5eAttackRange`, default | Active when its flag is true |
| bHasReload | `bool = false` | Whether Reload is supplied |
| Reload | `int32 = 0` | Supplied count, active when its flag is true |

The canonical [damage expression](shared-models.md#damage-expression) and [attack range](shared-models.md#attack-range) definitions own those reusable values. AlternateDamage is independent of primary damage; its use conditions are not inferred as two-handed or executed. Source property templates can refer to it later. Reload stores a count without tracking shots, consuming ammunition or performing an action. Attack mode/proficiency and property definitions retain their separate roles.

### Armor data

| Field | C++ type and default | Meaning |
| --- | --- | --- |
| bHasArmorClass | `bool = false` | Whether an AC definition is supplied |
| ArmorClass | `FDnd5eArmorClassDefinition`, default | Active when its flag is true |
| StrengthPresence | `EDnd5eValuePresence = EDnd5eValuePresence::Unspecified` | Absent, explicit null or supplied threshold |
| StrengthMinimum | `FDnd5eAbilityScoreMinimum`, default | Active when Present; Ability must be the shared Strength identity |
| bHasStealthDisadvantage | `bool = false` | Whether the stealth flag is supplied |
| bStealthDisadvantage | `bool = false` | Supplied flag, active when its presence flag is true |

Armor data composes the canonical [AC definition](shared-models.md#armor-class-definition), [value presence](shared-models.md#value-presence) and [ability-score minimum](shared-models.md#ability-score-minimum). Missing, explicit null and supplied zero Strength thresholds remain distinct, as do absent and explicitly false stealth flags. The Strength threshold describes the source armor rule without independently prohibiting wear or executing a movement penalty. AC source interpretation requires type/override context; an AC number alone does not establish shield/armor type or a formula.

Editor conditions enable active payloads without clearing inactive data or validating it. Copying and reflected serialization retain both. No damage, AC, movement or equipment system is implemented.

## Remaining work

The inspected source separates base items, named items, type/property/mastery definitions, groups, entry templates, copy dependencies, variant inheritance and supplementary rows. The base-item reader consumes the baseitem array and the definition reader consumes the three definition arrays. Type/property/mastery records now exist, while their canonical IDs, lookup and supplied JSON reader are implemented. The readers map price and mass, and the ordinary-item reader maps readable attunement and the supported structured index fields. Additional source mappings, richer alignment expressions, validation and remaining item facts are separate stages. Base-item weapon and armor data is converted for the inspected field shapes. Direct base-item links and copy previews with array modifiers are available for ordinary rows. Direct typed copy import and selected item group reading are available. The magic variant matcher reports eligible base rows. Specific variant previews are available for supported inheritance rules. Source data and generated variants remain externally supplied; none is bundled with this model.


## Supplied base-item JSON

Pass an FDnd5eJsonDocument to FDnd5eJsonBaseItemReader::Read or the Blueprint Read Base Items node. The exact baseitem array is required. An explicit empty array succeeds. The reader validates the document with the shared limits, stages rows in input order and returns empty Items on any failure, including a duplicate canonical ID.

The factory uses Category.Item and the key format `dnd5e:item:v1:<escaped-name>|<escaped-source>`. Name and publication are trimmed and case-folded before escaping percent, pipe and colon. The trimmed dataset namespace is case-sensitive. Original names and publications remain in the row data. Type/property/mastery selectors preserve whether a publication was supplied. Property notes stay in the retained property array and mastery notes become inline Content.

The reader maps the snapshot's none and unknown rarity values, numeric standard prices in copper, explicit null prices and weights in pounds. Custom currency inputs stay in provenance. Boolean source memberships become Included or Excluded. Numeric page zero is retained as evidence with the typed page unset. Unconverted fields remain in AdditionalMetadataJson with diagnostics. [Native reader](../Source/Dnd5eJson/Public/Readers/Items/Dnd5eJsonBaseItemReader.h) · [Blueprint library](../Source/Dnd5eJson/Public/Readers/Items/Dnd5eJsonBaseItemLibrary.h)


### Base-item equipment conversion

| Source field | Typed result |
| --- | --- |
| weaponCategory | Simple or Martial |
| dmg1, dmg2, dmgType | Primary and alternate damage text using the 5etools syntax label and supported damage tags |
| range | Positive normal/long distances in feet, with long at least normal |
| reload | Positive integer with explicit presence |
| ac | Base AC for LA, MA and HA, or bonus AC for S |
| strength | Positive integer threshold from a string, or explicit null |
| stealth | Explicit Boolean presence and value |

LA uses uncapped Dexterity and MA caps it at 2. HA and S omit Dexterity. The mapping uses the type abbreviation independently of its publication. For other abbreviations or a supplied dexterityMax, the reader retains the raw AC inputs. Unknown damage codes and their expressions stay raw. The reader retains omitted-field defaults, including an unspecified damage tag when no type is supplied.

Converted exact fields are consumed before raw metadata is collected. Unconverted fields such as ammunition references and classification flags remain in provenance. Invalid equipment inputs in a supported shape return a field diagnostic and discard the staged array.


## Supplied ordinary-item JSON

Pass a document and a zero-based source index to FDnd5eJsonItemReader::ReadAtIndex or Read Item at Index in Blueprint. The reader validates the supplied JSON document, then converts the selected object in its exact item array. An out-of-range index fails at /item. A selected row with _copy fails at that dependency field, including when a direct base-item reference is also present. A failed read returns a default Item. Other rows are not converted during this operation. [Native reader](../Source/Dnd5eJson/Public/Readers/Items/Dnd5eJsonItemReader.h) · [Blueprint library](../Source/Dnd5eJson/Public/Readers/Items/Dnd5eJsonItemLibrary.h)

Ordinary rows use the shared item identity, header, Content, price, weight and equipment conversion. Type may be absent. The reader maps the ten observed rarity labels and accepts a single positive distance or a normal/long pair for ordinary range. The damage map includes Cold, Force and Psychic in addition to the base-item mappings.

Readable reqAttune and reqAttuneAlt accept Booleans and nonempty strings. True maps to Required and false to NotRequired. The exact string optional maps to Optional. Other nonempty strings become Required with converted inline conditions. Omission leaves the mode Unspecified. Alternate presence is tracked independently. The optional reqAttuneTags and reqAttuneAltTags fields populate the primary and alternative indexes independently. An alternative index sets alternative presence even when reqAttuneAlt is absent. Omission leaves index presence false. An empty array sets presence and is fully represented with zero groups.

Each nonempty object in the array becomes a group. Supported members include class, race/species, background and skill selectors with optional explicit source, spellcasting and psionics Booleans, a positive integer Intelligence minimum, single G or E alignment, language proficiency, fey or humanoid creature type, and Small size. Unknown members and unsupported member values stay in the group's UnsupportedContent. Non-object and empty-object rows stay in the index's UnsupportedContent, as do malformed top-level index values. These fragments set completeness to false and emit located warnings. Converted index keys are consumed from the row's additional provenance metadata, since the typed index carries its own evidence.

## Item groups

Call `FDnd5eJsonItemGroupReader::ReadAtIndex` or **Read Item Group at Index** with a supplied document and the zero-based index in its `itemGroup` array. `FDnd5eItemGroup::Details` uses the ordinary item field reader, with an original group provenance pointer and a distinct `Dnd5e.Category.ItemGroup` identity. The key uses `dnd5e:item-group:v1:` followed by the normalized name and source. A group can share its name and publication with an ordinary item and link to it as a base.

`Members` is a `TArray<FDnd5eItemGroupMember>` in authored order. Each member keeps its raw UID, name, and optional explicit source. Both `name` and `name|source` are accepted. Repeated declarations retain their positions. The visibility flag `bItemsHidden` defaults to false and preserves the member declarations when true. Group members remain selectors until catalog resolution. Failures leave the group at its default and report the offending input location. [Native reader](../Source/Dnd5eJson/Public/Readers/Items/Dnd5eJsonItemGroupReader.h)

## Reusable item entry templates

Call `FDnd5eJsonItemEntryTemplateReader::ReadAtIndex` or **Read Item Entry Template at Index** with supplied text and the zero-based index in its `itemEntry` array. The result contains an `FDnd5eItemEntryTemplate` with a source-qualified `Dnd5e.Category.ItemEntry` ID, original provenance and `EntriesTemplateJson`. The body preserves its array structure and placeholder strings for later evaluation. Additional outer metadata is retained in the header provenance. Failure leaves the template at its default.

Content recognizes whole-entry `{#itemEntry Name|Source}` strings and exact `refItemEntry` objects as paragraphs containing a typed reference inline. The selector identifies the reusable entry category. Explicit sources produce canonical target IDs. Omitted sources remain unresolved selectors with a located warning. The reference retains its original source form. Expansion and insertion into the description are later operations. [Native reader](../Source/Dnd5eJson/Public/Readers/Items/Dnd5eJsonItemEntryTemplateReader.h)

## Ordinary-item copy assembly

Call `FDnd5eJsonItemCopyAssembler::AssembleAtIndex` or **Assemble Item Copy at Index** in Blueprint with the supplied document and the original zero-based index in its `item` array. A selected row needs an exact `_copy` declaration. Its target name and explicit source identify a unique ordinary row in the same document. The assembler follows the chain with a limit of 32 copy links. Missing, ambiguous or cyclic targets fail with a diagnostic at the source declaration. Unsupported copy instructions fail at the offending member. [Native API](../Source/Dnd5eJson/Public/Readers/Items/Dnd5eJsonItemCopyAssembler.h)

Each hop overlays top-level fields on the assembled parent. A supplied value overrides the whole parent field, including false, zero and empty arrays. A child null removes the field when the parent has it. Null stays explicit when the parent omits that field. Protected publication, membership, presentation and item metadata such as page, SRD markers, fluff flags, tier and loot tables require a true field-specific or wildcard `_preserve` value to inherit. Protection applies at each hop. Child-authored protected values are retained.

### Array modifiers

Each row applies its `_mod` after inheriting and overlaying fields, before the next row in the chain. The supported targets are the exact top-level `entries` and `property` keys. Each target accepts a single operation object or an ordered array of objects.

| Target | Mode | Supported behavior |
| --- | --- | --- |
| entries | `appendArr` | Append one JSON value or the elements of an array. Create the target array when absent or null. |
| entries | `insertArr` | Insert into an existing array. Index -1 appends. A nonnegative index past the end clamps to the end. |
| entries | `replaceArr` | Replace one existing element selected by a zero-based `replace.index`. An empty items array removes that element. |
| property | `appendIfNotExistsArr` | Append strings missing from the array at the start of the operation, using exact case-sensitive comparison. Create the array when absent or null. |

Each directive reads the preceding result. Effective edits attribute the top-level field to the modifying row. No-op appends leave its previous origin intact. An array edited by several rows still has one top-level origin. The assembler limits a field to 64 directives per row and a modified array to 4,096 values. Nested targets, other modes, copy variables and regex or name replacement selectors produce located failures. A failed directive discards the whole preview.

| Result field | C++ type and default | Meaning |
| --- | --- | --- |
| Status | `EDnd5eJsonReadStatus`, NotAttempted | Whether assembly succeeded |
| AssembledJson | `FString`, empty | Preview document containing one ordinary item at index 0 |
| SourceRowIndices | `TArray<int32>`, empty | Original indices from the selected child to the terminal ancestor |
| FieldSourceRows | `TMap<FString, int32>`, empty | Surviving top-level field names and their original row indices |
| Diagnostics | `TArray<FDnd5eDiagnostic>`, empty | Located assembly diagnostics |

A successful preview retains the selected name and source and omits the applied copy declaration. Failure publishes diagnostics with empty JSON and origin collections. To convert the preview into an item, pass it to **Read Item at Index** at index 0 with a preview-specific relative file path. Typed provenance then describes the preview, and the origin collections identify rows in the original document.

### Typed copy reading

Call `FDnd5eJsonItemCopyAssembler::ReadCopyAtIndex` or **Read Item Copy at Index** with the original document and the original copy row index. The typed reader shares the preview assembler's validation and overlay rules. It passes the assembled object directly to the ordinary-item converter. The original source document remains the context for typed provenance and diagnostics.

| Result field | C++ type and default | Meaning |
| --- | --- | --- |
| Status | `EDnd5eJsonReadStatus`, NotAttempted | Whether assembly and typed conversion succeeded |
| Item | `FDnd5eItem`, default | Assembled item with the selected child's canonical ID |
| SourceRowIndices | `TArray<int32>`, empty | Original indices from the selected child to the terminal ancestor |
| FieldSourcePointers | `TMap<FString, FString>`, empty | Surviving top-level field names and their original source pointers |
| Diagnostics | `TArray<FDnd5eDiagnostic>`, empty | Located assembly or conversion diagnostics |

The selected row is the primary record origin. Ancestor origins appear in chain order. A represented page or membership inherited from an ancestor is recorded on that ancestor. AdditionalMetadataJson on the primary origin retains metadata from the assembled row. Its individual original fields can be located through FieldSourcePointers.

Inherited whole fields retain exact nested paths. A modified array points to its last effective modifier, and its descendants use that operation as a location anchor. This mapping has top-level granularity for modified arrays. An explicitly supplied Content source still takes precedence for that Content and its descendants. If either assembly or typed conversion fails, the item and lineage remain at their defaults and diagnostics identify the source location when available. The raw preview remains available for inspecting assembly separately. [Typed copy API](../Source/Dnd5eJson/Public/Readers/Items/Dnd5eJsonItemCopyAssembler.h)

## Magic variant matching

Call `FDnd5eJsonItemVariantMatcher::MatchAtIndex` or **Match Magic Item Variant at Index** with the variant document, original zero-based template index, base-item document and edition policy. The exact `magicvariant` and `baseitem` arrays are read from documents in the same dataset namespace. The result carries the template name and its explicit `inherits.source` publication. Successful results list the original base row indices in input order, including an empty list when no base qualifies. Failure leaves the result data empty and reports the offending file and JSON pointer.

A base row qualifies when it satisfies every field in at least one `requires` group and no field in `excludes`. Predicates compare strings case-sensitively and Booleans by exact value. Array-valued predicates and base fields match on any shared value. An absent field cannot satisfy false. An empty requirements array matches nothing. An empty requirement object has no conditions. A supplied `packContents` array, including an empty array, excludes a bundled base. Template copies and unsupported predicate shapes report located failures.

| Base edition | SourceDefault accepts | PreferClassic accepts |
| --- | --- | --- |
| Unspecified | Unspecified, classic, one | Unspecified, classic, one |
| classic | classic | Unspecified, classic |
| one | Unspecified, one | one |

Unspecified means an absent or null source edition. The policies preserve exact edition matches and differ in how they pair unspecified templates with declared base editions. Matching evaluates raw fields such as sword, scfType and property, alongside fields already represented in the item model. For a copied template, the matcher evaluates the assembled template's rules. [Native matcher](../Source/Dnd5eJson/Public/Readers/Items/Dnd5eJsonItemVariantMatcher.h)

### Copied variant templates

An explicit `_copy.name` and `_copy.source` select a unique template within the supplied variant document. Template lookup uses the root source, falling back to `inherits.source` when the root source is absent or empty. Matching follows up to 32 links, applies the existing top-level copy preserve rules, then applies each child's supported nested modifiers. The nested forms cover appending entries, appending exact property and resistance strings, setting name parts, rarity, publication and page, and removing the inspected membership, class feature and loot fields. Missing targets, ambiguous targets, cycles and unsupported modifiers report the declaring source location.

### Specific variant generation

Call `FDnd5eJsonItemVariantGenerator::GenerateAtIndex` or **Generate Magic Item Variant at Index** with both original documents, the template and base indices, and the edition policy. The matcher validates the same dataset and eligibility before generating the selected pair. A successful result carries the single-row ordinary-item preview in GeneratedJson and the two original row locations in BaseOrigin and TemplateOrigin. TemplateSourceRowIndices lists the selected template followed by its ancestors, in order, using original zero-based indices. Failure leaves the preview, IDs, origins and indices at their defaults and returns diagnostics.

The preview starts from the base fields and clears fixed value, base publication membership and page fields, reprint and reference-source metadata, and fluff flags before applying template inheritance. Literal name removal runs before prefix and suffix. Template entries precede base entries. `propertyAdd` compares exact UIDs against the original property array, preserving property notes and source order. The `linkedLootTables` lookup uses the generated publication and name and appends the linked values. The preview includes source-qualified `baseItem` and `genericVariant` links.

GeneratedId uses the item category and a pair key derived from the canonical base and template keys. Each component is escaped separately. Two base records from different publications can therefore produce the same display name without sharing a generated ID. An eligible pair has a stable ID across matching policies. For a copied template, the pair ID, reverse template link and TemplateOrigin use the selected template's identity source. The generated item's source and linked loot lookup use the effective `inherits.source`. Use typed variant import to carry the pair ID into an `FDnd5eItem`.

The generator substitutes supported `{=...}` tokens in template entry strings before prepending them. Base-name tokens use the original name and support plain text, lowercase and the source's a/an and capitalized article forms. Damage codes render with the source default capitalization under either eligibility policy. Supported bonus tokens read strings or numeric literals from `inherits`. Substitution visits nested entry values and preserves existing markup and base entries. Unknown tokens, unsupported modifiers, missing inputs and exceeded rendering limits produce located failures.

Standalone transforms evaluate bounded arithmetic using the original base value or weight and one nonnegative integer operand with addition or multiplication. If the base amount is absent or null, the preview omits the calculated amount, retains the expression and reports its original source location. A supplied zero is evaluated normally. Barding retains the original base type in `bardingType`.

Property removal by exact UID runs before property addition. Flat damage trait arrays support clearing a list with null and removing overlapping base values from other trait lists before appending and deduplicating template traits. Condition immunity lists append and deduplicate exact strings. Unsupported syntax, nested damage traits and conflicting directives produce located failures. [Native generator](../Source/Dnd5eJson/Public/Readers/Items/Dnd5eJsonItemVariantGenerator.h)

### Typed variant import

Call `FDnd5eJsonItemVariantGenerator::ReadAtIndex` or **Read Magic Item Variant at Index** with the same inputs as the preview API. The reader converts the assembled row directly into `FDnd5eItem`. Its record ID equals the preview pair ID, and `BaseItemId` points to the canonical base item even when their names and publications coincide. The result includes BaseOrigin, TemplateOrigin and TemplateSourceRowIndices. The reverse `genericVariant` link is retained in additional metadata.

The first header provenance entry carries the generated publication, page, memberships and retained metadata. Its location is empty because the row is composed. The following entries locate the selected template and base. Diagnostics and Content metadata for unchanged base fields, appended base entries and literal inherited fields use their original input location. Composed fields use the selected template row when there is no exact mapping. Failure returns diagnostics with the item and its origins at their defaults.
