// Developed by Neko Creative Technologies

#pragma once
#include "CoreMinimal.h"
#include "Diagnostics/Dnd5eDiagnostic.h"
#include "Items/Dnd5eItemGroup.h"
#include "Dnd5eItemCatalog.generated.h"

USTRUCT(BlueprintType)
struct DND5E_API FDnd5eItemCatalogQuery
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|ItemCatalog")
	FString DatasetNamespace;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|ItemCatalog")
	FString SourceId;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|ItemCatalog")
	FString NameContains;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|ItemCatalog")
	FGameplayTag Category;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|ItemCatalog")
	FGameplayTag Rarity;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|ItemCatalog")
	bool bIncludeAliases = true;
};

class DND5E_API FDnd5eItemCatalog
{
public:
	bool ReplaceRecords(const TArray<FDnd5eItem>& Items, const TArray<FDnd5eItemGroup>& Groups,
		TArray<FDnd5eDiagnostic>& Diagnostics);
	bool TryGetItem(const FDnd5eRecordId& Id, FDnd5eItem& Out) const;
	bool TryGetGroup(const FDnd5eRecordId& Id, FDnd5eItemGroup& Out) const;
	bool QueryRecordIds(const FDnd5eItemCatalogQuery& Query, TArray<FDnd5eRecordId>& OutIds,
		TArray<FDnd5eDiagnostic>& Diagnostics) const;
	int32 GetRecordCount() const { return OrderedIds.Num(); }

private:
	TMap<FDnd5eRecordId, FDnd5eItem> ItemsById;
	TMap<FDnd5eRecordId, FDnd5eItemGroup> GroupsById;
	TArray<FDnd5eRecordId> OrderedIds;
};