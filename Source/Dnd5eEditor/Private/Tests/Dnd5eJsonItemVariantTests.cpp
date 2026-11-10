// Developed by Neko Creative Technologies
#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Content/Dnd5eContentLibrary.h"
#include "Items/Dnd5eItemIdentity.h"
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
		{"name":"Copy","source":"VAR","_copy":{"name":"Missing","source":"VAR"}},
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eItemVariantGenerationTest, "Dnd5e.Items.VariantGeneration.PreviewAndIdentity", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FDnd5eItemVariantGenerationTest::RunTest(const FString& Parameters)
{
	const auto Variants = VariantMatchDocument(TEXT(R"json({"magicvariant":[
		{"name":"Invented Pattern","requires":[{"sword":true}],"hasFluffImages":true,"inherits":{
		 "source":"VAR","rarity":"rare","page":23,"srd":false,"nameRemove":" Training","namePrefix":"Glowing ",
		 "nameSuffix":" of Dawn","entries":["Variant text"],"propertyAdd":["P|SYN","Q|SYN"],"lootTables":["VAR-A"]}}
	],"linkedLootTables":{"VAR":{"Glowing Invented Blade of Dawn":["VAR-B"]}}})json"), TEXT("fixtures/variants.json"));
	const auto Bases = VariantMatchDocument(TEXT(R"json({"baseitem":[
		{"name":"Invented Blade Training Training","source":"SYN","type":"M","sword":true,"rarity":"none","weight":3.00,
		 "value":25,"page":7,"srd":true,"srd52":true,"referenceSources":[{"source":"OLD"}],"hasFluff":true,
		 "property":[{"uid":"P|SYN","note":"Original note"}],"entries":["Base text"]},
		{"name":"Invented Blade Training Training","source":"ALT","type":"M","sword":true,"rarity":"none"}
	]})json"), TEXT("fixtures/base-items.json"));
	const auto Result = UDnd5eJsonItemLibrary::GenerateMagicItemVariantAtIndex(Variants, 0, Bases, 0, EPolicy::SourceDefault);
	if (!TestTrue(TEXT("The Blueprint wrapper stages one generated preview"), Result.Status == EDnd5eJsonReadStatus::Success &&
		Result.Diagnostics.IsEmpty() && Result.GeneratedId.SourceKey.StartsWith(TEXT("dnd5e:item-variant:v1:")) &&
		Result.GeneratedId.DatasetNamespace == Bases.Context.DatasetNamespace)) { return false; }

	TSharedPtr<FJsonObject> Root;
	const TArray<TSharedPtr<FJsonValue>>* Rows = nullptr;
	if (!TestTrue(TEXT("Preview has one ordinary item"), FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Result.GeneratedJson), Root) &&
		Root.IsValid() && Root->TryGetArrayField(TEXT("item"), Rows) && Rows && Rows->Num() == 1 &&
		(*Rows)[0]->Type == EJson::Object)) { return false; }
	const auto& Row = (*Rows)[0]->AsObject();
	TestTrue(TEXT("Name operations, base cleanup and inherited metadata"),
		Row->GetStringField(TEXT("name")) == TEXT("Glowing Invented Blade of Dawn") &&
		Row->GetStringField(TEXT("source")) == TEXT("VAR") && Row->GetStringField(TEXT("rarity")) == TEXT("rare") &&
		Row->GetNumberField(TEXT("page")) == 23 && !Row->GetBoolField(TEXT("srd")) &&
		!Row->HasField(TEXT("value")) && !Row->HasField(TEXT("srd52")) && !Row->HasField(TEXT("referenceSources")) &&
		!Row->HasField(TEXT("hasFluff")) && Row->GetBoolField(TEXT("hasFluffImages")) &&
		Row->GetStringField(TEXT("baseItem")) == TEXT("Invented Blade Training Training|SYN"));
	const TSharedPtr<FJsonObject>* Template = nullptr;
	TestTrue(TEXT("The preview contains a source-qualified template link"),
		Row->TryGetObjectField(TEXT("genericVariant"), Template) && Template && Template->IsValid() &&
		(*Template)->GetStringField(TEXT("name")) == TEXT("Invented Pattern") &&
		(*Template)->GetStringField(TEXT("source")) == TEXT("VAR"));
	const TArray<TSharedPtr<FJsonValue>>* Entries = nullptr;
	const TArray<TSharedPtr<FJsonValue>>* Properties = nullptr;
	const TArray<TSharedPtr<FJsonValue>>* Loot = nullptr;
	TestTrue(TEXT("Variant entries prepend and property notes are preserved without duplicating their UID"),
		Row->TryGetArrayField(TEXT("entries"), Entries) && Entries && Entries->Num() == 2 &&
		(*Entries)[0]->AsString() == TEXT("Variant text") && (*Entries)[1]->AsString() == TEXT("Base text") &&
		Row->TryGetArrayField(TEXT("property"), Properties) && Properties && Properties->Num() == 2 &&
		(*Properties)[0]->Type == EJson::Object && (*Properties)[0]->AsObject()->GetStringField(TEXT("note")) == TEXT("Original note") &&
		(*Properties)[1]->AsString() == TEXT("Q|SYN"));
	TestTrue(TEXT("Linked loot follows the template loot list"), Row->TryGetArrayField(TEXT("lootTables"), Loot) &&
		Loot && Loot->Num() == 2 && (*Loot)[0]->AsString() == TEXT("VAR-A") && (*Loot)[1]->AsString() == TEXT("VAR-B"));
	TestTrue(TEXT("Origins point to the original files and rows"), Result.BaseOrigin.SourceId == TEXT("SYN") &&
		Result.BaseOrigin.JsonPointer == TEXT("/baseitem/0") && Result.BaseOrigin.RelativeFilePath == Bases.Context.RelativeFilePath &&
		Result.TemplateOrigin.SourceId == TEXT("VAR") && Result.TemplateOrigin.JsonPointer == TEXT("/magicvariant/0") &&
		Result.TemplateOrigin.RelativeFilePath == Variants.Context.RelativeFilePath);

	const auto OtherBase = FDnd5eJsonItemVariantGenerator::GenerateAtIndex(Variants, 0, Bases, 1, EPolicy::SourceDefault);
	const auto SamePair = FDnd5eJsonItemVariantGenerator::GenerateAtIndex(Variants, 0, Bases, 0, EPolicy::PreferClassic);
	TestTrue(TEXT("Pair ID differs by base publication and is stable across matching policies"),
		OtherBase.Status == EDnd5eJsonReadStatus::Success && OtherBase.GeneratedId != Result.GeneratedId &&
		OtherBase.GeneratedJson.Contains(TEXT("Glowing Invented Blade of Dawn")) &&
		SamePair.Status == EDnd5eJsonReadStatus::Success && SamePair.GeneratedId == Result.GeneratedId);
	const auto Typed = FDnd5eJsonItemReader::ReadAtIndex(VariantMatchDocument(Result.GeneratedJson, TEXT("fixtures/preview.json")), 0);
	TestTrue(TEXT("The ordinary reader can inspect the generated preview"),
		Typed.Status == EDnd5eJsonReadStatus::Success && Typed.Item.bHasBaseItemId && Typed.Item.bHasWeight &&
		Typed.Item.Weight.Amount.SourceLiteral == TEXT("3.00") && Typed.Item.Description.Num() == 2);
	const auto* Node = UDnd5eJsonItemLibrary::StaticClass()->FindFunctionByName(TEXT("GenerateMagicItemVariantAtIndex"));
	TestTrue(TEXT("Generation is available as a Blueprint node"), Node && Node->HasAllFunctionFlags(FUNC_Static | FUNC_BlueprintCallable));
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eItemVariantGenerationFailureTest, "Dnd5e.Items.VariantGeneration.AtomicFailures", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FDnd5eItemVariantGenerationFailureTest::RunTest(const FString& Parameters)
{
	const TCHAR* VariantPath = TEXT("fixtures/variants.json");
	const TCHAR* BasePath = TEXT("fixtures/base-items.json");
	const auto Variants = VariantMatchDocument(TEXT(R"json({"magicvariant":[
		{"name":"Literal","requires":[{"sword":true}],"inherits":{"source":"VAR","namePrefix":"Magic "}},
		{"name":"Token","requires":[{}],"inherits":{"source":"VAR","entries":[{"type":"entries","entries":["{=unknown}"]}]}},
		{"name":"Expression","requires":[{}],"inherits":{"source":"VAR","valueExpression":"[[baseItem.value]] + 1d4"}},
		{"name":"Template Loot","requires":[{}],"inherits":{"source":"VAR","namePrefix":"Magic ","lootTables":"Invalid"}},
		{"name":"Copy","source":"VAR","_copy":{"name":"Missing","source":"VAR"}}
	],"linkedLootTables":{"VAR":{"Magic Blade":["SYN-A"]}}})json"), VariantPath);
	const auto Bases = VariantMatchDocument(TEXT(R"json({"baseitem":[
		{"name":"Blade","source":"SYN","sword":true,"lootTables":"Invalid"},
		{"name":"Fork","source":"SYN"},
		{"name":"Bad|Link","source":"SYN","sword":true}
	]})json"), BasePath);
	struct FCase { int32 Variant; int32 Base; const TCHAR* File; const TCHAR* Pointer; };
	for (const FCase& Case : {
		FCase{0, -1, BasePath, TEXT("/baseitem")}, {0, 1, BasePath, TEXT("/baseitem/1")},
		{0, 2, BasePath, TEXT("/baseitem/2")}, {0, 0, BasePath, TEXT("/baseitem/0/lootTables")},
		{1, 0, VariantPath, TEXT("/magicvariant/1/inherits/entries/0/entries/0")},
		{2, 0, VariantPath, TEXT("/magicvariant/2/inherits/valueExpression")},
		{3, 0, VariantPath, TEXT("/magicvariant/3/inherits/lootTables")},
		{4, 0, VariantPath, TEXT("/magicvariant/4/_copy")}})
	{
		const auto Result = FDnd5eJsonItemVariantGenerator::GenerateAtIndex(Variants, Case.Variant, Bases, Case.Base, EPolicy::SourceDefault);
		TestTrue(FString::Printf(TEXT("Failure at %s %s withholds staged output"), Case.File, Case.Pointer),
			Result.Status == EDnd5eJsonReadStatus::Failure && Result.GeneratedJson.IsEmpty() &&
			Result.GeneratedId == FDnd5eRecordId{} && Result.BaseOrigin.JsonPointer.IsEmpty() &&
			Result.TemplateOrigin.JsonPointer.IsEmpty() && Result.TemplateSourceRowIndices.IsEmpty() && Result.Diagnostics.Num() == 1 &&
			Result.Diagnostics[0].RelativeFilePath == Case.File && Result.Diagnostics[0].JsonPointer == Case.Pointer);
	}
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eVariantTemplateCopyTest, "Dnd5e.Items.VariantGeneration.TemplateCopyChain", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FDnd5eVariantTemplateCopyTest::RunTest(const FString& Parameters)
{
	const auto Variants = VariantMatchDocument(TEXT(R"json({"magicvariant":[
		{"name":"Root Pattern","hasFluff":true,"hasFluffImages":true,"requires":[{"sword":true}],
		 "inherits":{"source":"ROOT","rarity":"rare","namePrefix":"Root ","entries":["Root"],
		             "lootTables":["REMOVED"],"propertyAdd":["P|SYN"],"resist":["fire"]}},
		{"name":"Middle Pattern","source":"MID","_copy":{"name":"root pattern","source":"root",
		 "_preserve":{"*":true},"_mod":{
		 "inherits.source":{"mode":"setProp","value":"GEN"},
		 "inherits.namePrefix":{"mode":"setProp","value":"Bright "},
		 "inherits.page":{"mode":"setProp","value":15},
		 "inherits.entries":{"mode":"appendArr","items":{"type":"entries","entries":["Middle"]}},
		 "inherits.lootTables":"remove"}}},
		{"name":"Final Pattern","source":"SEL","hasFluffImages":null,"_copy":{"name":"Middle Pattern","source":"MID",
		 "_preserve":{"hasFluff":true,"hasFluffImages":true},"_mod":{
		 "inherits.nameSuffix":{"mode":"setProp","value":" of Testing"},
		 "inherits.entries":{"mode":"appendArr","items":["Final"]},
		 "inherits.propertyAdd":{"mode":"appendIfNotExistsArr","items":["P|SYN","Q|SYN"]},
		 "inherits.resist":[{"mode":"appendIfNotExistsArr","items":"fire"},{"mode":"appendIfNotExistsArr","items":"cold"}]}}},
		{"name":"Other Pattern","source":"ALT","_copy":{"name":"Root Pattern","source":"ROOT"}}
	],"linkedLootTables":{"GEN":{"Bright Invented Blade of Testing":["LINKED"]}}})json"), TEXT("fixtures/variants.json"));
	const auto Bases = VariantMatchDocument(TEXT(R"({"baseitem":[
		{"name":"Invented Blade","source":"SYN","sword":true,"entries":["Base"]},
		{"name":"Invented Coat","source":"SYN"}
	]})"), TEXT("fixtures/base-items.json"));

	const auto Match = UDnd5eJsonItemLibrary::MatchMagicItemVariantAtIndex(Variants, 2, Bases, EPolicy::SourceDefault);
	const TArray<int32> ExpectedBases{0};
	TestTrue(TEXT("Copied requirements filter bases and match reports the effective source"),
		Match.Status == EDnd5eJsonReadStatus::Success && Match.VariantName == TEXT("Final Pattern") &&
		Match.VariantSourceId == TEXT("GEN") && Match.MatchingBaseIndices == ExpectedBases && Match.Diagnostics.IsEmpty());
	const auto Result = UDnd5eJsonItemLibrary::GenerateMagicItemVariantAtIndex(Variants, 2, Bases, 0, EPolicy::SourceDefault);
	const TArray<int32> ExpectedChain{2, 1, 0};
	if (!TestTrue(TEXT("The copied preview has original provenance and ancestry"),
		Result.Status == EDnd5eJsonReadStatus::Success && Result.Diagnostics.IsEmpty() && Result.TemplateSourceRowIndices == ExpectedChain &&
		Result.TemplateOrigin.SourceId == TEXT("SEL") && Result.TemplateOrigin.JsonPointer == TEXT("/magicvariant/2") &&
		Result.TemplateOrigin.RelativeFilePath == Variants.Context.RelativeFilePath && Result.BaseOrigin.JsonPointer == TEXT("/baseitem/0")))
	{
		return false;
	}
	const auto ReadRow = [](const FString& Json) -> TSharedPtr<FJsonObject>
	{
		TSharedPtr<FJsonObject> Root;
		const TArray<TSharedPtr<FJsonValue>>* Rows = nullptr;
		if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json), Root) || !Root.IsValid() ||
			!Root->TryGetArrayField(TEXT("item"), Rows) || !Rows || Rows->Num() != 1) { return nullptr; }
		return (*Rows)[0]->AsObject();
	};
	const auto Row = ReadRow(Result.GeneratedJson);
	if (!TestTrue(TEXT("Preview has one item row"), Row.IsValid())) { return false; }
	const TSharedPtr<FJsonObject>* Link = nullptr;
	TestTrue(TEXT("Effective publication, template link and flags follow separate rules"),
		Row->GetStringField(TEXT("name")) == TEXT("Bright Invented Blade of Testing") &&
		Row->GetStringField(TEXT("source")) == TEXT("GEN") && Row->GetNumberField(TEXT("page")) == 15 &&
		Row->GetBoolField(TEXT("hasFluff")) && !Row->HasField(TEXT("hasFluffImages")) && !Row->HasField(TEXT("_copy")) &&
		Row->TryGetObjectField(TEXT("genericVariant"), Link) && Link && Link->IsValid() &&
		(*Link)->GetStringField(TEXT("name")) == TEXT("Final Pattern") &&
		(*Link)->HasTypedField<EJson::String>(TEXT("source")) && (*Link)->GetStringField(TEXT("source")) == TEXT("SEL"));
	const auto& Entries = Row->GetArrayField(TEXT("entries"));
	const auto& Properties = Row->GetArrayField(TEXT("property"));
	const auto& Resist = Row->GetArrayField(TEXT("resist"));
	const auto& Loot = Row->GetArrayField(TEXT("lootTables"));
	TestTrue(TEXT("Copies append entries in order and preserve exact array and loot rules"),
		Entries.Num() == 4 && Entries[0]->AsString() == TEXT("Root") && Entries[1]->Type == EJson::Object &&
		Entries[1]->AsObject()->GetArrayField(TEXT("entries"))[0]->AsString() == TEXT("Middle") &&
		Entries[2]->AsString() == TEXT("Final") && Entries[3]->AsString() == TEXT("Base") &&
		Properties.Num() == 2 && Properties[0]->AsString() == TEXT("P|SYN") && Properties[1]->AsString() == TEXT("Q|SYN") &&
		Resist.Num() == 2 && Resist[0]->AsString() == TEXT("fire") && Resist[1]->AsString() == TEXT("cold") &&
		Loot.Num() == 1 && Loot[0]->AsString() == TEXT("LINKED"));

	auto ChangedPublication = Variants;
	ChangedPublication.JsonText = Variants.JsonText.Replace(TEXT("\"GEN\""), TEXT("\"OTHER\""), ESearchCase::CaseSensitive);
	const auto SamePair = FDnd5eJsonItemVariantGenerator::GenerateAtIndex(ChangedPublication, 2, Bases, 0, EPolicy::SourceDefault);
	TestTrue(TEXT("Changing the inherited publication keeps the generated pair identity"),
		SamePair.Status == EDnd5eJsonReadStatus::Success && SamePair.GeneratedId == Result.GeneratedId);
	const auto Other = FDnd5eJsonItemVariantGenerator::GenerateAtIndex(Variants, 3, Bases, 0, EPolicy::SourceDefault);
	const auto OtherRow = ReadRow(Other.GeneratedJson);
	const TArray<int32> OtherChain{3, 0};
	TestTrue(TEXT("A sibling gets the original template data without protected flags"),
		Other.Status == EDnd5eJsonReadStatus::Success && Other.TemplateSourceRowIndices == OtherChain &&
		Other.GeneratedId != Result.GeneratedId && OtherRow.IsValid() &&
		OtherRow->GetStringField(TEXT("name")) == TEXT("Root Invented Blade") && !OtherRow->HasField(TEXT("hasFluff")) &&
		OtherRow->GetArrayField(TEXT("lootTables")).Num() == 1 &&
		OtherRow->GetArrayField(TEXT("lootTables"))[0]->AsString() == TEXT("REMOVED") &&
		OtherRow->GetArrayField(TEXT("entries")).Num() == 2);
	const auto* ChainProperty = FDnd5eJsonItemVariantGenerationResult::StaticStruct()->FindPropertyByName(TEXT("TemplateSourceRowIndices"));
	TestTrue(TEXT("Blueprint can read the original template indices"),
		ChainProperty && ChainProperty->HasAllPropertyFlags(CPF_BlueprintVisible | CPF_BlueprintReadOnly));
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eVariantTemplateCopyFailureTest, "Dnd5e.Items.VariantGeneration.TemplateCopyFailures", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FDnd5eVariantTemplateCopyFailureTest::RunTest(const FString& Parameters)
{
	const auto Bases = VariantMatchDocument(TEXT(R"({"baseitem":[{"name":"Invented Base","source":"BAS"}]})"),
		TEXT("fixtures/base-items.json"));
	const auto Check = [&](const FString& Json, int32 Index, const TCHAR* Code, const TCHAR* Pointer)
	{
		const auto Variants = VariantMatchDocument(Json, TEXT("fixtures/variants.json"));
		const auto Result = FDnd5eJsonItemVariantGenerator::GenerateAtIndex(Variants, Index, Bases, 0, EPolicy::SourceDefault);
		TestTrue(FString::Printf(TEXT("Template failure is atomic and located at %s"), Pointer),
			Result.Status == EDnd5eJsonReadStatus::Failure && Result.GeneratedJson.IsEmpty() &&
			Result.GeneratedId == FDnd5eRecordId{} && Result.TemplateSourceRowIndices.IsEmpty() &&
			Result.BaseOrigin.JsonPointer.IsEmpty() && Result.TemplateOrigin.JsonPointer.IsEmpty() &&
			Result.Diagnostics.Num() == 1 && Result.Diagnostics[0].Code == FName(Code) &&
			Result.Diagnostics[0].RelativeFilePath == Variants.Context.RelativeFilePath && Result.Diagnostics[0].JsonPointer == Pointer);
	};
	Check(TEXT(R"({"magicvariant":[{"name":"A","source":"SYN","_copy":{"name":"B","source":"SYN"}},
		{"name":"B","source":"SYN","_copy":{"name":"A","source":"SYN"}}]})"), 0,
		TEXT("Dnd5e.Json.InvalidFieldValue"), TEXT("/magicvariant/1/_copy"));
	Check(TEXT(R"({"magicvariant":[{"name":"A","source":"SYN","_copy":{"name":"B","source":"SYN"}},
		{"name":"B","source":"SYN"},{"name":"b","source":"syn"}]})"), 0,
		TEXT("Dnd5e.Json.InvalidFieldValue"), TEXT("/magicvariant/0/_copy"));
	Check(TEXT(R"({"magicvariant":[{"name":"A","source":"SYN","_copy":{"name":"B","source":"SYN"}},
		{"name":"B","source":"SYN","_copy":{"name":"Missing","source":"SYN"}}]})"), 0,
		TEXT("Dnd5e.Json.UnresolvedReference"), TEXT("/magicvariant/1/_copy"));
	Check(TEXT(R"({"magicvariant":[{"name":"A","source":"SYN","_copy":{"name":"B","source":"SYN","_mod":{
		"inherits.entries.0":{"mode":"setProp","value":"Unsupported"}}}},{"name":"B","requires":[{}],"inherits":{"source":"SYN"}}]})"),
		0, TEXT("Dnd5e.Json.UnsupportedRequiredValue"), TEXT("/magicvariant/0/_copy/_mod/inherits.entries.0"));
	Check(TEXT(R"({"magicvariant":[{"name":"A","source":"SYN","_copy":{"name":"B","source":"SYN","_mod":{
		"inherits.page":{"mode":"setProp","value":1.5}}}},{"name":"B","requires":[{}],"inherits":{"source":"SYN"}}]})"),
		0, TEXT("Dnd5e.Json.InvalidFieldValue"), TEXT("/magicvariant/0/_copy/_mod/inherits.page/value"));

	FString Deep = TEXT("{\"magicvariant\":[");
	for (int32 Index = 0; Index < 33; ++Index)
	{
		Deep += FString::Printf(TEXT("{\"name\":\"Template %d\",\"source\":\"SYN\",\"_copy\":{\"name\":\"Template %d\",\"source\":\"SYN\"}},"),
			Index, Index + 1);
	}
	Deep += TEXT("{\"name\":\"Template 33\",\"source\":\"SYN\",\"requires\":[{}],\"inherits\":{\"source\":\"SYN\"}}]}");
	Check(Deep, 0, TEXT("Dnd5e.Json.ResourceLimitExceeded"), TEXT("/magicvariant/32/_copy"));
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eTypedItemVariantTest, "Dnd5e.Items.VariantGeneration.TypedPairAndOrigins", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FDnd5eTypedItemVariantTest::RunTest(const FString& Parameters)
{
	const auto Variants = VariantMatchDocument(TEXT(R"json({"magicvariant":[
		{"name":"Invented Root","requires":[{"type":"M|BAS"}],
		 "inherits":{"source":"BAS","rarity":"rare","page":15,"srd":true,"entries":["Root entry."]}},
		{"name":"Invented Copy","source":"SEL","_copy":{"name":"Invented Root","source":"BAS",
		 "_mod":{"inherits.entries":{"mode":"appendArr","items":"Copy entry."}}}}
	]})json"), TEXT("fixtures/variants.json"));
	const auto Bases = VariantMatchDocument(TEXT(R"({"baseitem":[
		{"name":"Invented Blade","source":"BAS","type":"M|BAS","rarity":"none","weight":3.00,"entries":["Base entry."]}
	]})"), TEXT("fixtures/base-items.json"));
	const auto Preview = FDnd5eJsonItemVariantGenerator::GenerateAtIndex(Variants, 1, Bases, 0, EPolicy::SourceDefault);
	const auto Typed = UDnd5eJsonItemLibrary::ReadMagicItemVariantAtIndex(Variants, 1, Bases, 0, EPolicy::SourceDefault);
	if (!TestTrue(TEXT("Typed generation and preview succeed for a copy with the same name and publication as its base"),
		Preview.Status == EDnd5eJsonReadStatus::Success && Typed.Status == EDnd5eJsonReadStatus::Success)) { return false; }

	const auto& Item = Typed.Item;
	FDnd5eRecordId BaseId;
	TestTrue(TEXT("The typed identity matches the preview, and the item links to the distinct original base"),
		Dnd5e::Items::TryMakeId(Bases.Context.DatasetNamespace, TEXT("Invented Blade"), TEXT("BAS"), BaseId) &&
		Item.Header.RecordId == Preview.GeneratedId && Item.Header.RecordId != BaseId && Item.bHasBaseItemId &&
		Item.BaseItemId == BaseId && Item.Header.DisplayName == TEXT("Invented Blade"));
	const TArray<int32> Chain{1, 0};
	TestTrue(TEXT("The typed result preserves both original source locations and ancestry"),
		Typed.TemplateSourceRowIndices == Chain && Typed.TemplateSourceRowIndices == Preview.TemplateSourceRowIndices &&
		Typed.TemplateOrigin.SourceId == TEXT("SEL") && Typed.TemplateOrigin.RelativeFilePath == Variants.Context.RelativeFilePath &&
		Typed.TemplateOrigin.JsonPointer == TEXT("/magicvariant/1") && Typed.BaseOrigin.SourceId == TEXT("BAS") &&
		Typed.BaseOrigin.RelativeFilePath == Bases.Context.RelativeFilePath && Typed.BaseOrigin.JsonPointer == TEXT("/baseitem/0"));
	const auto& Provenance = Item.Header.Provenance;
	TestTrue(TEXT("Generated metadata is followed by the two original record locators"),
		Provenance.Num() == 3 && Provenance[0].SourceId == TEXT("BAS") && Provenance[0].Page.Number == 15 &&
		Provenance[0].Memberships.Num() == 1 && Provenance[0].RelativeFilePath.IsEmpty() && Provenance[0].JsonPointer.IsEmpty() &&
		Provenance[0].AdditionalMetadataJson.Contains(TEXT("genericVariant")) &&
		Provenance[1].SourceId == TEXT("SEL") && Provenance[1].RelativeFilePath == Variants.Context.RelativeFilePath &&
		Provenance[1].JsonPointer == TEXT("/magicvariant/1") &&
		Provenance[2].SourceId == TEXT("BAS") && Provenance[2].RelativeFilePath == Bases.Context.RelativeFilePath &&
		Provenance[2].JsonPointer == TEXT("/baseitem/0"));
	TestTrue(TEXT("An unchanged numeric source literal survives direct conversion"),
		Item.bHasWeight && Item.Weight.Amount.SourceLiteral == TEXT("3.00"));
	if (!TestEqual(TEXT("Entries retain their reading order"), Item.Description.Num(), 3)) { return false; }
	const TCHAR* ExpectedText[] = {TEXT("Root entry."), TEXT("Copy entry."), TEXT("Base entry.")};
	for (int32 Index = 0; Index < Item.Description.Num(); ++Index)
	{
		const auto& Block = Item.Description[Index];
		FDnd5eParagraphContent Paragraph;
		FDnd5eTextContent Text;
		TestTrue(FString::Printf(TEXT("Description block %d retains its text"), Index),
			UDnd5eContentLibrary::TryGetParagraph(Block, Paragraph) && Paragraph.Inlines.Num() == 1 &&
			UDnd5eContentLibrary::TryGetInlineText(Paragraph.Inlines[0], Text) && Text.Text == ExpectedText[Index]);
		const bool bBase = Index == 2;
		TestTrue(FString::Printf(TEXT("Description block %d carries the correct source document and anchor"), Index),
			Block.Metadata.SourceId == (bBase ? TEXT("BAS") : TEXT("SEL")) &&
			Block.Metadata.RelativeFilePath == (bBase ? Bases.Context.RelativeFilePath : Variants.Context.RelativeFilePath) &&
			Block.Metadata.JsonPointer == (bBase ? TEXT("/baseitem/0/entries/0") : TEXT("/magicvariant/1")));
	}
	const auto* ResultType = FDnd5eJsonItemVariantReadResult::StaticStruct();
	const auto* Node = UDnd5eJsonItemLibrary::StaticClass()->FindFunctionByName(TEXT("ReadMagicItemVariantAtIndex"));
	TestTrue(TEXT("Typed variant import is callable from Blueprint"), ResultType->GetBoolMetaData(TEXT("BlueprintType")) &&
		Node && Node->HasAllFunctionFlags(FUNC_Static | FUNC_BlueprintCallable));
	for (const TCHAR* Name : {TEXT("Status"), TEXT("Item"), TEXT("BaseOrigin"), TEXT("TemplateOrigin"),
		TEXT("TemplateSourceRowIndices"), TEXT("Diagnostics")})
	{
		const auto* Field = ResultType->FindPropertyByName(Name);
		TestTrue(Name, Field && Field->HasAllPropertyFlags(CPF_BlueprintVisible | CPF_BlueprintReadOnly));
	}
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eTypedItemVariantFailureTest, "Dnd5e.Items.VariantGeneration.TypedAtomicFailure", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FDnd5eTypedItemVariantFailureTest::RunTest(const FString& Parameters)
{
	const auto Variants = VariantMatchDocument(TEXT(R"({"magicvariant":[
		{"name":"Root","requires":[{}],"inherits":{"source":"VAR","rarity":"rare","entries":["Template entry."]}},
		{"name":"Child","source":"SEL","_copy":{"name":"Root","source":"VAR"}}
	]})"), TEXT("fixtures/variants.json"));
	const auto Bases = VariantMatchDocument(TEXT(R"({"baseitem":[
		{"name":"Base","source":"BAS","weight":3.00,"entries":[{"type":"entries","name":"Base heading","entries":[false]}]}
	]})"), TEXT("fixtures/base-items.json"));
	auto BadBaseWeight = Bases;
	BadBaseWeight.JsonText = TEXT(R"({"baseitem":[{"name":"Base","source":"BAS","weight":"invalid"}]})");
	auto BadAncestorWeight = Variants;
	BadAncestorWeight.JsonText = Variants.JsonText.Replace(TEXT("\"rarity\":\"rare\""), TEXT("\"rarity\":\"rare\",\"weight\":\"invalid\""));
	auto BadTemplateContent = Variants;
	BadTemplateContent.JsonText = Variants.JsonText.Replace(TEXT("\"Template entry.\""), TEXT("false"));
	const auto ValidBases = VariantMatchDocument(TEXT(R"({"baseitem":[{"name":"Base","source":"BAS","weight":3.00}]})"),
		TEXT("fixtures/base-items.json"));
	const auto Check = [&](const FDnd5eJsonDocument& Templates, const FDnd5eJsonDocument& BaseItems, int32 BaseIndex,
		const FString& File, const TCHAR* Pointer)
	{
		const auto Result = FDnd5eJsonItemVariantGenerator::ReadAtIndex(Templates, 1, BaseItems, BaseIndex, EPolicy::SourceDefault);
		TestTrue(FString::Printf(TEXT("Typed failure withholds all values and reports %s %s"), *File, Pointer),
			Result.Status == EDnd5eJsonReadStatus::Failure && Result.Item.Header.RecordId == FDnd5eRecordId{} &&
			Result.Item.Header.Provenance.IsEmpty() && Result.Item.Description.IsEmpty() && !Result.Item.bHasBaseItemId &&
			Result.TemplateSourceRowIndices.IsEmpty() && Result.TemplateOrigin.JsonPointer.IsEmpty() &&
			Result.BaseOrigin.JsonPointer.IsEmpty() && Result.Diagnostics.Num() == 1 &&
			Result.Diagnostics[0].Severity == EDnd5eDiagnosticSeverity::Error &&
			Result.Diagnostics[0].RelativeFilePath == File && Result.Diagnostics[0].JsonPointer == Pointer);
	};
	Check(Variants, BadBaseWeight, 0, Bases.Context.RelativeFilePath, TEXT("/baseitem/0/weight"));
	Check(BadAncestorWeight, ValidBases, 0, Variants.Context.RelativeFilePath, TEXT("/magicvariant/0/inherits/weight"));
	Check(Variants, Bases, 0, Bases.Context.RelativeFilePath, TEXT("/baseitem/0/entries/0/entries/0"));
	Check(BadTemplateContent, ValidBases, 0, Variants.Context.RelativeFilePath, TEXT("/magicvariant/1"));
	Check(Variants, ValidBases, 1, Bases.Context.RelativeFilePath, TEXT("/baseitem"));
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eVariantEntrySubstitutionTest, "Dnd5e.Items.VariantGeneration.EntrySubstitution", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FDnd5eVariantEntrySubstitutionTest::RunTest(const FString& Parameters)
{
	const auto Templates = VariantMatchDocument(TEXT(R"json({"magicvariant":[
		{"name":"Invented Pattern","requires":[{}],"inherits":{
		 "source":"VAR","rarity":"rare","namePrefix":"Glowing ","bonusWeapon":"+2","bonusAc":1.00,
		 "bonusWeaponAttack":"+3","bonusWeaponDamage":"+4","bonusSavingThrow":"+5",
		 "entries":[{"type":"entries","name":"{=baseName}","page":3,"entries":[
		  "{=baseName/at} {=baseName/l} deals {@damage 1d4} {=dmgType} damage.",
		  "{@b {=bonusWeapon}}{=bonusAc}; {=bonusWeaponAttack} {=bonusWeaponDamage} {=bonusSavingThrow}",
		  "{=baseName/a}"]}]}}
	]})json"), TEXT("fixtures/variants.json"));
	const auto Bases = VariantMatchDocument(TEXT(R"json({"baseitem":[
		{"name":"Invented Axe","source":"SYN","type":"M","rarity":"none","dmg1":"1d6","dmgType":"S","entries":["Original base text"]},
		{"name":"Training Axe","source":"ALT","type":"M","rarity":"none","dmg1":"1d6","dmgType":"B"}
	]})json"), TEXT("fixtures/base-items.json"));

	for (int32 Index = 0; Index < 2; ++Index)
	{
		const auto Result = FDnd5eJsonItemVariantGenerator::GenerateAtIndex(Templates, 0, Bases, Index, EPolicy::SourceDefault);
		TSharedPtr<FJsonObject> Root;
		const TArray<TSharedPtr<FJsonValue>>* Rows = nullptr;
		if (!TestTrue(TEXT("Generation produces one preview row after substitution"),
			Result.Status == EDnd5eJsonReadStatus::Success && Result.Diagnostics.IsEmpty() &&
			FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Result.GeneratedJson), Root) && Root.IsValid() &&
			Root->TryGetArrayField(TEXT("item"), Rows) && Rows && Rows->Num() == 1 && (*Rows)[0]->Type == EJson::Object))
		{
			for (const auto& Diagnostic : Result.Diagnostics) { AddError(Diagnostic.JsonPointer + TEXT(" ") + Diagnostic.Message); }
			return false;
		}

		const auto& Row = (*Rows)[0]->AsObject();
		const TArray<TSharedPtr<FJsonValue>>* Blocks = nullptr;
		const TArray<TSharedPtr<FJsonValue>>* Entries = nullptr;
		if (!TestTrue(TEXT("Section shape and source order remain intact"), Row->TryGetArrayField(TEXT("entries"), Blocks) &&
			Blocks && Blocks->Num() == (Index == 0 ? 2 : 1) && (*Blocks)[0]->Type == EJson::Object &&
			(*Blocks)[0]->AsObject()->TryGetArrayField(TEXT("entries"), Entries) && Entries && Entries->Num() == 3)) { return false; }
		const auto& Section = (*Blocks)[0]->AsObject();
		const TCHAR* BaseName = Index == 0 ? TEXT("Invented Axe") : TEXT("Training Axe");
		TestTrue(TEXT("Heading uses the original name and the page stays numeric"),
			Section->GetStringField(TEXT("name")).Equals(BaseName, ESearchCase::CaseSensitive) &&
			Section->GetNumberField(TEXT("page")) == 3 && Row->GetStringField(TEXT("name")) == FString(TEXT("Glowing ")) + BaseName);
		const TCHAR* Line = Index == 0 ? TEXT("An invented axe deals {@damage 1d4} Slashing damage.") :
			TEXT("A training axe deals {@damage 1d4} Bludgeoning damage.");
		TestTrue(TEXT("Article, lowercase name and damage display name match exact spelling"),
			(*Entries)[0]->AsString().Equals(Line, ESearchCase::CaseSensitive) &&
			(*Entries)[2]->AsString().Equals(Index == 0 ? TEXT("an") : TEXT("a"), ESearchCase::CaseSensitive));
		TestEqual(TEXT("Adjacent tokens and tokens inside markup preserve the original markup and number literal"),
			(*Entries)[1]->AsString(), FString(TEXT("{@b +2}1.00; +3 +4 +5")));
		if (Index == 0) { TestEqual(TEXT("Base entries stay unchanged"), (*Blocks)[1]->AsString(), FString(TEXT("Original base text"))); }
	}

	const auto Default = FDnd5eJsonItemVariantGenerator::GenerateAtIndex(Templates, 0, Bases, 0, EPolicy::SourceDefault);
	const auto Classic = FDnd5eJsonItemVariantGenerator::GenerateAtIndex(Templates, 0, Bases, 0, EPolicy::PreferClassic);
	TestTrue(TEXT("Matching policy changes neither the generated identity nor damage spelling for the same eligible pair"),
		Classic.Status == EDnd5eJsonReadStatus::Success && Classic.GeneratedId == Default.GeneratedId &&
		Classic.GeneratedJson == Default.GeneratedJson);
	const auto Typed = FDnd5eJsonItemReader::ReadAtIndex(VariantMatchDocument(Default.GeneratedJson, TEXT("fixtures/preview.json")), 0);
	TestTrue(TEXT("The ordinary reader can consume the substituted preview"), Typed.Status == EDnd5eJsonReadStatus::Success &&
		Typed.Item.Description.Num() == 2 && Typed.Item.Description[0].Kind.IsValid());
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eVariantEntrySubstitutionFailureTest, "Dnd5e.Items.VariantGeneration.EntrySubstitutionFailures", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FDnd5eVariantEntrySubstitutionFailureTest::RunTest(const FString& Parameters)
{
	const auto Bases = VariantMatchDocument(TEXT(R"({"baseitem":[{"name":"Invented Blade","source":"SYN","type":"M"}]})"),
		TEXT("fixtures/base-items.json"));
	struct FCase { FString Text; const TCHAR* Code; };
	FString TooMany;
	for (int32 Index = 0; Index < 257; ++Index) { TooMany += TEXT("{=baseName}"); }
	const FCase Cases[] = {
		{TEXT("{=unknown}"), TEXT("Dnd5e.Json.UnsupportedRequiredValue")},
		{TEXT("{=baseName/at/a}"), TEXT("Dnd5e.Json.UnsupportedRequiredValue")},
		{TEXT("{=baseName"), TEXT("Dnd5e.Json.InvalidFieldValue")},
		{TEXT("{=bonusAc}"), TEXT("Dnd5e.Json.InvalidFieldValue")},
		{TEXT("{=dmgType}"), TEXT("Dnd5e.Json.InvalidFieldValue")},
		{MoveTemp(TooMany), TEXT("Dnd5e.Json.ResourceLimitExceeded")}
	};
	for (const auto& Case : Cases)
	{
		const FString Json = FString::Printf(TEXT(R"json({"magicvariant":[
			{"name":"Invented Pattern","requires":[{}],"inherits":{"source":"VAR","entries":[
			 "{=baseName}",{"type":"entries","entries":["%s"]}]}}
		]})json"), *Case.Text);
		const auto Templates = VariantMatchDocument(Json, TEXT("fixtures/variants.json"));
		const auto Result = FDnd5eJsonItemVariantGenerator::GenerateAtIndex(Templates, 0, Bases, 0, EPolicy::SourceDefault);
		TestTrue(FString::Printf(TEXT("Substitution failure is atomic and located for %s"), Case.Code),
			Result.Status == EDnd5eJsonReadStatus::Failure && Result.GeneratedJson.IsEmpty() && Result.GeneratedId == FDnd5eRecordId{} &&
			Result.BaseOrigin.JsonPointer.IsEmpty() && Result.TemplateOrigin.JsonPointer.IsEmpty() && Result.Diagnostics.Num() == 1 &&
			Result.Diagnostics[0].RelativeFilePath == Templates.Context.RelativeFilePath &&
			Result.Diagnostics[0].JsonPointer == TEXT("/magicvariant/0/inherits/entries/1/entries/0") &&
			Result.Diagnostics[0].Code == FName(Case.Code));
	}
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eVariantStandaloneTransformsTest, "Dnd5e.Items.VariantGeneration.StandaloneTransforms", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FDnd5eVariantStandaloneTransformsTest::RunTest(const FString& Parameters)
{
	const auto Templates = VariantMatchDocument(TEXT(R"json({"magicvariant":[
		{"name":"Invented Transform","requires":[{"type":"HA|SYN"}],"inherits":{
		 "source":"VAR","rarity":"rare","namePrefix":"Altered ","valueExpression":"[[baseItem.value]] + 1000",
		 "weightExpression":"[[baseItem.weight]] * 2","barding":true,
		 "propertyRemove":["P|SYN"],"propertyAdd":["Q|SYN","P|SYN","R|SYN"],
		 "vulnerable":null,"resist":["fire","lightning","fire"],"immune":["poison"],"conditionImmune":["poisoned","stunned"]}},
		{"name":"Invented Removal","requires":[{}],"inherits":{"source":"VAR","rarity":"rare","propertyRemove":["P|SYN","Q|SYN"]}},
		{"name":"Invented Multiplication","requires":[{}],"inherits":{
		 "source":"VAR","rarity":"rare","valueExpression":"[[baseItem.value]] * 4","weightExpression":"[[baseItem.weight]] + 1"}}
	]})json"), TEXT("fixtures/variants.json"));
	const auto Bases = VariantMatchDocument(TEXT(R"json({"baseitem":[
		{"name":"Invented Base","source":"SYN","type":"HA|SYN","rarity":"none","value":100,"weight":2.25,
		 "property":[{"uid":"P|SYN","note":"Original note"},"Q|SYN"],
		 "vulnerable":["cold","fire"],"resist":["acid","poison"],"immune":["lightning","poison"],
		 "conditionImmune":["prone","poisoned"]},
		{"name":"Invented Unpriced Base","source":"SYN","type":"HA|SYN","rarity":"none","weight":1},
		{"name":"Invented Null Base","source":"SYN","type":"HA|SYN","rarity":"none","value":null,"weight":null},
		{"name":"Invented Zero Base","source":"SYN","type":"HA|SYN","rarity":"none","value":0,"weight":0}
	]})json"), TEXT("fixtures/base-items.json"));

	const auto ReadPreview = [&](const FDnd5eJsonItemVariantGenerationResult& Result, TSharedPtr<FJsonObject>& Row)
	{
		TSharedPtr<FJsonObject> Root;
		const TArray<TSharedPtr<FJsonValue>>* Rows = nullptr;
		if (!TestTrue(TEXT("Generated preview has one item row"), Result.Status == EDnd5eJsonReadStatus::Success &&
			FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Result.GeneratedJson), Root) && Root.IsValid() &&
			Root->TryGetArrayField(TEXT("item"), Rows) && Rows && Rows->Num() == 1 && (*Rows)[0]->Type == EJson::Object))
		{
			for (const auto& Diagnostic : Result.Diagnostics) { AddError(Diagnostic.JsonPointer + TEXT(" ") + Diagnostic.Message); }
			return false;
		}
		Row = (*Rows)[0]->AsObject();
		return true;
	};
	const auto HasStrings = [](const FJsonObject& Row, const TCHAR* Key, std::initializer_list<const TCHAR*> Expected)
	{
		const TArray<TSharedPtr<FJsonValue>>* Values = nullptr;
		if (!Row.TryGetArrayField(Key, Values) || !Values || Values->Num() != static_cast<int32>(Expected.size())) { return false; }
		int32 Index = 0;
		for (const TCHAR* Text : Expected)
		{
			const auto& Value = (*Values)[Index++];
			if (Value->Type != EJson::String || !Value->AsString().Equals(Text, ESearchCase::CaseSensitive)) { return false; }
		}
		return true;
	};
	const auto Generated = FDnd5eJsonItemVariantGenerator::GenerateAtIndex(Templates, 0, Bases, 0, EPolicy::SourceDefault);
	TSharedPtr<FJsonObject> Row;
	if (!ReadPreview(Generated, Row)) { return false; }
	TestTrue(TEXT("Expressions read original base amounts and emit numbers without control fields"),
		Generated.Diagnostics.IsEmpty() && Row->GetNumberField(TEXT("value")) == 1100 && Row->GetNumberField(TEXT("weight")) == 4.5 &&
		!Row->HasField(TEXT("valueExpression")) && !Row->HasField(TEXT("weightExpression")) &&
		!Row->HasField(TEXT("barding")) && Row->GetStringField(TEXT("bardingType")) == TEXT("HA|SYN"));
	TestTrue(TEXT("Remove a noted UID, keep the surviving property, then apply additions"),
		HasStrings(*Row, TEXT("property"), {TEXT("Q|SYN"), TEXT("P|SYN"), TEXT("R|SYN")}));
	TestTrue(TEXT("Damage merges clear explicit null, remove overlapping base traits and deduplicate in order"),
		!Row->HasField(TEXT("vulnerable")) && HasStrings(*Row, TEXT("resist"), {TEXT("acid"), TEXT("fire"), TEXT("lightning")}) &&
		HasStrings(*Row, TEXT("immune"), {TEXT("poison")}));
	TestTrue(TEXT("Condition immunities are appended and deduplicated"),
		HasStrings(*Row, TEXT("conditionImmune"), {TEXT("prone"), TEXT("poisoned"), TEXT("stunned")}));

	const auto Unpriced = FDnd5eJsonItemVariantGenerator::GenerateAtIndex(Templates, 0, Bases, 1, EPolicy::SourceDefault);
	if (!ReadPreview(Unpriced, Row)) { return false; }
	TestTrue(TEXT("Missing price keeps the expression and an original-source warning"),
		!Row->HasField(TEXT("value")) && Row->GetNumberField(TEXT("weight")) == 2 &&
		Row->GetStringField(TEXT("valueExpression")) == TEXT("[[baseItem.value]] + 1000") &&
		Unpriced.Diagnostics.Num() == 1 && Unpriced.Diagnostics[0].Severity == EDnd5eDiagnosticSeverity::Warning &&
		Unpriced.Diagnostics[0].Code == FName(TEXT("Dnd5e.Json.PreservedUnsupportedMetadata")) &&
		Unpriced.Diagnostics[0].RelativeFilePath == Templates.Context.RelativeFilePath &&
		Unpriced.Diagnostics[0].JsonPointer == TEXT("/magicvariant/0/inherits/valueExpression"));
	const auto NullAmounts = FDnd5eJsonItemVariantGenerator::GenerateAtIndex(Templates, 0, Bases, 2, EPolicy::SourceDefault);
	if (!ReadPreview(NullAmounts, Row)) { return false; }
	TestTrue(TEXT("Explicit null inputs leave both generated amounts absent"),
		!Row->HasField(TEXT("value")) && !Row->HasField(TEXT("weight")) &&
		Row->HasField(TEXT("valueExpression")) && Row->HasField(TEXT("weightExpression")) && NullAmounts.Diagnostics.Num() == 2);
	const auto Zero = FDnd5eJsonItemVariantGenerator::GenerateAtIndex(Templates, 2, Bases, 3, EPolicy::SourceDefault);
	if (!ReadPreview(Zero, Row)) { return false; }
	TestTrue(TEXT("Zero is an actual numeric input"), Zero.Diagnostics.IsEmpty() && Row->HasTypedField<EJson::Number>(TEXT("value")) &&
		Row->GetNumberField(TEXT("value")) == 0 && Row->GetNumberField(TEXT("weight")) == 1);
	const auto Removed = FDnd5eJsonItemVariantGenerator::GenerateAtIndex(Templates, 1, Bases, 0, EPolicy::SourceDefault);
	if (!ReadPreview(Removed, Row)) { return false; }
	TestFalse(TEXT("Removing the last base property omits the field"), Row->HasField(TEXT("property")));
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eVariantStandaloneTransformFailureTest, "Dnd5e.Items.VariantGeneration.StandaloneTransformFailures", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FDnd5eVariantStandaloneTransformFailureTest::RunTest(const FString& Parameters)
{
	const auto Templates = VariantMatchDocument(TEXT(R"json({"magicvariant":[
		{"name":"Conflict","requires":[{}],"inherits":{"source":"VAR","value":3,"valueExpression":"[[baseItem.value]] * 2"}},
		{"name":"Type","requires":[{}],"inherits":{"source":"VAR","valueExpression":"[[baseItem.value]] * 2"}},
		{"name":"Overflow","requires":[{}],"inherits":{"source":"VAR","valueExpression":"[[baseItem.value]] * 4"}},
		{"name":"Trait","requires":[{}],"inherits":{"source":"VAR","resist":[{"resist":["fire"]}]}},
		{"name":"Property","requires":[{}],"inherits":{"source":"VAR","propertyRemove":[false]}},
		{"name":"Barding","requires":[{}],"inherits":{"source":"VAR","barding":false}}
	]})json"), TEXT("fixtures/variants.json"));
	const auto Bases = VariantMatchDocument(TEXT(R"json({"baseitem":[
		{"name":"Invented Base","source":"SYN","type":"HA"},
		{"name":"Invented Bad Value","source":"SYN","type":"HA","value":"Not a number"},
		{"name":"Invented Large Value","source":"SYN","type":"HA","value":9007199254740991}
	]})json"), TEXT("fixtures/base-items.json"));
	struct FCase { int32 Variant; int32 Base; const TCHAR* Pointer; bool bBaseFile = false; };
	for (const FCase& Case : {
		FCase{0, 0, TEXT("/magicvariant/0/inherits/valueExpression")}, {1, 1, TEXT("/baseitem/1/value"), true},
		{2, 2, TEXT("/magicvariant/2/inherits/valueExpression")}, {3, 0, TEXT("/magicvariant/3/inherits/resist/0")},
		{4, 0, TEXT("/magicvariant/4/inherits/propertyRemove/0")}, {5, 0, TEXT("/magicvariant/5/inherits/barding")}})
	{
		const auto Result = FDnd5eJsonItemVariantGenerator::GenerateAtIndex(Templates, Case.Variant, Bases, Case.Base, EPolicy::SourceDefault);
		TestTrue(FString::Printf(TEXT("Failure is located at %s and withholds the preview"), Case.Pointer),
			Result.Status == EDnd5eJsonReadStatus::Failure && Result.GeneratedJson.IsEmpty() && Result.GeneratedId == FDnd5eRecordId{} &&
			Result.BaseOrigin.JsonPointer.IsEmpty() && Result.TemplateOrigin.JsonPointer.IsEmpty() && Result.Diagnostics.Num() == 1 &&
			Result.Diagnostics[0].JsonPointer == Case.Pointer &&
			Result.Diagnostics[0].RelativeFilePath == (Case.bBaseFile ? Bases.Context.RelativeFilePath : Templates.Context.RelativeFilePath));
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
	const TMap<int32, TArray<int32>> CopyChains{
		{33, {33, 184, 198, 227}}, {182, {182, 56}}, {184, {184, 198, 227}},
		{198, {198, 227}}, {200, {200, 182, 56}}};
	const TMap<int32, int32> CopyBases{{33, 75}, {182, 24}, {184, 75}, {198, 75}, {200, 24}};
	int32 Copies = 0, CopiedPreviews = 0, Generated = 0, MissingAmounts = 0;
	for (int32 Index = 0; Index < Rows->Num(); ++Index)
	{
		const auto& Row = (*Rows)[Index];
		if (Row->Type == EJson::Object && Row->AsObject()->HasField(TEXT("_copy")))
		{
			++Copies;
			const auto* ExpectedChain = CopyChains.Find(Index);
			const auto* ExpectedBase = CopyBases.Find(Index);
			if (!TestTrue(TEXT("Copy has a recorded chain and matching base"), ExpectedChain && ExpectedBase)) { continue; }
			const TArray<int32> ModernMatch{*ExpectedBase};
			for (const EPolicy Policy : Policies)
			{
				const auto Match = FDnd5eJsonItemVariantMatcher::MatchAtIndex(Variants, Index, Bases, Policy);
				TestTrue(FString::Printf(TEXT("Copied row %d matches under policy %d"), Index, static_cast<int32>(Policy)),
					Match.Status == EDnd5eJsonReadStatus::Success && Match.Diagnostics.IsEmpty() &&
					(Policy == EPolicy::SourceDefault ? Match.MatchingBaseIndices == ModernMatch : Match.MatchingBaseIndices.IsEmpty()));
			}
			const auto Preview = FDnd5eJsonItemVariantGenerator::GenerateAtIndex(Variants, Index, Bases, *ExpectedBase, EPolicy::SourceDefault);
			if (TestTrue(FString::Printf(TEXT("Copied row %d generates its eligible pair"), Index),
				Preview.Status == EDnd5eJsonReadStatus::Success && Preview.Diagnostics.IsEmpty() &&
				Preview.TemplateSourceRowIndices == *ExpectedChain && !Preview.GeneratedJson.IsEmpty() &&
				Preview.TemplateOrigin.SourceId == Row->AsObject()->GetStringField(TEXT("source")) &&
				Preview.TemplateOrigin.JsonPointer == FString::Printf(TEXT("/magicvariant/%d"), Index) &&
				Preview.BaseOrigin.JsonPointer == FString::Printf(TEXT("/baseitem/%d"), *ExpectedBase)))
			{
				++CopiedPreviews;
				const auto Typed = FDnd5eJsonItemVariantGenerator::ReadAtIndex(Variants, Index, Bases, *ExpectedBase, EPolicy::SourceDefault);
				if (!TestTrue(FString::Printf(TEXT("Copied row %d converts directly with its preview identity and original origins"), Index),
					Typed.Status == EDnd5eJsonReadStatus::Success && Typed.Item.Header.RecordId == Preview.GeneratedId &&
					Typed.Item.bHasBaseItemId && Typed.TemplateSourceRowIndices == *ExpectedChain &&
					Typed.TemplateOrigin.SourceId == Preview.TemplateOrigin.SourceId &&
					Typed.TemplateOrigin.JsonPointer == Preview.TemplateOrigin.JsonPointer &&
					Typed.BaseOrigin.JsonPointer == Preview.BaseOrigin.JsonPointer))
				{
					for (const auto& Diagnostic : Typed.Diagnostics)
					{
						AddError(Diagnostic.RelativeFilePath + TEXT(" ") + Diagnostic.JsonPointer + TEXT(" ") + Diagnostic.Message);
					}
				}
			}
			else
			{
				for (const auto& Diagnostic : Preview.Diagnostics)
				{
					AddError(Diagnostic.RelativeFilePath + TEXT(" ") + Diagnostic.JsonPointer + TEXT(" ") + Diagnostic.Message);
				}
			}
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
			if (Policy != 0 || Result.MatchingBaseIndices.IsEmpty()) { continue; }
			const int32 BaseIndex = Result.MatchingBaseIndices[0];
			const auto Preview = FDnd5eJsonItemVariantGenerator::GenerateAtIndex(Variants, Index, Bases, BaseIndex, Policies[Policy]);
			if (Preview.Status == EDnd5eJsonReadStatus::Success)
			{
				TestTrue(TEXT("Generated sample keeps both original locations"),
					!Preview.GeneratedJson.IsEmpty() && !Preview.GeneratedId.SourceKey.IsEmpty() &&
					Preview.BaseOrigin.JsonPointer == FString::Printf(TEXT("/baseitem/%d"), BaseIndex) &&
					Preview.TemplateOrigin.JsonPointer == FString::Printf(TEXT("/magicvariant/%d"), Index));
				++Generated;
				for (const auto& Diagnostic : Preview.Diagnostics)
				{
					TestTrue(TEXT("Sample diagnostic is a located missing-base-price warning"),
						Diagnostic.Severity == EDnd5eDiagnosticSeverity::Warning &&
						Diagnostic.Code == FName(TEXT("Dnd5e.Json.PreservedUnsupportedMetadata")) &&
						Diagnostic.JsonPointer == FString::Printf(TEXT("/magicvariant/%d/inherits/valueExpression"), Index));
					++MissingAmounts;
				}
			}
			else
			{
				for (const auto& Diagnostic : Preview.Diagnostics)
				{
					AddError(Diagnostic.RelativeFilePath + TEXT(" ") + Diagnostic.JsonPointer + TEXT(" ") + Diagnostic.Message);
				}
				AddError(FString::Printf(TEXT("Unexpected preview failure for template %d and base %d"), Index, BaseIndex));
			}
		}
	}
	TestEqual(TEXT("Template copies in the snapshot"), Copies, 5);
	TestEqual(TEXT("Eligible copied previews"), CopiedPreviews, 5);
	TestEqual(TEXT("SourceDefault pair count"), Pairs[0], 3508);
	TestEqual(TEXT("SourceDefault templates with matches"), TemplatesWithMatches[0], 221);
	TestEqual(TEXT("PreferClassic pair count"), Pairs[1], 2873);
	TestEqual(TEXT("PreferClassic templates with matches"), TemplatesWithMatches[1], 151);
	TestEqual(TEXT("First eligible pair generated for each supported template"), Generated, 221);
	TestEqual(TEXT("First eligible samples with missing base prices"), MissingAmounts, 2);
	return !HasAnyErrors();
}
#endif
