#include "Browser/Dnd5eSpellBrowserDetails.h"
#include "Browser/Dnd5eBrowserText.h"
#include "Dnd5eTags.h"
#include "Spells/Dnd5eSpell.h"

#define LOCTEXT_NAMESPACE "Dnd5eSpellBrowserDetails"

namespace
{
	FString TagLabel(const FGameplayTag& Tag)
	{
	if (!Tag.IsValid()) { return {}; }
	const FString Name = Tag.ToString();
	int32 Separator;
	return Name.FindLastChar(TEXT('.'), Separator) ? Name.Mid(Separator + 1) : Name;
	}

	void AddLine(TArray<FString>& Lines, const FText& Pattern, const FString& Value)
	{
	if (!Value.IsEmpty()) { Lines.Add(FText::Format(Pattern, FText::FromString(Value)).ToString()); }
	}

	void AppendNote(FString& Value, const TArray<FDnd5eContentBlock>& Blocks)
	{
	if (Blocks.IsEmpty()) { return; }
	if (!Value.IsEmpty()) { Value += TEXT(" — "); }
	Value += Dnd5eBrowserText::FormatDescription(Blocks).ToString();
	}

	FString FormatCastingTimes(const TArray<FDnd5eCastingTime>& Times)
	{
	TArray<FString> Parts;
	for (const auto& Time : Times)
	{
		if (!Time.Unit.IsValid()) { continue; }
		FString Value = TEXT("Special");
		if (Time.Unit != Dnd5e::Activation::Time::Special.GetTag())
		{
			FString Unit = Time.Unit == Dnd5e::Activation::Time::BonusAction.GetTag()
				? TEXT("bonus action") : TagLabel(Time.Unit).ToLower();
			if (Time.Quantity != 1) { Unit += TEXT("s"); }
			Value = FText::AsNumber(Time.Quantity).ToString() + TEXT(" ") + Unit;
		}
		AppendNote(Value, Time.Condition);
		AppendNote(Value, Time.Note);
		Parts.Add(MoveTemp(Value));
	}
	return FString::Join(Parts, TEXT(" / "));
	}

	FString FormatRange(const FDnd5eSpell& Spell)
	{
	const auto& Range = Spell.Range;
	FString Value = Range.bHasDistance ? Dnd5eBrowserText::FormatDistance(Range.Distance) : TagLabel(Range.Kind);
	if (Range.Kind == Dnd5e::Range::Kind::UnlimitedSamePlane.GetTag()) { Value = TEXT("Unlimited on the same plane"); }
	if (Range.Kind == Dnd5e::Range::Kind::Self.GetTag())
	{
		TArray<FString> Areas;
		for (const auto& Area : Spell.Areas)
		{
			if (Area.Origin != Dnd5e::Area::Origin::Self.GetTag()) { continue; }
			const FString Shape = TagLabel(Area.Geometry.Shape).ToLower();
			if (Shape.IsEmpty()) { continue; }
			TArray<FString> Measures;
			for (const auto& Dimension : Area.Geometry.Dimensions)
			{
				const FString Measure = Dnd5eBrowserText::FormatDistance(Dimension.Measurement);
				if (!Measure.IsEmpty()) { Measures.Add(Measure); }
			}
			Areas.Add(Measures.IsEmpty() ? Shape : FString::Join(Measures, TEXT(" × ")) + TEXT(" ") + Shape);
		}
		if (!Areas.IsEmpty()) { Value += TEXT(" (") + FString::Join(Areas, TEXT(" / ")) + TEXT(")"); }
	}
	AppendNote(Value, Range.Description);
	return Value;
	}

	FString FormatComponents(const FDnd5eComponents& Components)
	{
	using namespace Dnd5e::Component::Type;
	TArray<FString> Types;
	if (Components.Types.HasTagExact(Verbal)) { Types.Add(TEXT("V")); }
	if (Components.Types.HasTagExact(Somatic)) { Types.Add(TEXT("S")); }
	if (Components.Types.HasTagExact(Material) || Components.bHasMaterial) { Types.Add(TEXT("M")); }
	if (Components.Types.HasTagExact(Royalty)) { Types.Add(TEXT("R")); }
	FString Value = FString::Join(Types, TEXT(", "));
	if (Components.bHasMaterial)
	{
		const auto& MaterialValue = Components.Material;
		TArray<FString> Parts;
		if (!MaterialValue.Description.IsEmpty())
		{
			Parts.Add(Dnd5eBrowserText::FormatDescription(MaterialValue.Description).ToString());
		}
		if (MaterialValue.bHasCost)
		{
			TArray<FString> Costs;
			for (const auto& Cost : MaterialValue.Cost)
			{
	const FString Amount = Dnd5eBrowserText::FormatNumber(Cost.Amount);
	if (Amount.IsEmpty()) { continue; }
	const FString Unit = Dnd5eBrowserText::FormatCurrency(Cost.Currency);
	Costs.Add(Unit.IsEmpty() ? Amount : Amount + TEXT(" ") + Unit);
			}
			if (!Costs.IsEmpty()) { Parts.Add(TEXT("Cost: ") + FString::Join(Costs, TEXT(", "))); }
		}
		switch (MaterialValue.Consumption)
		{
		case EDnd5eMaterialConsumption::Consumed: Parts.Add(TEXT("consumed")); break;
		case EDnd5eMaterialConsumption::Optional: Parts.Add(TEXT("optionally consumed")); break;
		case EDnd5eMaterialConsumption::NotConsumed: Parts.Add(TEXT("not consumed")); break;
		default: break;
		}
		if (!Parts.IsEmpty()) { Value += TEXT(" (") + FString::Join(Parts, TEXT(" | ")) + TEXT(")"); }
	}
	AppendNote(Value, Components.Description);
	return Value;
	}

	FString FormatDurations(const TArray<FDnd5eSpellDuration>& Durations)
	{
	TArray<FString> Parts;
	for (const auto& Entry : Durations)
	{
		const auto& Duration = Entry.Duration;
		FString Value;
		switch (Duration.Kind)
		{
		case EDnd5eDurationKind::Instantaneous: Value = TEXT("Instantaneous"); break;
		case EDnd5eDurationKind::Timed:
			if (Duration.bHasTime)
			{
				const auto& Time = Duration.Time;
				Value = Dnd5eBrowserText::FormatNumber(Time.Amount);
				if (!Value.IsEmpty() && Time.Unit != EDnd5eTimeUnit::Unspecified)
				{
					const bool bOne = (Time.Amount.Kind == EDnd5eNumberKind::Integer && Time.Amount.Integer == 1) ||
						(Time.Amount.Kind == EDnd5eNumberKind::Decimal && Time.Amount.Decimal == 1.0);
					FString Unit = StaticEnum<EDnd5eTimeUnit>()->GetNameStringByValue(static_cast<int64>(Time.Unit)).ToLower();
					Value += TEXT(" ") + Unit + (bOne ? TEXT("") : TEXT("s"));
				}
				if (Duration.bIsMaximum && !Value.IsEmpty()) { Value = TEXT("up to ") + Value; }
			}
			break;
		case EDnd5eDurationKind::Permanent:
		{
			TArray<FString> Endings;
			for (const auto& Ending : Duration.EndingConditions) { Endings.Add(TagLabel(Ending).ToLower()); }
			Value = Endings.IsEmpty() ? TEXT("Permanent") : TEXT("Until ") + FString::Join(Endings, TEXT(" or "));
			break;
		}
		case EDnd5eDurationKind::Special: Value = TEXT("Special"); break;
		default: break;
		}
		if (Entry.Requirements.HasTagExact(Dnd5e::Trait::Concentration))
		{
			Value = Value.IsEmpty() ? TEXT("Concentration") : TEXT("Concentration, ") + Value;
		}
		AppendNote(Value, Duration.Description);
		if (!Value.IsEmpty()) { Parts.Add(MoveTemp(Value)); }
	}
	return FString::Join(Parts, TEXT(" / "));
	}
}

FDnd5eSpellBrowserDetails Dnd5eSpellBrowserDetails::Build(const FDnd5eSpell& Spell)
{
	FDnd5eSpellBrowserDetails Result;
	Result.Title = FText::FromString(Spell.Header.DisplayName);
	Result.Source = Dnd5eBrowserText::FormatSource(Spell.Header);
	TArray<FString> Schools;
	for (const auto& School : Spell.Schools) { Schools.Add(TagLabel(School)); }
	const FString SchoolText = FString::Join(Schools, TEXT(", "));
	FString Classification = SchoolText;
	if (Spell.Level == 0) { Classification = SchoolText.IsEmpty() ? TEXT("Cantrip") : SchoolText + TEXT(" cantrip"); }
	else if (Spell.Level > 0)
	{
		Classification = FText::Format(LOCTEXT("Level", "Level {0}"), FText::AsNumber(Spell.Level)).ToString();
		if (!SchoolText.IsEmpty()) { Classification += TEXT(" ") + SchoolText; }
	}
	if (Spell.Traits.HasTagExact(Dnd5e::Spell::Trait::Ritual))
	{
		Classification += Classification.IsEmpty() ? TEXT("Ritual") : TEXT(" (ritual)");
	}
	Result.Classification = FText::FromString(Classification);

	TArray<FString> Lines;
	AddLine(Lines, LOCTEXT("Cast", "Casting time: {0}"), FormatCastingTimes(Spell.CastingTimes));
	AddLine(Lines, LOCTEXT("Range", "Range: {0}"), FormatRange(Spell));
	AddLine(Lines, LOCTEXT("Components", "Components: {0}"), FormatComponents(Spell.Components));
	AddLine(Lines, LOCTEXT("Duration", "Duration: {0}"), FormatDurations(Spell.Durations));
	Result.Mechanics = FText::FromString(FString::Join(Lines, TEXT("\n")));

	FString Rules;
	Dnd5eBrowserText::AppendBlocks(Spell.Description, Rules, 0);
	if (!Spell.HigherLevelDescription.IsEmpty())
	{
		if (!Rules.IsEmpty()) { Dnd5eBrowserText::AppendPreview(Rules, TEXT("\n")); }
		Dnd5eBrowserText::AppendBlocks(Spell.HigherLevelDescription, Rules, 0);
	}
	Result.Description = Dnd5eBrowserText::FinishPreview(MoveTemp(Rules));
	return Result;
}

#undef LOCTEXT_NAMESPACE
