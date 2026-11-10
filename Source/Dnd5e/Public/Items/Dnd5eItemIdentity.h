// Developed by Neko Creative Technologies
#pragma once

#include "CoreMinimal.h"
#include "CoreMinimal.h"
#include "Records/Dnd5eRecordId.h"

namespace Dnd5e::Items
{
	DND5E_API bool TryMakeId(const FString& DatasetNamespace, const FString& Name,
		const FString& SourceId, FDnd5eRecordId& OutId);

	DND5E_API bool TryMakeGroupId(const FString& DatasetNamespace, const FString& Name,
	const FString& SourceId, FDnd5eRecordId& OutId);
}
