// Developed by Neko Creative Technologies

#pragma once
#include "Catalog/Dnd5eItemCatalogObject.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Readers/Dnd5eJsonDocument.h"
#include "Readers/Dnd5eJsonReadStatus.h"
#include "Dnd5eJsonItemCatalogBuilder.generated.h"

USTRUCT(BlueprintType)
struct DND5EJSON_API FDnd5eJsonItemCatalogBuildRequest
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|ItemCatalog")
	TArray<FDnd5eJsonDocument> BaseDocuments;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|ItemCatalog")
	TArray<FDnd5eJsonDocument> ItemDocuments;
};

USTRUCT(BlueprintType)
struct DND5EJSON_API FDnd5eJsonItemCatalogBuildResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Dnd5e|ItemCatalog")
	EDnd5eJsonReadStatus Status = EDnd5eJsonReadStatus::NotAttempted;
	UPROPERTY(BlueprintReadOnly, Category = "Dnd5e|ItemCatalog")
	int32 RecordCount = 0;
	UPROPERTY(BlueprintReadOnly, Category = "Dnd5e|ItemCatalog")
	TArray<FDnd5eDiagnostic> Diagnostics;
};

UCLASS()
class DND5EJSON_API UDnd5eJsonItemCatalogBuilder : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Dnd5e|ItemCatalog")
	static FDnd5eJsonItemCatalogBuildResult BuildItemCatalog(const FDnd5eJsonItemCatalogBuildRequest& Request,
		UDnd5eItemCatalogObject* Catalog);
};