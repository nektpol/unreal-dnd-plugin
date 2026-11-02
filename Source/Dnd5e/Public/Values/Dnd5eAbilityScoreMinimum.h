// Developed by Neko Creative Technologies

#pragma once
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Dnd5eAbilityScoreMinimum.generated.h"

USTRUCT(BlueprintType)
struct DND5E_API FDnd5eAbilityScoreMinimum
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Requirement")
	FGameplayTag Ability;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Requirement")
	int32 Minimum = 0;
};