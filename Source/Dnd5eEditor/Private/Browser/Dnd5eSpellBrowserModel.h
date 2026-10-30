#pragma once

#include "CoreMinimal.h"
#include "Records/Dnd5eRecordId.h"

class UDnd5eSpellCatalogObject;

struct FDnd5eSpellBrowserRow
{
	FDnd5eRecordId Id;
	FString Name;
	FString SourceId;
	int32 Level = INDEX_NONE;
};

using FDnd5eSpellBrowserRowPtr = TSharedPtr<FDnd5eSpellBrowserRow>;

class FDnd5eSpellBrowserModel
{
public:
	void Rebuild(const UDnd5eSpellCatalogObject* Catalog);
	void SetSearchText(const FString& Text);
	const TArray<FDnd5eSpellBrowserRowPtr>& GetRows() const { return FilteredRows; }
	int32 GetTotalCount() const { return AllRows.Num(); }

private:
	void ApplyFilter();
	FString SearchText;
	TArray<FDnd5eSpellBrowserRowPtr> AllRows;
	TArray<FDnd5eSpellBrowserRowPtr> FilteredRows;
};
