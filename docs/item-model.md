# Item model

Status: foundation, shared price/mass values, readable attunement and structured attunement indexes and weapon/armor data, definition records and named mastery-reference storage are implemented. The supplied base-item reader covers identity, selectors, Content, rarity, fixed price, weight, and weapon and armor fields. The ordinary-item reader converts a selected row with readable attunement, structured attunement indexes and direct base-item links. A separate assembler produces previews and field origins for modifier-free ordinary-item copy chains. Copy modifiers, variant generation and an item catalog are later stages. The spell reader/catalog do not import or index these values automatically.

The exported Blueprint-visible [FDnd5eItem](../Source/Dnd5e/Public/Items/Dnd5eItem.h) composes existing shared identity and Content. It is a reference-data value, with no UObject ownership, equipment state, inventory system, generated identity, or gameplay execution. Build/test evidence is recorded in [Automated tests](testing.md).

## Contents

- [Fields](#fields)
- [Base-item references](#base-item-references)
- [Ordinary-item copy assembly](#ordinary-item-copy-assembly)
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

The inspected source separates base items, named items, type/property/mastery definitions, groups, entry templates, copy dependencies, variant inheritance and supplementary rows. The base-item reader consumes the baseitem array and the definition reader consumes the three definition arrays. Type/property/mastery records now exist, while their canonical IDs, lookup and supplied JSON reader are implemented. The readers map price and mass, and the ordinary-item reader maps readable attunement and the supported structured index fields. Additional source mappings, richer alignment expressions, validation and remaining item facts are separate stages. Base-item weapon and armor data is converted for the inspected field shapes. Direct base-item links and modifier-free copy previews are available for ordinary rows. Copy modifiers and variant assembly remain later stages. Source data and generated variants remain externally supplied; none is bundled with this model.


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

## Ordinary-item copy assembly

Call `FDnd5eJsonItemCopyAssembler::AssembleAtIndex` or **Assemble Item Copy at Index** in Blueprint with the supplied document and the original zero-based index in its `item` array. A selected row needs an exact `_copy` declaration. Its target name and explicit source identify a unique ordinary row in the same document. The assembler follows the chain with a limit of 32 copy links. Missing, ambiguous or cyclic targets fail with a diagnostic at the source declaration. `_mod` and other unsupported copy instructions fail at the offending member. [Native API](../Source/Dnd5eJson/Public/Readers/Items/Dnd5eJsonItemCopyAssembler.h)

Each hop overlays top-level fields on the assembled parent. A supplied value overrides the whole parent field, including false, zero and empty arrays. A child null removes the field when the parent has it. Null stays explicit when the parent omits that field. Protected publication, membership, presentation and item metadata such as page, SRD markers, fluff flags, tier and loot tables require a true field-specific or wildcard `_preserve` value to inherit. Protection applies at each hop. Child-authored protected values are retained.

| Result field | C++ type and default | Meaning |
| --- | --- | --- |
| Status | `EDnd5eJsonReadStatus`, NotAttempted | Whether assembly succeeded |
| AssembledJson | `FString`, empty | Preview document containing one ordinary item at index 0 |
| SourceRowIndices | `TArray<int32>`, empty | Original indices from the selected child to the terminal ancestor |
| FieldSourceRows | `TMap<FString, int32>`, empty | Surviving top-level field names and their original row indices |
| Diagnostics | `TArray<FDnd5eDiagnostic>`, empty | Located assembly diagnostics |

A successful preview retains the selected name and source and omits the applied copy declaration. Failure publishes diagnostics with empty JSON and origin collections. To convert the preview into an item, pass it to **Read Item at Index** at index 0 with a preview-specific relative file path. Typed provenance then describes the preview, and the origin collections identify rows in the original document.
