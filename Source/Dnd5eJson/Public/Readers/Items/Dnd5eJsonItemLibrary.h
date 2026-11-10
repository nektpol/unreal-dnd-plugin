// Developed by Neko Creative Technologies

#pragma once
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Readers/Items/Dnd5eJsonItemReader.h"
#include "Readers/Items/Dnd5eJsonItemCopyAssembler.h"
#include "Dnd5eJsonItemLibrary.generated.h"

UCLASS()
class DND5EJSON_API UDnd5eJsonItemLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Dnd5e|Items")
	static FDnd5eJsonItemReadResult ReadItemAtIndex(const FDnd5eJsonDocument& Document, int32 Index);

	UFUNCTION(BlueprintCallable, Category = "Dnd5e|Items")
	static FDnd5eJsonItemCopyAssemblyResult AssembleItemCopyAtIndex(const FDnd5eJsonDocument& Document, int32 Index);

	UFUNCTION(BlueprintCallable, Category = "Dnd5e|Items")
	static FDnd5eJsonItemCopyReadResult ReadItemCopyAtIndex(const FDnd5eJsonDocument& Document, int32 Index);
};
