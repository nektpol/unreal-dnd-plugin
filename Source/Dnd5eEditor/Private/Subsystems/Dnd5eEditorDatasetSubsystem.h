// Developed by Neko Creative Technologies

#pragma once

#include "CoreMinimal.h"
#include "EditorSubsystem.h"
#include "Readers/Dnd5eJsonSpellCatalogLoader.h"
#include "Dnd5eEditorDatasetSubsystem.generated.h"

/**
 *
 */

class UDnd5eSpellCatalogObject;

UCLASS(Transient)
class DND5EEDITOR_API UDnd5eEditorDatasetSubsystem : public UEditorSubsystem
{
	GENERATED_BODY()

public:
	const FDnd5eJsonSpellCatalogBuildResult& LoadSelection(const FDnd5eJsonSpellCatalogLoadRequest& Request);
	UDnd5eSpellCatalogObject* GetCatalog() const;
	int32 GetSpellCount() const;
	const FDnd5eJsonSpellCatalogBuildResult& GetLastLoadResult() const { return LastLoadResult; }
	virtual void Deinitialize() override;

	FSimpleMulticastDelegate& OnCatalogChanged() { return CatalogChanged; }

private:
	UPROPERTY(Transient)
	TObjectPtr<UDnd5eSpellCatalogObject> Catalog;

	UPROPERTY(Transient)
	FDnd5eJsonSpellCatalogBuildResult LastLoadResult;

	FSimpleMulticastDelegate CatalogChanged;
};
