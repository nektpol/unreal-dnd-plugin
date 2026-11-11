// Developed by Neko Creative Technologies

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "UObject/SoftObjectPath.h"
#include "Readers/Items/Dnd5eJsonItemCatalogLoader.h"
#include "Dnd5eEditorItemDatasetSettings.generated.h"

UCLASS(Config = EditorPerProjectUserSettings, meta = (DisplayName = "Dnd5e Item Dataset"))
class DND5EEDITOR_API UDnd5eEditorItemDatasetSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UPROPERTY(Config, EditAnywhere, Category = "Dataset")
	FDirectoryPath DatasetRoot;

	UPROPERTY(Config, EditAnywhere, Category = "Dataset")
	FString DatasetNamespace;

	UPROPERTY(Config, EditAnywhere, Category = "Dataset", meta = (FilePathFilter = "json"))
	TArray<FFilePath> BaseFiles;

	UPROPERTY(Config, EditAnywhere, Category = "Dataset", meta = (FilePathFilter = "json"))
	TArray<FFilePath> ItemFiles;

	UPROPERTY(Config, EditAnywhere, Category = "Dataset", meta = (FilePathFilter = "json"))
	TArray<FFilePath> DefinitionFiles;

	UPROPERTY(Config, EditAnywhere, Category = "Dataset", meta = (FilePathFilter = "json"))
	TArray<FFilePath> SupplementFiles;

	virtual FName GetContainerName() const override { return TEXT("Editor"); }
	virtual FName GetCategoryName() const override { return TEXT("General"); }
	virtual FName GetSectionName() const override { return TEXT("Dnd5eItemDataset"); }

	FDnd5eJsonItemCatalogLoadRequest MakeLoadRequest() const;
};