// Developed by Neko Creative Technologies

#pragma once
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Dnd5eItemTypes.generated.h"

USTRUCT(BlueprintType)
struct DND5E_API FDnd5eItemDefinitionSelector
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|ItemDefinitionSelector")
	FGameplayTag Category;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|ItemDefinitionSelector")
	FString Abbreviation;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|ItemDefinitionSelector")
	bool bHasSource = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|ItemDefinitionSelector", meta = (EditCondition = "bHasSource"))
	FString SourceId;
};