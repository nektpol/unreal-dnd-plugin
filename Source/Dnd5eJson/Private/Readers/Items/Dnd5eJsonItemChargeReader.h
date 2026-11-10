// Developed by Neko Creative Technologies

#pragma once
#include "CoreMinimal.h"
#include "Items/Dnd5eItemChargeData.h"

class FJsonObject;

namespace Dnd5eJsonReader
{
	void ReadItemChargeFields(const FJsonObject& Object, FDnd5eItemChargeData& Out, TSet<FString>& ConsumedFields);
}