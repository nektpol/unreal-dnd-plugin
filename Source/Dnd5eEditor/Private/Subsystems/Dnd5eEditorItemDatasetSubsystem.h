// Developed by Neko Creative Technologies

#pragma once

#include "CoreMinimal.h"
#include "EditorSubsystem.h"
#include "Readers/Items/Dnd5eJsonItemCatalogLoader.h"
#include "Dnd5eEditorItemDatasetSubsystem.generated.h"

UCLASS(Transient)
class DND5EEDITOR_API UDnd5eEditorItemDatasetSubsystem : public UEditorSubsystem
{
	GENERATED_BODY()

public:
	const FDnd5eJsonItemCatalogLoadResult& LoadSelection(const FDnd5eJsonItemCatalogLoadRequest& Request);
	UDnd5eItemCatalogObject* GetItemCatalog() const { return ItemCatalog.Get(); }
	UDnd5eItemDefinitionCatalogObject* GetDefinitionCatalog() const { return DefinitionCatalog.Get(); }
	int32 GetRecordCount() const;
	int32 GetDefinitionCount() const;
	const FDnd5eJsonItemCatalogLoadResult& GetLastLoadResult() const { return LastLoadResult; }
	FSimpleMulticastDelegate& OnCatalogChanged() { return CatalogChanged; }
	virtual void Deinitialize() override;

private:
	UPROPERTY(Transient)
	TObjectPtr<UDnd5eItemCatalogObject> ItemCatalog;

	UPROPERTY(Transient)
	TObjectPtr<UDnd5eItemDefinitionCatalogObject> DefinitionCatalog;

	UPROPERTY(Transient)
	FDnd5eJsonItemCatalogLoadResult LastLoadResult;

	FSimpleMulticastDelegate CatalogChanged;
};