// Developed by Neko Creative Technologies

#include "Subsystems/Dnd5eEditorItemDatasetSubsystem.h"

const FDnd5eJsonItemCatalogLoadResult& UDnd5eEditorItemDatasetSubsystem::LoadSelection(
	const FDnd5eJsonItemCatalogLoadRequest& Request)
{
	LastLoadResult = UDnd5eJsonItemCatalogLoader::LoadItemCatalog(Request);
	if (LastLoadResult.Status == EDnd5eJsonReadStatus::Success)
	{
		ItemCatalog = LastLoadResult.ItemCatalog;
		DefinitionCatalog = LastLoadResult.DefinitionCatalog;
		CatalogChanged.Broadcast();
	}
	return LastLoadResult;
}

int32 UDnd5eEditorItemDatasetSubsystem::GetRecordCount() const
{
	return ItemCatalog ? ItemCatalog->GetRecordCount() : 0;
}

int32 UDnd5eEditorItemDatasetSubsystem::GetDefinitionCount() const
{
	return DefinitionCatalog ? DefinitionCatalog->GetDefinitionCount() : 0;
}

void UDnd5eEditorItemDatasetSubsystem::Deinitialize()
{
	CatalogChanged.Clear();
	LastLoadResult = {};
	ItemCatalog = nullptr;
	DefinitionCatalog = nullptr;
	Super::Deinitialize();
}