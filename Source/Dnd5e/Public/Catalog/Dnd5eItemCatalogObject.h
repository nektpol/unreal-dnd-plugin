// Developed by Neko Creative Technologies

#pragma once
#include "UObject/Object.h"
#include "Catalog/Dnd5eItemCatalog.h"
#include "Dnd5eItemCatalogObject.generated.h"

UCLASS(BlueprintType, Transient)
class DND5E_API UDnd5eItemCatalogObject : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Dnd5e|ItemCatalog")
	bool ReplaceRecords(const TArray<FDnd5eItem>& Items, const TArray<FDnd5eItemGroup>& Groups, TArray<FDnd5eDiagnostic>& Diagnostics);

	UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "Dnd5e|ItemCatalog")
	bool TryGetItem(const FDnd5eRecordId& Id, FDnd5eItem& Out) const;

	UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "Dnd5e|ItemCatalog")
	bool TryGetGroup(const FDnd5eRecordId& Id, FDnd5eItemGroup& Out) const;

	UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "Dnd5e|ItemCatalog")
	bool QueryRecordIds(const FDnd5eItemCatalogQuery& Query, TArray<FDnd5eRecordId>& OutIds, TArray<FDnd5eDiagnostic>& Diagnostics) const;

	UFUNCTION(BlueprintPure, Category = "Dnd5e|ItemCatalog")
	int32 GetRecordCount() const;

private:
	FDnd5eItemCatalog Catalog;
};