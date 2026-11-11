#include "Browser/Dnd5eBrowserText.h"
#include "Content/Dnd5eContentLibrary.h"
#include "Records/Dnd5eRecordHeader.h"
#include "Values/Dnd5eDistance.h"
#include "Values/Dnd5eMoney.h"

#define LOCTEXT_NAMESPACE "Dnd5eBrowserText"

namespace Dnd5eBrowserText
{
	namespace
	{
		constexpr int32 MaxPreviewDepth = 24;
	}

	FString FormatNumber(const FDnd5eNumber& Number)
	{
		switch (Number.Kind)
		{
		case EDnd5eNumberKind::Integer: return FText::AsNumber(Number.Integer).ToString();
		case EDnd5eNumberKind::Decimal:
			return Number.SourceLiteral.IsEmpty() ? FText::AsNumber(Number.Decimal).ToString() : Number.SourceLiteral;
		case EDnd5eNumberKind::Literal: return Number.SourceLiteral;
		default: return {};
		}
	}

	FString FormatCurrency(EDnd5eCurrency Currency)
	{
		switch (Currency)
		{
		case EDnd5eCurrency::Copper: return TEXT("cp");
		case EDnd5eCurrency::Silver: return TEXT("sp");
		case EDnd5eCurrency::Electrum: return TEXT("ep");
		case EDnd5eCurrency::Gold: return TEXT("gp");
		case EDnd5eCurrency::Platinum: return TEXT("pp");
		default: return {};
		}
	}

	FString FormatDistance(const FDnd5eDistance& Distance)
	{
		const FString Amount = FormatNumber(Distance.Amount);
		if (Amount.IsEmpty()) { return {}; }
		switch (Distance.Unit)
		{
		case EDnd5eDistanceUnit::Inches: return Amount + TEXT(" in");
		case EDnd5eDistanceUnit::Feet: return Amount + TEXT(" ft");
		case EDnd5eDistanceUnit::Yards: return Amount + TEXT(" yd");
		case EDnd5eDistanceUnit::Miles: return Amount + TEXT(" mi");
		default: return Amount;
		}
	}

	FText FormatSource(const FDnd5eRecordHeader& Header)
	{
		if (Header.Provenance.IsEmpty()) { return FText::GetEmpty(); }
		const auto& Origin = Header.Provenance[0];
		const FText Source = FText::FromString(Origin.SourceId);
		FText Page;
		switch (Origin.Page.Kind)
		{
		case EDnd5ePageReferenceKind::Number: Page = FText::AsNumber(Origin.Page.Number); break;
		case EDnd5ePageReferenceKind::Text: Page = FText::FromString(Origin.Page.Text); break;
		default: break;
		}
		if (Page.IsEmpty()) { return Source; }
		if (Source.IsEmpty()) { return FText::Format(LOCTEXT("PageOnly", "p. {0}"), Page); }
		return FText::Format(LOCTEXT("SourcePage", "{0} p. {1}"), Source, Page);
	}

	void AppendPreview(FString& Out, const FString& Text)
	{
		const int32 Remaining = MaxPreviewChars - Out.Len();
		if (Remaining > 0) { Out += Text.Left(Remaining); }
	}

	void AppendInlines(const TArray<FDnd5eInlineContent>& Inlines, FString& Out, int32 Depth)
	{
		if (Depth >= MaxPreviewDepth)
		{
			AppendPreview(Out, LOCTEXT("InlineDepth", "[More in typed details]").ToString());
			return;
		}
		for (const auto& Inline : Inlines)
		{
			if (Out.Len() >= MaxPreviewChars) { return; }
			FDnd5eTextContent Text;
			FDnd5eFormattedContent Formatted;
			FDnd5eReferenceContent Reference;
			FDnd5eAnnotationContent Annotation;
			FDnd5eNumberContent Number;
			FDnd5eExternalLinkContent Link;
			FDnd5eUnsupportedContent Unsupported;
			if (UDnd5eContentLibrary::TryGetInlineText(Inline, Text)) { AppendPreview(Out, Text.Text); }
			else if (UDnd5eContentLibrary::TryGetFormatted(Inline, Formatted)) { AppendInlines(Formatted.Inlines, Out, Depth + 1); }
			else if (UDnd5eContentLibrary::TryGetReference(Inline, Reference))
			{
				if (!Reference.Display.IsEmpty()) { AppendInlines(Reference.Display, Out, Depth + 1); }
				else if (Reference.bHasTargetSelector) { AppendPreview(Out, Reference.TargetSelector.Name); }
				else { AppendPreview(Out, LOCTEXT("Reference", "[Reference]").ToString()); }
			}
			else if (UDnd5eContentLibrary::TryGetAnnotation(Inline, Annotation))
			{
				if (!Annotation.Display.IsEmpty()) { AppendInlines(Annotation.Display, Out, Depth + 1); }
				else
				{
					FDnd5eDiceExpression Dice;
					if (UDnd5eContentLibrary::TryGetAnnotationDiceExpression(Annotation, Dice)) { AppendPreview(Out, Dice.Expression); }
					else { AppendPreview(Out, LOCTEXT("Annotation", "[Annotation]").ToString()); }
				}
			}
			else if (UDnd5eContentLibrary::TryGetNumber(Inline, Number))
			{
				if (!Number.Display.IsEmpty()) { AppendInlines(Number.Display, Out, Depth + 1); }
				else { AppendPreview(Out, FormatNumber(Number.Value)); }
			}
			else if (UDnd5eContentLibrary::TryGetExternalLink(Inline, Link))
			{
				if (!Link.Display.IsEmpty()) { AppendInlines(Link.Display, Out, Depth + 1); }
				else { AppendPreview(Out, Link.Url); }
			}
			else if (UDnd5eContentLibrary::TryGetUnsupportedInline(Inline, Unsupported) && !Unsupported.DisplayText.IsEmpty())
			{
				AppendPreview(Out, Unsupported.DisplayText);
			}
			else { AppendPreview(Out, LOCTEXT("InlineFallback", "[See typed details]").ToString()); }
		}
	}

	void AppendBlocks(const TArray<FDnd5eContentBlock>& Blocks, FString& Out, int32 Depth)
	{
		if (Depth >= MaxPreviewDepth)
		{
			AppendPreview(Out, LOCTEXT("BlockDepth", "[More in typed details]").ToString());
			return;
		}
		for (const auto& Block : Blocks)
		{
			if (Out.Len() >= MaxPreviewChars) { return; }
			FDnd5eParagraphContent Paragraph;
			FDnd5eSectionContent Section;
			FDnd5eListContent List;
			FDnd5eInsetContent Inset;
			FDnd5eQuoteContent Quote;
			FDnd5eUnsupportedContent Unsupported;
			if (UDnd5eContentLibrary::TryGetParagraph(Block, Paragraph)) { AppendInlines(Paragraph.Inlines, Out, Depth + 1); }
			else if (UDnd5eContentLibrary::TryGetSection(Block, Section))
			{
				AppendInlines(Section.Heading, Out, Depth + 1);
				if (!Section.Heading.IsEmpty()) { AppendPreview(Out, TEXT("\n")); }
				AppendBlocks(Section.Blocks, Out, Depth + 1);
			}
			else if (UDnd5eContentLibrary::TryGetList(Block, List))
			{
				AppendInlines(List.Heading, Out, Depth + 1);
				if (!List.Heading.IsEmpty()) { AppendPreview(Out, TEXT("\n")); }
				for (const auto& Entry : List.Items)
				{
					if (Out.Len() >= MaxPreviewChars) { break; }
					AppendPreview(Out, TEXT("• "));
					AppendInlines(Entry.Label, Out, Depth + 1);
					if (!Entry.Label.IsEmpty() && !Entry.Blocks.IsEmpty()) { AppendPreview(Out, TEXT(" ")); }
					AppendBlocks(Entry.Blocks, Out, Depth + 1);
					if (Entry.Blocks.IsEmpty()) { AppendPreview(Out, TEXT("\n")); }
				}
			}
			else if (UDnd5eContentLibrary::TryGetInset(Block, Inset))
			{
				AppendInlines(Inset.Heading, Out, Depth + 1);
				if (!Inset.Heading.IsEmpty()) { AppendPreview(Out, TEXT("\n")); }
				AppendBlocks(Inset.Blocks, Out, Depth + 1);
			}
			else if (UDnd5eContentLibrary::TryGetQuote(Block, Quote))
			{
				AppendBlocks(Quote.Blocks, Out, Depth + 1);
				AppendBlocks(Quote.Attribution, Out, Depth + 1);
				AppendInlines(Quote.Source, Out, Depth + 1);
			}
			else if (UDnd5eContentLibrary::TryGetUnsupportedBlock(Block, Unsupported) && !Unsupported.DisplayText.IsEmpty())
			{
				AppendPreview(Out, Unsupported.DisplayText);
			}
			else { AppendPreview(Out, LOCTEXT("BlockFallback", "[See typed details for this block]").ToString()); }
			AppendPreview(Out, TEXT("\n"));
		}
	}

	FText FinishPreview(FString Description)
	{
		const bool bAtLimit = Description.Len() >= MaxPreviewChars;
		Description.TrimStartAndEndInline();
		if (bAtLimit) { Description += LOCTEXT("Truncated", "\n[Preview shortened. See typed details.]").ToString(); }
		return Description.IsEmpty() ? LOCTEXT("NoDescription", "No description supplied.") : FText::FromString(Description);
	}

	FText FormatDescription(const TArray<FDnd5eContentBlock>& Blocks)
	{
		FString Description;
		AppendBlocks(Blocks, Description, 0);
		return FinishPreview(MoveTemp(Description));
	}
}

#undef LOCTEXT_NAMESPACE