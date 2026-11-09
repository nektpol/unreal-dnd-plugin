// Developed by Neko Creative Technologies

#pragma once
#include "CoreMinimal.h"
#include "Diagnostics/Dnd5eDiagnostic.h"
#include "Items/Dnd5eItemDefinitions.h"
#include "Readers/Dnd5eJsonDocument.h"
#include "Readers/Dnd5eJsonReadStatus.h"
#include "Dnd5eJsonItemDefinitionReader.generated.h"

USTRUCT(BlueprintType)
struct DND5EJSON_API FDnd5eJsonItemDefinitionReadResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Dnd5e|Json")
	EDnd5eJsonReadStatus Status = EDnd5eJsonReadStatus::NotAttempted;
	UPROPERTY(BlueprintReadOnly, Category = "Dnd5e|Json")
	TArray<FDnd5eItemTypeDefinition> Types;
	UPROPERTY(BlueprintReadOnly, Category = "Dnd5e|Json")
	TArray<FDnd5eItemPropertyDefinition> Properties;
	UPROPERTY(BlueprintReadOnly, Category = "Dnd5e|Json")
	TArray<FDnd5eItemMasteryDefinition> Masteries;
	UPROPERTY(BlueprintReadOnly, Category = "Dnd5e|Json")
	TArray<FDnd5eDiagnostic> Diagnostics;
};

class DND5EJSON_API FDnd5eJsonItemDefinitionReader
{
public:
	static FDnd5eJsonItemDefinitionReadResult Read(const FDnd5eJsonDocument& Document);
};