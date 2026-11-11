// Developed by Neko Creative Technologies

#pragma once
#include "Readers/Items/Dnd5eJsonItemCatalogBuilder.h"
#include "Catalog/Dnd5eItemDefinitionCatalogObject.h"
#include "Dnd5eJsonItemCatalogLoader.generated.h"

USTRUCT(BlueprintType)
struct DND5EJSON_API FDnd5eJsonItemCatalogLoadRequest
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Json|ItemCatalog")
	FString DatasetRoot;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Json|ItemCatalog")
	FString DatasetNamespace;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Json|ItemCatalog")
	TArray<FString> BaseFiles;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Json|ItemCatalog")
	TArray<FString> ItemFiles;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Json|ItemCatalog")
	TArray<FString> VariantFiles;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Json|ItemCatalog")
	TArray<FDnd5eJsonItemCatalogVariantSelection> VariantSelections;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Json|ItemCatalog")
	TArray<FString> SupplementFiles;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Json|ItemCatalog")
	TArray<FString> DefinitionFiles;
};

USTRUCT(BlueprintType)
struct DND5EJSON_API FDnd5eJsonItemCatalogLoadResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Dnd5e|Json|ItemCatalog")
	EDnd5eJsonReadStatus Status = EDnd5eJsonReadStatus::NotAttempted;
	UPROPERTY(BlueprintReadOnly, Category = "Dnd5e|Json|ItemCatalog")
	TObjectPtr<UDnd5eItemCatalogObject> ItemCatalog = nullptr;
	UPROPERTY(BlueprintReadOnly, Category = "Dnd5e|Json|ItemCatalog")
	TObjectPtr<UDnd5eItemDefinitionCatalogObject> DefinitionCatalog = nullptr;
	UPROPERTY(BlueprintReadOnly, Category = "Dnd5e|Json|ItemCatalog")
	int32 RecordCount = 0;
	UPROPERTY(BlueprintReadOnly, Category = "Dnd5e|Json|ItemCatalog")
	int32 DefinitionCount = 0;
	UPROPERTY(BlueprintReadOnly, Category = "Dnd5e|Json|ItemCatalog")
	TArray<FDnd5eDiagnostic> Diagnostics;
};

UCLASS()
class DND5EJSON_API UDnd5eJsonItemCatalogLoader : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Dnd5e|Json|ItemCatalog")
	static FDnd5eJsonItemCatalogLoadResult LoadItemCatalog(const FDnd5eJsonItemCatalogLoadRequest& Request);
};