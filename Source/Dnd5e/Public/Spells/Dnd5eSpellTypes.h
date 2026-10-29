// Developed by Neko Creative Technologies

#pragma once

#include "CoreMinimal.h"
#include "Values/Dnd5eArea.h"
#include "Values/Dnd5eDuration.h"
#include "GameplayTagContainer.h"
#include "Content/Dnd5eContentType.h"
#include "Dnd5eSpellTypes.generated.h"

USTRUCT(BlueprintType)
struct DND5E_API FDnd5eSpellArea
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|SpellArea")
	FDnd5eArea Geometry;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|SpellArea")
	FGameplayTag Origin;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|SpellArea")
	TArray<FDnd5eContentBlock> OriginDescription;
	
};

USTRUCT(BlueprintType)
struct DND5E_API FDnd5eSpellDuration
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|SpellDuration")
	FDnd5eDuration Duration;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|SpellDuration")
	FGameplayTagContainer Requirements;
	
};