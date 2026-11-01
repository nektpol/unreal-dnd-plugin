// Developed by Neko Creative Technologies

#pragma once

#include "CoreMinimal.h"
#include "Values/Dnd5eNumber.h"
#include "Dnd5eD20Roll.generated.h"

USTRUCT(BlueprintType)
struct DND5E_API FDnd5eD20Roll
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Roll")
	int32 DieSides = 20;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Roll")
	FDnd5eNumber Modifier;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Roll")
	FString RollName;
};
