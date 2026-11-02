// Developed by Neko Creative Technologies

#pragma once
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Values/Dnd5eDiceExpression.h"
#include "Dnd5eDamageExpression.generated.h"

USTRUCT(BlueprintType)
struct DND5E_API FDnd5eDamageExpression
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Damage")
	FDnd5eDiceExpression Amount;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Damage")
	FGameplayTag Type;
};