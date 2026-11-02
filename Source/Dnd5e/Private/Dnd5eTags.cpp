// Developed by Neko Creative Technologies

#include "Dnd5eTags.h"

namespace Dnd5e
{
	namespace Content::Block
	{
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Unsupported, "Dnd5e.Content.Block.Unsupported", "Block retaining unsupported source content");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Section, "Dnd5e.Content.Block.Section", "Tag for section content block");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Paragraph, "Dnd5e.Content.Block.Paragraph", "Tag for paragraph content block");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(List, "Dnd5e.Content.Block.List", "Tag for list content block");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Table, "Dnd5e.Content.Block.Table", "Tag for table content block");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Quote, "Dnd5e.Content.Block.Quote", "Tag for quote content block");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Inset, "Dnd5e.Content.Block.Inset", "Tag for inset content block");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Image, "Dnd5e.Content.Block.Image", "Tag for image content block");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Gallery, "Dnd5e.Content.Block.Gallery", "Tag for gallery content block");
	}

	namespace Content::Inline
	{
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Unsupported, "Dnd5e.Content.Inline.Unsupported", "Inline content retaining unsupported source content");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Text, "Dnd5e.Content.Inline.Text", "Tag for inline text content");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Formatted, "Dnd5e.Content.Inline.Formatted", "Inline content containing formatting and child inline items");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Reference, "Dnd5e.Content.Inline.Reference", "Inline content referencing another content item");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Annotation, "Dnd5e.Content.Inline.Annotation", "Inline content containing an annotation value and display content");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Number, "Dnd5e.Content.Inline.Number", "Inline content containing a number value");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(ExternalLink, "Dnd5e.Content.Inline.ExternalLink", "Inline content linking to an external URL");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(AnchorLink, "Dnd5e.Content.Inline.AnchorLink", "Inline content linking to an anchor within the content");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(DocumentLink, "Dnd5e.Content.Inline.DocumentLink", "Document destination");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(QuickReference, "Dnd5e.Content.Inline.QuickReference", "Quick-reference destination");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(QueryLink, "Dnd5e.Content.Inline.QueryLink", "Retained query destination");
	}

	namespace Content::Annotation
	{
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(DiceRoll, "Dnd5e.Content.Annotation.DiceRoll", "Annotation content containing a dice roll expression");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(DamageRoll, "Dnd5e.Content.Annotation.DamageRoll", "Annotation content containing a damage roll expression");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(DifficultyClass, "Dnd5e.Content.Annotation.DifficultyClass", "Annotation content containing a difficulty class roll expression");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(AttackRoll, "Dnd5e.Content.Annotation.AttackRoll", "Annotation content containing an attack roll expression");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(D20Roll, "Dnd5e.Content.Annotation.D20Roll", "Annotation content containing a d20 roll expression");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(PercentChance, "Dnd5e.Content.Annotation.PercentChance", "Annotation content containing a percent chance roll expression");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(ScaledDiceRoll, "Dnd5e.Content.Annotation.ScaledDiceRoll", "Annotation content containing a scaled dice roll expression");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(ScaledDamageRoll, "Dnd5e.Content.Annotation.ScaledDamageRoll", "Annotation content containing a scaled damage roll expression");

	}

	namespace Content::Formatting
	{
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Bold, "Dnd5e.Content.Formatting.Bold", "Tag for bold formatting");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Italic, "Dnd5e.Content.Formatting.Italic", "Tag for italic formatting");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Note, "Dnd5e.Content.Formatting.Note", "Note presentation, distinct from ordinary italic text");
	}

	namespace Content::Presentation
	{
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(ReadAloud, "Dnd5e.Content.Presentation.ReadAloud", "Tag for content that should be read aloud");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(ExpectsLightBackground, "Dnd5e.Content.Presentation.ExpectsLightBackground", "Tag for content that expects a light background");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(ExpectsDarkBackground, "Dnd5e.Content.Presentation.ExpectsDarkBackground", "Tag for content that expects a dark background");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(HangingList, "Dnd5e.Content.Presentation.HangingList", "List with hanging item labels");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(NoWrap, "Dnd5e.Content.Presentation.NoWrap", "Content should not wrap within its layout cell");
	}

	namespace Content::ImageRole
	{
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Map, "Dnd5e.Content.ImageRole.Map", "Tag for images that are maps");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(PlayerMap, "Dnd5e.Content.ImageRole.Map.Player", "Tag for images that are player maps");
	}

	namespace Category
	{
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Spell, "Dnd5e.Category.Spell", "Tag for spell content");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Class, "Dnd5e.Category.Class", "Class record category");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Subclass, "Dnd5e.Category.Subclass", "Subclass record category; parent class is part of its source identity");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Species, "Dnd5e.Category.Species", "Species record category, including source-format race selectors");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Background, "Dnd5e.Category.Background", "Background record category");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Feat, "Dnd5e.Category.Feat", "Feat record category");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(OptionalFeature, "Dnd5e.Category.OptionalFeature", "Optional feature record category");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(CharacterOption, "Dnd5e.Category.CharacterOption", "Character creation option record category");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Reward, "Dnd5e.Category.Reward", "Reward record category");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Item, "Dnd5e.Category.Item", "Item selector domain");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Creature, "Dnd5e.Category.Creature", "Creature selector domain");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Condition, "Dnd5e.Category.Condition", "Condition selector domain");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Status, "Dnd5e.Category.Status", "Status selector domain");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Action, "Dnd5e.Category.Action", "Action selector domain");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Skill, "Dnd5e.Category.Skill", "Skill selector domain");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Sense, "Dnd5e.Category.Sense", "Sense selector domain");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Hazard, "Dnd5e.Category.Hazard", "Hazard selector domain");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Rule, "Dnd5e.Category.Rule", "Rule selector domain");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(ClassFeature, "Dnd5e.Category.ClassFeature", "Class-feature selector domain");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(ItemType, "Dnd5e.Category.ItemType", "Item type definition selector");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(ItemProperty, "Dnd5e.Category.ItemProperty", "Item property definition selector");
	}

	namespace Document::Kind
	{
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Book, "Dnd5e.Document.Kind.Book", "Book document destination");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Adventure, "Dnd5e.Document.Kind.Adventure", "Adventure document destination");
	}

	namespace Activation::Time
	{
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Action, "Dnd5e.Activation.Time.Action", "Activation timing designation: Action");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(BonusAction, "Dnd5e.Activation.Time.BonusAction", "Activation timing designation: BonusAction");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Reaction, "Dnd5e.Activation.Time.Reaction", "Activation timing designation: Reaction");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Minute, "Dnd5e.Activation.Time.Minute", "Activation timing designation: Minute");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Hour, "Dnd5e.Activation.Time.Hour", "Activation timing designation: Hour");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Round, "Dnd5e.Activation.Time.Round", "Activation timing measured in rounds; no implicit conversion to seconds");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Special, "Dnd5e.Activation.Time.Special", "Special activation timing; not a physical unit or an inferred numeric quantity");
	}

	namespace Range::Kind
	{
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Distance, "Dnd5e.Range.Kind.Distance", "Tag for range kind Distance");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Self, "Dnd5e.Range.Kind.Self", "Tag for range kind Self");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Touch, "Dnd5e.Range.Kind.Touch", "Tag for range kind Touch");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Sight, "Dnd5e.Range.Kind.Sight", "Tag for range kind Sight");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Unlimited, "Dnd5e.Range.Kind.Unlimited", "Tag for range kind Unlimited");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(UnlimitedSamePlane, "Dnd5e.Range.Kind.UnlimitedSamePlane", "Tag for range kind UnlimitedSamePlane");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Special, "Dnd5e.Range.Kind.Special", "Tag for range kind Special");
	}

	namespace Area::Shape
	{
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cone, "Dnd5e.Area.Shape.Cone", "Tag for area shape Cone");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cube, "Dnd5e.Area.Shape.Cube", "Tag for area shape Cube");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cylinder, "Dnd5e.Area.Shape.Cylinder", "Tag for area shape Cylinder");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Line, "Dnd5e.Area.Shape.Line", "Tag for area shape Line");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Sphere, "Dnd5e.Area.Shape.Sphere", "Tag for area shape Sphere");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Hemisphere, "Dnd5e.Area.Shape.Hemisphere", "Tag for area shape Hemisphere");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Emanation, "Dnd5e.Area.Shape.Emanation", "Tag for area shape Emanation");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Radius, "Dnd5e.Area.Shape.Radius", "Tag for area shape Radius");
	}

	namespace Area::Origin
	{
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Self, "Dnd5e.Area.Origin.Self", "Tag for area origin Self");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(SelectedPoint, "Dnd5e.Area.Origin.SelectedPoint", "Tag for area origin SelectedPoint");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Target, "Dnd5e.Area.Origin.Target", "Tag for area origin Target");
	}

	namespace Duration::Ending
	{
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Dispelled, "Dnd5e.Duration.Ending.Dispelled", "Tag for duration ending Dispelled");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Triggered, "Dnd5e.Duration.Ending.Triggered", "Tag for duration ending Triggered");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Discharged, "Dnd5e.Duration.Ending.Discharged", "Tag for duration ending Discharged");
	}

	namespace Component::Type
	{
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Verbal, "Dnd5e.Component.Type.Verbal", "Tag for component type Verbal");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Somatic, "Dnd5e.Component.Type.Somatic", "Tag for component type Somatic");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Material, "Dnd5e.Component.Type.Material", "Tag for component type Material");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Royalty, "Dnd5e.Component.Type.Royalty", "Tag for component type Royalty");
	}

	namespace Magic::School
	{
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Abjuration, "Dnd5e.Magic.School.Abjuration", "School-of-magic classification: Abjuration");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Conjuration, "Dnd5e.Magic.School.Conjuration", "School-of-magic classification: Conjuration");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Divination, "Dnd5e.Magic.School.Divination", "School-of-magic classification: Divination");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Enchantment, "Dnd5e.Magic.School.Enchantment", "School-of-magic classification: Enchantment");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Evocation, "Dnd5e.Magic.School.Evocation", "School-of-magic classification: Evocation");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Illusion, "Dnd5e.Magic.School.Illusion", "School-of-magic classification: Illusion");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Necromancy, "Dnd5e.Magic.School.Necromancy", "School-of-magic classification: Necromancy");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Transmutation, "Dnd5e.Magic.School.Transmutation", "School-of-magic classification: Transmutation");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Psionic, "Dnd5e.Magic.School.Psionic", "Parser-supported Psionic school label; not a standard ninth school or a magical-effect assertion");
	}

	namespace Spell::Trait
	{
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ritual, "Dnd5e.Spell.Trait.Ritual", "Spell definition supports ritual casting; not a selected casting mode");
	}

	namespace Trait
	{
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Concentration, "Dnd5e.Trait.Concentration", "Requires concentration in the context carrying this trait; not active character state");
	}

	namespace Item::Rarity
	{
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(None, "Dnd5e.Item.Rarity.None", "Source explicitly reports no rarity");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Common, "Dnd5e.Item.Rarity.Common", "Common item rarity");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Uncommon, "Dnd5e.Item.Rarity.Uncommon", "Uncommon item rarity");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Rare, "Dnd5e.Item.Rarity.Rare", "Rare item rarity");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(VeryRare, "Dnd5e.Item.Rarity.VeryRare", "Very rare item rarity");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Legendary, "Dnd5e.Item.Rarity.Legendary", "Legendary item rarity");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Artifact, "Dnd5e.Item.Rarity.Artifact", "Artifact item rarity");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Unknown, "Dnd5e.Item.Rarity.Unknown", "Source reports unknown rarity");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(UnknownMagic, "Dnd5e.Item.Rarity.UnknownMagic", "Source reports unknown magical rarity");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Varies, "Dnd5e.Item.Rarity.Varies", "Source reports variable rarity");
	}

	namespace Attack::Mode
	{
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Melee, "Dnd5e.Attack.Mode.Melee", "Attack mode, independent of weapon or spell origin: Melee");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ranged, "Dnd5e.Attack.Mode.Ranged", "Attack mode, independent of weapon or spell origin: Ranged");
	}

	namespace Damage::Type
	{
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Acid, "Dnd5e.Damage.Type.Acid", "Reference classification: Acid");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Bludgeoning, "Dnd5e.Damage.Type.Bludgeoning", "Reference classification: Bludgeoning");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cold, "Dnd5e.Damage.Type.Cold", "Reference classification: Cold");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Fire, "Dnd5e.Damage.Type.Fire", "Reference classification: Fire");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Force, "Dnd5e.Damage.Type.Force", "Reference classification: Force");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Lightning, "Dnd5e.Damage.Type.Lightning", "Reference classification: Lightning");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Necrotic, "Dnd5e.Damage.Type.Necrotic", "Reference classification: Necrotic");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Piercing, "Dnd5e.Damage.Type.Piercing", "Reference classification: Piercing");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Poison, "Dnd5e.Damage.Type.Poison", "Reference classification: Poison");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Psychic, "Dnd5e.Damage.Type.Psychic", "Reference classification: Psychic");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Radiant, "Dnd5e.Damage.Type.Radiant", "Reference classification: Radiant");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Slashing, "Dnd5e.Damage.Type.Slashing", "Reference classification: Slashing");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Thunder, "Dnd5e.Damage.Type.Thunder", "Reference classification: Thunder");
	}

	namespace Condition::Type
	{
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Blinded, "Dnd5e.Condition.Type.Blinded", "Reference classification: Blinded");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Charmed, "Dnd5e.Condition.Type.Charmed", "Reference classification: Charmed");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Deafened, "Dnd5e.Condition.Type.Deafened", "Reference classification: Deafened");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Frightened, "Dnd5e.Condition.Type.Frightened", "Reference classification: Frightened");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Grappled, "Dnd5e.Condition.Type.Grappled", "Reference classification: Grappled");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Incapacitated, "Dnd5e.Condition.Type.Incapacitated", "Reference classification: Incapacitated");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Invisible, "Dnd5e.Condition.Type.Invisible", "Reference classification: Invisible");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Paralyzed, "Dnd5e.Condition.Type.Paralyzed", "Reference classification: Paralyzed");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Petrified, "Dnd5e.Condition.Type.Petrified", "Reference classification: Petrified");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Poisoned, "Dnd5e.Condition.Type.Poisoned", "Reference classification: Poisoned");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Prone, "Dnd5e.Condition.Type.Prone", "Reference classification: Prone");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Restrained, "Dnd5e.Condition.Type.Restrained", "Reference classification: Restrained");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Stunned, "Dnd5e.Condition.Type.Stunned", "Reference classification: Stunned");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Unconscious, "Dnd5e.Condition.Type.Unconscious", "Reference classification: Unconscious");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Exhaustion, "Dnd5e.Condition.Type.Exhaustion", "Exhaustion classification; edition-specific levels and effects are represented separately");
	}

	namespace Ability
	{
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Strength, "Dnd5e.Ability.Strength", "Reference classification: Strength");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Dexterity, "Dnd5e.Ability.Dexterity", "Reference classification: Dexterity");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Constitution, "Dnd5e.Ability.Constitution", "Reference classification: Constitution");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Intelligence, "Dnd5e.Ability.Intelligence", "Reference classification: Intelligence");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Wisdom, "Dnd5e.Ability.Wisdom", "Reference classification: Wisdom");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Charisma, "Dnd5e.Ability.Charisma", "Reference classification: Charisma");
	}

	namespace Alignment
	{
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Lawful, "Dnd5e.Alignment.Lawful", "Reference classification: Lawful");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Neutral, "Dnd5e.Alignment.Neutral", "Reference classification: Neutral");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Chaotic, "Dnd5e.Alignment.Chaotic", "Reference classification: Chaotic");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Good, "Dnd5e.Alignment.Good", "Reference classification: Good");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Evil, "Dnd5e.Alignment.Evil", "Reference classification: Evil");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Unaligned, "Dnd5e.Alignment.Unaligned", "Reference classification: Unaligned");
	}

	namespace Creature::Type
	{
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Aberration, "Dnd5e.Creature.Type.Aberration", "Reference classification: Aberration");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Beast, "Dnd5e.Creature.Type.Beast", "Reference classification: Beast");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Celestial, "Dnd5e.Creature.Type.Celestial", "Reference classification: Celestial");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Construct, "Dnd5e.Creature.Type.Construct", "Reference classification: Construct");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Dragon, "Dnd5e.Creature.Type.Dragon", "Reference classification: Dragon");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Elemental, "Dnd5e.Creature.Type.Elemental", "Reference classification: Elemental");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Fey, "Dnd5e.Creature.Type.Fey", "Reference classification: Fey");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Giant, "Dnd5e.Creature.Type.Giant", "Reference classification: Giant");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Humanoid, "Dnd5e.Creature.Type.Humanoid", "Reference classification: Humanoid");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Monstrosity, "Dnd5e.Creature.Type.Monstrosity", "Reference classification: Monstrosity");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ooze, "Dnd5e.Creature.Type.Ooze", "Reference classification: Ooze");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Plant, "Dnd5e.Creature.Type.Plant", "Reference classification: Plant");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Undead, "Dnd5e.Creature.Type.Undead", "Reference classification: Undead");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Fiend, "Dnd5e.Creature.Type.Fiend", "Creature-type classification: fiend");
	}

	namespace Record::Relationship
	{
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(ReprintedAs, "Dnd5e.Record.Relationship.ReprintedAs", "Reference classification: ReprintedAs");
	}

	namespace Scaling::Input
	{
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(SpellSlotLevel, "Dnd5e.Scaling.Input.SpellSlotLevel", "Scaling input represents a spell slot level");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(PsiPoints, "Dnd5e.Scaling.Input.PsiPoints", "Scaling input represents a psi-point expenditure");
	}

	namespace Rules::Edition
	{
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Original2014, "Dnd5e.Rules.Edition.Original2014", "Explicit original fifth-edition classification");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Revised2024, "Dnd5e.Rules.Edition.Revised2024", "Explicit revised fifth-edition classification");
	}
}
