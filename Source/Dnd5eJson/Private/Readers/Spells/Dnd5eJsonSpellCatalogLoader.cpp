#include "Readers/Spells/Dnd5eJsonSpellCatalogLoader.h"
#include "Readers/Dnd5eJsonReaderInternal.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/Archive.h"

namespace
{
	constexpr int64 MaxFileBytes = 16777216;
	constexpr int64 MaxTotalBytes = 67108864;
	bool LoadError(TArray<FDnd5eDiagnostic>& Diagnostics, FName Code, const TCHAR* Message, const FString& RelativeFilePath = FString{})
	{
		FDnd5eDiagnostic Diagnostic;
		Diagnostic.Severity = EDnd5eDiagnosticSeverity::Error;
		Diagnostic.Code = Code;
		Diagnostic.Message = Message;
		Diagnostic.RelativeFilePath = RelativeFilePath;
		Diagnostics.Add(MoveTemp(Diagnostic));
		return false;
	}

	bool ResolveSelectedFile(const FString& Root, const FString& RelativeFilePath, const FString& DatasetNamespace, FString& FullPath, TArray<FDnd5eDiagnostic>& Diagnostics)
	{
		FullPath.Reset();
		if (RelativeFilePath.IsEmpty())
		{
			return LoadError(Diagnostics, TEXT("Dnd5e.Json.Load.InvalidFilePath"), TEXT("Supply a relative JSON filename."));
		}
		FDnd5eJsonReadContext Context;
		Context.DatasetNamespace = DatasetNamespace;
		Context.RelativeFilePath = RelativeFilePath;
		const FString EmptyText;
		Dnd5eJsonReader::FReadState State{Context, EmptyText, Diagnostics, FString{}};
		if (!Dnd5eJsonReader::ValidateReadContext(State)) { return false; }
		for (TCHAR Character : RelativeFilePath)
		{
			if (FCString::Strchr(TEXT("*?<>|\""), Character))
			{
				return LoadError(Diagnostics, TEXT("Dnd5e.Json.Load.InvalidFilePath"), TEXT("Selected filenames cannot contain wildcards or reserved characters."), RelativeFilePath);
			}
		}
		if (!FPaths::GetExtension(RelativeFilePath).Equals(TEXT("json"), ESearchCase::IgnoreCase))
		{
			return LoadError(Diagnostics, TEXT("Dnd5e.Json.Load.InvalidFilePath"), TEXT("Select a JSON file."), RelativeFilePath);
		}
		FString Candidate = FPaths::Combine(Root, RelativeFilePath);
		if (!FPaths::CollapseRelativeDirectories(Candidate) || !FPaths::IsUnderDirectory(Candidate, Root))
		{
			return LoadError(Diagnostics, TEXT("Dnd5e.Json.Load.InvalidFilePath"), TEXT("The selected file must be inside the dataset root."), RelativeFilePath);
		}
		FullPath = MoveTemp(Candidate);
		return true;
	}

	bool ValidateLoadSelection(const FDnd5eJsonSpellCatalogLoadRequest& Request, FString& Root, TArray<FString>& SpellPaths, TArray<FString>& FluffPaths, FString& AvailabilityPath, TArray<FDnd5eDiagnostic>& Diagnostics)
	{
		Root.Reset();
		SpellPaths.Reset(); FluffPaths.Reset();
		AvailabilityPath.Reset();
	    if (Request.SpellFiles.IsEmpty())
		{
		    return LoadError(Diagnostics, TEXT("Dnd5e.Json.Load.InvalidRequest"), TEXT("Select at least one spell file."));
		}
		const int64 Count = static_cast<int64>(Request.SpellFiles.Num()) + Request.SpellFluffFiles.Num() + (Request.bHasAvailabilityLookup ? 1 : 0);
		if (Count > 1024)
		{
			return LoadError(Diagnostics, TEXT("Dnd5e.Json.ResourceLimitExceeded"), TEXT("Select at most 1024 total input files."));
		}
		FDnd5eJsonReadContext Context;
		Context.DatasetNamespace = Request.DatasetNamespace;
		const FString EmptyText;
		Dnd5eJsonReader::FReadState State{Context, EmptyText, Diagnostics, FString{}};
		if (!Dnd5eJsonReader::ValidateReadContext(State)) { return false; }

		Root = Request.DatasetRoot;
		if (Root.IsEmpty() || !Root.Equals(Root.TrimStartAndEnd(), ESearchCase::CaseSensitive) || Root.Contains(TEXT("://")))
		{
			return LoadError(Diagnostics, TEXT("Dnd5e.Json.Load.InvalidDatasetRoot"), TEXT("Supply an absolute dataset directory without edge whitespace or a URL scheme."));
		}
		for (TCHAR Character : Root)
		{
			if (FChar::IsControl(Character) || FCString::Strchr(TEXT("*?<>|\""), Character))
			{
				return LoadError(Diagnostics, TEXT("Dnd5e.Json.Load.InvalidDatasetRoot"), TEXT("The dataset directory contains unsupported characters."));
			}
		}
		FPaths::NormalizeDirectoryName(Root);
		const bool bDriveRelative = Root.Len() >= 2 && Root[1] == TEXT(':') && (Root.Len() < 3 || Root[2] != TEXT('/'));
		if (FPaths::IsRelative(Root) || bDriveRelative)
		{
			return LoadError(Diagnostics, TEXT("Dnd5e.Json.Load.InvalidDatasetRoot"), TEXT("The dataset directory must be absolute."));
		}
	#if PLATFORM_WINDOWS
		const bool bDriveRooted = Root.Len() >= 3 && FChar::IsAlpha(Root[0]) && Root[1] == TEXT(':') && Root[2] == TEXT('/');
		if (!bDriveRooted && !Root.StartsWith(TEXT("//")))
		{
			return LoadError(Diagnostics, TEXT("Dnd5e.Json.Load.InvalidDatasetRoot"), TEXT("Supply a drive-qualified or UNC dataset directory."));
		}
	#endif
		if (!FPaths::CollapseRelativeDirectories(Root) || !IFileManager::Get().DirectoryExists(*Root))
		{
			return LoadError(Diagnostics, TEXT("Dnd5e.Json.Load.InvalidDatasetRoot"), TEXT("The dataset directory does not exist or cannot be resolved."));
		}
		Root = FPaths::ConvertRelativePathToFull(Root);
		SpellPaths.Reserve(Request.SpellFiles.Num());
		for (const FString& RelativeFilePath : Request.SpellFiles)
		{
		    FString FullPath;
		    if (!ResolveSelectedFile(Root, RelativeFilePath, Request.DatasetNamespace, FullPath, Diagnostics)) { return false; }
		    if (SpellPaths.ContainsByPredicate([&FullPath](const FString& Existing) { return FPaths::IsSamePath(Existing, FullPath); }))
		    {
		        return LoadError(Diagnostics, TEXT("Dnd5e.Json.Load.DuplicateFile"), TEXT("Each selected file must occur only once."), RelativeFilePath);
		    }
		    SpellPaths.Add(MoveTemp(FullPath));
		}
		TArray<FString> AllPaths = SpellPaths;
		for (const FString& Relative : Request.SpellFluffFiles)
		{
			FString Full;
			if (!ResolveSelectedFile(Root, Relative, Request.DatasetNamespace, Full, Diagnostics)) { return false; }
			if (AllPaths.ContainsByPredicate([&Full](const FString& Existing) { return FPaths::IsSamePath(Existing, Full); }))
			{
				return LoadError(Diagnostics, TEXT("Dnd5e.Json.Load.DuplicateFile"), TEXT("Each selected input file must be distinct."), Relative);
			}
			AllPaths.Add(Full); FluffPaths.Add(MoveTemp(Full));
		}
		if (Request.bHasAvailabilityLookup)
		{
		    if (!ResolveSelectedFile(Root, Request.AvailabilityFile, Request.DatasetNamespace, AvailabilityPath, Diagnostics)) { return false; }
		    if (AllPaths.ContainsByPredicate([&AvailabilityPath](const FString& Existing) { return FPaths::IsSamePath(Existing, AvailabilityPath); }))
		    {
			    return LoadError(Diagnostics, TEXT("Dnd5e.Json.Load.DuplicateFile"), TEXT("The availability lookup must be a separate selected file."), Request.AvailabilityFile);
		    }
		}
		return true;
	}

	bool ReadDocument(const FString& FullPath, const FString& RelativeFilePath, const FString& DatasetNamespace, int64& TotalBytes, FDnd5eJsonDocument& Document, TArray<FDnd5eDiagnostic>& Diagnostics)
	{
		Document = FDnd5eJsonDocument{};
		TUniquePtr<FArchive> Reader(IFileManager::Get().CreateFileReader(*FullPath, FILEREAD_Silent));
		if (!Reader)
		{
			return LoadError(Diagnostics, TEXT("Dnd5e.Json.Load.FileReadFailed"), TEXT("Could not open the selected file."), RelativeFilePath);
		}
		const int64 Size = Reader->TotalSize();
		if (Size < 0)
		{
			return LoadError(Diagnostics, TEXT("Dnd5e.Json.Load.FileReadFailed"), TEXT("Could not determine the selected file size."), RelativeFilePath);
		}
		if (Size > MaxFileBytes || Size > MaxTotalBytes - TotalBytes)
		{
			return LoadError(Diagnostics, TEXT("Dnd5e.Json.ResourceLimitExceeded"), TEXT("Selected files exceed the byte limits."), RelativeFilePath);
		}
		TArray<uint8> Bytes;
		Bytes.SetNumUninitialized(static_cast<int32>(Size));
		if (Size > 0) { Reader->Serialize(Bytes.GetData(), Size); }
		if (Reader->IsError() || Reader->Tell() != Size)
		{
			return LoadError(Diagnostics, TEXT("Dnd5e.Json.Load.FileReadFailed"), TEXT("Could not read the complete selected file."), RelativeFilePath);
		}
		if (Size > 0) { FFileHelper::BufferToString(Document.JsonText, Bytes.GetData(), Bytes.Num()); }
		Document.Context.DatasetNamespace = DatasetNamespace;
		Document.Context.RelativeFilePath = RelativeFilePath;
		TotalBytes += Size;
		return true;
	}
}

FDnd5eJsonSpellCatalogBuildResult FDnd5eJsonSpellCatalogLoader::LoadCatalog(const FDnd5eJsonSpellCatalogLoadRequest& Request,
	FDnd5eSpellCatalog& Catalog)
{
	FDnd5eJsonSpellCatalogBuildResult Result;
	Result.Status = EDnd5eJsonReadStatus::Failure;
	FString Root, AvailabilityPath;
	TArray<FString> SpellPaths, FluffPaths;
	if (!ValidateLoadSelection(Request, Root, SpellPaths, FluffPaths, AvailabilityPath, Result.Diagnostics)) { return Result; }

	FDnd5eJsonSpellCatalogBuildRequest BuildRequest;
	BuildRequest.bValidateContent = Request.bValidateContent;
	BuildRequest.EditionPolicyId = Request.EditionPolicyId;
	BuildRequest.EditionAssignments = Request.EditionAssignments;
	int64 TotalBytes = 0;
	for (int32 Index = 0; Index < SpellPaths.Num(); ++Index)
	{
		FDnd5eJsonDocument Document;
		if (!ReadDocument(SpellPaths[Index], Request.SpellFiles[Index], Request.DatasetNamespace, TotalBytes, Document, Result.Diagnostics)) { return Result; }
		BuildRequest.SpellDocuments.Add(MoveTemp(Document));
	}
	for (int32 Index = 0; Index < FluffPaths.Num(); ++Index)
	{
		FDnd5eJsonDocument Document;
		if (!ReadDocument(FluffPaths[Index], Request.SpellFluffFiles[Index], Request.DatasetNamespace, TotalBytes, Document, Result.Diagnostics)) { return Result; }
		BuildRequest.SpellFluffDocuments.Add(MoveTemp(Document));
	}
	BuildRequest.bHasAvailabilityLookup = Request.bHasAvailabilityLookup;
	if (Request.bHasAvailabilityLookup)
	{
		if (!ReadDocument(AvailabilityPath, Request.AvailabilityFile, Request.DatasetNamespace, TotalBytes, BuildRequest.AvailabilityLookup, Result.Diagnostics)) { return Result; }
	}
	return FDnd5eJsonSpellCatalogBuilder::BuildCatalog(BuildRequest, Catalog);
}
