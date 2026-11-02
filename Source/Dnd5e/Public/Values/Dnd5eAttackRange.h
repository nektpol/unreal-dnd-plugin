// Developed by Neko Creative Technologies

#pragma once
#include "CoreMinimal.h"
#include "Values/Dnd5eDistance.h"
#include "Dnd5eAttackRange.generated.h"

USTRUCT(BlueprintType)
struct DND5E_API FDnd5eAttackRange
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|AttackRange")
	FDnd5eDistance Normal;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|AttackRange")
	bool bHasLong = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|AttackRange", meta = (EditCondition = "bHasLong"))
	FDnd5eDistance Long;
};