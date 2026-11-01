// Developed by Neko Creative Technologies


#include "Content/Dnd5eContentLibrary.h"
#include "Dnd5eTags.h"
#include "Content/Dnd5eContentType.h"


bool UDnd5eContentLibrary::TryGetSection(const FDnd5eContentBlock& ContentBlock, FDnd5eSectionContent& Section)
{
	if (ContentBlock.Kind == Dnd5e::Content::Block::Section
		&& ContentBlock.Payload.IsValid()
		&& ContentBlock.Payload.GetScriptStruct() == FDnd5eSectionContent::StaticStruct())
	{
		Section = ContentBlock.Payload.Get<FDnd5eSectionContent>();
		return true;
	}
	Section = FDnd5eSectionContent{};
	return false;
}

bool UDnd5eContentLibrary::TryGetParagraph(const FDnd5eContentBlock& ContentBlock, FDnd5eParagraphContent& Paragraph)
{
	if (ContentBlock.Kind == Dnd5e::Content::Block::Paragraph
		&& ContentBlock.Payload.IsValid()
		&& ContentBlock.Payload.GetScriptStruct() == FDnd5eParagraphContent::StaticStruct())
	{
		Paragraph = ContentBlock.Payload.Get<FDnd5eParagraphContent>();
		return true;
	}
	Paragraph = FDnd5eParagraphContent{};
	return false;
}

bool UDnd5eContentLibrary::TryGetInlineText(const FDnd5eInlineContent& InlineContent, FDnd5eTextContent& Text)
{
	if (InlineContent.Kind == Dnd5e::Content::Inline::Text
		&& InlineContent.Payload.IsValid()
		&& InlineContent.Payload.GetScriptStruct() == FDnd5eTextContent::StaticStruct())
	{
		Text = InlineContent.Payload.Get<FDnd5eTextContent>();
		return true;
	}
	Text = FDnd5eTextContent{};
	return false;
}

bool UDnd5eContentLibrary::TryGetFormatted(const FDnd5eInlineContent& InlineContent, FDnd5eFormattedContent& FormattedInline)
{
	if (InlineContent.Kind == Dnd5e::Content::Inline::Formatted
		&& InlineContent.Payload.IsValid()
		&& InlineContent.Payload.GetScriptStruct() == FDnd5eFormattedContent::StaticStruct())
	{
		FormattedInline = InlineContent.Payload.Get<FDnd5eFormattedContent>();
		return true;
	}
	FormattedInline = FDnd5eFormattedContent{};
	return false;
}

bool UDnd5eContentLibrary::TryGetList(const FDnd5eContentBlock& ContentBlock, FDnd5eListContent& List)
{
	if (ContentBlock.Kind == Dnd5e::Content::Block::List
		&& ContentBlock.Payload.IsValid()
		&& ContentBlock.Payload.GetScriptStruct() == FDnd5eListContent::StaticStruct())
	{
		List = ContentBlock.Payload.Get<FDnd5eListContent>();
		return true;
	}
	List = FDnd5eListContent{};
	return false;
}

bool UDnd5eContentLibrary::TryGetTable(const FDnd5eContentBlock& ContentBlock, FDnd5eTableContent& Table)
{
	if (ContentBlock.Kind == Dnd5e::Content::Block::Table
		&& ContentBlock.Payload.IsValid()
		&& ContentBlock.Payload.GetScriptStruct() == FDnd5eTableContent::StaticStruct())
	{
		Table = ContentBlock.Payload.Get<FDnd5eTableContent>();
		return true;
	}
	Table = FDnd5eTableContent{};
	return false;
}

bool UDnd5eContentLibrary::TryGetQuote(const FDnd5eContentBlock& ContentBlock, FDnd5eQuoteContent& Quote)
{
	if (ContentBlock.Kind == Dnd5e::Content::Block::Quote
		&& ContentBlock.Payload.IsValid()
		&& ContentBlock.Payload.GetScriptStruct() == FDnd5eQuoteContent::StaticStruct())
	{
		Quote = ContentBlock.Payload.Get<FDnd5eQuoteContent>();
		return true;
	}
	Quote = FDnd5eQuoteContent{};
	return false;
}

bool UDnd5eContentLibrary::TryGetInset(const FDnd5eContentBlock& ContentBlock, FDnd5eInsetContent& Inset)
{
	if (ContentBlock.Kind == Dnd5e::Content::Block::Inset
		&& ContentBlock.Payload.IsValid()
		&& ContentBlock.Payload.GetScriptStruct() == FDnd5eInsetContent::StaticStruct())
	{
		Inset = ContentBlock.Payload.Get<FDnd5eInsetContent>();
		return true;
	}
	Inset = FDnd5eInsetContent{};
	return false;
}

bool UDnd5eContentLibrary::TryGetReference(const FDnd5eInlineContent& InlineContent, FDnd5eReferenceContent& Reference)
{
	if (InlineContent.Kind == Dnd5e::Content::Inline::Reference
		&& InlineContent.Payload.IsValid()
		&& InlineContent.Payload.GetScriptStruct() == FDnd5eReferenceContent::StaticStruct())
	{
		Reference = InlineContent.Payload.Get<FDnd5eReferenceContent>();
		return true;
	}
	Reference = FDnd5eReferenceContent{};
	return false;
}

bool UDnd5eContentLibrary::TryGetAnnotation(const FDnd5eInlineContent& InlineContent, FDnd5eAnnotationContent& Annotation)
{
	if (InlineContent.Kind == Dnd5e::Content::Inline::Annotation
		&& InlineContent.Payload.IsValid()
		&& InlineContent.Payload.GetScriptStruct() == FDnd5eAnnotationContent::StaticStruct())
	{
		Annotation = InlineContent.Payload.Get<FDnd5eAnnotationContent>();
		return true;
	}
	Annotation = FDnd5eAnnotationContent{};
	return false;
}

bool UDnd5eContentLibrary::TryGetNumber(const FDnd5eInlineContent& InlineContent, FDnd5eNumberContent& Number)
{
	if (InlineContent.Kind == Dnd5e::Content::Inline::Number
		&& InlineContent.Payload.IsValid()
		&& InlineContent.Payload.GetScriptStruct() == FDnd5eNumberContent::StaticStruct())
	{
		Number = InlineContent.Payload.Get<FDnd5eNumberContent>();
		return true;
	}
	Number = FDnd5eNumberContent{};
	return false;
}

bool UDnd5eContentLibrary::TryGetAnnotationDiceExpression(const FDnd5eAnnotationContent& Annotation, FDnd5eDiceExpression& Expression)
{
	if (Annotation.Value.IsValid()
		&& Annotation.Value.GetScriptStruct() == FDnd5eDiceExpression::StaticStruct()
		&& (Annotation.Kind == Dnd5e::Content::Annotation::DiceRoll || Annotation.Kind == Dnd5e::Content::Annotation::DamageRoll))
	{
		Expression = Annotation.Value.Get<FDnd5eDiceExpression>();
		return true;
	}
	Expression = FDnd5eDiceExpression{};
	return false;
}

bool UDnd5eContentLibrary::TryGetExternalLink(const FDnd5eInlineContent& InlineContent, FDnd5eExternalLinkContent& Link)
{
	if (InlineContent.Kind == Dnd5e::Content::Inline::ExternalLink
		&& InlineContent.Payload.IsValid()
		&& InlineContent.Payload.GetScriptStruct() == FDnd5eExternalLinkContent::StaticStruct())
	{
		Link = InlineContent.Payload.Get<FDnd5eExternalLinkContent>();
		return true;
	}
	Link = FDnd5eExternalLinkContent{};
	return false;
}

bool UDnd5eContentLibrary::TryGetAnchorLink(const FDnd5eInlineContent& InlineContent, FDnd5eAnchorLinkContent& Link)
{
	if (InlineContent.Kind == Dnd5e::Content::Inline::AnchorLink
		&& InlineContent.Payload.IsValid()
		&& InlineContent.Payload.GetScriptStruct() == FDnd5eAnchorLinkContent::StaticStruct())
	{
		Link = InlineContent.Payload.Get<FDnd5eAnchorLinkContent>();
		return true;
	}
	Link = FDnd5eAnchorLinkContent{};
	return false;
}

bool UDnd5eContentLibrary::TryGetImage(const FDnd5eContentBlock& ContentBlock, FDnd5eImageContent& Image)
{
	if (ContentBlock.Kind == Dnd5e::Content::Block::Image
		&& ContentBlock.Payload.IsValid()
		&& ContentBlock.Payload.GetScriptStruct() == FDnd5eImageContent::StaticStruct())
	{
		Image = ContentBlock.Payload.Get<FDnd5eImageContent>();
		return true;
	}
	Image = FDnd5eImageContent{};
	return false;
}

bool UDnd5eContentLibrary::TryGetGallery(const FDnd5eContentBlock& ContentBlock, FDnd5eGalleryContent& Gallery)
{
	if (ContentBlock.Kind == Dnd5e::Content::Block::Gallery
		&& ContentBlock.Payload.IsValid()
		&& ContentBlock.Payload.GetScriptStruct() == FDnd5eGalleryContent::StaticStruct())
	{
		Gallery = ContentBlock.Payload.Get<FDnd5eGalleryContent>();
		return true;
	}
	Gallery = FDnd5eGalleryContent{};
	return false;
}

bool UDnd5eContentLibrary::TryGetUnsupportedBlock(const FDnd5eContentBlock& ContentBlock, FDnd5eUnsupportedContent& Unsupported)
{
	if (ContentBlock.Kind == Dnd5e::Content::Block::Unsupported
		&& ContentBlock.Payload.IsValid()
		&& ContentBlock.Payload.GetScriptStruct() == FDnd5eUnsupportedContent::StaticStruct())
	{
		Unsupported = ContentBlock.Payload.Get<FDnd5eUnsupportedContent>();
		return true;
	}
	Unsupported = FDnd5eUnsupportedContent{};
	return false;
}

bool UDnd5eContentLibrary::TryGetUnsupportedInline(const FDnd5eInlineContent& InlineContent, FDnd5eUnsupportedContent& Unsupported)
{
	if (InlineContent.Kind == Dnd5e::Content::Inline::Unsupported
		&& InlineContent.Payload.IsValid()
		&& InlineContent.Payload.GetScriptStruct() == FDnd5eUnsupportedContent::StaticStruct())
	{
		Unsupported = InlineContent.Payload.Get<FDnd5eUnsupportedContent>();
		return true;
	}
	Unsupported = FDnd5eUnsupportedContent{};
	return false;
}

bool UDnd5eContentLibrary::TryGetAnnotationDifficultyClass(const FDnd5eAnnotationContent& Annotation, FDnd5eNumber& Value)
{
	Value = {};
	if (Annotation.Kind != Dnd5e::Content::Annotation::DifficultyClass || Annotation.Value.GetScriptStruct() != FDnd5eNumber::StaticStruct()) { return false; }
	Value = Annotation.Value.Get<FDnd5eNumber>();
	return true;
}

bool UDnd5eContentLibrary::TryGetAnnotationD20Roll(const FDnd5eAnnotationContent& Annotation, FDnd5eD20Roll& Value)
{
	Value = {};
	if ((Annotation.Kind != Dnd5e::Content::Annotation::AttackRoll && Annotation.Kind != Dnd5e::Content::Annotation::D20Roll) ||
		Annotation.Value.GetScriptStruct() != FDnd5eD20Roll::StaticStruct()) { return false; }
	Value = Annotation.Value.Get<FDnd5eD20Roll>();
	return true;
}

bool UDnd5eContentLibrary::TryGetAnnotationPercentChance(const FDnd5eAnnotationContent& Annotation, FDnd5eChanceContentValue& Value)
{
	Value = {};
	if (Annotation.Kind != Dnd5e::Content::Annotation::PercentChance || Annotation.Value.GetScriptStruct() != FDnd5eChanceContentValue::StaticStruct()) { return false; }
	Value = Annotation.Value.Get<FDnd5eChanceContentValue>();
	return true;
}

bool UDnd5eContentLibrary::TryGetAnnotationScaledRoll(const FDnd5eAnnotationContent& Annotation, FDnd5eScaledRoll& Value)
{
	Value = {};
	if ((Annotation.Kind != Dnd5e::Content::Annotation::ScaledDiceRoll && Annotation.Kind != Dnd5e::Content::Annotation::ScaledDamageRoll) ||
		Annotation.Value.GetScriptStruct() != FDnd5eScaledRoll::StaticStruct()) { return false; }
	Value = Annotation.Value.Get<FDnd5eScaledRoll>();
	return true;
}

bool UDnd5eContentLibrary::TryGetDocumentLink(const FDnd5eInlineContent& Inline, FDnd5eDocumentLinkContent& Value)
{
	Value = {};
	if (Inline.Kind != Dnd5e::Content::Inline::DocumentLink || Inline.Payload.GetScriptStruct() != FDnd5eDocumentLinkContent::StaticStruct()) { return false; }
	Value = Inline.Payload.Get<FDnd5eDocumentLinkContent>(); return true;
}

bool UDnd5eContentLibrary::TryGetQuickReference(const FDnd5eInlineContent& Inline, FDnd5eQuickReferenceContent& Value)
{
	Value = {};
	if (Inline.Kind != Dnd5e::Content::Inline::QuickReference || Inline.Payload.GetScriptStruct() != FDnd5eQuickReferenceContent::StaticStruct()) { return false; }
	Value = Inline.Payload.Get<FDnd5eQuickReferenceContent>(); return true;
}

bool UDnd5eContentLibrary::TryGetQueryLink(const FDnd5eInlineContent& Inline, FDnd5eQueryLinkContent& Value)
{
	Value = {};
	if (Inline.Kind != Dnd5e::Content::Inline::QueryLink || Inline.Payload.GetScriptStruct() != FDnd5eQueryLinkContent::StaticStruct()) { return false; }
	Value = Inline.Payload.Get<FDnd5eQueryLinkContent>(); return true;
}
