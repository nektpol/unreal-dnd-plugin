// Developed by Neko Creative Technologies

#pragma once
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Readers/Items/Dnd5eJsonBaseItemReader.h"
#include "Dnd5eJsonBaseItemLibrary.generated.h"

UCLASS()
class DND5EJSON_API UDnd5eJsonBaseItemLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Dnd5e|Items")
	static FDnd5eJsonBaseItemReadResult ReadBaseItems(const FDnd5eJsonDocument& Document);
};