// Developed by Neko Creative Technologies

#pragma once

#include "CoreMinimal.h"
#include "Dnd5eJsonSpellCatalogBuilder.h"
#include "Dnd5eJsonSpellCatalogLoader.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Readers/Dnd5eJsonSpellReferenceNormalizer.h"
#include "Readers/Dnd5eJsonSpellReferenceReporter.h"
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
	static FDnd5eJsonSpellCatalogBuildResult BuildSpellCatalog(
		UDnd5eSpellCatalogObject* CatalogObject,
		const FDnd5eJsonSpellCatalogBuildRequest& Request);

	UFUNCTION(BlueprintCallable, Category = "Dnd5e|Json|SpellCatalog")
	static FDnd5eJsonSpellCatalogBuildResult LoadSpellCatalog(
		UDnd5eSpellCatalogObject* CatalogObject,
		const FDnd5eJsonSpellCatalogLoadRequest& Request);

	UFUNCTION(BlueprintCallable, Category = "Dnd5e|Json|SpellCatalog")
	static EDnd5eJsonSpellReferenceNormalizeStatus NormalizeSpellReference(
		UDnd5eSpellCatalogObject* CatalogObject,
		const FString& DatasetNamespace,
		const FDnd5eReferenceContent& Reference,
		FDnd5eReferenceContent& NormalizedReference,
		TArray<FDnd5eDiagnostic>& Diagnostics);

	UFUNCTION(BlueprintCallable, Category = "Dnd5e|Json|SpellCatalog")
	static FDnd5eJsonSpellReferenceReport ReportSpellReferences(
		UDnd5eSpellCatalogObject* CatalogObject,
		const FDnd5eSpell& Spell);
};
