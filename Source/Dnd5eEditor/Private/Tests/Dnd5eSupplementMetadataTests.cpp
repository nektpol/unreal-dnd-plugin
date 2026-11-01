#if WITH_DEV_AUTOMATION_TESTS
#include "Dnd5eCoverageTestSupport.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
using namespace Dnd5eTests;
using namespace Dnd5eCoverageTests;

namespace
{
	const FString FluffRow = TEXT(R"json([{"name":"Invented Beacon","source":"SYN","entries":["Supplement {@item Invented|SYN}"],"images":[{"type":"image","href":{"type":"internal","path":"art/invented.png"},"width":320,"height":200,"credit":"Invented credit"}]}])json");
	FDnd5eJsonSpellCatalogBuildRequest BuildRequest()
	{
		FDnd5eJsonSpellCatalogBuildRequest Request; Request.SpellDocuments.Add(Document()); Request.SpellFluffDocuments.Add(Supplement(FluffRow)); return Request;
	}
	struct FFiles
	{
		FString Base = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("Automation/CoverageFixtures"));
		FString Root = Base / FGuid::NewGuid().ToString(EGuidFormats::Digits);
		bool Write(const TCHAR* Name, const FString& Json) { IFileManager::Get().MakeDirectory(*Root, true); return FFileHelper::SaveStringToFile(Json, *(Root / Name)); }
		~FFiles() { FString Path = FPaths::ConvertRelativePathToFull(Root); if (FPaths::CollapseRelativeDirectories(Path) && FPaths::IsUnderDirectory(Path, Base) && !FPaths::IsSamePath(Path, Base)) { IFileManager::Get().DeleteDirectory(*Path, false, true); } }
	};
}

IMPLEMENT_COMPLEX_AUTOMATION_TEST(FDnd5eSupplementCasesTest, "Dnd5e.Json.Coverage.Supplements", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
void FDnd5eSupplementCasesTest::GetTests(TArray<FString>& Names, TArray<FString>& Commands) const
{
	for (const TCHAR* Name : {TEXT("Joined"),TEXT("EmptyRow"),TEXT("NoInput"),TEXT("Unmatched"),TEXT("NamespaceCase"),TEXT("SourceCase"),TEXT("DuplicateRows"),TEXT("DuplicateFiles"),TEXT("DuplicatePrimary"),TEXT("LaterMalformed"),TEXT("InvalidImageWidth"),TEXT("UnsafeImagePath"),TEXT("UnknownImage"),TEXT("WrongImageKind"),TEXT("CombinedFileLimit"),TEXT("ValidationFailure"),TEXT("ValidationInactive")}) { Names.Add(Name); Commands.Add(Name); }
}
bool FDnd5eSupplementCasesTest::RunTest(const FString& Parameters)
{
	FDnd5eSpellCatalog Catalog; auto Request = BuildRequest();
	if (!TestEqual(TEXT("Seed catalog"), FDnd5eJsonSpellCatalogBuilder::BuildCatalog(Request, Catalog).Status, EDnd5eJsonReadStatus::Success)) { return false; }
	const auto OriginalId = Catalog.GetSpellIds()[0]; FDnd5eSpell Before; Catalog.TryGetSpell(OriginalId, Before);
	bool Failure = false, Joined = true;
	if (Parameters == TEXT("EmptyRow")) { Request.SpellFluffDocuments[0] = Supplement(TEXT("[{\"name\":\"Invented Beacon\",\"source\":\"SYN\"}]")); }
	if (Parameters == TEXT("NoInput")) { Request.SpellFluffDocuments.Reset(); Joined = false; }
	if (Parameters == TEXT("Unmatched")) { Request.SpellFluffDocuments[0].JsonText.ReplaceInline(TEXT("Invented Beacon"), TEXT("Other Invented")); Joined = false; }
	if (Parameters == TEXT("NamespaceCase")) { Request.SpellFluffDocuments[0].Context.DatasetNamespace = TEXT("Coverage-synthetic"); Joined = false; }
	if (Parameters == TEXT("SourceCase")) { Request.SpellFluffDocuments[0].JsonText.ReplaceInline(TEXT("\"SYN\""), TEXT("\"syn\"")); Joined = false; }
	if (Parameters == TEXT("DuplicateRows")) { Request.SpellFluffDocuments[0] = Supplement(FluffRow.LeftChop(1) + TEXT(",") + FluffRow.Mid(1)); Failure = true; }
	if (Parameters == TEXT("DuplicateFiles")) { const auto Copy = Request.SpellFluffDocuments[0]; Request.SpellFluffDocuments.Add(Copy); Failure = true; }
	if (Parameters == TEXT("DuplicatePrimary")) { const auto Copy = Request.SpellDocuments[0]; Request.SpellDocuments.Add(Copy); Failure = true; }
	if (Parameters == TEXT("LaterMalformed")) { auto Bad = Supplement(TEXT("[{\"name\":\"Broken\"}]")); Request.SpellFluffDocuments.Add(Bad); Failure = true; }
	if (Parameters == TEXT("InvalidImageWidth")) { Request.SpellFluffDocuments[0].JsonText.ReplaceInline(TEXT("320"), TEXT("0")); Failure = true; }
	if (Parameters == TEXT("UnsafeImagePath")) { Request.SpellFluffDocuments[0].JsonText.ReplaceInline(TEXT("art/invented.png"), TEXT("../outside.png")); Failure = true; }
	if (Parameters == TEXT("UnknownImage")) { Request.SpellFluffDocuments[0].JsonText.ReplaceInline(TEXT("\"width\":320"), TEXT("\"futureImageField\":320")); }
	if (Parameters == TEXT("WrongImageKind")) { Request.SpellFluffDocuments[0] = Supplement(TEXT("[{\"name\":\"Invented Beacon\",\"source\":\"SYN\",\"images\":[\"text\"]}]")); Failure = true; }
	if (Parameters == TEXT("CombinedFileLimit")) { const auto Empty = Supplement(TEXT("[]")); Request.SpellFluffDocuments.Init(Empty, 1024); Failure = true; }
	if (Parameters.StartsWith(TEXT("Validation"))) { Request.SpellDocuments[0] = Document(TEXT("[{\"type\":\"image\",\"href\":{\"type\":\"external\",\"url\":\"https://\"}}]")); Request.bValidateContent = Parameters == TEXT("ValidationFailure"); Failure = Request.bValidateContent; }
	const auto InputBefore = Request; const auto Result = FDnd5eJsonSpellCatalogBuilder::BuildCatalog(Request, Catalog);
	TestEqual(TEXT("Expected atomic status"), Result.Status, Failure ? EDnd5eJsonReadStatus::Failure : EDnd5eJsonReadStatus::Success);
	FDnd5eSpell After; TestTrue(TEXT("Stable primary identity available"), Catalog.TryGetSpell(OriginalId, After));
	if (Failure)
	{
		TestTrue(TEXT("Failed candidate leaves entire previous record intact"), Equal(FDnd5eSpell::StaticStruct(), &Before, &After));
		TestTrue(TEXT("Failed candidates do not leak unmatched rows"), Result.UnmatchedSupplements.IsEmpty());
		if (Parameters == TEXT("ValidationFailure")) { TestTrue(TEXT("Model errors are separate from source pointers"), Result.ContentValidationIssues.Num() == 1 && !Result.ContentValidationIssues[0].ContentPath.IsEmpty() && Result.ContentValidationIssues[0].Diagnostic.JsonPointer.IsEmpty()); }
	}
	else
	{
		TestEqual(TEXT("Supplied versus absent supplement"), After.bHasSupplementaryContent, Joined);
		if (Parameters == TEXT("Joined")) { TestTrue(TEXT("Independent provenance and content role"), After.Header.Provenance.Num() == 2 && After.SupplementaryContent.Entries.Num() == 1 && After.SupplementaryContent.Images.Num() == 1); const auto& Image = After.SupplementaryContent.Images[0].Payload.Get<FDnd5eImageContent>(); TestTrue(TEXT("Media metadata retained without opening files"), Image.Resource.RelativePath == TEXT("art/invented.png") && Image.Width == 320 && Image.Height == 200 && Image.Credit.Num() == 1); }
		if (!Joined && Parameters != TEXT("NoInput")) { TestEqual(TEXT("Unmatched input retained"), Result.UnmatchedSupplements.Num(), 1); }
		if (Parameters == TEXT("UnknownImage")) { TestTrue(TEXT("Unknown image is an explicit opaque leaf"), After.SupplementaryContent.Images[0].Kind == Dnd5e::Content::Block::Unsupported); }
		if (Parameters == TEXT("EmptyRow")) { TestTrue(TEXT("Known empty differs from absent"), After.bHasSupplementaryContent && After.SupplementaryContent.Images.IsEmpty() && After.SupplementaryContent.Entries.IsEmpty()); }
	}
	TestTrue(TEXT("Build input immutable"), Equal(FDnd5eJsonSpellCatalogBuildRequest::StaticStruct(), &Request, &InputBefore));
	return !HasAnyErrors();
}

IMPLEMENT_COMPLEX_AUTOMATION_TEST(FDnd5eMembershipCasesTest, "Dnd5e.Json.Coverage.Memberships", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
void FDnd5eMembershipCasesTest::GetTests(TArray<FString>& Names, TArray<FString>& Commands) const
{
	for (const TCHAR* Name : {TEXT("Absent"),TEXT("True"),TEXT("False"),TEXT("Alternate"),TEXT("Hints"),TEXT("EmptyAlternate"),TEXT("Numeric"),TEXT("Null"),TEXT("Array"),TEXT("BasicName"),TEXT("HintString")}) { Names.Add(Name); Commands.Add(Name); }
}
bool FDnd5eMembershipCasesTest::RunTest(const FString& Parameters)
{
	FString Extra;
	if (Parameters == TEXT("True")) { Extra = TEXT(",\"srd\":true,\"srd52\":true,\"basicRules\":true,\"basicRules2024\":true"); }
	else if (Parameters == TEXT("False")) { Extra = TEXT(",\"srd\":false"); }
	else if (Parameters == TEXT("Alternate")) { Extra = TEXT(",\"srd\":\"Invented Alternate\""); }
	else if (Parameters == TEXT("Hints")) { Extra = TEXT(",\"hasFluff\":false,\"hasFluffImages\":true"); }
	else if (Parameters == TEXT("EmptyAlternate")) { Extra = TEXT(",\"srd\":\" \""); }
	else if (Parameters == TEXT("Numeric")) { Extra = TEXT(",\"srd\":1"); }
	else if (Parameters == TEXT("Null")) { Extra = TEXT(",\"srd\":null"); }
	else if (Parameters == TEXT("Array")) { Extra = TEXT(",\"srd\":[]"); }
	else if (Parameters == TEXT("BasicName")) { Extra = TEXT(",\"basicRules\":\"Invented\""); }
	else if (Parameters == TEXT("HintString")) { Extra = TEXT(",\"hasFluff\":\"yes\""); }
	const bool Valid = Parameters == TEXT("Absent") || Parameters == TEXT("True") || Parameters == TEXT("False") || Parameters == TEXT("Alternate") || Parameters == TEXT("Hints");
	const auto Result = Read(Document(TEXT("[\"Invented text\"]"), Extra));
	if (!TestEqual(TEXT("Known metadata type policy"), Result.Status, Valid ? EDnd5eJsonReadStatus::Success : EDnd5eJsonReadStatus::Failure) || !Valid) { return !HasAnyErrors(); }
	const auto& Spell = Result.Spell; const auto& Memberships = Spell.Header.Provenance[0].Memberships;
	if (Parameters == TEXT("True")) { TestEqual(TEXT("Four independent membership identities"), Memberships.Num(), 4); for (const auto& V : Memberships) { TestEqual(TEXT("Included state"), V.Inclusion, EDnd5eSourceInclusion::Included); } }
	if (Parameters == TEXT("False")) { TestTrue(TEXT("False is an explicit exclusion"), Memberships.Num() == 1 && Memberships[0].Inclusion == EDnd5eSourceInclusion::Excluded); }
	if (Parameters == TEXT("Alternate")) { TestTrue(TEXT("Alternate SRD name stays typed"), Memberships.Num() == 1 && Memberships[0].Inclusion == EDnd5eSourceInclusion::IncludedWithAlternateName && Memberships[0].AlternateName == TEXT("Invented Alternate")); }
	if (Parameters == TEXT("Absent")) { TestTrue(TEXT("Absence remains unknown"), Memberships.IsEmpty() && Spell.SupplementaryTextHint == EDnd5eDataAvailability::Unknown); }
	if (Parameters == TEXT("Hints")) { TestTrue(TEXT("Hints are not loaded content"), Spell.SupplementaryTextHint == EDnd5eDataAvailability::Absent && Spell.SupplementaryImagesHint == EDnd5eDataAvailability::Present && !Spell.bHasSupplementaryContent); }
	return !HasAnyErrors();
}

IMPLEMENT_COMPLEX_AUTOMATION_TEST(FDnd5eEditionCasesTest, "Dnd5e.Json.Coverage.EditionPolicy", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
void FDnd5eEditionCasesTest::GetTests(TArray<FString>& Names, TArray<FString>& Commands) const
{
	for (const TCHAR* Name : {TEXT("Original"),TEXT("Revised"),TEXT("Both"),TEXT("NamespaceCase"),TEXT("SourceCase"),TEXT("Duplicate"),TEXT("UnsupportedTag"),TEXT("NoPolicyName"),TEXT("Inactive"),TEXT("NoTags")}) { Names.Add(Name); Commands.Add(Name); }
}
bool FDnd5eEditionCasesTest::RunTest(const FString& Parameters)
{
	FDnd5eJsonSpellCatalogBuildRequest Request; Request.SpellDocuments.Add(Document()); Request.EditionPolicyId = TEXT("synthetic-policy-v1");
	FDnd5eSourceEditionAssignment Assignment; Assignment.DatasetNamespace = TEXT("coverage-synthetic"); Assignment.SourceId = TEXT("SYN"); Assignment.EditionTags.AddTag(Dnd5e::Rules::Edition::Original2014);
	if (Parameters == TEXT("Revised")) { Assignment.EditionTags.Reset(); Assignment.EditionTags.AddTag(Dnd5e::Rules::Edition::Revised2024); }
	if (Parameters == TEXT("Both")) { Assignment.EditionTags.AddTag(Dnd5e::Rules::Edition::Revised2024); }
	if (Parameters == TEXT("NamespaceCase")) { Assignment.DatasetNamespace = TEXT("Coverage-synthetic"); }
	if (Parameters == TEXT("SourceCase")) { Assignment.SourceId = TEXT("syn"); }
	if (Parameters == TEXT("UnsupportedTag")) { Assignment.EditionTags.AddTag(Dnd5e::Category::Spell); }
	if (Parameters == TEXT("NoTags")) { Assignment.EditionTags.Reset(); }
	Request.EditionAssignments.Add(Assignment);
	if (Parameters == TEXT("Duplicate")) { Request.EditionAssignments.Add(Assignment); }
	if (Parameters == TEXT("NoPolicyName")) { Request.EditionPolicyId.Reset(); }
	if (Parameters == TEXT("Inactive")) { Request.EditionAssignments.Reset(); Request.EditionPolicyId.Reset(); }
	const bool Failure = Parameters == TEXT("Duplicate") || Parameters == TEXT("UnsupportedTag") || Parameters == TEXT("NoTags") || Parameters == TEXT("NoPolicyName");
	FDnd5eSpellCatalog Catalog; const auto Result = FDnd5eJsonSpellCatalogBuilder::BuildCatalog(Request, Catalog);
	if (!TestEqual(TEXT("Explicit edition policy validation"), Result.Status, Failure ? EDnd5eJsonReadStatus::Failure : EDnd5eJsonReadStatus::Success) || Failure) { return !HasAnyErrors(); }
	FDnd5eSpell Spell; Catalog.TryGetSpell(Catalog.GetSpellIds()[0], Spell);
	const bool Match = Parameters == TEXT("Original") || Parameters == TEXT("Revised") || Parameters == TEXT("Both");
	TestEqual(TEXT("Only exact policy mappings classify"), Spell.Header.Classification.HasTagExact(Dnd5e::Rules::Edition::Original2014) || Spell.Header.Classification.HasTagExact(Dnd5e::Rules::Edition::Revised2024), Match);
	TestTrue(TEXT("Classification evidence preserved alongside school"), Spell.Header.Provenance[0].ClassificationEvidenceJson.Contains(TEXT("school")));
	TestEqual(TEXT("Policy basis recorded only when applied"), Spell.Header.Provenance[0].ClassificationEvidenceJson.Contains(TEXT("editionPolicyId")), Match);
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eSupplementLoaderTest, "Dnd5e.Json.Coverage.SupplementLoaderAndForwarding", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FDnd5eSupplementLoaderTest::RunTest(const FString& Parameters)
{
	FFiles Files; if (!TestTrue(TEXT("Write original synthetic fixtures"), Files.Write(TEXT("primary.json"), Document().JsonText) && Files.Write(TEXT("fluff.json"), Supplement(FluffRow).JsonText))) { return false; }
	FDnd5eJsonSpellCatalogLoadRequest Request; Request.DatasetRoot = Files.Root; Request.DatasetNamespace = TEXT("coverage-synthetic"); Request.SpellFiles.Add(TEXT("primary.json")); Request.SpellFluffFiles.Add(TEXT("fluff.json")); Request.bValidateContent = true;
	Request.EditionPolicyId = TEXT("synthetic-policy-v1"); FDnd5eSourceEditionAssignment Assignment; Assignment.DatasetNamespace = Request.DatasetNamespace; Assignment.SourceId = TEXT("SYN"); Assignment.EditionTags.AddTag(Dnd5e::Rules::Edition::Original2014); Request.EditionAssignments.Add(Assignment);
	FDnd5eSpellCatalog Catalog; const auto Result = FDnd5eJsonSpellCatalogLoader::LoadCatalog(Request, Catalog);
	if (!TestEqual(TEXT("Files convert with validation and edition forwarding"), Result.Status, EDnd5eJsonReadStatus::Success)) { return false; }
	FDnd5eSpell Spell; const auto Id = Catalog.GetSpellIds()[0]; Catalog.TryGetSpell(Id, Spell);
	TestTrue(TEXT("All loader options forwarded"), Spell.bHasSupplementaryContent && Spell.Header.Classification.HasTagExact(Dnd5e::Rules::Edition::Original2014));
	for (const TCHAR* Invalid : {TEXT("primary.json"),TEXT("../escape.json"),TEXT("missing.json")})
	{
		Request.SpellFluffFiles = {Invalid}; TestEqual(TEXT("Invalid supplied file rejected"), FDnd5eJsonSpellCatalogLoader::LoadCatalog(Request, Catalog).Status, EDnd5eJsonReadStatus::Failure);
		FDnd5eSpell After; Catalog.TryGetSpell(Id, After); TestTrue(TEXT("Failed load preserves previous attached content"), Equal(FDnd5eSpell::StaticStruct(), &After, &Spell));
	}
	Request.SpellFluffFiles = {TEXT("fluff.json")};
	TestTrue(TEXT("Write syntactically readable but invalid canonical Content"), Files.Write(TEXT("primary.json"), Document(TEXT("[{\"type\":\"image\",\"href\":{\"type\":\"external\",\"url\":\"https://\"}}]")).JsonText));
	const auto InvalidContent = FDnd5eJsonSpellCatalogLoader::LoadCatalog(Request, Catalog);
	TestTrue(TEXT("Loader actually forwards the validation flag"), InvalidContent.Status == EDnd5eJsonReadStatus::Failure && InvalidContent.ContentValidationIssues.Num() == 1);
	FDnd5eSpell Preserved; Catalog.TryGetSpell(Id, Preserved); TestTrue(TEXT("Forwarded validation failure is atomic"), Equal(FDnd5eSpell::StaticStruct(), &Preserved, &Spell));
	Request.bValidateContent = false;
	TestEqual(TEXT("Inactive canonical validation remains opt-in"), FDnd5eJsonSpellCatalogLoader::LoadCatalog(Request, Catalog).Status, EDnd5eJsonReadStatus::Success);
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eSupplementReaderAtomicTest, "Dnd5e.Json.Coverage.SupplementReaderOutputAtomicity", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FDnd5eSupplementReaderAtomicTest::RunTest(const FString& Parameters)
{
	TArray<FDnd5eJsonSpellSupplement> Rows; TArray<FDnd5eDiagnostic> Diagnostics; int32 Values = -1;
	const auto Valid = Supplement(FluffRow); const auto Before = Valid;
	if (!TestTrue(TEXT("Standalone reader publishes complete rows"), FDnd5eJsonSpellSupplementReader::ReadDocument(Valid, Rows, Values, Diagnostics) && Rows.Num() == 1 && Values > 0)) { return false; }
	const auto Bad = Supplement(FluffRow.LeftChop(1) + TEXT(",{\"name\":\"broken\"}]") );
	TestFalse(TEXT("Malformed later row fails"), FDnd5eJsonSpellSupplementReader::ReadDocument(Bad, Rows, Values, Diagnostics));
	TestTrue(TEXT("No stale or partially converted output"), Rows.IsEmpty() && Values == 0 && !Diagnostics.IsEmpty());
	TestTrue(TEXT("Original supplied document remains immutable"), Equal(FDnd5eJsonDocument::StaticStruct(), &Valid, &Before));
	return !HasAnyErrors();
}
#endif
