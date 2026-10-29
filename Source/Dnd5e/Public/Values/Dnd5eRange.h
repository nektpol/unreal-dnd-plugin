// Developed by Neko Creative Technologies

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Dnd5eDistance.h"
#include "Content/Dnd5eContentType.h"
#include "Dnd5eRange.generated.h"

USTRUCT(BlueprintType)
struct DND5E_API FDnd5eRange
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Range")
	FGameplayTag Kind;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Range")
	bool bHasDistance = false;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Range", meta = (EditCondition = "bHasDistance"))
	FDnd5eDistance Distance;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Range")
	TArray<FDnd5eContentBlock> Description;
};