// Developed by Neko Creative Technologies

#pragma once

#include "CoreMinimal.h"
#include "Readers/Bestiary/Dnd5eJsonCreatureReader.h"
#include "Dnd5eJsonCreatureCatalogLoader.generated.h"

class FDnd5eCreatureCatalog;

USTRUCT(BlueprintType)
struct DND5EJSON_API FDnd5eJsonCreatureCatalogLoadRequest
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|CreatureCatalog")
	FString DatasetRoot;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|CreatureCatalog")
	FString DatasetNamespace;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|CreatureCatalog")
	FString TemplateFile;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|CreatureCatalog")
	FString LoreFile;
};

struct DND5EJSON_API FDnd5eJsonCreatureCatalogLoadResult
{
	EDnd5eJsonReadStatus Status = EDnd5eJsonReadStatus::NotAttempted;
	int32 CreatureCount = 0;
	TArray<FDnd5eDiagnostic> Diagnostics;
	TArray<FDnd5eCreatureDocumentAudit> Documents;
};

class DND5EJSON_API FDnd5eJsonCreatureCatalogLoader
{
public:
	static FDnd5eJsonCreatureCatalogLoadResult LoadCatalog(
		const FDnd5eJsonCreatureCatalogLoadRequest& Request, FDnd5eCreatureCatalog& Catalog);
};