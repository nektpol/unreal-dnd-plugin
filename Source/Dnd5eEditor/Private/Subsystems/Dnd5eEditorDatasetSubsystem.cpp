// Developed by Neko Creative Technologies


#include "Subsystems/Dnd5eEditorDatasetSubsystem.h"

#include "Catalog/Dnd5eSpellCatalogObject.h"
#include "Readers/Dnd5eJsonSpellCatalogLibrary.h"

const FDnd5eJsonSpellCatalogBuildResult& UDnd5eEditorDatasetSubsystem::LoadSelection(const FDnd5eJsonSpellCatalogLoadRequest& Request)
{
	if (!IsValid(Catalog)) { Catalog = NewObject<UDnd5eSpellCatalogObject>(this); }
	LastLoadResult = UDnd5eJsonSpellCatalogLibrary::LoadSpellCatalog(Catalog, Request);
	return LastLoadResult;
}

UDnd5eSpellCatalogObject* UDnd5eEditorDatasetSubsystem::GetCatalog() const
{
	return Catalog.Get();
}

int32 UDnd5eEditorDatasetSubsystem::GetSpellCount() const
{
	return IsValid(Catalog) ? Catalog->GetSpellCount() : 0;
}

void UDnd5eEditorDatasetSubsystem::Deinitialize()
{
	Catalog = nullptr;
	LastLoadResult = FDnd5eJsonSpellCatalogBuildResult{};
	Super::Deinitialize();
}
