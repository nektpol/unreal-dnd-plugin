// Developed by Neko Creative Technologies

#include "Readers/Items/Dnd5eJsonItemCatalogLoader.h"
#include "Readers/Dnd5eJsonLocalDocuments.h"
#include "Readers/Items/Dnd5eJsonItemDefinitionCatalogLibrary.h"
#include "Misc/Paths.h"
#include "UObject/StrongObjectPtr.h"

namespace
{
    bool ResolveItemFileList(const FString& Root, const FString& DatasetNamespace, const TArray<FString>& RelativeFiles,
        TArray<FString>& FullPaths, TArray<FDnd5eDiagnostic>& Diagnostics)
    {
        using namespace Dnd5eJsonLocalDocuments;
        FullPaths.Reset();
        for (const FString& Relative : RelativeFiles)
        {
            FString FullPath;
            if (!ResolveSelectedFile(Root, Relative, DatasetNamespace, FullPath, Diagnostics)) { return false; }
            if (FullPaths.ContainsByPredicate([&FullPath](const FString& Existing) { return FPaths::IsSamePath(Existing, FullPath); }))
            {
                return LoadError(Diagnostics, TEXT("Dnd5e.Json.Load.DuplicateFile"),
                    TEXT("Each selected file must occur only once in a file list."), Relative);
            }
            FullPaths.Add(MoveTemp(FullPath));
        }
        return true;
    }

    bool ReadItemFileList(const TArray<FString>& FullPaths, const TArray<FString>& RelativeFiles,
        const FString& DatasetNamespace, int64& TotalBytes, TMap<FString, FDnd5eJsonDocument>& Cache,
        TArray<FDnd5eJsonDocument>& Documents, TArray<FDnd5eDiagnostic>& Diagnostics)
    {
        Documents.Reserve(FullPaths.Num());
        for (int32 Index = 0; Index < FullPaths.Num(); ++Index)
        {
            const FString& Path = FullPaths[Index];
            const FString& Relative = RelativeFiles[Index];
            const FDnd5eJsonDocument* Cached = nullptr;
            for (const auto& Entry : Cache)
            {
                if (!FPaths::IsSamePath(Entry.Key, Path)) { continue; }
                Cached = &Entry.Value;
                break;
            }
            if (!Cached)
            {
                FDnd5eJsonDocument Read;
                if (!Dnd5eJsonLocalDocuments::ReadDocument(Path, Relative, DatasetNamespace, TotalBytes, Read, Diagnostics))
                {
                    return false;
                }
                Cached = &Cache.Add(Path, MoveTemp(Read));
            }

            FDnd5eJsonDocument Document = *Cached;
            Document.Context.RelativeFilePath = Relative;
            Documents.Add(MoveTemp(Document));
        }
        return true;
    }
}

FDnd5eJsonItemCatalogLoadResult UDnd5eJsonItemCatalogLoader::LoadItemCatalog(const FDnd5eJsonItemCatalogLoadRequest& Request)
{
    using namespace Dnd5eJsonLocalDocuments;
    FDnd5eJsonItemCatalogLoadResult Result;
    Result.Status = EDnd5eJsonReadStatus::Failure;
    auto& Diagnostics = Result.Diagnostics;
    if (Request.BaseFiles.IsEmpty() && Request.ItemFiles.IsEmpty())
    {
        LoadError(Diagnostics, TEXT("Dnd5e.Json.Load.InvalidRequest"), TEXT("Select a base or primary item file."));
        return Result;
    }
    const int64 FileCount = int64(Request.BaseFiles.Num()) + Request.ItemFiles.Num() +
        Request.VariantFiles.Num() + Request.SupplementFiles.Num() + Request.DefinitionFiles.Num();
    if (FileCount > 1024)
    {
        LoadError(Diagnostics, TEXT("Dnd5e.Json.ResourceLimitExceeded"), TEXT("Select at most 1024 total input files."));
        return Result;
    }

    FString Root;
    const FString& Namespace = Request.DatasetNamespace;
    if (!ResolveDatasetRoot(Request.DatasetRoot, Namespace, Root, Diagnostics)) { return Result; }

    TArray<FString> BasePaths, ItemPaths, VariantPaths, SupplementPaths, DefinitionPaths;
    if (!ResolveItemFileList(Root, Namespace, Request.BaseFiles, BasePaths, Diagnostics) ||
        !ResolveItemFileList(Root, Namespace, Request.ItemFiles, ItemPaths, Diagnostics) ||
        !ResolveItemFileList(Root, Namespace, Request.VariantFiles, VariantPaths, Diagnostics) ||
        !ResolveItemFileList(Root, Namespace, Request.SupplementFiles, SupplementPaths, Diagnostics) ||
        !ResolveItemFileList(Root, Namespace, Request.DefinitionFiles, DefinitionPaths, Diagnostics))
    {
        return Result;
    }

    FDnd5eJsonItemCatalogBuildRequest BuildRequest;
    BuildRequest.VariantSelections = Request.VariantSelections;
    TArray<FDnd5eJsonDocument> DefinitionDocuments;
    TMap<FString, FDnd5eJsonDocument> Cache;
    int64 TotalBytes = 0;
    if (!ReadItemFileList(BasePaths, Request.BaseFiles, Namespace, TotalBytes, Cache, BuildRequest.BaseDocuments, Diagnostics) ||
        !ReadItemFileList(ItemPaths, Request.ItemFiles, Namespace, TotalBytes, Cache, BuildRequest.ItemDocuments, Diagnostics) ||
        !ReadItemFileList(VariantPaths, Request.VariantFiles, Namespace, TotalBytes, Cache, BuildRequest.VariantDocuments, Diagnostics) ||
        !ReadItemFileList(SupplementPaths, Request.SupplementFiles, Namespace, TotalBytes, Cache, BuildRequest.SupplementDocuments, Diagnostics) ||
        !ReadItemFileList(DefinitionPaths, Request.DefinitionFiles, Namespace, TotalBytes, Cache, DefinitionDocuments, Diagnostics))
    {
        return Result;
    }

    TStrongObjectPtr<UDnd5eItemDefinitionCatalogObject> Definitions(NewObject<UDnd5eItemDefinitionCatalogObject>());
    TStrongObjectPtr<UDnd5eItemCatalogObject> Items(NewObject<UDnd5eItemCatalogObject>());

    TArray<FDnd5eDiagnostic> DefinitionDiagnostics;
    const bool bDefinitionsBuilt = UDnd5eJsonItemDefinitionCatalogLibrary::BuildItemDefinitionCatalogFromDocuments(
        Definitions.Get(), DefinitionDocuments, DefinitionDiagnostics);
    Result.Diagnostics.Append(MoveTemp(DefinitionDiagnostics));
    if (!bDefinitionsBuilt) { return Result; }

    auto ItemBuild = UDnd5eJsonItemCatalogBuilder::BuildItemCatalog(BuildRequest, Items.Get());
    Result.Diagnostics.Append(MoveTemp(ItemBuild.Diagnostics));
    if (ItemBuild.Status != EDnd5eJsonReadStatus::Success) { return Result; }

    Result.ItemCatalog = Items.Get();
    Result.DefinitionCatalog = Definitions.Get();
    Result.RecordCount = ItemBuild.RecordCount;
    Result.DefinitionCount = Definitions->GetDefinitionCount();
    Result.Status = EDnd5eJsonReadStatus::Success;
    return Result;
}