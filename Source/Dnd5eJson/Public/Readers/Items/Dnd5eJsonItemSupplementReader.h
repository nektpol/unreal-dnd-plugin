// Developed by Neko Creative Technologies

#pragma once
#include "CoreMinimal.h"
#include "Diagnostics/Dnd5eDiagnostic.h"
#include "Items/Dnd5eItem.h"
#include "Readers/Dnd5eJsonDocument.h"
#include "Readers/Dnd5eJsonReadStatus.h"
#include "Dnd5eJsonItemSupplementReader.generated.h"

USTRUCT(BlueprintType)
struct DND5EJSON_API FDnd5eJsonItemSupplement
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Dnd5e|Items")
	FDnd5eRecordId TargetItemId;
	UPROPERTY(BlueprintReadOnly, Category = "Dnd5e|Items")
	FDnd5eSupplementaryContent Content;
	UPROPERTY(BlueprintReadOnly, Category = "Dnd5e|Items")
	TArray<FDnd5eRecordProvenance> Provenance;
};

USTRUCT(BlueprintType)
struct DND5EJSON_API FDnd5eJsonItemSupplementReadResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Dnd5e|Items")
	EDnd5eJsonReadStatus Status = EDnd5eJsonReadStatus::NotAttempted;
	UPROPERTY(BlueprintReadOnly, Category = "Dnd5e|Items")
	TArray<FDnd5eJsonItemSupplement> Rows;
	UPROPERTY(BlueprintReadOnly, Category = "Dnd5e|Items")
	TArray<FDnd5eDiagnostic> Diagnostics;
};

class DND5EJSON_API FDnd5eJsonItemSupplementReader
{
public:
	static FDnd5eJsonItemSupplementReadResult ReadDocument(const FDnd5eJsonDocument& Document);
	static bool TryApply(const FDnd5eItem& Item, const FDnd5eJsonItemSupplement& Supplement, FDnd5eItem& OutItem);
};