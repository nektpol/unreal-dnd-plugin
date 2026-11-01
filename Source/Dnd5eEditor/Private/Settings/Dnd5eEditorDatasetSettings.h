// Developed by Neko Creative Technologies

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "UObject/SoftObjectPath.h"
#include "Readers/Spells/Dnd5eJsonSpellCatalogLoader.h"
#include "Dnd5eEditorDatasetSettings.generated.h"

/**
 *
 */
UCLASS(Config = EditorPerProjectUserSettings, meta=(DisplayName = "Dnd5e Dataset"))
class DND5EEDITOR_API UDnd5eEditorDatasetSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UPROPERTY(Config, EditAnywhere, Category = "Dataset")
	FDirectoryPath DatasetRoot;

	UPROPERTY(Config, EditAnywhere, Category = "Dataset")
	FString DatasetNamespace;

	UPROPERTY(Config, EditAnywhere, Category = "Dataset", meta = (FilePathFilter = "json"))
	TArray<FFilePath> SpellFiles;

	UPROPERTY(Config, EditAnywhere, Category = "Dataset")
	bool bHasAvailabilityLookup = false;

	UPROPERTY(Config, EditAnywhere, Category = "Dataset", meta = (FilePathFilter = "json", EditCondition = "bHasAvailabilityLookup", EditConditionHides))
	FFilePath AvailabilityFile;

	virtual FName GetContainerName() const override { return TEXT("Editor"); }
	virtual FName GetCategoryName() const override { return TEXT("General"); }
	virtual FName GetSectionName() const override { return TEXT("Dnd5eDataset"); }

	FDnd5eJsonSpellCatalogLoadRequest MakeLoadRequest() const;
};
