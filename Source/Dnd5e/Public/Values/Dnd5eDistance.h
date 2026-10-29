// Developed by Neko Creative Technologies

#pragma once

#include "CoreMinimal.h"
#include "Dnd5eNumber.h"
#include "Dnd5eDistance.generated.h"

UENUM(BlueprintType)
enum class EDnd5eDistanceUnit : uint8 
{
	Unspecified,
	Inches,
	Feet,
	Yards,
	Miles
};


USTRUCT(BlueprintType)
struct DND5E_API FDnd5eDistance
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Distance")
	FDnd5eNumber Amount;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Distance")
	EDnd5eDistanceUnit Unit = EDnd5eDistanceUnit::Unspecified;
};
