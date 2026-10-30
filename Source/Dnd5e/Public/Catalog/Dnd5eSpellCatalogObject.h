// Developed by Neko Creative Technologies

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Catalog/Dnd5eSpellCatalog.h"
#include "Records/Dnd5eReferenceResolution.h"
#include "Dnd5eSpellCatalogObject.generated.h"

/**
 *
 */
UCLASS(BlueprintType, Transient)
class DND5E_API UDnd5eSpellCatalogObject : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Dnd5e|SpellCatalog")
	bool ReplaceSpells(const TArray<FDnd5eSpell>& Records, TArray<FDnd5eDiagnostic>& Diagnostics);

	UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "Dnd5e|SpellCatalog")
	bool TryGetSpell(const FDnd5eRecordId& Id, FDnd5eSpell& Spell) const;

	UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "Dnd5e|SpellCatalog")
	EDnd5eReferenceResolveStatus ResolveSpellReference(const FDnd5eReferenceContent& Reference, FDnd5eSpell& Spell, TArray<FDnd5eDiagnostic>& Diagnostics) const;

	UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "Dnd5e|SpellCatalog")
	bool QuerySpellIds(const FDnd5eSpellQuery& Query, TArray<FDnd5eRecordId>& Ids, TArray<FDnd5eDiagnostic>& Diagnostics) const;

	UFUNCTION(BlueprintPure, Category = "Dnd5e|SpellCatalog")
	int32 GetSpellCount() const;

	UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "Dnd5e|SpellCatalog")
	TArray<FDnd5eRecordId> GetSpellIds() const;

	FDnd5eSpellCatalog& GetMutableCatalog() { return Catalog; }

private:
	FDnd5eSpellCatalog Catalog;
};