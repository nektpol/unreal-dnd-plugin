// Developed by Neko Creative Technologies

#pragma once

#include "CoreMinimal.h"
#include "Dnd5eMoney.h"
#include "Content/Dnd5eContentType.h"
#include "GameplayTagContainer.h"
#include "Dnd5eComponents.generated.h"

UENUM(BlueprintType)
enum class EDnd5eMaterialConsumption : uint8
{
	Unspecified,
	Consumed,
	Optional,
	NotConsumed,
};

USTRUCT(BlueprintType)
struct DND5E_API FDnd5eMaterialComponent
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|MaterialComponents")
	TArray<FDnd5eContentBlock> Description;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|MaterialComponents")
	bool bHasCost = false;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|MaterialComponents", meta = (EditCondition = "bHasCost"))
	TArray<FDnd5eMoney> Cost;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|MaterialComponents")
	EDnd5eMaterialConsumption Consumption = EDnd5eMaterialConsumption::Unspecified;
	
};

USTRUCT(BlueprintType)
struct DND5E_API FDnd5eComponents
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Components")
	FGameplayTagContainer Types;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Components")
	bool bHasMaterial = false;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Components", meta = (EditCondition = "bHasMaterial"))
	FDnd5eMaterialComponent Material;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Components")
	TArray<FDnd5eContentBlock> Description;
};