// Developed by Neko Creative Technologies

#pragma once

#include "CoreMinimal.h"
#include "Content/Dnd5eContentType.h"
#include "GameplayTagContainer.h"
#include "Dnd5eCastingTime.generated.h"

USTRUCT(BlueprintType)
struct DND5E_API FDnd5eCastingTime
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|CastingTime", meta = (ClampMin = "0", ToolTip = "Number of casting-time units"))
	int32 Quantity = 0;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|CastingTime", meta = (ToolTip = "Action, BonusAction, Reaction, Minute, or Hour"))
	FGameplayTag Unit;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|CastingTime", meta = (ToolTip = "Restrictions describing when that casting time applies"))
	TArray<FDnd5eContentBlock> Condition;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|CastingTime", meta = (ToolTip = "Additional source-provided explanation"))
	TArray<FDnd5eContentBlock> Note;
	
};
