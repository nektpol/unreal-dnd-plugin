#if WITH_DEV_AUTOMATION_TESTS

#include "Dnd5eTestSupport.h"
#include "Catalog/Dnd5eSpellCatalogObject.h"
#include "Readers/Spells/Dnd5eJsonSpellCatalogLibrary.h"
#include "HAL/FileManager.h"
#include "Dom/JsonObject.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Guid.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Serialization/Archive.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/StructOnScope.h"

using namespace Dnd5eTests;

namespace
{
	FString LoaderSpell(const TCHAR* Name = TEXT("Invented Lantern"))
	{
		return FString::Printf(TEXT(R"json({"spell":[{
			"name":"%s","source":"SYN","level":0,"school":"V",
			"time":[{"number":1,"unit":"action"}],"range":{"type":"point","distance":{"type":"self"}},
			"components":{"v":true},"duration":[{"type":"instant"}],"entries":["An invented light uses {@dice 1d4}."]
		}]})json"), Name);
	}

	struct FLoaderFiles
	{
		FString Base = FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Automation/Dnd5eLoaderFixtures")));
		FString Root = FPaths::Combine(Base, FGuid::NewGuid().ToString(EGuidFormats::Digits));

		~FLoaderFiles()
		{
			// Remove only this generated fixture directory, never the shared parent or a caller's dataset.
			FString Resolved = FPaths::ConvertRelativePathToFull(Root);
			if (FPaths::CollapseRelativeDirectories(Resolved) && FPaths::IsUnderDirectory(Resolved, Base) && !FPaths::IsSamePath(Resolved, Base))
			{
				IFileManager::Get().DeleteDirectory(*Resolved, false, true);
			}
		}

		bool Write(const FString& Relative, const FString& Text, FFileHelper::EEncodingOptions Encoding = FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM) const
		{
			const FString Path = FPaths::Combine(Root, Relative);
			if (!FPaths::IsUnderDirectory(Path, Root) || !IFileManager::Get().MakeDirectory(*FPaths::GetPath(Path), true)) { return false; }
			return FFileHelper::SaveStringToFile(Text, *Path, Encoding);
		}

		bool SizedFile(const FString& Relative, int64 Size) const
		{
			const FString Path = FPaths::Combine(Root, Relative);
			if (!FPaths::IsUnderDirectory(Path, Root) || !IFileManager::Get().MakeDirectory(*FPaths::GetPath(Path), true)) { return false; }
			TUniquePtr<FArchive> Writer(IFileManager::Get().CreateFileWriter(*Path));
			if (!Writer || Size <= 0) { return false; }
			Writer->Seek(Size - 1);
			uint8 Byte = ' ';
			Writer->Serialize(&Byte, 1);
			return !Writer->IsError();
		}

		FDnd5eJsonSpellCatalogLoadRequest Request() const
		{
			FDnd5eJsonSpellCatalogLoadRequest Result;
			Result.DatasetRoot = Root;
			Result.DatasetNamespace = TEXT("loader-synthetic");
			Result.SpellFiles = {TEXT("spells/first.json")};
			return Result;
		}
	};

	bool HasLoadError(FAutomationTestBase& Test, const FDnd5eJsonSpellCatalogBuildResult& Result, const TCHAR* Code, const TCHAR* Label = nullptr)
	{
		Test.TestTrue(TEXT("Failed load returns failure and no installed count"), Result.Status == EDnd5eJsonReadStatus::Failure && Result.SpellCount == 0);
		const auto* Error = Result.Diagnostics.FindByPredicate([&](const auto& Diagnostic) { return Diagnostic.Severity == EDnd5eDiagnosticSeverity::Error && Diagnostic.Code == FName(Code); });
		if (!Test.TestNotNull(Code, Error)) { return false; }
		if (Label) { Test.TestEqual(TEXT("Failure retains its relative source label"), Error->RelativeFilePath, FString(Label)); }
		return true;
	}

	struct FSelectionCase
	{
		const TCHAR* Name;
		const TCHAR* Code;
		TFunction<void(FDnd5eJsonSpellCatalogLoadRequest&)> Change;
	};

	const TArray<FSelectionCase>& SelectionCases()
	{
		static const TArray<FSelectionCase> Cases = {
			{TEXT("NoFiles"), TEXT("Dnd5e.Json.Load.InvalidRequest"), [](auto& R) { R.SpellFiles.Reset(); }},
			{TEXT("MissingNamespace"), TEXT("Dnd5e.Json.InvalidContext"), [](auto& R) { R.DatasetNamespace.Reset(); }},
			{TEXT("WhitespaceNamespace"), TEXT("Dnd5e.Json.InvalidContext"), [](auto& R) { R.DatasetNamespace = TEXT(" invalid "); }},
			{TEXT("MissingRoot"), TEXT("Dnd5e.Json.Load.InvalidDatasetRoot"), [](auto& R) { R.DatasetRoot.Reset(); }},
			{TEXT("RelativeRoot"), TEXT("Dnd5e.Json.Load.InvalidDatasetRoot"), [](auto& R) { R.DatasetRoot = TEXT("relative-dataset"); }},
			{TEXT("DriveRelativeRoot"), TEXT("Dnd5e.Json.Load.InvalidDatasetRoot"), [](auto& R) { R.DatasetRoot = TEXT("C:relative-dataset"); }},
			{TEXT("NonexistentRoot"), TEXT("Dnd5e.Json.Load.InvalidDatasetRoot"), [](auto& R) { R.DatasetRoot += TEXT("/not-created"); }},
			{TEXT("RootWhitespace"), TEXT("Dnd5e.Json.Load.InvalidDatasetRoot"), [](auto& R) { R.DatasetRoot = TEXT(" ") + R.DatasetRoot; }},
			{TEXT("RootControl"), TEXT("Dnd5e.Json.Load.InvalidDatasetRoot"), [](auto& R) { R.DatasetRoot += TEXT("\n"); }},
			{TEXT("RootWildcard"), TEXT("Dnd5e.Json.Load.InvalidDatasetRoot"), [](auto& R) { R.DatasetRoot += TEXT("*"); }},
			{TEXT("RootUrl"), TEXT("Dnd5e.Json.Load.InvalidDatasetRoot"), [](auto& R) { R.DatasetRoot = TEXT("https://invalid.example/dataset"); }},
			{TEXT("EmptyFilename"), TEXT("Dnd5e.Json.Load.InvalidFilePath"), [](auto& R) { R.SpellFiles = {TEXT("")}; }},
			{TEXT("AbsoluteFilename"), TEXT("Dnd5e.Json.InvalidContext"), [](auto& R) { R.SpellFiles = {FPaths::Combine(R.DatasetRoot, TEXT("spells/first.json"))}; }},
			{TEXT("BackslashFilename"), TEXT("Dnd5e.Json.InvalidContext"), [](auto& R) { R.SpellFiles = {TEXT("spells\\first.json")}; }},
			{TEXT("ParentTraversal"), TEXT("Dnd5e.Json.InvalidContext"), [](auto& R) { R.SpellFiles = {TEXT("../first.json")}; }},
			{TEXT("EmptySegment"), TEXT("Dnd5e.Json.InvalidContext"), [](auto& R) { R.SpellFiles = {TEXT("spells//first.json")}; }},
			{TEXT("DotSegment"), TEXT("Dnd5e.Json.InvalidContext"), [](auto& R) { R.SpellFiles = {TEXT("spells/./first.json")}; }},
			{TEXT("WildcardFilename"), TEXT("Dnd5e.Json.Load.InvalidFilePath"), [](auto& R) { R.SpellFiles = {TEXT("spells/*.json")}; }},
			{TEXT("WrongExtension"), TEXT("Dnd5e.Json.Load.InvalidFilePath"), [](auto& R) { R.SpellFiles = {TEXT("spells/first.txt")}; }},
			{TEXT("DuplicateSpellFile"), TEXT("Dnd5e.Json.Load.DuplicateFile"), [](auto& R) { R.SpellFiles.Add(FString(R.SpellFiles[0])); }},
			{TEXT("MissingLookupFilename"), TEXT("Dnd5e.Json.Load.InvalidFilePath"), [](auto& R) { R.bHasAvailabilityLookup = true; }},
			{TEXT("DuplicateLookupFile"), TEXT("Dnd5e.Json.Load.DuplicateFile"), [](auto& R) { R.bHasAvailabilityLookup = true; R.AvailabilityFile = R.SpellFiles[0]; }},
			{TEXT("LookupTraversal"), TEXT("Dnd5e.Json.InvalidContext"), [](auto& R) { R.bHasAvailabilityLookup = true; R.AvailabilityFile = TEXT("../lookup.json"); }},
			{TEXT("DocumentLimit"), TEXT("Dnd5e.Json.ResourceLimitExceeded"), [](auto& R) { R.SpellFiles.SetNum(1025); }},
#if PLATFORM_WINDOWS
			{TEXT("CurrentDriveRoot"), TEXT("Dnd5e.Json.Load.InvalidDatasetRoot"), [](auto& R) { R.DatasetRoot = TEXT("/current-drive-relative"); }},
			{TEXT("CaseAliasDuplicate"), TEXT("Dnd5e.Json.Load.DuplicateFile"), [](auto& R) { R.SpellFiles.Add(TEXT("SPELLS/FIRST.JSON")); }},
#endif
		};
		return Cases;
	}
}

IMPLEMENT_COMPLEX_AUTOMATION_TEST(FDnd5eLoaderSelectionTest, "Dnd5e.Json.Loader.Selection", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
void FDnd5eLoaderSelectionTest::GetTests(TArray<FString>& Names, TArray<FString>& Commands) const
{
	for (const auto& Case : SelectionCases()) { Names.Add(Case.Name); Commands.Add(Case.Name); }
}
bool FDnd5eLoaderSelectionTest::RunTest(const FString& Parameters)
{
	const auto* Case = SelectionCases().FindByPredicate([&](const auto& Value) { return Parameters == Value.Name; });
	if (!TestNotNull(TEXT("Selection scenario exists"), Case)) { return false; }
	FLoaderFiles Files;
	if (!TestTrue(TEXT("Create synthetic file"), Files.Write(TEXT("spells/first.json"), LoaderSpell()))) { return false; }
	FDnd5eSpellCatalog Catalog;
	const auto Valid = Files.Request();
	if (!TestTrue(TEXT("Seed active catalog"), FDnd5eJsonSpellCatalogLoader::LoadCatalog(Valid, Catalog).Status == EDnd5eJsonReadStatus::Success)) { return false; }
	const auto Before = Catalog.GetSpellIds();
	auto Request = Valid;
	Case->Change(Request);
	const auto InputBefore = Request;
	const auto Result = FDnd5eJsonSpellCatalogLoader::LoadCatalog(Request, Catalog);
	HasLoadError(*this, Result, Case->Code);
	TestTrue(TEXT("Invalid selection preserves active IDs"), Catalog.GetSpellIds() == Before);
	TestTrue(TEXT("Validation does not rewrite the request"), Equal(FDnd5eJsonSpellCatalogLoadRequest::StaticStruct(), &Request, &InputBefore));
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eLoaderReadTest, "Dnd5e.Json.Loader.ReadBuildAndProvenance", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FDnd5eLoaderReadTest::RunTest(const FString& Parameters)
{
	FLoaderFiles Files;
	if (!TestTrue(TEXT("Write ordered synthetic records"), Files.Write(TEXT("spells/first.json"), LoaderSpell(TEXT("Invented Lantern Ω"))) && Files.Write(TEXT("spells/second.JSON"), LoaderSpell(TEXT("Invented Chime"))))) { return false; }
	auto Request = Files.Request();
	Request.SpellFiles.Add(TEXT("spells/second.JSON"));
	Request.AvailabilityFile = TEXT("../inactive-invalid-value.json");
	const auto Before = Request;
	FDnd5eSpellCatalog Catalog;
	const auto Result = FDnd5eJsonSpellCatalogLoader::LoadCatalog(Request, Catalog);
	if (!TestTrue(TEXT("Load all selected documents and ignore inactive lookup"), Result.Status == EDnd5eJsonReadStatus::Success && Result.SpellCount == 2 && Result.Diagnostics.IsEmpty())) { return false; }
	TestTrue(TEXT("Load preserves original path selection"), Equal(FDnd5eJsonSpellCatalogLoadRequest::StaticStruct(), &Request, &Before));
	const auto Ids = Catalog.GetSpellIds();
	if (!TestEqual(TEXT("Two installed records"), Ids.Num(), 2)) { return false; }
	for (int32 Index = 0; Index < Ids.Num(); ++Index)
	{
		FDnd5eSpell Spell;
		if (!TestTrue(TEXT("Returned ID resolves"), Catalog.TryGetSpell(Ids[Index], Spell))) { return false; }
		TestEqual(TEXT("Dataset namespace is caller-supplied"), Ids[Index].DatasetNamespace, Request.DatasetNamespace);
		TestFalse(TEXT("Absolute root is not identity"), Ids[Index].SourceKey.Contains(Request.DatasetRoot));
		if (!TestEqual(TEXT("One primary provenance entry"), Spell.Header.Provenance.Num(), 1)) { return false; }
		TestEqual(TEXT("Original relative filename and order"), Spell.Header.Provenance[0].RelativeFilePath, Request.SpellFiles[Index]);
		TestEqual(TEXT("Each file has its own record index"), Spell.Header.Provenance[0].JsonPointer, FString(TEXT("/spell/0")));
		TestEqual(TEXT("Publication is retained separately from the root"), Spell.Header.Provenance[0].SourceId, FString(TEXT("SYN")));
		TestFalse(TEXT("No lookup leaves availability unknown"), Spell.bHasAvailabilityData);
		TestEqual(TEXT("File order and Unicode name survive"), Spell.Header.DisplayName, FString(Index == 0 ? TEXT("Invented Lantern Ω") : TEXT("Invented Chime")));
	}
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eLoaderAtomicTest, "Dnd5e.Json.Loader.AtomicFailureAndClear", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FDnd5eLoaderAtomicTest::RunTest(const FString& Parameters)
{
	FLoaderFiles Files;
	if (!TestTrue(TEXT("Write seed record"), Files.Write(TEXT("spells/first.json"), LoaderSpell()))) { return false; }
	const auto Valid = Files.Request();
	FDnd5eSpellCatalog Catalog;
	if (!TestTrue(TEXT("Seed catalog"), FDnd5eJsonSpellCatalogLoader::LoadCatalog(Valid, Catalog).Status == EDnd5eJsonReadStatus::Success)) { return false; }
	const auto Before = Catalog.GetSpellIds();
	auto Request = Valid; Request.SpellFiles.Add(TEXT("spells/missing.json"));
	auto Result = FDnd5eJsonSpellCatalogLoader::LoadCatalog(Request, Catalog);
	if (HasLoadError(*this, Result, TEXT("Dnd5e.Json.Load.FileReadFailed"), TEXT("spells/missing.json"))) { TestTrue(TEXT("Read failure has no JSON pointer"), Result.Diagnostics.Last().JsonPointer.IsEmpty()); }
	TestTrue(TEXT("A missing later file cannot partially replace"), Catalog.GetSpellIds() == Before);
	Request.SpellFiles = {TEXT("spells/missing.json"), TEXT("../escape.json")};
	Result = FDnd5eJsonSpellCatalogLoader::LoadCatalog(Request, Catalog);
	HasLoadError(*this, Result, TEXT("Dnd5e.Json.InvalidContext"), TEXT("../escape.json"));
	TestTrue(TEXT("All selection validation precedes reads"), !Result.Diagnostics.ContainsByPredicate([](const auto& Value) { return Value.Code == FName(TEXT("Dnd5e.Json.Load.FileReadFailed")); }));
	if (!TestTrue(TEXT("Write malformed and duplicate documents"), Files.Write(TEXT("spells/bad.json"), TEXT("{")) && Files.Write(TEXT("spells/duplicate.json"), LoaderSpell()) && Files.Write(TEXT("spells/empty.json"), TEXT("")))) { return false; }
	Request = Valid; Request.SpellFiles.Add(TEXT("spells/bad.json"));
	Result = FDnd5eJsonSpellCatalogLoader::LoadCatalog(Request, Catalog);
	HasLoadError(*this, Result, TEXT("Dnd5e.Json.InvalidSyntax"), TEXT("spells/bad.json"));
	TestTrue(TEXT("Later parsing failure preserves old IDs"), Catalog.GetSpellIds() == Before);
	Request = Valid; Request.SpellFiles.Add(TEXT("spells/duplicate.json"));
	Result = FDnd5eJsonSpellCatalogLoader::LoadCatalog(Request, Catalog);
	HasLoadError(*this, Result, TEXT("Dnd5e.Catalog.DuplicateRecordId"));
	TestTrue(TEXT("Unique files with duplicate record IDs fail atomically"), Catalog.GetSpellIds() == Before);
	Request = Valid; Request.SpellFiles = {TEXT("spells/empty.json")};
	HasLoadError(*this, FDnd5eJsonSpellCatalogLoader::LoadCatalog(Request, Catalog), TEXT("Dnd5e.Json.EmptyInput"), TEXT("spells/empty.json"));
	TestTrue(TEXT("Empty text is not an implicit clear"), Catalog.GetSpellIds() == Before);
	if (!TestTrue(TEXT("Create a directory whose name ends in json"), IFileManager::Get().MakeDirectory(*FPaths::Combine(Files.Root, TEXT("spells/folder.json")), true))) { return false; }
	Request.SpellFiles = {TEXT("spells/folder.json")};
	HasLoadError(*this, FDnd5eJsonSpellCatalogLoader::LoadCatalog(Request, Catalog), TEXT("Dnd5e.Json.Load.FileReadFailed"), TEXT("spells/folder.json"));
	if (!TestTrue(TEXT("Write an explicit empty spell array"), Files.Write(TEXT("spells/clear.json"), TEXT("{\"spell\":[]}")))) { return false; }
	Request.SpellFiles = {TEXT("spells/clear.json")};
	Result = FDnd5eJsonSpellCatalogLoader::LoadCatalog(Request, Catalog);
	TestTrue(TEXT("Explicit empty document clears after a successful load"), Result.Status == EDnd5eJsonReadStatus::Success && Result.SpellCount == 0 && Catalog.GetSpellCount() == 0);
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eLoaderAvailabilityTest, "Dnd5e.Json.Loader.AvailabilityAndWarnings", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FDnd5eLoaderAvailabilityTest::RunTest(const FString& Parameters)
{
	FLoaderFiles Files;
	const FString Lookup = TEXT("{\"syn\":{\"invented lantern\":{\"class\":{\"SYN\":{\"Lantern Adept\":true}}}}}");
	if (!TestTrue(TEXT("Write spell and lookup"), Files.Write(TEXT("spells/first.json"), LoaderSpell()) && Files.Write(TEXT("generated/lookup.json"), Lookup))) { return false; }
	auto Request = Files.Request(); Request.bHasAvailabilityLookup = true; Request.AvailabilityFile = TEXT("generated/lookup.json");
	FDnd5eSpellCatalog Catalog;
	const auto Result = FDnd5eJsonSpellCatalogLoader::LoadCatalog(Request, Catalog);
	if (!TestTrue(TEXT("Selected lookup joins through the builder"), Result.Status == EDnd5eJsonReadStatus::Success && Result.SpellCount == 1)) { return false; }
	FDnd5eSpell Spell;
	if (!TestTrue(TEXT("Retrieve joined record"), Catalog.TryGetSpell(Catalog.GetSpellIds()[0], Spell) && Spell.Availability.Num() == 1)) { return false; }
	TestTrue(TEXT("Lookup presence and relative origin survive"), Spell.bHasAvailabilityData && Spell.Availability[0].Provider.Name == TEXT("Lantern Adept") && Spell.Availability[0].Provenance.RelativeFilePath == Request.AvailabilityFile);
	const auto Before = Catalog.GetSpellIds();
	Request.AvailabilityFile = TEXT("generated/missing.json");
	HasLoadError(*this, FDnd5eJsonSpellCatalogLoader::LoadCatalog(Request, Catalog), TEXT("Dnd5e.Json.Load.FileReadFailed"), TEXT("generated/missing.json"));
	TestTrue(TEXT("Missing lookup preserves installed data"), Catalog.GetSpellIds() == Before);
	if (!TestTrue(TEXT("Write malformed lookup"), Files.Write(TEXT("generated/bad.json"), TEXT("[]")))) { return false; }
	Request.AvailabilityFile = TEXT("generated/bad.json");
	HasLoadError(*this, FDnd5eJsonSpellCatalogLoader::LoadCatalog(Request, Catalog), TEXT("Dnd5e.Json.InvalidFieldValue"), TEXT("generated/bad.json"));
	TestTrue(TEXT("Malformed lookup preserves installed data"), Catalog.GetSpellIds() == Before);
	const FString Unsupported = LoaderSpell().Replace(TEXT("An invented light uses {@dice 1d4}."), TEXT("An invented {@futureRule value}."));
	if (!TestTrue(TEXT("Write a future annotation and empty lookup"), Files.Write(TEXT("spells/first.json"), Unsupported) && Files.Write(TEXT("generated/empty.json"), TEXT("{}")))) { return false; }
	Request.AvailabilityFile = TEXT("generated/empty.json");
	const auto Warned = FDnd5eJsonSpellCatalogLoader::LoadCatalog(Request, Catalog);
	TestTrue(TEXT("Preservation warning passes through"), Warned.Status == EDnd5eJsonReadStatus::Success && Warned.Diagnostics.ContainsByPredicate([](const auto& Value) { return Value.Code == FName(TEXT("Dnd5e.Json.PreservedUnsupportedContent")) && Value.Severity == EDnd5eDiagnosticSeverity::Warning; }));
	TestTrue(TEXT("Missing lookup entry stays informational"), Warned.Diagnostics.ContainsByPredicate([](const auto& Value) { return Value.Code == FName(TEXT("Dnd5e.Json.AvailabilityEntryNotFound")) && Value.Severity == EDnd5eDiagnosticSeverity::Info; }));
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eLoaderEncodingLimitsTest, "Dnd5e.Json.Loader.EncodingAndByteLimits", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FDnd5eLoaderEncodingLimitsTest::RunTest(const FString& Parameters)
{
	FLoaderFiles Files;
	if (!TestTrue(TEXT("Write supported BOM encodings"), Files.Write(TEXT("encodings/utf8.json"), LoaderSpell(TEXT("Invented Beacon Ω")), FFileHelper::EEncodingOptions::ForceUTF8) && Files.Write(TEXT("encodings/utf16.json"), LoaderSpell(TEXT("Invented Prism Ω")), FFileHelper::EEncodingOptions::ForceUnicode))) { return false; }
	auto Request = Files.Request(); Request.SpellFiles = {TEXT("encodings/utf8.json"), TEXT("encodings/utf16.json")};
	FDnd5eSpellCatalog Catalog;
	auto Result = FDnd5eJsonSpellCatalogLoader::LoadCatalog(Request, Catalog);
	if (!TestTrue(TEXT("Engine decoding accepts both BOM forms"), Result.Status == EDnd5eJsonReadStatus::Success && Result.SpellCount == 2)) { return false; }
	constexpr int32 FileLimit = 16777216;
	const FString Prefix = TEXT("{\"spell\":[]}");
	const FString Exact = Prefix + FString::ChrN(FileLimit - Prefix.Len(), TEXT(' '));
	if (!TestTrue(TEXT("Write an exact byte-limit file"), Files.Write(TEXT("limits/part-0.json"), Exact))) { return false; }
	TestEqual(TEXT("Fixture has the intended byte length"), IFileManager::Get().FileSize(*FPaths::Combine(Files.Root, TEXT("limits/part-0.json"))), static_cast<int64>(FileLimit));
	Request.SpellFiles = {TEXT("limits/part-0.json")};
	Result = FDnd5eJsonSpellCatalogLoader::LoadCatalog(Request, Catalog);
	TestTrue(TEXT("Per-file byte boundary is inclusive"), Result.Status == EDnd5eJsonReadStatus::Success && Result.SpellCount == 0);
	if (!TestTrue(TEXT("Write oversized file without allocating its contents"), Files.SizedFile(TEXT("limits/too-large.json"), static_cast<int64>(FileLimit) + 1))) { return false; }
	Request.SpellFiles = {TEXT("limits/too-large.json")};
	HasLoadError(*this, FDnd5eJsonSpellCatalogLoader::LoadCatalog(Request, Catalog), TEXT("Dnd5e.Json.ResourceLimitExceeded"), TEXT("limits/too-large.json"));
	Request.SpellFiles = {TEXT("limits/part-0.json")};
	for (int32 Index = 1; Index < 4; ++Index)
	{
		const FString Relative = FString::Printf(TEXT("limits/part-%d.json"), Index);
		if (!TestTrue(TEXT("Write additional bounded document"), Files.Write(Relative, Exact))) { return false; }
		Request.SpellFiles.Add(Relative);
	}
	Result = FDnd5eJsonSpellCatalogLoader::LoadCatalog(Request, Catalog);
	TestTrue(TEXT("Aggregate byte boundary is inclusive"), Result.Status == EDnd5eJsonReadStatus::Success && Catalog.GetSpellCount() == 0);
	if (!TestTrue(TEXT("Write lookup beyond aggregate capacity"), Files.Write(TEXT("generated/over-budget.json"), TEXT("{}")))) { return false; }
	Request.bHasAvailabilityLookup = true; Request.AvailabilityFile = TEXT("generated/over-budget.json");
	HasLoadError(*this, FDnd5eJsonSpellCatalogLoader::LoadCatalog(Request, Catalog), TEXT("Dnd5e.Json.ResourceLimitExceeded"), TEXT("generated/over-budget.json"));
	Request = Files.Request(); Request.SpellFiles = {TEXT("encodings/utf8.json"), TEXT("encodings/utf16.json")};
	Result = FDnd5eJsonSpellCatalogLoader::LoadCatalog(Request, Catalog);
	TestTrue(TEXT("Byte budget belongs to one load, not a persistent global"), Result.Status == EDnd5eJsonReadStatus::Success && Result.SpellCount == 2);
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eLoaderBlueprintTest, "Dnd5e.Blueprint.SpellCatalog.LoadFiles", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FDnd5eLoaderBlueprintTest::RunTest(const FString& Parameters)
{
	FLoaderFiles Files;
	if (!TestTrue(TEXT("Write reflected-load fixture"), Files.Write(TEXT("spells/first.json"), LoaderSpell()))) { return false; }
	TStrongObjectPtr<UDnd5eSpellCatalogObject> Catalog(NewObject<UDnd5eSpellCatalogObject>());
	UFunction* Function = UDnd5eJsonSpellCatalogLibrary::StaticClass()->FindFunctionByName(TEXT("LoadSpellCatalog"));
	if (!TestNotNull(TEXT("Reflected loader exists"), Function)) { return false; }
	auto* ObjectProperty = FindFProperty<FObjectPropertyBase>(Function, TEXT("CatalogObject"));
	auto* RequestProperty = FindFProperty<FStructProperty>(Function, TEXT("Request"));
	auto* ResultProperty = FindFProperty<FStructProperty>(Function, TEXT("ReturnValue"));
	if (!TestNotNull(TEXT("Object parameter"), ObjectProperty) || !TestNotNull(TEXT("Request parameter"), RequestProperty) || !TestNotNull(TEXT("Result parameter"), ResultProperty)) { return false; }
	if (!TestTrue(TEXT("Reflected storage types match"), ObjectProperty->PropertyClass == UDnd5eSpellCatalogObject::StaticClass() && RequestProperty->Struct == FDnd5eJsonSpellCatalogLoadRequest::StaticStruct() && ResultProperty->Struct == FDnd5eJsonSpellCatalogBuildResult::StaticStruct())) { return false; }
	FStructOnScope Frame(Function);
	ObjectProperty->SetObjectPropertyValue_InContainer(Frame.GetStructMemory(), Catalog.Get());
	const auto Request = Files.Request();
	auto* ReflectedRequest = RequestProperty->ContainerPtrToValuePtr<FDnd5eJsonSpellCatalogLoadRequest>(Frame.GetStructMemory());
	*ReflectedRequest = Request;
	GetMutableDefault<UDnd5eJsonSpellCatalogLibrary>()->ProcessEvent(Function, Frame.GetStructMemory());
	const auto* Result = ResultProperty->ContainerPtrToValuePtr<FDnd5eJsonSpellCatalogBuildResult>(Frame.GetStructMemory());
	TestTrue(TEXT("Reflected load installs into the supplied object"), Result->Status == EDnd5eJsonReadStatus::Success && Result->SpellCount == 1 && Catalog->GetSpellCount() == 1);
	TestTrue(TEXT("Reflected call preserves its input"), Equal(FDnd5eJsonSpellCatalogLoadRequest::StaticStruct(), ReflectedRequest, &Request));
	const auto Null = UDnd5eJsonSpellCatalogLibrary::LoadSpellCatalog(nullptr, FDnd5eJsonSpellCatalogLoadRequest{});
	HasLoadError(*this, Null, TEXT("Dnd5e.Json.InvalidCatalogObject"), TEXT(""));
	TestEqual(TEXT("Invalid target is checked before an empty request"), Null.Diagnostics.Num(), 1);
	return !HasAnyErrors();
}

IMPLEMENT_COMPLEX_AUTOMATION_TEST(FDnd5eExternalLoaderTest, "Dnd5e.Json.ExternalLoader", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
void FDnd5eExternalLoaderTest::GetTests(TArray<FString>& Names, TArray<FString>& Commands) const
{
	FString Directory;
	if (FParse::Value(FCommandLine::Get(), TEXT("Dnd5eJsonSmokeDirectory="), Directory))
	{
		Names.Add(TEXT("ReadSelectedDirectory")); Commands.Add(Directory);
	}
}
bool FDnd5eExternalLoaderTest::RunTest(const FString& Parameters)
{
	FDnd5eJsonSpellCatalogLoadRequest Request;
	Request.DatasetRoot = FPaths::ConvertRelativePathToFull(Parameters);
	FPaths::NormalizeDirectoryName(Request.DatasetRoot);
	Request.DatasetNamespace = TEXT("external-file-audit");
	IFileManager::Get().FindFiles(Request.SpellFiles, *FPaths::Combine(Request.DatasetRoot, TEXT("spells-*.json")), true, false);
	Request.SpellFiles.Sort();
	if (!TestFalse(TEXT("Caller-selected directory contains spell files"), Request.SpellFiles.IsEmpty())) { return false; }
	int32 ExpectedRecords = 0;
	for (const auto& Relative : Request.SpellFiles)
	{
		FString Text;
		if (!TestTrue(TEXT("Read selected source to count expected entries independently"), FFileHelper::LoadFileToString(Text, *FPaths::Combine(Request.DatasetRoot, Relative)))) { return false; }
		TSharedPtr<FJsonObject> Root;
		if (!TestTrue(TEXT("Source has an object root"), FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Root))) { return false; }
		const TArray<TSharedPtr<FJsonValue>>* Spells = nullptr;
		if (!TestTrue(TEXT("Source has a spell array"), Root->TryGetArrayField(TEXT("spell"), Spells) && Spells)) { return false; }
		ExpectedRecords += Spells->Num();
	}
	FDnd5eSpellCatalog Catalog;
	const auto Result = FDnd5eJsonSpellCatalogLoader::LoadCatalog(Request, Catalog);
	for (const auto& Diagnostic : Result.Diagnostics)
	{
		if (Diagnostic.Severity == EDnd5eDiagnosticSeverity::Error) { AddError(Diagnostic.Code.ToString() + TEXT(" at ") + Diagnostic.RelativeFilePath + Diagnostic.JsonPointer); }
	}
	TestTrue(TEXT("Real selected files load through the bounded filesystem API"), Result.Status == EDnd5eJsonReadStatus::Success);
	TestEqual(TEXT("Every selected record installs"), Catalog.GetSpellCount(), ExpectedRecords);
	TestEqual(TEXT("Result reports the installed count"), Result.SpellCount, ExpectedRecords);
	for (const auto& Id : Catalog.GetSpellIds())
	{
		FDnd5eSpell Spell;
		if (!TestTrue(TEXT("Loaded identity resolves"), Catalog.TryGetSpell(Id, Spell) && !Spell.Header.Provenance.IsEmpty())) { return false; }
		TestTrue(TEXT("Provenance uses a selected relative label"), Request.SpellFiles.Contains(Spell.Header.Provenance[0].RelativeFilePath));
		TestFalse(TEXT("No availability file was supplied to this audit"), Spell.bHasAvailabilityData);
	}
	AddInfo(FString::Printf(TEXT("File loader installed %d records from %d explicitly selected files; returned diagnostics remain API data. No source passages are logged."), Catalog.GetSpellCount(), Request.SpellFiles.Num()));
	return !HasAnyErrors();
}

#endif
