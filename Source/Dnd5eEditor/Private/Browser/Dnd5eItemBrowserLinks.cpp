// Developed by Neko Creative Technologies

#include "Browser/Dnd5eItemBrowserLinks.h"
#include "Browser/Dnd5eItemBrowserDetails.h"
#include "Catalog/Dnd5eItemCatalogObject.h"
#include "Catalog/Dnd5eItemDefinitionCatalogObject.h"
#include "Catalog/Dnd5eSpellCatalogObject.h"
#include "Dnd5eTags.h"
#include "Items/Dnd5eItemLibrary.h"

#define LOCTEXT_NAMESPACE "Dnd5eItemBrowserLinks"

namespace
{
	template<typename TStatus>
	FText MatchStatus(TStatus Status)
	{
		switch (Status)
		{
		case TStatus::Matched: return LOCTEXT("Available", "Available");
		case TStatus::MissingSource: return LOCTEXT("MissingSource", "Source omitted");
		case TStatus::InvalidSelector: return LOCTEXT("Invalid", "Invalid reference");
		case TStatus::NoMatch: return LOCTEXT("NoMatch", "Target unavailable");
		default: return LOCTEXT("NotAttempted", "Catalog unavailable");
		}
	}

	template<typename TStatus>
	FDnd5eItemBrowserLink MakeLink(EDnd5eItemBrowserLinkKind Kind, const FText& Label, TStatus Status,
		const TArray<FDnd5eRecordId>& Candidates)
	{
		FDnd5eItemBrowserLink Link{Kind, Label, MatchStatus(Status), {}};
		if (Status == TStatus::Matched && Candidates.Num() == 1) { Link.Target = Candidates[0]; }
		return Link;
	}

	FString SelectorLabel(const FString& Name, bool bHasSource, const FString& Source)
	{
		return bHasSource ? Name + TEXT(" | ") + Source : Name;
	}
}

TArray<FDnd5eItemBrowserLink> Dnd5eItemBrowserLinks::Build(const FDnd5eItem& Item, const FDnd5eItemGroup* Group,
	const UDnd5eItemCatalogObject* Items, const UDnd5eItemDefinitionCatalogObject* Definitions,
	const UDnd5eSpellCatalogObject* Spells)
{
	TArray<FDnd5eItemBrowserLink> Links;
	TArray<FDnd5eRecordId> Candidates;
	TArray<FDnd5eDiagnostic> Diagnostics;
	const FString& Namespace = Item.Header.RecordId.DatasetNamespace;
	using EKind = EDnd5eItemBrowserLinkKind;
	using EDefinition = EDnd5eItemDefinitionMatchStatus;
	using EMember = EDnd5eItemSelectorMatchStatus;
	using ESpell = EDnd5eSpellSelectorMatchStatus;

	if (Group)
	{
		for (const auto& Member : Group->Members)
		{
			Candidates.Reset();
			const auto Status = Items ? Items->MatchItemSelector(Namespace, Member, Candidates) : EMember::NotAttempted;
			const FText Label = FText::Format(LOCTEXT("Member", "Member: {0}"),
				FText::FromString(SelectorLabel(Member.Name, Member.bHasSource, Member.SourceId)));
			auto& Link = Links.Add_GetRef(MakeLink(EKind::Member, Label, Status, Candidates));
			if (Status == EMember::Ambiguous) { Link.Status = LOCTEXT("MemberAmbiguous", "Multiple item matches"); }
		}
	}

	const auto AddDefinition = [&](const FDnd5eItemDefinitionSelector& Selector, const FText& Pattern)
	{
		Candidates.Reset();
		const auto Status = Definitions ? Definitions->MatchDefinitionSelector(Namespace, Selector, Candidates, Diagnostics)
			: EDefinition::NotAttempted;
		const FText Label = FText::Format(Pattern,
			FText::FromString(SelectorLabel(Selector.Abbreviation, Selector.bHasSource, Selector.SourceId)));
		Links.Add(MakeLink(EKind::Definition, Label, Status, Candidates));
	};
	if (Item.bHasType) { AddDefinition(Item.Type, LOCTEXT("Type", "Type: {0}")); }
	for (const auto& Property : Item.Properties) { AddDefinition(Property, LOCTEXT("Property", "Property: {0}")); }
	for (const auto& Mastery : Item.Masteries)
	{
		Candidates.Reset();
		const auto Status = Definitions ? Definitions->MatchMasterySelector(Namespace, Mastery.Selector, Candidates, Diagnostics)
			: EDefinition::NotAttempted;
		const auto& Selector = Mastery.Selector;
		const FText Label = FText::Format(LOCTEXT("Mastery", "Mastery: {0}"),
			FText::FromString(SelectorLabel(Selector.Name, Selector.bHasSource, Selector.SourceId)));
		Links.Add(MakeLink(EKind::Definition, Label, Status, Candidates));
	}

	for (const auto& Attachment : Item.AttachedSpells.Spells)
	{
		Candidates.Reset();
		const auto Status = Spells ? UDnd5eItemLibrary::MatchItemAttachedSpell(Namespace, Attachment, Spells, Candidates, Diagnostics)
			: ESpell::NotAttempted;
		const auto& Selector = Attachment.Selector;
		const FText Label = FText::Format(LOCTEXT("Spell", "Spell: {0}"),
			FText::FromString(SelectorLabel(Selector.Name, Selector.bHasSource, Selector.SourceId)));
		auto& Link = Links.Add_GetRef(MakeLink(EKind::Spell, Label, Status, Candidates));
		if (Status == ESpell::Ambiguous) { Link.Status = LOCTEXT("SpellAmbiguous", "Multiple spell matches"); }
	}
	return Links;
}

bool Dnd5eItemBrowserLinks::ReadDefinition(const UDnd5eItemDefinitionCatalogObject& Catalog, const FDnd5eRecordId& Id,
	FText& Title, FText& Description)
{
	Title = FText::GetEmpty();
	Description = FText::GetEmpty();
	const auto SetPreview = [&](const auto& Definition)
	{
		Title = FText::FromString(Definition.Header.DisplayName);
		Description = Dnd5eItemBrowserDetails::FormatDescription(Definition.Description);
		return true;
	};
	FDnd5eItemTypeDefinition Type;
	FDnd5eItemPropertyDefinition Property;
	FDnd5eItemMasteryDefinition Mastery;
	if (Id.Category == Dnd5e::Category::ItemType.GetTag() && Catalog.TryGetType(Id, Type)) { return SetPreview(Type); }
	if (Id.Category == Dnd5e::Category::ItemProperty.GetTag() && Catalog.TryGetProperty(Id, Property)) { return SetPreview(Property); }
	if (Id.Category == Dnd5e::Category::ItemMastery.GetTag() && Catalog.TryGetMastery(Id, Mastery)) { return SetPreview(Mastery); }
	return false;
}

#undef LOCTEXT_NAMESPACE