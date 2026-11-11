// Developed by Neko Creative Technologies

#include "Browser/Dnd5eItemBrowserDetails.h"
#include "Content/Dnd5eContentLibrary.h"
#include "Dnd5eTags.h"
#include "Items/Dnd5eItem.h"

#define LOCTEXT_NAMESPACE "Dnd5eItemBrowserDetails"

namespace
{
	constexpr int32 MaxPreviewDepth = 24;
	constexpr int32 MaxPreviewChars = 32768;

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

	FString FormatPrice(const FDnd5ePrice& Price)
	{
		if (Price.CostPresence == EDnd5eValuePresence::ExplicitNull)
		{
			return LOCTEXT("NullPrice", "No listed price").ToString();
		}
		TArray<FString> Parts;
		if (Price.CostPresence == EDnd5eValuePresence::Present)
		{
			for (const FDnd5eMoney& Money : Price.Cost)
			{
				FString Amount = FormatNumber(Money.Amount);
				if (Amount.IsEmpty()) { continue; }
				const FString Unit = FormatCurrency(Money.Currency);
				if (!Unit.IsEmpty()) { Amount += TEXT(" ") + Unit; }
				Parts.Add(MoveTemp(Amount));
			}
		}
		if (Price.bHasMultiplier)
		{
			const FString Factor = FormatNumber(Price.Multiplier);
			if (!Factor.IsEmpty()) { Parts.Add(TEXT("×") + Factor); }
		}
		return FString::Join(Parts, TEXT(", "));
	}

	FString FormatWeight(const FDnd5eMass& Mass)
	{
		const FString Amount = FormatNumber(Mass.Amount);
		if (Amount.IsEmpty()) { return {}; }
		switch (Mass.Unit)
		{
		case EDnd5eMassUnit::Ounces: return Amount + TEXT(" oz");
		case EDnd5eMassUnit::Pounds: return Amount + TEXT(" lb");
		case EDnd5eMassUnit::Grams: return Amount + TEXT(" g");
		case EDnd5eMassUnit::Kilograms: return Amount + TEXT(" kg");
		default: return Amount;
		}
	}

	FString FormatCharges(const FDnd5eItemChargeAmount& Charges)
	{
		switch (Charges.Kind)
		{
		case EDnd5eItemChargeAmountKind::Fixed: return FText::AsNumber(Charges.Fixed).ToString();
		case EDnd5eItemChargeAmountKind::Dice:
			return Charges.Dice.Expression.IsEmpty() ? Charges.SourceLiteral : Charges.Dice.Expression;
		case EDnd5eItemChargeAmountKind::ExplicitNull:
			return LOCTEXT("NullCharges", "No listed charge capacity").ToString();
		default: return {};
		}
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

	void AddFact(TArray<FString>& Facts, const FText& Pattern, const FString& Value)
	{
		if (!Value.IsEmpty()) { Facts.Add(FText::Format(Pattern, FText::FromString(Value)).ToString()); }
	}
}

FText Dnd5eItemBrowserDetails::FormatRarity(const FGameplayTag& Rarity)
{
	using namespace Dnd5e::Item::Rarity;
	if (Rarity == None.GetTag()) { return LOCTEXT("None", "No rarity"); }
	if (Rarity == Common.GetTag()) { return LOCTEXT("Common", "Common"); }
	if (Rarity == Uncommon.GetTag()) { return LOCTEXT("Uncommon", "Uncommon"); }
	if (Rarity == Rare.GetTag()) { return LOCTEXT("Rare", "Rare"); }
	if (Rarity == VeryRare.GetTag()) { return LOCTEXT("VeryRare", "Very rare"); }
	if (Rarity == Legendary.GetTag()) { return LOCTEXT("Legendary", "Legendary"); }
	if (Rarity == Artifact.GetTag()) { return LOCTEXT("Artifact", "Artifact"); }
	if (Rarity == UnknownMagic.GetTag()) { return LOCTEXT("UnknownMagic", "Unknown magic rarity"); }
	if (Rarity == Varies.GetTag()) { return LOCTEXT("Varies", "Varies"); }
	if (Rarity == Unknown.GetTag()) { return LOCTEXT("Unknown", "Unknown rarity"); }
	return Rarity.IsValid() ? FText::FromName(Rarity.GetTagName()) : LOCTEXT("Unspecified", "Rarity unspecified");
}

FDnd5eItemBrowserDetails Dnd5eItemBrowserDetails::Build(const FDnd5eItem& Item)
{
	FDnd5eItemBrowserDetails Result;
	Result.Title = FText::FromString(Item.Header.DisplayName);
	TArray<FString> Facts;
	if (!Item.Header.Provenance.IsEmpty())
	{
		AddFact(Facts, LOCTEXT("Source", "Source: {0}"), Item.Header.Provenance[0].SourceId);
	}
	Facts.Add(FormatRarity(Item.Rarity).ToString());
	AddFact(Facts, LOCTEXT("Price", "Price: {0}"), FormatPrice(Item.Price));
	if (Item.bHasWeight) { AddFact(Facts, LOCTEXT("Weight", "Weight: {0}"), FormatWeight(Item.Weight)); }
	AddFact(Facts, LOCTEXT("Charges", "Charges: {0}"), FormatCharges(Item.ChargeData.Charges));
	Result.Facts = FText::FromString(FString::Join(Facts, TEXT("\n")));

	FString Description;
	AppendBlocks(Item.Description, Description, 0);
	if (Item.bHasSupplementaryContent && !Item.SupplementaryContent.Entries.IsEmpty())
	{
		if (!Description.IsEmpty()) { AppendPreview(Description, TEXT("\n")); }
		AppendBlocks(Item.SupplementaryContent.Entries, Description, 0);
	}
	const bool bAtLimit = Description.Len() >= MaxPreviewChars;
	Description.TrimStartAndEndInline();
	if (bAtLimit) { Description += LOCTEXT("Truncated", "\n[Preview shortened. See typed details.]").ToString(); }
	Result.Description = Description.IsEmpty() ? LOCTEXT("NoDescription", "No description supplied.") : FText::FromString(Description);
	return Result;
}

#undef LOCTEXT_NAMESPACE