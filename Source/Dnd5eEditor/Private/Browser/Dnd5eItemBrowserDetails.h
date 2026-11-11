// Developed by Neko Creative Technologies

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

struct FDnd5eItem;
struct FDnd5eContentBlock;

struct FDnd5eItemBrowserDetails
{
	FText Title;
	FText Facts;
	FText Description;
};

namespace Dnd5eItemBrowserDetails
{
	FText FormatRarity(const FGameplayTag& Rarity);
	FDnd5eItemBrowserDetails Build(const FDnd5eItem& Item);
	FText FormatDescription(const TArray<FDnd5eContentBlock>& Blocks);
}