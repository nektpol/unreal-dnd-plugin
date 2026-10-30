// Developed by Neko Creative Technologies

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Spells/Dnd5eSpell.h"
#include "Diagnostics/Dnd5eDiagnostic.h"
#include "Dnd5eSpellContentLibrary.generated.h"

/**
 *
 */

USTRUCT(BlueprintType)
struct DND5E_API FDnd5eContentReferenceOccurrence
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Content")
	FString ContentPath;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Content")
	FDnd5eReferenceContent Reference;
};

UCLASS()
class DND5E_API UDnd5eSpellContentLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Category = "Dnd5e|SpellContent")
	static bool CollectSpellReferences(const FDnd5eSpell& Spell, TArray<FDnd5eContentReferenceOccurrence>& Occurrences, TArray<FDnd5eDiagnostic>& Diagnostics);
};
