// Developed by Neko Creative Technologies

#pragma once
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Items/Dnd5eItem.h"
#include "Catalog/Dnd5eSpellCatalogObject.h"
#include "Dnd5eItemLibrary.generated.h"

UCLASS()
class DND5E_API UDnd5eItemLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Dnd5e|Items")
	static bool TryGetReferencedBaseItem(const FDnd5eItem& Item, const TArray<FDnd5eItem>& BaseItems,
		FDnd5eItem& OutBaseItem);

	UFUNCTION(BlueprintCallable, Category = "Dnd5e|Items")
	static EDnd5eSpellSelectorMatchStatus MatchItemAttachedSpell(const FString& DatasetNamespace,
		const FDnd5eItemAttachedSpell& AttachedSpell, const UDnd5eSpellCatalogObject* SpellCatalog,
		TArray<FDnd5eRecordId>& CandidateIds, TArray<FDnd5eDiagnostic>& Diagnostics);
};
