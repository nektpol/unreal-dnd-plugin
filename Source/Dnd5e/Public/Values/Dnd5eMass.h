// Developed by Neko Creative Technologies

#pragma once
#include "CoreMinimal.h"
#include "Values/Dnd5eNumber.h"
#include "Dnd5eMass.generated.h"

UENUM(BlueprintType)
enum class EDnd5eMassUnit : uint8
{
	Unspecified,
	Ounces,
	Pounds,
	Grams,
	Kilograms
};

USTRUCT(BlueprintType)
struct DND5E_API FDnd5eMass
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Mass")
	FDnd5eNumber Amount;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Mass")
	EDnd5eMassUnit Unit = EDnd5eMassUnit::Unspecified;
};