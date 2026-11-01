#pragma once

#include "CoreMinimal.h"
#include "Readers/Dnd5eJsonSpellCatalogBuilder.h"
#include "Dnd5eJsonSpellCatalogLoader.generated.h"

USTRUCT(BlueprintType)
struct DND5EJSON_API FDnd5eJsonSpellCatalogLoadRequest
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Json|Catalog")
	FString DatasetRoot;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Json|Catalog")
	FString DatasetNamespace;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Json|Catalog")
	TArray<FString> SpellFiles;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Json|Catalog")
	bool bHasAvailabilityLookup = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Json|Catalog")
	FString AvailabilityFile;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dnd5e|Json|Catalog")
	TArray<FString> SpellFluffFiles;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dnd5e|Json|Catalog")
	FString EditionPolicyId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dnd5e|Json|Catalog")
	TArray<FDnd5eSourceEditionAssignment> EditionAssignments;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dnd5e|Json|Catalog")
	bool bValidateContent = false;
};

class DND5EJSON_API FDnd5eJsonSpellCatalogLoader
{
public:
	static FDnd5eJsonSpellCatalogBuildResult LoadCatalog(const FDnd5eJsonSpellCatalogLoadRequest& Request, FDnd5eSpellCatalog& Catalog);
};
