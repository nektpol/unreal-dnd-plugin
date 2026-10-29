#pragma once

#include "CoreMinimal.h"
#include "Catalog/Dnd5eSpellCatalog.h"
#include "Readers/Dnd5eJsonDocument.h"
#include "Readers/Dnd5eJsonReadStatus.h"
#include "Dnd5eJsonSpellCatalogBuilder.generated.h"

USTRUCT(BlueprintType)
struct DND5EJSON_API FDnd5eJsonSpellCatalogBuildRequest
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Json|Catalog")
	TArray<FDnd5eJsonDocument> SpellDocuments;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Json|Catalog")
	bool bHasAvailabilityLookup = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Json|Catalog")
	FDnd5eJsonDocument AvailabilityLookup;
};

USTRUCT(BlueprintType)
struct DND5EJSON_API FDnd5eJsonSpellCatalogBuildResult
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Json|Catalog")
	EDnd5eJsonReadStatus Status = EDnd5eJsonReadStatus::NotAttempted;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Json|Catalog")
	int32 SpellCount = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Json|Catalog")
	TArray<FDnd5eDiagnostic> Diagnostics;
};

class DND5EJSON_API FDnd5eJsonSpellCatalogBuilder
{
public:
	// A failed build preserves the caller's active catalog.
	static FDnd5eJsonSpellCatalogBuildResult BuildCatalog(const FDnd5eJsonSpellCatalogBuildRequest& Request, FDnd5eSpellCatalog& Catalog);
};
