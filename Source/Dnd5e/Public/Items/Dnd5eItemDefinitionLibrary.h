// Developed by Neko Creative Technologies

#pragma once
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Records/Dnd5eRecordId.h"
#include "Dnd5eItemDefinitionLibrary.generated.h"

UCLASS()
class DND5E_API UDnd5eItemDefinitionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Dnd5e|ItemDefinitions")
	static bool TryMakeDefinitionId(const FString& DatasetNamespace, FGameplayTag Category,
		const FString& Key, const FString& SourceId, FDnd5eRecordId& OutId);
};
