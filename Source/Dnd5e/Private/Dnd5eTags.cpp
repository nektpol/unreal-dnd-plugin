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
}