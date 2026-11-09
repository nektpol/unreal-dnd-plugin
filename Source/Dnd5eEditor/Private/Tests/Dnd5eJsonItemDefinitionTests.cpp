// Developed by Neko Creative Technologies
#if WITH_DEV_AUTOMATION_TESTS
#include "Dnd5eTestSupport.h"
#include "Readers/Items/Dnd5eJsonItemDefinitionCatalogLibrary.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace
{
	FDnd5eJsonDocument DefinitionDocument(const FString& Text)
	{
		FDnd5eJsonDocument Document;
		Document.Context.DatasetNamespace = TEXT("synthetic-definitions");
		Document.Context.RelativeFilePath = TEXT("fixtures/definitions.json");
		Document.JsonText = Text;
		return Document;
	}

	const TCHAR* DefinitionFixture = TEXT(R"json({
		"itemType":[
			{"name":"Child","abbreviation":"C","source":"SYN-B","_copy":{"abbreviation":"B","source":"SYN-A"}},
			{"name":"Base","abbreviation":"B","source":"SYN-A","entries":["Invented entry."]},
			{"name":"Override","abbreviation":"O","source":"SYN-B","entries":[],"_copy":{"abbreviation":"B","source":"SYN-A"}}
		],
		"itemProperty":[{"abbreviation":"P","source":"SYN-A","page":0,"template":"","basicRules2024":false,"reprintedAs":["P|SYN-B"],"future":true}],
		"itemMastery":[{"name":"Invented mastery","source":"SYN-B"}]
	})json");
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eDefinitionReaderConversionTest, "Dnd5e.Items.Json.Conversion", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FDnd5eDefinitionReaderConversionTest::RunTest(const FString& Parameters)
{
	const auto Result = FDnd5eJsonItemDefinitionReader::Read(DefinitionDocument(DefinitionFixture));
	if (!TestTrue(TEXT("Read all domains"), Result.Status == EDnd5eJsonReadStatus::Success &&
		Result.Types.Num() == 3 && Result.Properties.Num() == 1 && Result.Masteries.Num() == 1)) { return false; }
	const auto& Child = Result.Types[0];
	TestTrue(TEXT("Forward copy inherits entries and retains both origins"), Child.Description.Num() == 1 &&
		Child.Header.Provenance.Num() == 2 && Child.Header.Provenance[0].SourceId == TEXT("SYN-B") &&
		Child.Header.Provenance[1].JsonPointer == TEXT("/itemType/1"));
	TestTrue(TEXT("Explicit empty entries override inheritance"), Result.Types[2].Description.IsEmpty() &&
		Result.Types[2].Header.Provenance.Num() == 1);
	const auto& Property = Result.Properties[0];
	TestTrue(TEXT("An empty template is supplied and the display name stays absent"),
		Property.bHasTemplate && Property.TemplateText.IsEmpty() && Property.Header.DisplayName.IsEmpty());
	TestTrue(TEXT("False membership and unknown evidence are retained"), Property.Header.Provenance.Num() == 1 &&
		Property.Header.Provenance[0].Memberships.Num() == 1 &&
		Property.Header.Provenance[0].Memberships[0].Inclusion == EDnd5eSourceInclusion::Excluded &&
		Property.Header.Provenance[0].AdditionalMetadataJson.Contains(TEXT("future")));
	if (Property.Header.Provenance.Num() != 1) { return false; }
	const auto& Origin = Property.Header.Provenance[0];
	TSharedPtr<FJsonObject> Evidence;
	double Page = -1;
	const auto EvidenceReader = TJsonReaderFactory<>::Create(Origin.AdditionalMetadataJson);
	TestTrue(TEXT("Numeric page zero remains evidence without a typed page"),
		FJsonSerializer::Deserialize(EvidenceReader, Evidence) && Evidence.IsValid() &&
		Evidence->TryGetNumberField(TEXT("page"), Page) && Page == 0 &&
		Origin.Page.Kind == EDnd5ePageReferenceKind::Unspecified);

	TestTrue(TEXT("Reprint remains unresolved"), Property.Relationships.Num() == 1 &&
		!Property.Relationships[0].bHasTargetRecord && Property.Relationships[0].SourceReference == TEXT("P|SYN-B"));
	TestEqual(TEXT("Two preservation diagnostics"), Result.Diagnostics.Num(), 2);
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eDefinitionReaderFailureTest, "Dnd5e.Items.Json.FailurePaths", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FDnd5eDefinitionReaderFailureTest::RunTest(const FString& Parameters)
{
	struct FCase { const TCHAR* Json; const TCHAR* Pointer; };
	const FCase Cases[] = {
		{TEXT(R"({"baseitem":[]})"), TEXT("")},
		{TEXT(R"({"itemProperty":null})"), TEXT("/itemProperty")},
		{TEXT(R"({"itemProperty":[{"abbreviation":"P","source":"S","template":1}]})"), TEXT("/itemProperty/0/template")},
		{TEXT(R"({"itemProperty":[{"abbreviation":"P","source":"S"},{"abbreviation":"p","source":"s"}]})"), TEXT("/itemProperty/1")},
		{TEXT(R"({"itemType":[{"name":"Child","abbreviation":"C","source":"S","_copy":{"abbreviation":"B","source":"S"}}]})"), TEXT("/itemType/0/_copy")},
		{TEXT(R"({"itemType":[{"name":"Child","abbreviation":"C","source":"S","_copy":{"abbreviation":"C","source":"S"}}]})"), TEXT("/itemType/0/_copy")},
		{TEXT(R"({"itemType":[{"name":"Child","abbreviation":"C","source":"S","_copy":{"abbreviation":"B","source":"S","_mod":{}}}]})"), TEXT("/itemType/0/_copy/_mod")}
	};
	for (const auto& Case : Cases)
	{
		const auto Result = FDnd5eJsonItemDefinitionReader::Read(DefinitionDocument(Case.Json));
		TestTrue(FString::Printf(TEXT("Failure publishes no partial rows at %s"), Case.Pointer),
			Result.Status == EDnd5eJsonReadStatus::Failure && Result.Types.IsEmpty() &&
			Result.Properties.IsEmpty() && Result.Masteries.IsEmpty());
		TestTrue(TEXT("Failure identifies the field"), Result.Diagnostics.ContainsByPredicate([&](const FDnd5eDiagnostic& Diagnostic)
		{
			return Diagnostic.Severity == EDnd5eDiagnosticSeverity::Error && Diagnostic.JsonPointer == Case.Pointer;
		}));
	}
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eDefinitionReaderPublishTest, "Dnd5e.Items.Json.AtomicPublication", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FDnd5eDefinitionReaderPublishTest::RunTest(const FString& Parameters)
{
	auto* Catalog = NewObject<UDnd5eItemDefinitionCatalogObject>();
	TArray<FDnd5eDiagnostic> Diagnostics;
	if (!TestTrue(TEXT("Publish through Blueprint library"), UDnd5eJsonItemDefinitionCatalogLibrary::BuildItemDefinitionCatalog(
		Catalog, DefinitionDocument(DefinitionFixture), Diagnostics))) { return false; }
	const auto Ids = Catalog->GetDefinitionIds();
	TestTrue(TEXT("Preserve warnings after publication"), Ids.Num() == 5 && Diagnostics.Num() == 2);
	TestFalse(TEXT("Failed read rejects replacement"), UDnd5eJsonItemDefinitionCatalogLibrary::BuildItemDefinitionCatalog(
		Catalog, DefinitionDocument(TEXT(R"({"itemType":[],"itemMastery":[{}]})")), Diagnostics));
	TestTrue(TEXT("Failed replacement preserves ordered IDs"), Catalog->GetDefinitionIds() == Ids);
	TestTrue(TEXT("Explicit empty input clears catalog"), UDnd5eJsonItemDefinitionCatalogLibrary::BuildItemDefinitionCatalog(
		Catalog, DefinitionDocument(TEXT(R"({"itemType":[]})")), Diagnostics) && Catalog->GetDefinitionCount() == 0);
	return !HasAnyErrors();
}

IMPLEMENT_COMPLEX_AUTOMATION_TEST(FDnd5eDefinitionSnapshotTest, "Dnd5e.Items.Json.ExternalDefinitions", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
void FDnd5eDefinitionSnapshotTest::GetTests(TArray<FString>& Names, TArray<FString>& Commands) const
{
	FString Path;
	if (FParse::Value(FCommandLine::Get(), TEXT("Dnd5eJsonDefinitionFile="), Path))
	{
		Names.Add(TEXT("InspectedSnapshot"));
		Commands.Add(Path);
	}
}

bool FDnd5eDefinitionSnapshotTest::RunTest(const FString& Parameters)
{
	FString Text;
	if (!TestTrue(TEXT("Read explicitly supplied snapshot"), FFileHelper::LoadFileToString(Text, *Parameters))) { return false; }
	const auto Result = FDnd5eJsonItemDefinitionReader::Read(DefinitionDocument(Text));
	for (const auto& Diagnostic : Result.Diagnostics)
	{
		if (Diagnostic.Severity == EDnd5eDiagnosticSeverity::Error)
		{
			AddError(Diagnostic.JsonPointer + TEXT(" ") + Diagnostic.Message);
		}
	}
	TestTrue(TEXT("Snapshot yields all 102 definitions"), Result.Status == EDnd5eJsonReadStatus::Success &&
		Result.Types.Num() == 67 && Result.Properties.Num() == 27 && Result.Masteries.Num() == 8);
	int32 Inherited = 0;
	for (const auto& Type : Result.Types)
	{
		if (Type.Header.Provenance.Num() == 2 && !Type.Description.IsEmpty()) { ++Inherited; }
	}
	TestEqual(TEXT("Both direct copies inherit descriptions"), Inherited, 2);
	return !HasAnyErrors();
}
#endif
