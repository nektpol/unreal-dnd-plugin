// Developed by Neko Creative Technologies

#include "Catalog/Dnd5eItemCatalogObject.h"

bool UDnd5eItemCatalogObject::ReplaceRecords(const TArray<FDnd5eItem>& Items, const TArray<FDnd5eItemGroup>& Groups,
	TArray<FDnd5eDiagnostic>& Diagnostics)
{
	return Catalog.ReplaceRecords(Items, Groups, Diagnostics);
}

bool UDnd5eItemCatalogObject::TryGetItem(const FDnd5eRecordId& Id, FDnd5eItem& Out) const
{
	return Catalog.TryGetItem(Id, Out);
}

bool UDnd5eItemCatalogObject::TryGetGroup(const FDnd5eRecordId& Id, FDnd5eItemGroup& Out) const
{
	return Catalog.TryGetGroup(Id, Out);
}

EDnd5eItemSelectorMatchStatus UDnd5eItemCatalogObject::MatchItemSelector(const FString& DatasetNamespace,
	const FDnd5eItemGroupMember& Selector, TArray<FDnd5eRecordId>& CandidateIds) const
{
	return Catalog.MatchItemSelector(DatasetNamespace, Selector, CandidateIds);
}

bool UDnd5eItemCatalogObject::QueryRecordIds(const FDnd5eItemCatalogQuery& Query, TArray<FDnd5eRecordId>& OutIds,
	TArray<FDnd5eDiagnostic>& Diagnostics) const
{
	return Catalog.QueryRecordIds(Query, OutIds, Diagnostics);
}

int32 UDnd5eItemCatalogObject::GetRecordCount() const
{
	return Catalog.GetRecordCount();
}