// Developed by Neko Creative Technologies

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Readers/Bestiary/Dnd5eJsonCreatureCatalogLoader.h"
#include "Dnd5eJsonCreatureCatalogLibrary.generated.h"

class UDnd5eCreatureCatalogObject;

USTRUCT(BlueprintType)
struct DND5EJSON_API FDnd5eJsonCreatureCatalogBlueprintLoadResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Dnd5e|CreatureCatalog")
	EDnd5eJsonReadStatus Status = EDnd5eJsonReadStatus::NotAttempted;

	UPROPERTY(BlueprintReadOnly, Category = "Dnd5e|CreatureCatalog")
	int32 CreatureCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Dnd5e|CreatureCatalog")
	TArray<FDnd5eDiagnostic> Diagnostics;
};

UCLASS()
class DND5EJSON_API UDnd5eJsonCreatureCatalogLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Dnd5e|Json|CreatureCatalog")
	static FDnd5eJsonCreatureCatalogBlueprintLoadResult LoadCreatureCatalog(
		UDnd5eCreatureCatalogObject* CatalogObject, const FDnd5eJsonCreatureCatalogLoadRequest& Request);
};