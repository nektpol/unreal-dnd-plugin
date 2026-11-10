// Developed by Neko Creative Technologies

#pragma once
#include "CoreMinimal.h"

class FJsonObject;
struct FDnd5eItem;

namespace Dnd5eJsonReader
{
	void ReadItemMagicFields(const FJsonObject& Object, const FString& DatasetNamespace, FDnd5eItem& Out,
		TSet<FString>& ConsumedFields);
}