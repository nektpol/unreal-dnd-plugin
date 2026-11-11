// Developed by Neko Creative Technologies

#pragma once

#include "CoreMinimal.h"
#include "Misc/Paths.h"

namespace Dnd5eEditorDatasetPaths
{
	inline FString ToDatasetRelativePath(const FString& Filename, const FString& DatasetRoot)
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