// Developed by Neko Creative Technologies

#pragma once
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Readers/Items/Dnd5eJsonItemReader.h"
#include "Dnd5eJsonItemLibrary.generated.h"

UCLASS()
class DND5EJSON_API UDnd5eJsonItemLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Dnd5e|Items")
	static FDnd5eJsonItemReadResult ReadItemAtIndex(const FDnd5eJsonDocument& Document, int32 Index);
};