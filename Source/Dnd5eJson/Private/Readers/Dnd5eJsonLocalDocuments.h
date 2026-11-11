// Developed by Neko Creative Technologies

#pragma once
#include "Diagnostics/Dnd5eDiagnostic.h"
#include "Readers/Dnd5eJsonDocument.h"

namespace Dnd5eJsonLocalDocuments
{
	bool LoadError(TArray<FDnd5eDiagnostic>& Diagnostics, FName Code, const TCHAR* Message,
		const FString& RelativeFilePath = FString{});
	bool ResolveDatasetRoot(const FString& RequestedRoot, const FString& DatasetNamespace, FString& Root,
		TArray<FDnd5eDiagnostic>& Diagnostics);
	bool ResolveSelectedFile(const FString& Root, const FString& RelativeFilePath, const FString& DatasetNamespace,
		FString& FullPath, TArray<FDnd5eDiagnostic>& Diagnostics);
	bool ReadDocument(const FString& FullPath, const FString& RelativeFilePath, const FString& DatasetNamespace,
		int64& TotalBytes, FDnd5eJsonDocument& Document, TArray<FDnd5eDiagnostic>& Diagnostics);
}