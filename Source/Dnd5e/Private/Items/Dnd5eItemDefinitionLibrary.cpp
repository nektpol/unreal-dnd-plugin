// Developed by Neko Creative Technologies

#include "Items/Dnd5eItemDefinitionLibrary.h"
#include "Items/Dnd5eItemDefinitionIdentity.h"

bool UDnd5eItemDefinitionLibrary::TryMakeDefinitionId(const FString& DatasetNamespace,
	FGameplayTag Category, const FString& Key, const FString& SourceId, FDnd5eRecordId& OutId)
{
	return Dnd5e::ItemDefinitions::TryMakeId(DatasetNamespace, Category, Key, SourceId, OutId);
}