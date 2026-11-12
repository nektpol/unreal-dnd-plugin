// Developed by Neko Creative Technologies

#include "Settings/Dnd5eEditorBestiaryDatasetSettings.h"
#include "Settings/Dnd5eEditorDatasetPaths.h"

FDnd5eJsonCreatureCatalogLoadRequest UDnd5eEditorBestiaryDatasetSettings::MakeLoadRequest() const
{
	FDnd5eJsonCreatureCatalogLoadRequest Request;
	Request.DatasetRoot = DatasetRoot.Path;
	Request.DatasetNamespace = DatasetNamespace;
	Request.TemplateFile = Dnd5eEditorDatasetPaths::ToDatasetRelativePath(TemplateFile.FilePath, DatasetRoot.Path);
	Request.LoreFile = Dnd5eEditorDatasetPaths::ToDatasetRelativePath(LoreFile.FilePath, DatasetRoot.Path);
	return Request;
}