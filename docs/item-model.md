# Item model

Status: foundation, shared price/mass values, readable attunement and structured attunement indexes and weapon/armor data storage are implemented. Item JSON importing, index normalization/validation, equipment source mapping, definition lookup, copy/variant assembly, item-specific validation, and an item catalog remain pending. The spell reader/catalog do not import or index these values automatically.

The exported Blueprint-visible [FDnd5eItem](../Source/Dnd5e/Public/Items/Dnd5eItem.h) composes existing shared identity and Content. It is a reference-data value, with no UObject ownership, equipment state, inventory system, generated identity, or gameplay execution. Build/test evidence is recorded in [Automated tests](testing.md).

## Contents

- [Fields](#fields)
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

Default construction supplies no Category::Item or canonical key. Authors/adapters provide identity deliberately. Empty Properties means no normalized properties have been represented; this declaration does not define whether a missing raw template field inherits or an explicit empty array clears earlier properties. The future assembly stage must retain source presence/override evidence.

## Definition selection

The shared [item-definition selector](shared-models.md#item-definition-selector) carries an abbreviation rather than pretending it is a canonical record name. Native Category.ItemType and Category.ItemProperty identify different definition domains; Category.Item remains the item-record category. No default publication, dataset scope, target ID or lookup result is inferred. The existing named FDnd5eRecordSelector continues to represent named references inside Content.

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

The inspected source separates base items, named items, type/property/mastery definitions, groups, entry templates, copy dependencies, variant inheritance and supplementary rows. Foundation storage alone does not import them. Shared price/mass/attunement values and structured index storage now exist; source mappings, combination-aware alignment normalization, validation and remaining item facts remain prerequisites for full item conversion. Weapon/armor declarations now exist; their source conversion and assembly rules still need implementation. Source data and generated variants remain externally supplied; none is bundled with this model.
