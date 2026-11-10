// Developed by Neko Creative Technologies

#pragma once
#include "CoreMinimal.h"
#include "Diagnostics/Dnd5eDiagnostic.h"
#include "Readers/Dnd5eJsonDocument.h"
#include "Readers/Dnd5eJsonReadStatus.h"
#include "Dnd5eJsonItemVariantMatcher.generated.h"

UENUM(BlueprintType)
enum class EDnd5eJsonItemVariantEditionPolicy : uint8
{
	SourceDefault,
	PreferClassic
};

USTRUCT(BlueprintType)
struct DND5EJSON_API FDnd5eJsonItemVariantMatchResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Dnd5e|Items")
	EDnd5eJsonReadStatus Status = EDnd5eJsonReadStatus::NotAttempted;
	UPROPERTY(BlueprintReadOnly, Category = "Dnd5e|Items")
	FString VariantName;
	UPROPERTY(BlueprintReadOnly, Category = "Dnd5e|Items")
	FString VariantSourceId;
	UPROPERTY(BlueprintReadOnly, Category = "Dnd5e|Items")
	TArray<int32> MatchingBaseIndices;
	UPROPERTY(BlueprintReadOnly, Category = "Dnd5e|Items")
	TArray<FDnd5eDiagnostic> Diagnostics;
};

class DND5EJSON_API FDnd5eJsonItemVariantMatcher
{
public:
	static FDnd5eJsonItemVariantMatchResult MatchAtIndex(const FDnd5eJsonDocument& VariantDocument,
		int32 VariantIndex, const FDnd5eJsonDocument& BaseItemDocument, EDnd5eJsonItemVariantEditionPolicy Policy);
};
