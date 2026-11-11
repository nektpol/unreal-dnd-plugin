// Developed by Neko Creative Technologies


#include "Settings/Dnd5eEditorDatasetSettings.h"
#include "Settings/Dnd5eEditorDatasetPaths.h"

FDnd5eJsonSpellCatalogLoadRequest UDnd5eEditorDatasetSettings::MakeLoadRequest() const
{
	FDnd5eJsonSpellCatalogLoadRequest Request;
	Request.DatasetRoot = DatasetRoot.Path;
	Request.DatasetNamespace = DatasetNamespace;
	Request.SpellFiles.Reserve(SpellFiles.Num());
	for (const FFilePath& File : SpellFiles)
	{
		Request.SpellFiles.Add(Dnd5eEditorDatasetPaths::ToDatasetRelativePath(File.FilePath, DatasetRoot.Path));
	}
	Request.bHasAvailabilityLookup = bHasAvailabilityLookup;
	if (bHasAvailabilityLookup)
	{
		Request.AvailabilityFile = Dnd5eEditorDatasetPaths::ToDatasetRelativePath(AvailabilityFile.FilePath, DatasetRoot.Path);
	}
	return Request;
}
