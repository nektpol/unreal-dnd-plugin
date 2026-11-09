// Developed by Neko Creative Technologies
#if WITH_DEV_AUTOMATION_TESTS
#include "Dnd5eTestSupport.h"
#include "Items/Dnd5eItemIdentity.h"
#include "Readers/Items/Dnd5eJsonBaseItemLibrary.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"

namespace
{
	FDnd5eJsonDocument BaseItemDocument(const FString& Json)
	{
		FDnd5eJsonDocument Document;
		Document.Context.DatasetNamespace = TEXT("items-test");
		Document.Context.RelativeFilePath = TEXT("fixtures/base-items.json");
		Document.JsonText = Json;
		return Document;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eBaseItemConversionTest, "Dnd5e.Items.BaseJson.Conversion", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FDnd5eBaseItemConversionTest::RunTest(const FString& Parameters)
{
	const auto Result = UDnd5eJsonBaseItemLibrary::ReadBaseItems(BaseItemDocument(TEXT(R"json({
		"baseitem":[{"name":"Invented Gear","source":"SYN-A","type":"T1",
		"property":[{"uid":"P9|SYN-A","note":"Invented note"}],
		"mastery":[{"uid":"Invented Mastery|SYN-A","note":"Invented mastery note"}],
		"rarity":"none","weight":0.25,"value":12.5,"page":0,"basicRules2024":false,
		"entries":["Invented description."],"dmg1":"1d6"}]
	})json")));
	if (!TestTrue(TEXT("One item through the Blueprint library"), Result.Status == EDnd5eJsonReadStatus::Success && Result.Items.Num() == 1)) { return false; }
	const auto& Item = Result.Items[0];
	TestEqual(TEXT("Canonical key"), Item.Header.RecordId.SourceKey, FString(TEXT("dnd5e:item:v1:invented gear|syn-a")));
	TestTrue(TEXT("Type keeps omitted source and property keeps explicit source"), Item.bHasType && !Item.Type.bHasSource &&
		Item.Properties.Num() == 1 && Item.Properties[0].bHasSource && Item.Properties[0].SourceId == TEXT("SYN-A"));
	TestTrue(TEXT("Mastery note is converted"), Item.Masteries.Num() == 1 && Item.Masteries[0].Note.Num() == 1);
	TestTrue(TEXT("Fractional price and weight retained"), Item.Price.CostPresence == EDnd5eValuePresence::Present &&
		Item.Price.Cost.Num() == 1 && Item.Price.Cost[0].Currency == EDnd5eCurrency::Copper &&
		Item.Price.Cost[0].Amount.SourceLiteral == TEXT("12.5") && Item.bHasWeight &&
		Item.Weight.Unit == EDnd5eMassUnit::Pounds && Item.Weight.Amount.SourceLiteral == TEXT("0.25"));
	if (!TestEqual(TEXT("One primary origin"), Item.Header.Provenance.Num(), 1)) { return false; }
	const auto& Origin = Item.Header.Provenance[0];
	TestTrue(TEXT("Page zero and unconverted equipment and property note stay as evidence"),
		Origin.Page.Kind == EDnd5ePageReferenceKind::Unspecified &&
		Origin.AdditionalMetadataJson.Contains(TEXT("\"page\"")) &&
		Origin.AdditionalMetadataJson.Contains(TEXT("Invented note")) &&
		Origin.AdditionalMetadataJson.Contains(TEXT("1d6")));
	TestTrue(TEXT("Explicit false membership stays excluded"), Origin.Memberships.Num() == 1 &&
		Origin.Memberships[0].Inclusion == EDnd5eSourceInclusion::Excluded);
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eBaseItemFailureTest, "Dnd5e.Items.BaseJson.FailureAndPresence", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FDnd5eBaseItemFailureTest::RunTest(const FString& Parameters)
{
	const auto Empty = FDnd5eJsonBaseItemReader::Read(BaseItemDocument(TEXT(R"({"baseitem":[]})")));
	TestTrue(TEXT("Explicit empty array succeeds"), Empty.Status == EDnd5eJsonReadStatus::Success && Empty.Items.IsEmpty());
	const auto Invalid = FDnd5eJsonBaseItemReader::Read(BaseItemDocument(TEXT(R"({"baseitem":[
		{"name":"Gear","source":"SYN","type":"T","rarity":"none"},
		{"name":"gear","source":"syn","type":"T","rarity":"none"}]})")));
	TestTrue(TEXT("Case-folded duplicate discards staged rows"), Invalid.Status == EDnd5eJsonReadStatus::Failure &&
		Invalid.Items.IsEmpty() && Invalid.Diagnostics.Num() == 1 && Invalid.Diagnostics[0].JsonPointer == TEXT("/baseitem/1"));
	const auto Price = FDnd5eJsonBaseItemReader::Read(BaseItemDocument(TEXT(R"({"baseitem":[
		{"name":"Null","source":"SYN","type":"T","rarity":"none","value":null},
		{"name":"Custom","source":"SYN","type":"T","rarity":"none","value":7,"currencyConversion":"SYN","weight":2}]})")));
	if (!TestTrue(TEXT("Both price states read"), Price.Status == EDnd5eJsonReadStatus::Success && Price.Items.Num() == 2)) { return false; }
	TestTrue(TEXT("Null and custom currency are distinct"), Price.Items[0].Price.CostPresence == EDnd5eValuePresence::ExplicitNull &&
		Price.Items[1].Price.CostPresence == EDnd5eValuePresence::Unspecified && Price.Items[1].bHasWeight);
	FDnd5eRecordId Id;
	TestTrue(TEXT("Item ID escapes delimiters"), Dnd5e::Items::TryMakeId(TEXT("scope"), TEXT("A|B"), TEXT("S:%"), Id) &&
		Id.SourceKey == TEXT("dnd5e:item:v1:a%7Cb|s%3A%25"));
	TestTrue(TEXT("Failed construction clears old ID"), !Dnd5e::Items::TryMakeId(TEXT("scope"), TEXT(""), TEXT("S"), Id) && Id == FDnd5eRecordId{});
	return !HasAnyErrors();
}

IMPLEMENT_COMPLEX_AUTOMATION_TEST(FDnd5eBaseItemSnapshotTest, "Dnd5e.Items.BaseJson.External", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
void FDnd5eBaseItemSnapshotTest::GetTests(TArray<FString>& Names, TArray<FString>& Commands) const
{
	FString Path;
	if (FParse::Value(FCommandLine::Get(), TEXT("Dnd5eJsonDefinitionFile="), Path))
	{
		Names.Add(TEXT("InspectedSnapshot"));
		Commands.Add(Path);
	}
}

bool FDnd5eBaseItemSnapshotTest::RunTest(const FString& Parameters)
{
	FString Text;
	if (!TestTrue(TEXT("Read supplied local snapshot"), FFileHelper::LoadFileToString(Text, *Parameters))) { return false; }
	const auto Result = FDnd5eJsonBaseItemReader::Read(BaseItemDocument(Text));
	for (const auto& Diagnostic : Result.Diagnostics)
	{
		if (Diagnostic.Severity == EDnd5eDiagnosticSeverity::Error)
		{
			AddError(Diagnostic.JsonPointer + TEXT(" ") + Diagnostic.Message);
		}
	}
	TestTrue(TEXT("Read all 230 base items"), Result.Status == EDnd5eJsonReadStatus::Success && Result.Items.Num() == 230);
	return !HasAnyErrors();
}
#endif
