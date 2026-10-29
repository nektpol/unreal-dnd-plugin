// Developed by Neko Creative Technologies

#pragma once

#include "CoreMinimal.h"
#include "Dnd5eTimeSpan.h"
#include "GameplayTagContainer.h"
#include "Content/Dnd5eContentType.h"
#include "Dnd5eDuration.generated.h"

UENUM(BlueprintType)
enum class EDnd5eDurationKind : uint8
{
	Unspecified,
	Instantaneous,
	Timed,
	Permanent,
	Special,
};

USTRUCT(BlueprintType)
struct DND5E_API FDnd5eDuration
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Duration")
	EDnd5eDurationKind Kind = EDnd5eDurationKind::Unspecified;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Duration")
	bool bHasTime = false;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Duration", meta = (EditCondition = "bHasTime"))
	FDnd5eTimeSpan Time;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Duration")
	bool bIsMaximum = false;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Duration")
	FGameplayTagContainer EndingConditions;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Duration")
	TArray<FDnd5eContentBlock> Description;
};