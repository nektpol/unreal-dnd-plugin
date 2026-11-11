// Developed by Neko Creative Technologies

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Records/Dnd5eRecordId.h"

class UDnd5eItemCatalogObject;

struct FDnd5eItemBrowserRow
{
	FDnd5eRecordId Id;
	FString Name;
	FString SourceId;
	FGameplayTag Rarity;
};

using FDnd5eItemBrowserRowPtr = TSharedPtr<FDnd5eItemBrowserRow>;

class FDnd5eItemBrowserModel
{
public:
	void Rebuild(const UDnd5eItemCatalogObject* Catalog);
	void SetSearchText(const FString& Text);
	const TArray<FDnd5eItemBrowserRowPtr>& GetRows() const { return FilteredRows; }
	int32 GetTotalCount() const { return AllRows.Num(); }

private:
	void ApplyFilter();

	FString SearchText;
	TArray<FDnd5eItemBrowserRowPtr> AllRows;
	TArray<FDnd5eItemBrowserRowPtr> FilteredRows;
};