// Developed by Neko Creative Technologies

#pragma once
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Items/Dnd5eItem.h"
#include "Dnd5eItemLibrary.generated.h"

UCLASS()
class DND5E_API UDnd5eItemLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Dnd5e|Items")
	static bool TryGetReferencedBaseItem(const FDnd5eItem& Item, const TArray<FDnd5eItem>& BaseItems,
		FDnd5eItem& OutBaseItem);
};
