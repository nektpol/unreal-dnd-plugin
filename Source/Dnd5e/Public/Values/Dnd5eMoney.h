// Developed by Neko Creative Technologies

#pragma once

#include "CoreMinimal.h"
#include "Dnd5eNumber.h"
#include "Dnd5eMoney.generated.h"

UENUM(BlueprintType)
enum class EDnd5eCurrency : uint8 
{
	Unspecified,
	Copper,
	Silver,
	Electrum,
	Gold,
	Platinum
};

USTRUCT(BlueprintType)
struct DND5E_API FDnd5eMoney
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Money")
	FDnd5eNumber Amount;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Money")
	EDnd5eCurrency Currency = EDnd5eCurrency::Unspecified;
};