// Developed by Neko Creative Technologies

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

struct FDnd5eItem;
struct FDnd5eContentBlock;
class UDnd5eItemDefinitionCatalogObject;

struct FDnd5eItemBrowserDetails
{
	FText Title;
	FText Source;
	FText Classification;
	FText Facts;
	FText Equipment;
	FText Description;
};

namespace Dnd5eItemBrowserDetails
{
	FText FormatRarity(const FGameplayTag& Rarity);
	FDnd5eItemBrowserDetails Build(const FDnd5eItem& Item, const UDnd5eItemDefinitionCatalogObject* Definitions = nullptr);
	FText FormatDescription(const TArray<FDnd5eContentBlock>& Blocks);
}
