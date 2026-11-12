// Developed by Neko Creative Technologies

#pragma once

#include "CoreMinimal.h"
#include "Readers/Bestiary/Dnd5eJsonCreatureReader.h"

class FDnd5eCreatureCatalog;

struct DND5EJSON_API FDnd5eJsonCreatureCatalogLoadRequest
{
	FString DatasetRoot;
	FString DatasetNamespace;
	FString TemplateFile;
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