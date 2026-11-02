// Developed by Neko Creative Technologies

#pragma once
#include "CoreMinimal.h"
#include "Values/Dnd5eMoney.h"
#include "Values/Dnd5eValuePresence.h"
#include "Dnd5ePrice.generated.h"

USTRUCT(BlueprintType)
struct DND5E_API FDnd5ePrice
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Price")
	EDnd5eValuePresence CostPresence = EDnd5eValuePresence::Unspecified;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Price", meta = (EditCondition = "CostPresence == EDnd5eValuePresence::Present"))
	TArray<FDnd5eMoney> Cost;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Price")
	bool bHasMultiplier = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Price", meta = (EditCondition = "bHasMultiplier"))
	FDnd5eNumber Multiplier;
};