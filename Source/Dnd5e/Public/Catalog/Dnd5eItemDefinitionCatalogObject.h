// Developed by Neko Creative Technologies

#pragma once
#include "UObject/Object.h"
#include "Catalog/Dnd5eItemDefinitionCatalog.h"
#include "Dnd5eItemDefinitionCatalogObject.generated.h"

UCLASS(BlueprintType, Transient)
class DND5E_API UDnd5eItemDefinitionCatalogObject : public UObject
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Dnd5e|ItemDefinitions")
    bool ReplaceDefinitions(const TArray<FDnd5eItemTypeDefinition>& Types, const TArray<FDnd5eItemPropertyDefinition>& Properties, const TArray<FDnd5eItemMasteryDefinition>& Masteries, TArray<FDnd5eDiagnostic>& Diagnostics);

    UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "Dnd5e|ItemDefinitions")
    bool TryGetType(const FDnd5eRecordId& Id, FDnd5eItemTypeDefinition& Out) const;

    UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "Dnd5e|ItemDefinitions")
    bool TryGetProperty(const FDnd5eRecordId& Id, FDnd5eItemPropertyDefinition& Out) const;

    UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "Dnd5e|ItemDefinitions")
    bool TryGetMastery(const FDnd5eRecordId& Id, FDnd5eItemMasteryDefinition& Out) const;

    UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "Dnd5e|ItemDefinitions")
    EDnd5eItemDefinitionMatchStatus MatchDefinitionSelector(const FString& DatasetNamespace, const FDnd5eItemDefinitionSelector& Selector, TArray<FDnd5eRecordId>& CandidateIds, TArray<FDnd5eDiagnostic>& Diagnostics) const;

    UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "Dnd5e|ItemDefinitions")
    EDnd5eItemDefinitionMatchStatus MatchMasterySelector(const FString& DatasetNamespace, const FDnd5eRecordSelector& Selector, TArray<FDnd5eRecordId>& CandidateIds, TArray<FDnd5eDiagnostic>& Diagnostics) const;

    UFUNCTION(BlueprintPure, Category = "Dnd5e|ItemDefinitions")
    int32 GetDefinitionCount() const;

    UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "Dnd5e|ItemDefinitions")
    TArray<FDnd5eRecordId> GetDefinitionIds() const;

private:
    FDnd5eItemDefinitionCatalog Catalog;
};