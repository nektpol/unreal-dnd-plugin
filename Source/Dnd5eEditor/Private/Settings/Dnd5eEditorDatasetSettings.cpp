// Developed by Neko Creative Technologies


#include "Settings/Dnd5eEditorDatasetSettings.h"
#include "Misc/Paths.h"

namespace
{
	FString ToDatasetRelativePath(const FString& Filename, const FString& DatasetRoot)
	{
		FString Result = Filename;
		FPaths::NormalizeFilename(Result);
		if (Result.IsEmpty() || FPaths::IsRelative(Result)) { return Result; }

		FString Root = DatasetRoot;
		FPaths::NormalizeDirectoryName(Root);
		if (Root.IsEmpty() || FPaths::IsRelative(Root)) { return Result; }
		if (!Root.EndsWith(TEXT("/"))) { Root += TEXT("/"); }

		FString Relative = Result;
		if (FPaths::MakePathRelativeTo(Relative, *Root)) { return Relative; }
		return Result;
	}
}

FDnd5eJsonSpellCatalogLoadRequest UDnd5eEditorDatasetSettings::MakeLoadRequest() const
{
	FDnd5eJsonSpellCatalogLoadRequest Request;
	Request.DatasetRoot = DatasetRoot.Path;
	Request.DatasetNamespace = DatasetNamespace;
	Request.SpellFiles.Reserve(SpellFiles.Num());
	for (const FFilePath& File : SpellFiles)
	{
		Request.SpellFiles.Add(ToDatasetRelativePath(File.FilePath, DatasetRoot.Path));
	}
	Request.bHasAvailabilityLookup = bHasAvailabilityLookup;
	if (bHasAvailabilityLookup)
	{
		Request.AvailabilityFile = ToDatasetRelativePath(AvailabilityFile.FilePath, DatasetRoot.Path);
	}
	return Request;
}
