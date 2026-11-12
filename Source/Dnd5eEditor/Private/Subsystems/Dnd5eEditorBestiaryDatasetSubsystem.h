// Developed by Neko Creative Technologies

#pragma once

#include "CoreMinimal.h"
#include "EditorSubsystem.h"
#include "Catalog/Dnd5eCreatureCatalogObject.h"
#include "Readers/Bestiary/Dnd5eJsonCreatureCatalogLibrary.h"
#include "Dnd5eEditorBestiaryDatasetSubsystem.generated.h"

UCLASS(Transient)
class DND5EEDITOR_API UDnd5eEditorBestiaryDatasetSubsystem : public UEditorSubsystem
{
	GENERATED_BODY()

public:
	const FDnd5eJsonCreatureCatalogBlueprintLoadResult& LoadSelection(
		const FDnd5eJsonCreatureCatalogLoadRequest& Request);
	UDnd5eCreatureCatalogObject* GetCatalog() const { return Catalog.Get(); }
	int32 GetCreatureCount() const;
	const FDnd5eJsonCreatureCatalogBlueprintLoadResult& GetLastLoadResult() const { return LastLoadResult; }
	FSimpleMulticastDelegate& OnCatalogChanged() { return CatalogChanged; }

	virtual void Deinitialize() override;

private:
	UPROPERTY(Transient)
	TObjectPtr<UDnd5eCreatureCatalogObject> Catalog;

	UPROPERTY(Transient)
	FDnd5eJsonCreatureCatalogBlueprintLoadResult LastLoadResult;

	FSimpleMulticastDelegate CatalogChanged;
};