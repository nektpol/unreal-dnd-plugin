// Developed by Neko Creative Technologies

#pragma once

#include "CoreMinimal.h"
#include "Dnd5eJsonSpellCatalogBuilder.h"
#include "Dnd5eJsonSpellCatalogLoader.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Dnd5eJsonSpellCatalogLibrary.generated.h"

class UDnd5eSpellCatalogObject;
/**
 * 
 */
UCLASS()
class DND5EJSON_API UDnd5eJsonSpellCatalogLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintCallable, Category = "Dnd5e|Json|SpellCatalog")
	static FDnd5eJsonSpellCatalogBuildResult BuildSpellCatalog(UDnd5eSpellCatalogObject* CatalogObject, const FDnd5eJsonSpellCatalogBuildRequest& Request);

	UFUNCTION(BlueprintCallable, Category = "Dnd5e|Json|SpellCatalog")
	static FDnd5eJsonSpellCatalogBuildResult LoadSpellCatalog(UDnd5eSpellCatalogObject* CatalogObject, const FDnd5eJsonSpellCatalogLoadRequest& Request);
};
