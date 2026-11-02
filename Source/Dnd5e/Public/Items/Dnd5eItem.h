// Developed by Neko Creative Technologies

#pragma once
#include "CoreMinimal.h"
#include "Records/Dnd5eRecordHeader.h"
#include "Items/Dnd5eItemTypes.h"
#include "Content/Dnd5eSupplementaryContent.h"
#include "Values/Dnd5eMass.h"
#include "Values/Dnd5ePrice.h"
#include "Values/Dnd5eAttunementRequirement.h"
#include "Dnd5eItem.generated.h"

USTRUCT(BlueprintType)
struct DND5E_API FDnd5eItem
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Item")
	FDnd5eRecordHeader Header;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Item")
	bool bHasType = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Item", meta = (EditCondition = "bHasType"))
	FDnd5eItemDefinitionSelector Type;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Item")
	TArray<FDnd5eItemDefinitionSelector> Properties;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Item")
	FGameplayTag Rarity;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Item")
	TArray<FDnd5eContentBlock> Description;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Item")
	bool bHasSupplementaryContent = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Item", meta = (EditCondition = "bHasSupplementaryContent"))
	FDnd5eSupplementaryContent SupplementaryContent;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Item")
	FDnd5ePrice Price;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Item")
	bool bHasWeight = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Item", meta = (EditCondition = "bHasWeight"))
	FDnd5eMass Weight;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Item")
	bool bHasWeightMultiplier = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Item", meta = (EditCondition = "bHasWeightMultiplier"))
	FDnd5eNumber WeightMultiplier;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Item")
	TArray<FDnd5eInlineContent> WeightNote;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Item")
	FDnd5eAttunementRequirement Attunement;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Item")
	bool bHasAlternativeAttunement = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Item", meta = (EditCondition = "bHasAlternativeAttunement"))
	FDnd5eAttunementRequirement AlternativeAttunement;
};