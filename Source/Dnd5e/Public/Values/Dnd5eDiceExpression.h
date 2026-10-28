// Developed by Neko Creative Technologies

#pragma once

#include "CoreMinimal.h"
#include "Dnd5eDiceExpression.generated.h"

USTRUCT(BlueprintType)
struct DND5E_API FDnd5eDiceExpression
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|DiceExpression")
	FString Expression;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|DiceExpression")
	FString Syntax;
};
