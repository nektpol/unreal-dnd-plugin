// Developed by Neko Creative Technologies

#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Diagnostics/Dnd5eDiagnostic.h"
#include "Spells/Dnd5eSpell.h"
#include "Dnd5eContentValidationLibrary.generated.h"

USTRUCT(BlueprintType)
struct DND5E_API FDnd5eContentValidationIssue
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly, Category="Dnd5e|Validation") FDnd5eRecordId OwnerRecordId;
	UPROPERTY(BlueprintReadOnly, Category="Dnd5e|Validation") FString ContentPath;
	UPROPERTY(BlueprintReadOnly, Category="Dnd5e|Validation") FDnd5eDiagnostic Diagnostic;
};

USTRUCT(BlueprintType)
struct DND5E_API FDnd5eContentValidationResult
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly, Category="Dnd5e|Validation") bool bValid = false;
	UPROPERTY(BlueprintReadOnly, Category="Dnd5e|Validation") TArray<FDnd5eContentValidationIssue> Issues;
};

UCLASS()
class DND5E_API UDnd5eContentValidationLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Category="Dnd5e|Validation")
	static FDnd5eContentValidationResult ValidateSpellContent(const FDnd5eSpell& Spell);
};
