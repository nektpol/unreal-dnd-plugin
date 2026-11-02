// Developed by Neko Creative Technologies

#pragma once
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Dnd5eAlignmentRequirement.generated.h"

USTRUCT(BlueprintType)
struct DND5E_API FDnd5eAlignmentRequirement
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Requirement")
	FGameplayTagContainer RequiredTraits;
};