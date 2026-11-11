// Developed by Neko Creative Technologies

#if WITH_DEV_AUTOMATION_TESTS
#include "Dom/JsonObject.h"
#include "HAL/FileManager.h"
#include "Misc/AutomationTest.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Readers/Bestiary/Dnd5eJsonCreatureReader.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace
{
	struct FBestiaryAuditSample
	{
		int32 Count = 0;
		FString File;
		FString Pointer;
	};

	bool ReadIndex(const FString& Root, TArray<FString>& OutFiles)
	{
		FString Json;
		if (!FFileHelper::LoadFileToString(Json, *FPaths::Combine(Root, TEXT("index.json")))) { return false; }

		TSharedPtr<FJsonObject> Index;
		const auto Reader = TJsonReaderFactory<>::Create(Json);
		if (!FJsonSerializer::Deserialize(Reader, Index) || !Index.IsValid()) { return false; }

		TSet<FString> Files;
		for (const auto& Pair : Index->Values)
		{
			FString File;
			if (!Pair.Value.IsValid() || Pair.Value->Type != EJson::String || !Pair.Value->TryGetString(File) ||
				File != FPaths::GetCleanFilename(File) || File.Contains(TEXT("/")) ||
				File.Contains(TEXT("\\")) || File.Contains(TEXT(":")) ||
				!File.EndsWith(TEXT(".json"), ESearchCase::IgnoreCase))
			{
				return false;
			}
			Files.Add(MoveTemp(File));
		}
		OutFiles = Files.Array();
		OutFiles.Sort();
		return !OutFiles.IsEmpty();
	}

	FString NormalizeAuditPointer(const FString& Pointer)
	{
		TArray<FString> Parts;
		Pointer.ParseIntoArray(Parts, TEXT("/"), false);
		for (FString& Part : Parts)
		{
			if (!Part.IsEmpty() && Part.IsNumeric()) { Part = TEXT("*"); }
		}
		return FString::Join(Parts, TEXT("/"));
	}

	void AddSample(TMap<FString, FBestiaryAuditSample>& Groups, const FString& Key,
		const FString& File, const FString& Pointer)
	{
		auto& Sample = Groups.FindOrAdd(Key);
		if (Sample.Count == 0)
		{
			Sample.File = File;
			Sample.Pointer = Pointer;
		}
		++Sample.Count;
	}

	void AppendGroups(const TCHAR* Heading, const TMap<FString, FBestiaryAuditSample>& Groups, FString& Report)
	{
		Report += FString::Printf(TEXT("\n%s\nCount\tGroup\tFile\tPointer\n"), Heading);
		TArray<FString> Keys;
		Groups.GetKeys(Keys);
		Keys.Sort([&Groups](const FString& A, const FString& B)
		{
			const int32 CountA = Groups.FindChecked(A).Count;
			const int32 CountB = Groups.FindChecked(B).Count;
			return CountA == CountB ? A < B : CountA > CountB;
		});
		for (const FString& Key : Keys)
		{
			const auto& Sample = Groups.FindChecked(Key);
			Report += FString::Printf(TEXT("%d\t%s\t%s\t%s\n"),
				Sample.Count, *Key, *Sample.File, *Sample.Pointer);
		}
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eBestiaryImportAudit, "Dnd5e.Bestiary.Local.SnapshotAudit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FDnd5eBestiaryImportAudit::RunTest(const FString& Parameters)
{
	FString Root;
	if (!FParse::Value(FCommandLine::Get(), TEXT("Dnd5eBestiaryRoot="), Root))
	{
		AddInfo(TEXT("Supply -Dnd5eBestiaryRoot to run the local Bestiary snapshot audit."));
		return true;
	}

	TArray<FString> Files;
	if (!TestTrue(TEXT("Readable Bestiary index"), ReadIndex(Root, Files))) { return false; }

	int32 FilesOpened = 0;
	int32 BadDocuments = 0;
	int32 RowsVisited = 0;
	int32 Converted = 0;
	int32 ConvertedWithWarnings = 0;
	int32 Failed = 0;
	int32 Copies = 0;
	TArray<FString> Unreadable;
	TMap<FString, FBestiaryAuditSample> Shapes;
	TMap<FString, FBestiaryAuditSample> CopyShapes;
	TMap<FString, FBestiaryAuditSample> Diagnostics;

	for (const FString& File : Files)
	{
		FDnd5eJsonDocument Document;
		Document.Context.DatasetNamespace = TEXT("local-bestiary-audit");
		Document.Context.RelativeFilePath = File;
		if (!FFileHelper::LoadFileToString(Document.JsonText, *FPaths::Combine(Root, File)))
		{
			Unreadable.Add(File);
			continue;
		}
		++FilesOpened;

		const auto Audit = FDnd5eJsonCreatureReader::AuditDocument(Document);
		if (Audit.Status != EDnd5eJsonReadStatus::Success) { ++BadDocuments; }
		for (const auto& Diagnostic : Audit.Diagnostics)
		{
			const FString Key = Diagnostic.Code.ToString() + TEXT(" ") + NormalizeAuditPointer(Diagnostic.JsonPointer);
			AddSample(Diagnostics, Key, File, Diagnostic.JsonPointer);
		}

		for (const auto& Row : Audit.Rows)
		{
			++RowsVisited;
			if (Row.bIsCopy) { ++Copies; }
			if (Row.Status == EDnd5eJsonReadStatus::Success)
			{
				++Converted;
				if (Row.Diagnostics.ContainsByPredicate([](const FDnd5eDiagnostic& Diagnostic)
				{
					return Diagnostic.Severity == EDnd5eDiagnosticSeverity::Warning;
				}))
				{
					++ConvertedWithWarnings;
				}
			}
			else
			{
				++Failed;
			}

			auto& ShapeGroups = Row.bIsCopy ? CopyShapes : Shapes;
			for (const auto& Shape : Row.FieldShapes)
			{
				AddSample(ShapeGroups, Shape.Path + TEXT(" / ") + Shape.JsonType, File, Shape.JsonPointer);
			}
			for (const auto& Diagnostic : Row.Diagnostics)
			{
				const FString Key = Diagnostic.Code.ToString() + TEXT(" ") + NormalizeAuditPointer(Diagnostic.JsonPointer);
				AddSample(Diagnostics, Key, File, Diagnostic.JsonPointer);
			}
		}
	}

	FString Report = FString::Printf(
		TEXT("Indexed files: %d\nFiles opened: %d\nInvalid envelopes: %d\nRows visited: %d\n")
		TEXT("Converted: %d\nConverted with warnings: %d\nFailed conversion: %d\nCopy rows: %d\n"),
		Files.Num(), FilesOpened, BadDocuments, RowsVisited, Converted, ConvertedWithWarnings, Failed, Copies);
	for (const FString& File : Unreadable) { Report += TEXT("Unreadable file: ") + File + TEXT("\n"); }
	AppendGroups(TEXT("Direct row field shapes"), Shapes, Report);
	AppendGroups(TEXT("Copy row field shapes"), CopyShapes, Report);
	AppendGroups(TEXT("Reader diagnostics"), Diagnostics, Report);

	const FString ReportDir = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Automation/BestiaryAudit"));
	IFileManager::Get().MakeDirectory(*ReportDir, true);
	const FString ReportPath = FPaths::Combine(ReportDir, TEXT("summary.tsv"));
	TestTrue(TEXT("Audit report saved"), FFileHelper::SaveStringToFile(Report, *ReportPath));
	AddInfo(TEXT("Bestiary audit report: ") + ReportPath);
	TestEqual(TEXT("All indexed files opened"), FilesOpened, Files.Num());
	TestEqual(TEXT("All file envelopes traversed"), BadDocuments, 0);
	TestEqual(TEXT("Every visited row has a conversion outcome"), Converted + Failed, RowsVisited);
	return !HasAnyErrors();
}

#endif