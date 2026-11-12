// Developed by Neko Creative Technologies

#include "Readers/Bestiary/Dnd5eJsonCreatureCatalogLoader.h"
#include "Catalog/Dnd5eCreatureCatalog.h"
#include "Misc/Paths.h"
#include "Readers/Dnd5eJsonLocalDocuments.h"
#include "Readers/Dnd5eJsonReaderInternal.h"
#include "Readers/Bestiary/Dnd5eJsonCreatureLoreReader.h"

namespace
{
	bool ReadBestiaryIndex(const FDnd5eJsonDocument& Document, TArray<FString>& Files,
		TArray<FDnd5eDiagnostic>& Diagnostics)
	{
		using namespace Dnd5eJsonReader;
		FReadState State{Document.Context, Document.JsonText, Diagnostics};
		TSharedPtr<FJsonValue> RootValue;
		TSharedPtr<FJsonObject> Index;
		if (!ValidateReadContext(State) || !ValidateDocumentText(State) || !PreflightJson(State) ||
			!ParseJsonDocument(RootValue, State) || !RequireObject(RootValue, TEXT(""), Index, State))
		{
			return false;
		}
		if (Index->Values.IsEmpty())
		{
			return Fail(State, Codes::InvalidFieldValue, TEXT(""), TEXT("Bestiary index has no files."));
		}
		if (Index->Values.Num() > 1024)
		{
			return Fail(State, Codes::ResourceLimitExceeded, TEXT(""), TEXT("Bestiary index has more than 1024 entries."));
		}

		Files.Reserve(Index->Values.Num());
		for (const auto& Pair : Index->Values)
		{
			const FString Source(Pair.Key.Len(), *Pair.Key);
			FString File;
			if (!ValidateIdentityText(Source, AppendPointer(TEXT(""), Source), State) ||
				!ReadRequiredString(*Index, *Source, TEXT(""), File, State))
			{
				return false;
			}
			Files.Add(MoveTemp(File));
		}
		Files.Sort([](const FString& Left, const FString& Right)
		{
			return Left.Compare(Right, ESearchCase::CaseSensitive) < 0;
		});
		return true;
	}

	bool HasBestiaryReadErrors(const FDnd5eCreatureDocumentAudit& Document)
	{
		const auto IsError = [](const FDnd5eDiagnostic& Diagnostic)
		{
			return Diagnostic.Severity == EDnd5eDiagnosticSeverity::Error;
		};
		return Document.Status != EDnd5eJsonReadStatus::Success ||
			Document.Diagnostics.ContainsByPredicate(IsError) ||
			Document.Rows.ContainsByPredicate([&IsError](const FDnd5eCreatureAuditRow& Row)
			{
				return Row.Status != EDnd5eJsonReadStatus::Success || Row.Diagnostics.ContainsByPredicate(IsError);
			});
	}
}

FDnd5eJsonCreatureCatalogLoadResult FDnd5eJsonCreatureCatalogLoader::LoadCatalog(
	const FDnd5eJsonCreatureCatalogLoadRequest& Request, FDnd5eCreatureCatalog& Catalog)
{
	using namespace Dnd5eJsonLocalDocuments;
	FDnd5eJsonCreatureCatalogLoadResult Result;
	Result.Status = EDnd5eJsonReadStatus::Failure;

	const FString IndexFile = TEXT("index.json");
	FString Root;
	FString IndexPath;
	int64 TotalBytes = 0;
	FDnd5eJsonDocument Index;
	if (!ResolveDatasetRoot(Request.DatasetRoot, Request.DatasetNamespace, Root, Result.Diagnostics) ||
		!ResolveSelectedFile(Root, IndexFile, Request.DatasetNamespace, IndexPath, Result.Diagnostics) ||
		!ReadDocument(IndexPath, IndexFile, Request.DatasetNamespace, TotalBytes, Index, Result.Diagnostics))
	{
		return Result;
	}

	TArray<FString> Files;
	if (!ReadBestiaryIndex(Index, Files, Result.Diagnostics)) { return Result; }

	FString TemplatePath;
	const bool bHasTemplate = !Request.TemplateFile.IsEmpty();
	if (bHasTemplate)
	{
		if (!ResolveSelectedFile(Root, Request.TemplateFile, Request.DatasetNamespace, TemplatePath, Result.Diagnostics))
		{
			return Result;
		}
		if (FPaths::IsSamePath(IndexPath, TemplatePath))
		{
			LoadError(Result.Diagnostics, TEXT("Dnd5e.Json.Load.DuplicateFile"),
				TEXT("The template must be separate from the index."), Request.TemplateFile);
			return Result;
		}
	}

	TArray<FString> ReadPaths;
	TArray<FDnd5eJsonDocument> Documents;
	ReadPaths.Reserve(Files.Num());
	Documents.Reserve(Files.Num());
	for (const FString& File : Files)
	{
		FString FullPath;
		if (!ResolveSelectedFile(Root, File, Request.DatasetNamespace, FullPath, Result.Diagnostics))
		{
			return Result;
		}
		if (FPaths::IsSamePath(IndexPath, FullPath) || (bHasTemplate && FPaths::IsSamePath(TemplatePath, FullPath)))
		{
			LoadError(Result.Diagnostics, TEXT("Dnd5e.Json.Load.DuplicateFile"),
				TEXT("The index and template cannot also be creature documents."), File);
			return Result;
		}
		if (ReadPaths.ContainsByPredicate([&FullPath](const FString& Existing)
		{
			return FPaths::IsSamePath(Existing, FullPath);
		}))
		{
			continue;
		}

		FDnd5eJsonDocument Document;
		if (!ReadDocument(FullPath, File, Request.DatasetNamespace, TotalBytes, Document, Result.Diagnostics))
		{
			return Result;
		}
		ReadPaths.Add(MoveTemp(FullPath));
		Documents.Add(MoveTemp(Document));
	}

	FDnd5eJsonDocument Template;
	if (bHasTemplate &&
		!ReadDocument(TemplatePath, Request.TemplateFile, Request.DatasetNamespace, TotalBytes, Template, Result.Diagnostics))
	{
		return Result;
	}

	auto Batch = FDnd5eJsonCreatureReader::ReadDocuments(Documents, bHasTemplate ? &Template : nullptr);
	Result.Documents = MoveTemp(Batch.Documents);
	if (Result.Documents.ContainsByPredicate(HasBestiaryReadErrors))
	{
		return Result;
	}
	if (!Request.LoreFile.IsEmpty())
	{
		FString LorePath;
		if (!ResolveSelectedFile(Root, Request.LoreFile, Request.DatasetNamespace, LorePath, Result.Diagnostics))
		{
			return Result;
		}
		if (FPaths::IsSamePath(IndexPath, LorePath) || (bHasTemplate && FPaths::IsSamePath(TemplatePath, LorePath)) ||
			ReadPaths.ContainsByPredicate([&LorePath](const FString& Path) { return FPaths::IsSamePath(Path, LorePath); }))
		{
			LoadError(Result.Diagnostics, TEXT("Dnd5e.Json.Load.DuplicateFile"),
				TEXT("The lore file must be separate from the index, template, and creature documents."), Request.LoreFile);
			return Result;
		}

		FDnd5eJsonDocument Lore;
		if (!ReadDocument(LorePath, Request.LoreFile, Request.DatasetNamespace, TotalBytes, Lore, Result.Diagnostics) ||
			!Dnd5eJsonCreatureLoreReader::ReadAndAttach(Lore, Batch.Creatures, Result.Diagnostics))
		{
			return Result;
		}
	}
	TArray<FDnd5eDiagnostic> CatalogDiagnostics;
	const bool bPublished = Catalog.ReplaceCreatures(MoveTemp(Batch.Creatures), CatalogDiagnostics);
	Result.Diagnostics.Append(MoveTemp(CatalogDiagnostics));
	if (!bPublished) { return Result; }

	Result.CreatureCount = Catalog.GetCreatureCount();
	Result.Status = EDnd5eJsonReadStatus::Success;
	return Result;
}