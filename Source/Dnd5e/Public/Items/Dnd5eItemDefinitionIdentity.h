// Developed by Neko Creative Technologies

#pragma once
#include "CoreMinimal.h"
#include "Records/Dnd5eRecordId.h"

namespace Dnd5e::ItemDefinitions
{
	DND5E_API bool TryMakeId(const FString& DatasetNamespace, FGameplayTag Category,
		const FString& Key, const FString& SourceId, FDnd5eRecordId& OutId);
}