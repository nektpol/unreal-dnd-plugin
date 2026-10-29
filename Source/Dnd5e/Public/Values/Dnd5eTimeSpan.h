// Developed by Neko Creative Technologies

#pragma once

#include "CoreMinimal.h"
#include "Dnd5eNumber.h"
#include "Dnd5eTimeSpan.generated.h"

UENUM(BlueprintType)
enum class EDnd5eTimeUnit : uint8 
{
	Unspecified,
	Turn,
	Round,
	Minute,
	Hour,
	Day,
	Week,
	Month,
	Year
};

USTRUCT(BlueprintType)
struct DND5E_API FDnd5eTimeSpan
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|TimeSpan")
	FDnd5eNumber Amount;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|TimeSpan")
	EDnd5eTimeUnit Unit = EDnd5eTimeUnit::Unspecified;
};