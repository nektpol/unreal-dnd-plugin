// Developed by Neko Creative Technologies

#pragma once
#include "CoreMinimal.h"
#include "Records/Dnd5eRecordHeader.h"
#include "Readers/Items/Dnd5eJsonItemVariantMatcher.h"
#include "Dnd5eJsonItemVariantGenerator.generated.h"

USTRUCT(BlueprintType)
struct DND5EJSON_API FDnd5eJsonItemVariantGenerationResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Dnd5e|Items")
	EDnd5eJsonReadStatus Status = EDnd5eJsonReadStatus::NotAttempted;
	UPROPERTY(BlueprintReadOnly, Category = "Dnd5e|Items")
	FString GeneratedJson;
	UPROPERTY(BlueprintReadOnly, Category = "Dnd5e|Items")
	FDnd5eRecordId GeneratedId;
	UPROPERTY(BlueprintReadOnly, Category = "Dnd5e|Items")
	FDnd5eRecordProvenance BaseOrigin;
	UPROPERTY(BlueprintReadOnly, Category = "Dnd5e|Items")
	FDnd5eRecordProvenance TemplateOrigin;
	UPROPERTY(BlueprintReadOnly, Category = "Dnd5e|Items")
	TArray<int32> TemplateSourceRowIndices;
	UPROPERTY(BlueprintReadOnly, Category = "Dnd5e|Items")
	TArray<FDnd5eDiagnostic> Diagnostics;
};

class DND5EJSON_API FDnd5eJsonItemVariantGenerator
{
public:
	static FDnd5eJsonItemVariantGenerationResult GenerateAtIndex(const FDnd5eJsonDocument& VariantDocument,
		int32 VariantIndex, const FDnd5eJsonDocument& BaseItemDocument, int32 BaseIndex,
		EDnd5eJsonItemVariantEditionPolicy Policy);
};