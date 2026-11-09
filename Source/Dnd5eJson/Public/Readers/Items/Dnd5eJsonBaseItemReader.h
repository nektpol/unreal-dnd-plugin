// Developed by Neko Creative Technologies

#pragma once
#include "CoreMinimal.h"
#include "Diagnostics/Dnd5eDiagnostic.h"
#include "Items/Dnd5eItem.h"
#include "Readers/Dnd5eJsonDocument.h"
#include "Readers/Dnd5eJsonReadStatus.h"
#include "Dnd5eJsonBaseItemReader.generated.h"

USTRUCT(BlueprintType)
struct DND5EJSON_API FDnd5eJsonBaseItemReadResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Dnd5e|Items")
	EDnd5eJsonReadStatus Status = EDnd5eJsonReadStatus::NotAttempted;
	UPROPERTY(BlueprintReadOnly, Category = "Dnd5e|Items")
	TArray<FDnd5eItem> Items;
	UPROPERTY(BlueprintReadOnly, Category = "Dnd5e|Items")
	TArray<FDnd5eDiagnostic> Diagnostics;
};

class DND5EJSON_API FDnd5eJsonBaseItemReader
{
public:
	static FDnd5eJsonBaseItemReadResult Read(const FDnd5eJsonDocument& Document);
};