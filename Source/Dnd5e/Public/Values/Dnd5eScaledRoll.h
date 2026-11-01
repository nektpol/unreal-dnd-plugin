#pragma once

#include "CoreMinimal.h"
#include "Dnd5eDiceExpression.h"
#include "GameplayTagContainer.h"
#include "Dnd5eScaledRoll.generated.h"

USTRUCT(BlueprintType)
struct DND5E_API FDnd5eScaledRoll
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|ScaledRoll")
	TArray<FDnd5eDiceExpression> BaseExpressions;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|ScaledRoll")
	TArray<int32> InputValues;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|ScaledRoll")
	FDnd5eDiceExpression Increment;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|ScaledRoll")
	FGameplayTag InputKind;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|ScaledRoll")
	FString SourceProgression;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|ScaledRoll")
	FString SourceRenderMode;

};
