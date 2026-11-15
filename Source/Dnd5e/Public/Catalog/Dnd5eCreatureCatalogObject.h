// Developed by Neko Creative Technologies

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Catalog/Dnd5eCreatureCatalog.h"
#include "Dnd5eCreatureCatalogObject.generated.h"

UCLASS(BlueprintType, Transient)
class DND5E_API UDnd5eCreatureCatalogObject : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "Dnd5e|CreatureCatalog")
	bool TryGetCreature(const FDnd5eRecordId& Id, FDnd5eCreature& OutCreature) const;

	EDnd5eReferenceResolveStatus ResolveCreatureRelationship(const FDnd5eRecordRelationship& Relationship, FDnd5eCreature& OutCreature) const;

	UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "Dnd5e|CreatureCatalog")
	TArray<FDnd5eRecordId> QueryCreatureIds(const FDnd5eCreatureQuery& Query) const;

	UFUNCTION(BlueprintPure, Category = "Dnd5e|CreatureCatalog")
	int32 GetCreatureCount() const;

	FDnd5eCreatureCatalog& GetMutableCatalog() { return Catalog; }

private:
	FDnd5eCreatureCatalog Catalog;
};