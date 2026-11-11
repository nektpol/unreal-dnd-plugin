// Developed by Neko Creative Technologies

#include "Settings/Dnd5eEditorItemDatasetSettings.h"
#include "Settings/Dnd5eEditorDatasetPaths.h"

namespace
{
	TArray<FString> MakeRelativeItemFileList(const TArray<FFilePath>& Files, const FString& Root)
	{
		TArray<FString> Result;
		Result.Reserve(Files.Num());
		for (const FFilePath& File : Files)
		{
			Result.Add(Dnd5eEditorDatasetPaths::ToDatasetRelativePath(File.FilePath, Root));
		}
		return Result;
	}
}

FDnd5eJsonItemCatalogLoadRequest UDnd5eEditorItemDatasetSettings::MakeLoadRequest() const
{
	FDnd5eJsonItemCatalogLoadRequest Request;
	Request.DatasetRoot = DatasetRoot.Path;
	Request.DatasetNamespace = DatasetNamespace;
	Request.BaseFiles = MakeRelativeItemFileList(BaseFiles, DatasetRoot.Path);
	Request.ItemFiles = MakeRelativeItemFileList(ItemFiles, DatasetRoot.Path);
	Request.DefinitionFiles = MakeRelativeItemFileList(DefinitionFiles, DatasetRoot.Path);
	Request.SupplementFiles = MakeRelativeItemFileList(SupplementFiles, DatasetRoot.Path);
	return Request;
}