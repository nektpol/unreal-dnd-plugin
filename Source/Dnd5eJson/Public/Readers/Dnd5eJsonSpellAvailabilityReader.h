#pragma once

#include "CoreMinimal.h"
#include "Readers/Dnd5eJsonDocument.h"
#include "Readers/Dnd5eJsonReadStatus.h"
#include "Spells/Dnd5eSpellAvailability.h"
#include "Diagnostics/Dnd5eDiagnostic.h"
#include "Dnd5eJsonSpellAvailabilityReader.generated.h"

USTRUCT(BlueprintType)
struct DND5EJSON_API FDnd5eJsonSpellAvailabilityReadRequest
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Json|Availability")
	FDnd5eJsonDocument Document;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Json|Availability")
	FString SpellName;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Json|Availability")
	FString SpellSourceId;
};

USTRUCT(BlueprintType)
struct DND5EJSON_API FDnd5eJsonSpellAvailabilityReadResult
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Json|Availability")
	EDnd5eJsonReadStatus Status = EDnd5eJsonReadStatus::NotAttempted;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Json|Availability")
	bool bHasEntry = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Json|Availability")
	TArray<FDnd5eSpellAvailability> Availability;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Json|Availability")
	TArray<FDnd5eDiagnostic> Diagnostics;
};

class DND5EJSON_API FDnd5eJsonSpellAvailabilityReader
{
public:
	static FDnd5eJsonSpellAvailabilityReadResult ReadAvailability(const FDnd5eJsonSpellAvailabilityReadRequest& Request);
};
