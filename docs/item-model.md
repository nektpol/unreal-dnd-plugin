# Item model

Status: foundation, shared price/mass values, readable attunement and structured attunement index storage are implemented. Item JSON importing, index normalization/validation, equipment facts, definition lookup, copy/variant assembly, item-specific validation, and an item catalog remain pending. The spell reader/catalog do not import or index these values automatically.

The exported Blueprint-visible [FDnd5eItem](../Source/Dnd5e/Public/Items/Dnd5eItem.h) composes existing shared identity and Content. It is a reference-data value, with no UObject ownership, equipment state, inventory system, generated identity, or gameplay execution. Build/test evidence is recorded in [Automated tests](testing.md).

## Contents

- [Fields](#fields)
- [Definition selection](#definition-selection)
- [Rarity](#rarity)
- [Content and presence](#content-and-presence)
- [Price, mass and attunement](#price-mass-and-attunement)
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

## Remaining work

The inspected source separates base items, named items, type/property/mastery definitions, groups, entry templates, copy dependencies, variant inheritance and supplementary rows. Foundation storage alone does not import them. Shared price/mass/attunement values and structured index storage now exist; source mappings, combination-aware alignment normalization, validation and equipment facts remain prerequisites for full item conversion. Source data and generated variants remain externally supplied; none is bundled with this model.
