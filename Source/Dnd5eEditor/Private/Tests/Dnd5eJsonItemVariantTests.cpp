// Developed by Neko Creative Technologies
#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Readers/Items/Dnd5eJsonItemLibrary.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"

namespace
{
	using EPolicy = EDnd5eJsonItemVariantEditionPolicy;

	FDnd5eJsonDocument VariantMatchDocument(const FString& Json, const TCHAR* RelativePath)
	{
		FDnd5eJsonDocument Document;
		Document.Context.DatasetNamespace = TEXT("item-variant-test");
		Document.Context.RelativeFilePath = RelativePath;
		Document.JsonText = Json;
		return Document;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eItemVariantRulesTest, "Dnd5e.Items.VariantMatch.RulesAndEditions", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FDnd5eItemVariantRulesTest::RunTest(const FString& Parameters)
{
	const auto Variants = VariantMatchDocument(TEXT(R"json({"magicvariant":[
		{"name":"Invented Pattern","inherits":{"source":"VAR"},
		 "requires":[{"type":"M|SYN","sword":true},{"weapon":true,"property":"F|SYN"}],
		 "excludes":{"name":["Invented Relic","Invented Other"],"net":true}},
		{"name":"Invented Classic","edition":"classic","inherits":{"source":"VAR"},"requires":[{}]},
		{"name":"Invented Modern","edition":"one","inherits":{"source":"VAR"},"requires":[{}]},
		{"name":"Invented False","edition":"one","inherits":{"source":"VAR"},"requires":[{"weapon":false}]},
		{"name":"Invented Empty","inherits":{"source":"VAR"},"requires":[]},
		{"name":"Invented Alternatives","inherits":{"source":"VAR"},"requires":[{"property":["F|SYN","H|SYN"]}],
		 "excludes":{"property":["C|SYN","D|SYN"]}}
	]})json"), TEXT("fixtures/variants.json"));
	const auto Bases = VariantMatchDocument(TEXT(R"json({"baseitem":[
		{"name":"Classic Blade","type":"M|SYN","sword":true,"edition":"classic"},
		{"name":"Modern Blade","type":"M|SYN","sword":true,"edition":"one"},
		{"name":"Finesse Weapon","weapon":true,"property":["F|SYN"],"edition":"one"},
		{"name":"Wrong Case","weapon":true,"property":["f|syn"],"edition":"one"},
		{"name":"Invented Relic","type":"M|SYN","sword":true},
		{"name":"Bundle","type":"M|SYN","sword":true,"edition":"one","packContents":[]},
		{"name":"Unspecified Blade","type":"M|SYN","sword":true,"edition":null},
		{"name":"Explicit False","weapon":false,"edition":"one"},
		{"name":"Net","type":"M|SYN","sword":true,"net":true},
		{"name":"Half Match","type":"M|SYN","sword":"true"},
		{"name":"Blocked Finesse","property":["F|SYN","D|SYN"],"packContents":null},
		{"name":"Scalar Alternative","property":"H|SYN"}
	]})json"), TEXT("fixtures/base-items.json"));

	struct FCase { int32 Index; EPolicy Policy; TArray<int32> Matches; };
	for (const FCase& Case : {
		FCase{0, EPolicy::SourceDefault, {1, 2, 6}}, {0, EPolicy::PreferClassic, {0, 6}},
		{1, EPolicy::SourceDefault, {0, 4, 6, 8, 9, 10, 11}}, {1, EPolicy::PreferClassic, {0, 4, 6, 8, 9, 10, 11}},
		{2, EPolicy::SourceDefault, {1, 2, 3, 4, 6, 7, 8, 9, 10, 11}},
		{2, EPolicy::PreferClassic, {1, 2, 3, 4, 6, 7, 8, 9, 10, 11}},
		{3, EPolicy::SourceDefault, {7}}, {4, EPolicy::SourceDefault, {}},
		{5, EPolicy::SourceDefault, {2, 11}}})
	{
		const auto Result = UDnd5eJsonItemLibrary::MatchMagicItemVariantAtIndex(Variants, Case.Index, Bases, Case.Policy);
		TestTrue(FString::Printf(TEXT("Template %d policy %d returns original ordered matches"), Case.Index, static_cast<int32>(Case.Policy)),
			Result.Status == EDnd5eJsonReadStatus::Success && !Result.VariantName.IsEmpty() &&
			Result.VariantSourceId == TEXT("VAR") && Result.Diagnostics.IsEmpty() && Result.MatchingBaseIndices == Case.Matches);
	}

	const auto* ResultType = FDnd5eJsonItemVariantMatchResult::StaticStruct();
	const auto* Matches = ResultType->FindPropertyByName(TEXT("MatchingBaseIndices"));
	const auto* Node = UDnd5eJsonItemLibrary::StaticClass()->FindFunctionByName(TEXT("MatchMagicItemVariantAtIndex"));
	TestTrue(TEXT("The result and matching node are available to Blueprint"), ResultType->GetBoolMetaData(TEXT("BlueprintType")) &&
		Matches && Matches->HasAllPropertyFlags(CPF_BlueprintVisible | CPF_BlueprintReadOnly) &&
		Node && Node->HasAllFunctionFlags(FUNC_Static | FUNC_BlueprintCallable));
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eItemVariantFailureTest, "Dnd5e.Items.VariantMatch.AtomicFailures", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FDnd5eItemVariantFailureTest::RunTest(const FString& Parameters)
{
	const TCHAR* VariantPath = TEXT("fixtures/variants.json");
	const TCHAR* BasePath = TEXT("fixtures/base-items.json");
	const auto Variants = VariantMatchDocument(TEXT(R"json({"magicvariant":[
		{"name":"Valid","requires":[{}],"inherits":{"source":"VAR"}},
		{"name":"Copy","_copy":{"name":"Valid","source":"VAR"}},
		{"name":"Nested","requires":[{"odd/key~":[true,{}]}],"inherits":{"source":"VAR"}},
		{"name":"Missing Source","requires":[{}],"inherits":{}}
	]})json"), VariantPath);
	const auto BadBases = VariantMatchDocument(TEXT(R"({"baseitem":[{},{"edition":"unknown"}]})"), BasePath);
	const auto BadPack = VariantMatchDocument(TEXT(R"({"baseitem":[{},{"packContents":false}]})"), BasePath);
	auto OtherScope = VariantMatchDocument(TEXT(R"({"baseitem":[{}]})"), BasePath);
	OtherScope.Context.DatasetNamespace = TEXT("Item-variant-test");
	struct FCase { int32 Index; const FDnd5eJsonDocument* Bases; EPolicy Policy; const TCHAR* File; const TCHAR* Pointer; };
	for (const FCase& Case : {
		FCase{-1, &BadBases, EPolicy::SourceDefault, VariantPath, TEXT("/magicvariant")},
		{1, &BadBases, EPolicy::SourceDefault, VariantPath, TEXT("/magicvariant/1/_copy")},
		{2, &BadBases, EPolicy::SourceDefault, VariantPath, TEXT("/magicvariant/2/requires/0/odd~1key~0/1")},
		{3, &BadBases, EPolicy::SourceDefault, VariantPath, TEXT("/magicvariant/3/inherits/source")},
		{0, &BadBases, EPolicy::SourceDefault, BasePath, TEXT("/baseitem/1/edition")},
		{0, &BadPack, EPolicy::SourceDefault, BasePath, TEXT("/baseitem/1/packContents")},
		{0, &OtherScope, EPolicy::SourceDefault, BasePath, TEXT("")},
		{0, &BadBases, static_cast<EPolicy>(255), VariantPath, TEXT("")}})
	{
		const auto Result = FDnd5eJsonItemVariantMatcher::MatchAtIndex(Variants, Case.Index, *Case.Bases, Case.Policy);
		TestTrue(FString::Printf(TEXT("Failure at %s %s withholds all staged results"), Case.File, Case.Pointer),
			Result.Status == EDnd5eJsonReadStatus::Failure && Result.VariantName.IsEmpty() &&
			Result.VariantSourceId.IsEmpty() && Result.MatchingBaseIndices.IsEmpty() &&
			Result.Diagnostics.Num() == 1 && Result.Diagnostics[0].RelativeFilePath == Case.File &&
			Result.Diagnostics[0].JsonPointer == Case.Pointer);
	}
	return !HasAnyErrors();
}

IMPLEMENT_COMPLEX_AUTOMATION_TEST(FDnd5eItemVariantSnapshotTest, "Dnd5e.Items.VariantMatch.External", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
void FDnd5eItemVariantSnapshotTest::GetTests(TArray<FString>& Names, TArray<FString>& Commands) const
{
	FString Path;
	if (FParse::Value(FCommandLine::Get(), TEXT("Dnd5eJsonVariantFile="), Path))
	{
		Names.Add(TEXT("InspectedSnapshot"));
		Commands.Add(Path);
	}
}

bool FDnd5eItemVariantSnapshotTest::RunTest(const FString& Parameters)
{
	FString BasePath, BaseJson, VariantJson;
	if (!TestTrue(TEXT("Read explicitly supplied variant and base files"),
		FParse::Value(FCommandLine::Get(), TEXT("Dnd5eJsonDefinitionFile="), BasePath) &&
		FFileHelper::LoadFileToString(BaseJson, *BasePath) && FFileHelper::LoadFileToString(VariantJson, *Parameters))) { return false; }
	TSharedPtr<FJsonObject> Root;
	const TArray<TSharedPtr<FJsonValue>>* Rows = nullptr;
	if (!TestTrue(TEXT("Snapshot has the expected variant array"),
		FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(VariantJson), Root) && Root.IsValid() &&
		Root->TryGetArrayField(TEXT("magicvariant"), Rows) && Rows && Rows->Num() == 230)) { return false; }

	const auto Variants = VariantMatchDocument(VariantJson, TEXT("supplied/magicvariants.json"));
	const auto Bases = VariantMatchDocument(BaseJson, TEXT("supplied/items-base.json"));
	const EPolicy Policies[] = {EPolicy::SourceDefault, EPolicy::PreferClassic};
	int32 Pairs[] = {0, 0};
	int32 TemplatesWithMatches[] = {0, 0};
	int32 Copies = 0;
	for (int32 Index = 0; Index < Rows->Num(); ++Index)
	{
		const auto& Row = (*Rows)[Index];
		if (Row->Type == EJson::Object && Row->AsObject()->HasField(TEXT("_copy")))
		{
			++Copies;
			continue;
		}
		for (int32 Policy = 0; Policy < UE_ARRAY_COUNT(Policies); ++Policy)
		{
			const auto Result = FDnd5eJsonItemVariantMatcher::MatchAtIndex(Variants, Index, Bases, Policies[Policy]);
			if (!TestTrue(FString::Printf(TEXT("Standalone template %d matches under policy %d"), Index, Policy),
				Result.Status == EDnd5eJsonReadStatus::Success && Result.Diagnostics.IsEmpty()))
			{
				for (const auto& Diagnostic : Result.Diagnostics)
				{
					AddError(Diagnostic.RelativeFilePath + TEXT(" ") + Diagnostic.JsonPointer + TEXT(" ") + Diagnostic.Message);
				}
				continue;
			}
			Pairs[Policy] += Result.MatchingBaseIndices.Num();
			TemplatesWithMatches[Policy] += !Result.MatchingBaseIndices.IsEmpty();
		}
	}
	TestEqual(TEXT("Template copies in the snapshot"), Copies, 5);
	TestEqual(TEXT("SourceDefault pair count"), Pairs[0], 3508);
	TestEqual(TEXT("SourceDefault templates with matches"), TemplatesWithMatches[0], 221);
	TestEqual(TEXT("PreferClassic pair count"), Pairs[1], 2873);
	TestEqual(TEXT("PreferClassic templates with matches"), TemplatesWithMatches[1], 151);
	return !HasAnyErrors();
}
#endif
