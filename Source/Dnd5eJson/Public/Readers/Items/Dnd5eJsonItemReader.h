// Developed by Neko Creative Technologies

#pragma once
#include "CoreMinimal.h"
#include "Diagnostics/Dnd5eDiagnostic.h"
#include "Items/Dnd5eItem.h"
#include "Readers/Dnd5eJsonDocument.h"
#include "Readers/Dnd5eJsonReadStatus.h"
#include "Dnd5eJsonItemReader.generated.h"

USTRUCT(BlueprintType)
struct DND5EJSON_API FDnd5eJsonItemReadResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Dnd5e|Items")
	EDnd5eJsonReadStatus Status = EDnd5eJsonReadStatus::NotAttempted;

	UPROPERTY(BlueprintReadOnly, Category = "Dnd5e|Items")
	FDnd5eItem Item;

	UPROPERTY(BlueprintReadOnly, Category = "Dnd5e|Items")
	TArray<FDnd5eDiagnostic> Diagnostics;
};

class DND5EJSON_API FDnd5eJsonItemReader
{
public:
	static FDnd5eJsonItemReadResult ReadAtIndex(const FDnd5eJsonDocument& Document, int32 Index);
};