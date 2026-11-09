// Developed by Neko Creative Technologies

#pragma once
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Catalog/Dnd5eItemDefinitionCatalogObject.h"
#include "Readers/Items/Dnd5eJsonItemDefinitionReader.h"
#include "Dnd5eJsonItemDefinitionCatalogLibrary.generated.h"

UCLASS()
class DND5EJSON_API UDnd5eJsonItemDefinitionCatalogLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Dnd5e|ItemDefinitions")
	static FDnd5eJsonItemDefinitionReadResult ReadItemDefinitions(const FDnd5eJsonDocument& Document);

	UFUNCTION(BlueprintCallable, Category = "Dnd5e|ItemDefinitions")
	static bool BuildItemDefinitionCatalog(UDnd5eItemDefinitionCatalogObject* CatalogObject,
		const FDnd5eJsonDocument& Document, TArray<FDnd5eDiagnostic>& Diagnostics);
};