// Developed by Neko Creative Technologies

#include "Dnd5eTags.h"

namespace Dnd5e
{
	namespace Content
	{
		namespace Block
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
		
		namespace Inline
		{
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Unsupported, "Dnd5e.Content.Inline.Unsupported", "Inline content retaining unsupported source content");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Text, "Dnd5e.Content.Inline.Text", "Tag for inline text content");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Formatted, "Dnd5e.Content.Inline.Formatted", "Inline content containing formatting and child inline items");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Reference, "Dnd5e.Content.Inline.Reference", "Inline content referencing another content item");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Annotation, "Dnd5e.Content.Inline.Annotation", "Inline content containing an annotation value and display content");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Number, "Dnd5e.Content.Inline.Number", "Inline content containing a number value");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(ExternalLink, "Dnd5e.Content.Inline.ExternalLink", "Inline content linking to an external URL");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(AnchorLink, "Dnd5e.Content.Inline.AnchorLink", "Inline content linking to an anchor within the content");
		}
		
		namespace Annotation
		{
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(DiceRoll, "Dnd5e.Content.Annotation.DiceRoll", "Annotation content containing a dice roll expression");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(DamageRoll, "Dnd5e.Content.Annotation.DamageRoll", "Annotation content containing a damage roll expression");
		}
		
		namespace Formatting
		{
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Bold, "Dnd5e.Content.Formatting.Bold", "Tag for bold formatting");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Italic, "Dnd5e.Content.Formatting.Italic", "Tag for italic formatting");
		}
		
		namespace Presentation
		{
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(ReadAloud, "Dnd5e.Content.Presentation.ReadAloud", "Tag for content that should be read aloud");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(ExpectsLightBackground, "Dnd5e.Content.Presentation.ExpectsLightBackground", "Tag for content that expects a light background");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(ExpectsDarkBackground, "Dnd5e.Content.Presentation.ExpectsDarkBackground", "Tag for content that expects a dark background");
		}
		
		namespace ImageRole
		{
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Map, "Dnd5e.Content.ImageRole.Map", "Tag for images that are maps");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(PlayerMap, "Dnd5e.Content.ImageRole.Map.Player", "Tag for images that are player maps");
		}
		
	}

	namespace Category
	{
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Spell, "Dnd5e.Category.Spell", "Tag for spell content");
	}

	namespace Range
	{
		namespace Kind
		{
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Distance, "Dnd5e.Range.Kind.Distance", "Tag for range kind Distance");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Self, "Dnd5e.Range.Kind.Self", "Tag for range kind Self");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Touch, "Dnd5e.Range.Kind.Touch", "Tag for range kind Touch");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Sight, "Dnd5e.Range.Kind.Sight", "Tag for range kind Sight");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Unlimited, "Dnd5e.Range.Kind.Unlimited", "Tag for range kind Unlimited");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(UnlimitedSamePlane, "Dnd5e.Range.Kind.UnlimitedSamePlane", "Tag for range kind UnlimitedSamePlane");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Special, "Dnd5e.Range.Kind.Special", "Tag for range kind Special");
		}
	}

	namespace Area
	{
		namespace Shape
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

		namespace Origin
		{
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Self, "Dnd5e.Area.Origin.Self", "Tag for area origin Self");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(SelectedPoint, "Dnd5e.Area.Origin.SelectedPoint", "Tag for area origin SelectedPoint");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Target, "Dnd5e.Area.Origin.Target", "Tag for area origin Target");
		}
	}
	
	namespace Duration
	{
		namespace Ending
		{
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Dispelled, "Dnd5e.Duration.Ending.Dispelled", "Tag for duration ending Dispelled");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Triggered, "Dnd5e.Duration.Ending.Triggered", "Tag for duration ending Triggered");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Discharged, "Dnd5e.Duration.Ending.Discharged", "Tag for duration ending Discharged");
		}
	}

	namespace Component
	{
		namespace Type
		{
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Verbal, "Dnd5e.Component.Type.Verbal", "Tag for component type Verbal");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Somatic, "Dnd5e.Component.Type.Somatic", "Tag for component type Somatic");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Material, "Dnd5e.Component.Type.Material", "Tag for component type Material");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Royalty, "Dnd5e.Component.Type.Royalty", "Tag for component type Royalty");
		}
	}

	namespace Spell
	{
		namespace Duration
		{
			namespace Requirement
			{
				UE_DEFINE_GAMEPLAY_TAG_COMMENT(Concentration, "Dnd5e.Spell.Duration.Requirement.Concentration", "Tag for spell duration requirement Concentration");
			}
		}

		namespace CastingTime
		{
			namespace Unit
			{
				UE_DEFINE_GAMEPLAY_TAG_COMMENT(Action, "Dnd5e.Spell.CastingTime.Unit.Action", "Tag for spell casting time Action");
				UE_DEFINE_GAMEPLAY_TAG_COMMENT(BonusAction, "Dnd5e.Spell.CastingTime.Unit.BonusAction", "Tag for spell casting time BonusAction");
				UE_DEFINE_GAMEPLAY_TAG_COMMENT(Reaction, "Dnd5e.Spell.CastingTime.Unit.Reaction", "Tag for spell casting time Reaction");
				UE_DEFINE_GAMEPLAY_TAG_COMMENT(Minute, "Dnd5e.Spell.CastingTime.Unit.Minute", "Tag for spell casting time Minute");
				UE_DEFINE_GAMEPLAY_TAG_COMMENT(Hour, "Dnd5e.Spell.CastingTime.Unit.Hour", "Tag for spell casting time Hour");
			}
		}
	}
}
