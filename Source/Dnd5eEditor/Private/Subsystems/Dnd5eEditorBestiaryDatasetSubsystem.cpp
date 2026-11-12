// Developed by Neko Creative Technologies

#include "Subsystems/Dnd5eEditorBestiaryDatasetSubsystem.h"

const FDnd5eJsonCreatureCatalogBlueprintLoadResult& UDnd5eEditorBestiaryDatasetSubsystem::LoadSelection(
	const FDnd5eJsonCreatureCatalogLoadRequest& Request)
{
	auto* Candidate = Catalog.Get();
	if (!Candidate)
	{
		Candidate = NewObject<UDnd5eCreatureCatalogObject>(this);
	}

	LastLoadResult = UDnd5eJsonCreatureCatalogLibrary::LoadCreatureCatalog(Candidate, Request);
	if (LastLoadResult.Status == EDnd5eJsonReadStatus::Success)
	{
		Catalog = Candidate;
		CatalogChanged.Broadcast();
	}
	return LastLoadResult;
}

int32 UDnd5eEditorBestiaryDatasetSubsystem::GetCreatureCount() const
{
	return Catalog ? Catalog->GetCreatureCount() : 0;
}

void UDnd5eEditorBestiaryDatasetSubsystem::Deinitialize()
{
	CatalogChanged.Clear();
	LastLoadResult = {};
	Catalog = nullptr;
	Super::Deinitialize();
}