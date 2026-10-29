# Gameplay Tag vocabulary

Status: native vocabulary reviewed on 2026-10-29. There are **121 exported native tags**. Definitions are in [Dnd5eTags.h](../Source/Dnd5e/Public/Dnd5eTags.h) and [Dnd5eTags.cpp](../Source/Dnd5e/Private/Dnd5eTags.cpp). Record/model field contracts are in [Shared models](shared-models.md) and [Spell model](spell-model.md).

## Contents

- [Ownership and use](#ownership-and-use)
- [Registered groups](#registered-groups)
- [Activation timing](#activation-timing)
- [Concentration: shared identity, contextual requirement](#concentration-shared-identity-contextual-requirement)
- [Attacks, schools, and classifications](#attacks-schools-and-classifications)
- [Matching, constraints, and enums](#matching-constraints-and-enums)
- [Migration from early development names](#migration-from-early-development-names)

## Ownership and use

Tag namespaces describe the concept being classified, rather than the first model that happened to consume it. A spell, weapon, creature feature, or item can share a tag identity without sharing storage layout or runtime behavior. C++ namespaces normally mirror the dot-separated tag path. Native registration defines vocabulary; it does not implement an importer, validation, effect execution, or a complete rule definition.

Keep source roles in their fields. Fire can occur in DamageInflicted and DamageResisted without creating separate fire identities or merging those roles. Likewise, an ability score can appear in a saving-throw classification and an ability-check classification. A tag does not encode the target, formula, prerequisites, exceptions, timing, or edition-specific rules by itself.

Only register reviewed concepts. Unknown imported strings must retain their source evidence and receive explicit adapter handling; do not auto-register arbitrary tags. Missing metadata is not a negative tag such as NoConcentration. A complete record ID still needs its namespace and source key; Category::Spell is not a spell identity.

## Registered groups

Every C++ namespace below starts with Dnd5e::. Except for the intentional player-map spelling below, each string prefix is the namespace with dots and a trailing dot, followed by the exact member name.

| Namespace | Members | Count | Meaning |
| --- | --- | ---: | --- |
| Content::Block | Unsupported, Section, Paragraph, List, Table, Quote, Inset, Image, Gallery | 9 | Document block payload discriminators; exact kind/type agreement required |
| Content::Inline | Unsupported, Text, Formatted, Reference, Annotation, Number, ExternalLink, AnchorLink | 8 | Document inline payload discriminators; exact kind/type agreement required |
| Content::Annotation | DiceRoll, DamageRoll | 2 | Meaning of an annotation value, separate from its payload wrapper |
| Content::Formatting | Bold, Italic | 2 | Inline formatting classifications |
| Content::Presentation | ReadAloud, ExpectsLightBackground, ExpectsDarkBackground | 3 | Presentation hints; no rule execution |
| Content::ImageRole | Map, PlayerMap | 2 | Media role; player maps are also maps in the tag hierarchy |
| Category | Spell | 1 | Independently identifiable record categories, not record identities |
| Activation::Time | Action, BonusAction, Reaction, Minute, Hour, Round, Special | 7 | Shared timing designation for using/casting/performing an ability or activity |
| Range::Kind | Distance, Self, Touch, Sight, Unlimited, UnlimitedSamePlane, Special | 7 | Targeting range concepts, separately from measurements and area size |
| Area::Shape | Cone, Cube, Cylinder, Line, Sphere, Hemisphere, Emanation, Radius | 8 | Source-supported area forms; geometry may be partial |
| Area::Origin | Self, SelectedPoint, Target | 3 | Placement context, separate from geometry |
| Duration::Ending | Dispelled, Triggered, Discharged | 3 | Source-defined ways a duration can end |
| Component::Type | Verbal, Somatic, Material, Royalty | 4 | Required component kinds, independent of material detail/cost |
| Magic::School | Abjuration, Conjuration, Divination, Enchantment, Evocation, Illusion, Necromancy, Transmutation, Psionic | 9 | School-of-magic classifications referenced across domains |
| Spell::Trait | Ritual | 1 | Capabilities specific to a spell definition; Ritual means supports ritual casting |
| Trait | Concentration | 1 | Shared reference requirements/properties; Concentration is not active character state |
| Attack::Mode | Melee, Ranged | 2 | Melee/ranged axis, independent of whether the attack is a spell or weapon attack |
| Damage::Type | Acid, Bludgeoning, Cold, Fire, Force, Lightning, Necrotic, Piercing, Poison, Psychic, Radiant, Slashing, Thunder | 13 | Damage identity reused across distinct inflicted/resisted/immune/vulnerable roles |
| Condition::Type | Blinded, Charmed, Deafened, Frightened, Grappled, Incapacitated, Invisible, Paralyzed, Petrified, Poisoned, Prone, Restrained, Stunned, Unconscious, Exhaustion | 15 | Condition identity; condition effects/levels/edition differences need their own definitions |
| Ability | Strength, Dexterity, Constitution, Intelligence, Wisdom, Charisma | 6 | The six ability scores, not generic usable powers or ability definitions |
| Creature::Type | Aberration, Beast, Celestial, Construct, Dragon, Elemental, Fey, Giant, Humanoid, Monstrosity, Ooze, Plant, Undead, Fiend | 14 | Creature classifications, not an exhaustive targeting-eligibility rule |
| Record::Relationship | ReprintedAs | 1 | Directed record-link semantics, independent of descriptive inline references |

For example, Dnd5e::Attack::Mode::Melee is Dnd5e.Attack.Mode.Melee, and Dnd5e::Magic::School::Evocation is Dnd5e.Magic.School.Evocation.

Content::ImageRole::PlayerMap intentionally maps to Dnd5e.Content.ImageRole.Map.Player. Its parent is Dnd5e.Content.ImageRole.Map, so a player map also matches a map query. C++ cannot simultaneously use Map as both this native tag variable and a namespace. This is an explicit naming exception, not a reason to flatten the semantic hierarchy.

## Activation timing

Dnd5e.Activation.Time.* describes the timing designation of an activity. Action, BonusAction, and Reaction are action-economy designations; Round, Minute, and Hour name measured time; Special signals that a structured numeric measurement alone is insufficient. This branch is shared because source non-spell actions and spells use compatible timing shapes. It is not a taxonomy of action definitions such as Attack or Magic, nor an assertion that a bonus action is a subtype of the Action tag. Those tags are siblings.

FDnd5eCastingTime.Unit currently consumes these tags as a timing designation. Casting is still the correct contextual term for performing a spell. A future non-spell activation model can reuse the same tags without renaming their identity. A future generalized value can separate action-economy costs from measured elapsed time; that struct/API change is not implemented by the tag review. Numeric forms need a positive supplied Quantity; Special must not acquire an invented quantity merely to satisfy that rule. Registering Special does not implement parsing of free-form action times.

Effect duration remains separate. Preserve Turn versus Round in EDnd5eTimeUnit, and preserve original quantities/units. No implicit seconds conversion, timer, or action consumption is performed. Ritual remains Spell::Trait::Ritual (supports ritual casting), not an Activation::Time member or an automatically added source casting-time entry.

## Concentration: shared identity, contextual requirement

The canonical identity is **Dnd5e::Trait::Concentration**, with string Dnd5e.Trait.Concentration. It means that the carrying reference context requires concentration. It does not mean that a character is currently concentrating. It is neither a duration amount nor an effect-only concept: the source also expresses concentration in psionic modes and during longer casting.

When source duration[].concentration supplies the requirement, retain the tag on that particular FDnd5eSpellDuration.Requirements entry. A shared tag does not require flattening its source association. If FDnd5eSpell.Traits exposes Concentration for filtering, that entry is a derived summary of represented requirements, not an independent authoritative flag. It must be recomputed when those requirements change. The adapter is not implemented yet. Casting-time concentration needs its own future contextual rule representation; do not infer it merely from the presence of a Minute/Hour tag or mistake it for concentration throughout the effect.

This permits the same identity in a future non-spell requirement without putting the tag under Spell or Duration. The existing field names and struct layouts have not been redesigned by this review.

## Attacks, schools, and classifications

Melee/Ranged live under Attack::Mode. Weapon/spell/power classification is an independent axis; Melee must not be made a child of Spell or used to infer attack origin. The current spell AttackTypes collection can store these mode tags, but it does not implement a complete attack model, roll, or hit rule. Additional attack axes need explicit models before import.

Schools live under Magic::School because their identity can be referenced outside a spell record. The source stores structured school codes mainly on spells; the hierarchy does not assert that an item-school importer already exists. Psionic preserves a parser-supported school label, not a standard ninth school or a claim that every psionic effect is magical.

Condition::Type::Exhaustion identifies a condition concept; it does not encode exhaustion level or flatten 2014/2024 mechanics. Ability contains the six ability-score classifications. Creature::Type tags are indexes, not complete creature records or targeting predicates. Geometry remains separately contextualized by range and origin; Radius/Emanation preserve source area forms rather than silently imposing a collision volume.

## Matching, constraints, and enums

Unreal hierarchical matching is directional: a leaf can match its parent, while a parent does not match a more specific leaf. Use parent queries only for intended semantic ancestry. Category, attack mode, range, damage, and condition branches are not implicitly interchangeable because a struct stores FGameplayTag. Future validation must constrain each field to its allowed branch and cardinality. Editor tag filtering alone would not validate imported data.

Content payload extraction keeps exact discriminator checks plus exact payload-type checks. Do not change TryGet accessors to accept arbitrary child tags: a tag ancestry match cannot prove that an FInstancedStruct contains the expected payload type. Role-specific spell containers likewise retain their distinct meanings.

Existing structural enums remain enums: distance/time units, currency, area-dimension kind, duration kind, material consumption, numeric representation, page/media/source-fragment kinds, and Content formatting/layout discriminants. Single versus multiple selection is not the deciding rule. Tags cover extensible classifications; enums select a fixed representation or interpretation.

## Migration from early development names

The 2026-10-29 review changed these families. C++ consumers must use the current namespaces; no deprecated duplicate native tags or C++ aliases are registered.

| Previous identity | Current identity |
| --- | --- |
| Spell::CastingTime::Unit::* | Activation::Time::* |
| Spell::School::* | Magic::School::* |
| Spell::Attack::Type::{Melee,Ranged}, or Attack::Type::{Melee,Ranged} | Attack::Mode::{Melee,Ranged} |
| Spell::Duration::Requirement::Concentration or Spell::Trait::Concentration | Trait::Concentration |
| Dnd5.e.Damage.Type.Fire / Necrotic | Dnd5e.Damage.Type.Fire / Necrotic |
| Condition::Type::Restraint | Condition::Type::Restrained |
| Creature::Type::Abberation (C++ spelling) | Creature::Type::Aberration; existing correctly spelled tag string retained |

The development host has explicit GameplayTagRedirects in its DefaultGameplayTags.ini for 22 legacy leaf identities, including earlier guide spellings. That host file is outside the plugin repository. Projects that already serialized early names need equivalent redirects under [/Script/GameplayTags.GameplayTagsSettings] in their own configuration before loading/resaving those assets. For example:

```ini
[/Script/GameplayTags.GameplayTagsSettings]
+GameplayTagRedirects=(OldTagName="Dnd5e.Spell.CastingTime.Unit.Action",NewTagName="Dnd5e.Activation.Time.Action")
+GameplayTagRedirects=(OldTagName="Dnd5e.Spell.Duration.Requirement.Concentration",NewTagName="Dnd5e.Trait.Concentration")
```

Leaf redirects preserve legacy serialized leaf names; review old parent-tag queries and C++ references separately. No automated asset rewrite or gameplay-rule migration is implemented. New projects use the current native vocabulary directly.
