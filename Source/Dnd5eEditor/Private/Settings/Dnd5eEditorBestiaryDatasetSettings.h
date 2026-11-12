// Developed by Neko Creative Technologies

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "UObject/SoftObjectPath.h"
#include "Readers/Bestiary/Dnd5eJsonCreatureCatalogLoader.h"
#include "Dnd5eEditorBestiaryDatasetSettings.generated.h"

UCLASS(Config = EditorPerProjectUserSettings, meta = (DisplayName = "Dnd5e Bestiary Dataset"))
class DND5EEDITOR_API UDnd5eEditorBestiaryDatasetSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UPROPERTY(Config, EditAnywhere, Category = "Dataset")
	FDirectoryPath DatasetRoot;

	UPROPERTY(Config, EditAnywhere, Category = "Dataset")
	FString DatasetNamespace;

	UPROPERTY(Config, EditAnywhere, Category = "Dataset", meta = (FilePathFilter = "json"))
	FFilePath TemplateFile;

	UPROPERTY(Config, EditAnywhere, Category = "Dataset", meta = (FilePathFilter = "json"))
	FFilePath LoreFile;

	virtual FName GetContainerName() const override { return TEXT("Editor"); }
	virtual FName GetCategoryName() const override { return TEXT("General"); }
	virtual FName GetSectionName() const override { return TEXT("Dnd5eBestiaryDataset"); }

	FDnd5eJsonCreatureCatalogLoadRequest MakeLoadRequest() const;
};