#include "Readers/Spells/Dnd5eJsonSpellCatalogLoader.h"
#include "Readers/Dnd5eJsonLocalDocuments.h"
#include "Misc/Paths.h"

namespace
{
	bool ValidateLoadSelection(const FDnd5eJsonSpellCatalogLoadRequest& Request, FString& Root, TArray<FString>& SpellPaths, TArray<FString>& FluffPaths, FString& AvailabilityPath, TArray<FDnd5eDiagnostic>& Diagnostics)
	{
		using namespace Dnd5eJsonLocalDocuments;
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
		if (!ResolveDatasetRoot(Request.DatasetRoot, Request.DatasetNamespace, Root, Diagnostics)) { return false; }
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
}

FDnd5eJsonSpellCatalogBuildResult FDnd5eJsonSpellCatalogLoader::LoadCatalog(const FDnd5eJsonSpellCatalogLoadRequest& Request,
	FDnd5eSpellCatalog& Catalog)
{
	using namespace Dnd5eJsonLocalDocuments;
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