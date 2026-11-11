// Developed by Neko Creative Technologies

#include "Catalog/Dnd5eItemCatalog.h"
#include "Items/Dnd5eItemIdentity.h"
#include "Dnd5eTags.h"

namespace
{
	constexpr int32 MaxItemCatalogRecords = 100000;

	bool FailItemCatalog(TArray<FDnd5eDiagnostic>& Diagnostics, FName Code, const TCHAR* Message,
		const FDnd5eRecordHeader* Header = nullptr)
	{
		FDnd5eDiagnostic Diagnostic;
		Diagnostic.Severity = EDnd5eDiagnosticSeverity::Error;
		Diagnostic.Code = Code;
		Diagnostic.Message = Message;
		if (Header && !Header->Provenance.IsEmpty())
		{
			Diagnostic.RelativeFilePath = Header->Provenance[0].RelativeFilePath;
			Diagnostic.JsonPointer = Header->Provenance[0].JsonPointer;
		}
		Diagnostics.Add(MoveTemp(Diagnostic));
		return false;
	}

	bool IsItemCatalogIdentityText(const FString& Text)
	{
		if (Text.TrimStartAndEnd().IsEmpty()) { return false; }
		for (const TCHAR Character : Text)
		{
			if (FChar::IsControl(Character)) { return false; }
		}
		return true;
	}

	bool ValidateItemCatalogRecord(const FDnd5eRecordHeader& Header, const FGameplayTag& ExpectedCategory,
		TArray<FDnd5eDiagnostic>& Diagnostics)
	{
		const auto& Id = Header.RecordId;
		if (Id.Category != ExpectedCategory || !IsItemCatalogIdentityText(Id.DatasetNamespace) ||
			!IsItemCatalogIdentityText(Id.SourceKey) || !IsItemCatalogIdentityText(Header.DisplayName))
		{
			return FailItemCatalog(Diagnostics, TEXT("Dnd5e.Catalog.InvalidItemRecord"),
				TEXT("An item catalog record needs its expected category, complete ID and display name."), &Header);
		}
		return true;
	}

	bool MatchesItemCatalogName(const FDnd5eRecordHeader& Header, const FDnd5eItemCatalogQuery& Query)
	{
		if (Query.NameContains.IsEmpty() || Header.DisplayName.Contains(Query.NameContains, ESearchCase::IgnoreCase))
		{
			return true;
		}
		return Query.bIncludeAliases && Header.Aliases.ContainsByPredicate([&Query](const FString& Alias)
		{
			return Alias.Contains(Query.NameContains, ESearchCase::IgnoreCase);
		});
	}
}

bool FDnd5eItemCatalog::ReplaceRecords(const TArray<FDnd5eItem>& Items, const TArray<FDnd5eItemGroup>& Groups,
	TArray<FDnd5eDiagnostic>& Diagnostics)
{
	Diagnostics.Reset();
	if (int64(Items.Num()) + Groups.Num() > MaxItemCatalogRecords)
	{
		return FailItemCatalog(Diagnostics, TEXT("Dnd5e.Catalog.ResourceLimitExceeded"),
			TEXT("An item catalog accepts at most 100000 records."));
	}

	TMap<FDnd5eRecordId, FDnd5eItem> CandidateItems;
	TMap<FDnd5eRecordId, FDnd5eItemGroup> CandidateGroups;
	TArray<FDnd5eRecordId> CandidateIds;
	CandidateItems.Reserve(Items.Num());
	CandidateGroups.Reserve(Groups.Num());
	CandidateIds.Reserve(Items.Num() + Groups.Num());
	for (const auto& Item : Items)
	{
		const auto& Header = Item.Header;
		if (!ValidateItemCatalogRecord(Header, Dnd5e::Category::Item.GetTag(), Diagnostics)) { return false; }
		if (CandidateItems.Contains(Header.RecordId))
		{
			return FailItemCatalog(Diagnostics, TEXT("Dnd5e.Catalog.DuplicateRecordId"),
				TEXT("Duplicate item record ID."), &Header);
		}
		CandidateItems.Add(Header.RecordId, Item);
		CandidateIds.Add(Header.RecordId);
	}
	for (const auto& Group : Groups)
	{
		const auto& Header = Group.Details.Header;
		if (!ValidateItemCatalogRecord(Header, Dnd5e::Category::ItemGroup.GetTag(), Diagnostics)) { return false; }
		if (CandidateGroups.Contains(Header.RecordId))
		{
			return FailItemCatalog(Diagnostics, TEXT("Dnd5e.Catalog.DuplicateRecordId"),
				TEXT("Duplicate item group record ID."), &Header);
		}
		CandidateGroups.Add(Header.RecordId, Group);
		CandidateIds.Add(Header.RecordId);
	}
	ItemsById = MoveTemp(CandidateItems);
	GroupsById = MoveTemp(CandidateGroups);
	OrderedIds = MoveTemp(CandidateIds);
	return true;
}

bool FDnd5eItemCatalog::TryGetItem(const FDnd5eRecordId& Id, FDnd5eItem& Out) const
{
	const auto* Found = ItemsById.Find(Id);
	if (!Found) { Out = {}; return false; }
	Out = *Found;
	return true;
}

bool FDnd5eItemCatalog::TryGetGroup(const FDnd5eRecordId& Id, FDnd5eItemGroup& Out) const
{
	const auto* Found = GroupsById.Find(Id);
	if (!Found) { Out = {}; return false; }
	Out = *Found;
	return true;
}

EDnd5eItemSelectorMatchStatus FDnd5eItemCatalog::MatchItemSelector(const FString& DatasetNamespace,
	const FDnd5eItemGroupMember& Selector, TArray<FDnd5eRecordId>& CandidateIds) const
{
	using EStatus = EDnd5eItemSelectorMatchStatus;
	CandidateIds.Reset();

	const FString ComparisonSource = Selector.bHasSource ? Selector.SourceId : TEXT("name-only");
	FDnd5eRecordId Wanted;
	if (!Dnd5e::Items::TryMakeId(DatasetNamespace, Selector.Name, ComparisonSource, Wanted) ||
		!Wanted.DatasetNamespace.Equals(DatasetNamespace, ESearchCase::CaseSensitive))
	{
		return EStatus::InvalidSelector;
	}

	for (const auto& Id : OrderedIds)
	{
		if (!Id.DatasetNamespace.Equals(DatasetNamespace, ESearchCase::CaseSensitive)) { continue; }
		const auto* Item = ItemsById.Find(Id);
		if (!Item || (Selector.bHasSource && Item->Header.Provenance.IsEmpty())) { continue; }

		const FString Publication = Selector.bHasSource ? Item->Header.Provenance[0].SourceId : ComparisonSource;
		FDnd5eRecordId AuthoredId;
		if (Dnd5e::Items::TryMakeId(DatasetNamespace, Item->Header.DisplayName, Publication, AuthoredId) && AuthoredId == Wanted)
		{
			CandidateIds.Add(Id);
		}
	}

	if (!Selector.bHasSource) { return EStatus::MissingSource; }
	if (CandidateIds.IsEmpty()) { return EStatus::NoMatch; }
	if (CandidateIds.Num() > 1) { return EStatus::Ambiguous; }
	return EStatus::Matched;
}

bool FDnd5eItemCatalog::QueryRecordIds(const FDnd5eItemCatalogQuery& Query, TArray<FDnd5eRecordId>& OutIds,
	TArray<FDnd5eDiagnostic>& Diagnostics) const
{
	OutIds.Reset();
	Diagnostics.Reset();
	if (Query.Category.IsValid() && Query.Category != Dnd5e::Category::Item.GetTag() &&
		Query.Category != Dnd5e::Category::ItemGroup.GetTag())
	{
		return FailItemCatalog(Diagnostics, TEXT("Dnd5e.Catalog.InvalidQuery"),
			TEXT("An item query category must be Item or ItemGroup."));
	}
	for (const auto& Id : OrderedIds)
	{
		if (!Query.DatasetNamespace.IsEmpty() && !Id.DatasetNamespace.Equals(Query.DatasetNamespace, ESearchCase::CaseSensitive)) { continue; }
		if (Query.Category.IsValid() && Id.Category != Query.Category) { continue; }
		const auto* Item = ItemsById.Find(Id);
		if (!Item) { Item = &GroupsById.FindChecked(Id).Details; }
		const auto& Header = Item->Header;
		if (!Query.SourceId.IsEmpty() && (Header.Provenance.IsEmpty() ||
			!Header.Provenance[0].SourceId.Equals(Query.SourceId, ESearchCase::CaseSensitive))) { continue; }
		if (Query.Rarity.IsValid() && Item->Rarity != Query.Rarity) { continue; }
		if (!MatchesItemCatalogName(Header, Query)) { continue; }
		OutIds.Add(Id);
	}
	return true;
}