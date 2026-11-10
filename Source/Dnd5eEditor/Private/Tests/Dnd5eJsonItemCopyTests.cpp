// Developed by Neko Creative Technologies
#if WITH_DEV_AUTOMATION_TESTS
#include "Dnd5eTestSupport.h"
#include "Readers/Items/Dnd5eJsonItemLibrary.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"

namespace
{
	FDnd5eJsonDocument CopyDocument(const FString& Json)
	{
		FDnd5eJsonDocument Document;
		Document.Context.DatasetNamespace = TEXT("item-copy-test");
		Document.Context.RelativeFilePath = TEXT("fixtures/copy-items.json");
		Document.JsonText = Json;
		return Document;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eItemCopyOverlayTest, "Dnd5e.Items.CopyAssembly.OverlayAndOrigins", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FDnd5eItemCopyOverlayTest::RunTest(const FString& Parameters)
{
	const auto Document = CopyDocument(TEXT(R"json({"item":[
		{"name":"Invented Base: 10%","source":"BAS","type":"M","rarity":"none","entries":["Original"],
		 "page":7,"tier":"Invented tier","srd":true,"value":10,"weight":3.00,"property":["SYN-PROP"],"reqAttune":true},
		{"name":"Invented Middle","source":"MID","rarity":"rare","entries":["Overridden"],
		 "_copy":{"name":"invented base: 10%","source":"bas","_preserve":{"page":true,"tier":false}}},
		{"name":"Invented Child","source":"SYN","value":null,"property":[],"reqAttune":false,"future":null,
		 "_copy":{"name":"Invented Middle","source":"MID","_preserve":{"*":true,"page":false}}},
		{"name":"Invented Base: 10%","source":"ALT","weight":99},false
	]})json"));
	const auto Result = UDnd5eJsonItemLibrary::AssembleItemCopyAtIndex(Document, 2);
	if (!TestTrue(TEXT("Copy assembly succeeds without diagnostics"),
		Result.Status == EDnd5eJsonReadStatus::Success && Result.Diagnostics.IsEmpty())) { return false; }
	const TArray<int32> ExpectedChain{2, 1, 0};
	TestTrue(TEXT("Chain runs from child to ancestor"), Result.SourceRowIndices == ExpectedChain);
	const TMap<FString, int32> ExpectedOrigins{{TEXT("name"), 2}, {TEXT("source"), 2}, {TEXT("type"), 0},
		{TEXT("rarity"), 1}, {TEXT("entries"), 1}, {TEXT("page"), 0}, {TEXT("weight"), 0}, {TEXT("property"), 2},
		{TEXT("reqAttune"), 2}, {TEXT("future"), 2}};
	TestTrue(TEXT("Field origins preserve overlays and omit deleted or protected fields"), Result.FieldSourceRows.OrderIndependentCompareEqual(ExpectedOrigins));
	TestFalse(TEXT("The preview omits the copy directive"), Result.AssembledJson.Contains(TEXT("\"_copy\"")));
	TestTrue(TEXT("Null with no corresponding parent field remains in the preview"), Result.AssembledJson.Contains(TEXT("\"future\"")));

	auto Preview = CopyDocument(Result.AssembledJson);
	Preview.Context.RelativeFilePath = TEXT("fixtures/assembled-preview.json");
	const auto Read = FDnd5eJsonItemReader::ReadAtIndex(Preview, 0);
	if (TestTrue(TEXT("The preview feeds the ordinary reader"), Read.Status == EDnd5eJsonReadStatus::Success &&
		Read.Item.Header.Provenance.Num() == 1))
	{
		const auto& Item = Read.Item;
		TestTrue(TEXT("Child identity, source and empty-array or false overrides survive"),
			Item.Header.DisplayName == TEXT("Invented Child") && Item.Header.Provenance[0].SourceId == TEXT("SYN") &&
			Item.Rarity == Dnd5e::Item::Rarity::Rare && Item.Properties.IsEmpty() &&
			Item.Attunement.Mode == EDnd5eAttunementMode::NotRequired);
		TestTrue(TEXT("Inherited numeric literal stays intact and tombstone removes price"),
			Item.bHasWeight && Item.Weight.Amount.SourceLiteral == TEXT("3.00") &&
			Item.Price.CostPresence == EDnd5eValuePresence::Unspecified);
		TestEqual(TEXT("Typed provenance belongs to the preview"), Item.Header.Provenance[0].RelativeFilePath, Preview.Context.RelativeFilePath);
	}
	TestTrue(TEXT("Ordinary reader still requires explicit assembly for the raw copy"),
		FDnd5eJsonItemReader::ReadAtIndex(Document, 2).Status == EDnd5eJsonReadStatus::Failure);

	const auto* Type = FDnd5eJsonItemCopyAssemblyResult::StaticStruct();
	TestTrue(TEXT("Preview is Blueprint-visible in the JSON module"),
		Type->GetBoolMetaData(TEXT("BlueprintType")) && Type->GetOutermost()->GetName() == TEXT("/Script/Dnd5eJson"));
	for (const TCHAR* Name : {TEXT("Status"), TEXT("AssembledJson"), TEXT("SourceRowIndices"), TEXT("FieldSourceRows"), TEXT("Diagnostics")})
	{
		const auto* Property = Type->FindPropertyByName(Name);
		TestTrue(Name, Property && Property->HasAllPropertyFlags(CPF_BlueprintVisible | CPF_BlueprintReadOnly));
	}
	const auto* Node = UDnd5eJsonItemLibrary::StaticClass()->FindFunctionByName(TEXT("AssembleItemCopyAtIndex"));
	TestTrue(TEXT("Assembly is available as a Blueprint node"), Node && Node->HasAllFunctionFlags(FUNC_Static | FUNC_BlueprintCallable));
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eItemCopyFailureTest, "Dnd5e.Items.CopyAssembly.AtomicFailures", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FDnd5eItemCopyFailureTest::RunTest(const FString& Parameters)
{
	const auto Document = CopyDocument(TEXT(R"json({"item":[
		{"name":"Base","source":"SYN","entries":["Original"]},
		{"name":"Missing","source":"SYN","_copy":{"name":"Absent","source":"SYN"}},
		{"name":"Ambiguous","source":"SYN","_copy":{"name":"Duplicate","source":"SYN"}},
		{"name":"Duplicate","source":"SYN"},{"name":"DUPLICATE","source":"syn"},
		{"name":"Cycle A","source":"SYN","_copy":{"name":"Cycle B","source":"SYN"}},
		{"name":"Cycle B","source":"SYN","_copy":{"name":"Cycle A","source":"SYN"}},
		{"name":"Bad Preserve","source":"SYN","_copy":{"name":"Base","source":"SYN","_preserve":{"x/y~z":1}}},
		{"name":"Modified","source":"SYN","_copy":{"name":"Base","source":"SYN","_mod":{"entries":{"mode":"prependArr","items":"Unsupported"}}}},
		{"name":"Needs Modified","source":"SYN","_copy":{"name":"Modified","source":"SYN"}},
		{"name":"Case Collision","source":"SYN","Entries":[],"_copy":{"name":"Base","source":"SYN"}},
		{"name":"Missing Source","source":"SYN","_copy":{"name":"Base"}}
	]})json"));
	struct FCase { int32 Index; const TCHAR* Pointer; };
	for (const FCase& Case : {FCase{-1, TEXT("/item")}, {0, TEXT("/item/0/_copy")}, {1, TEXT("/item/1/_copy")},
		{2, TEXT("/item/2/_copy")}, {5, TEXT("/item/6/_copy")}, {7, TEXT("/item/7/_copy/_preserve/x~1y~0z")},
		{9, TEXT("/item/8/_copy/_mod/entries/mode")}, {10, TEXT("/item/10")}, {11, TEXT("/item/11/_copy")}})
	{
		const auto Result = FDnd5eJsonItemCopyAssembler::AssembleAtIndex(Document, Case.Index);
		TestTrue(FString::Printf(TEXT("Failure at %s leaves no staged output"), Case.Pointer),
			Result.Status == EDnd5eJsonReadStatus::Failure && Result.AssembledJson.IsEmpty() &&
			Result.SourceRowIndices.IsEmpty() && Result.FieldSourceRows.IsEmpty() && Result.Diagnostics.Num() == 1 &&
			Result.Diagnostics[0].Severity == EDnd5eDiagnosticSeverity::Error && Result.Diagnostics[0].JsonPointer == Case.Pointer);
	}
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eItemCopyDepthTest, "Dnd5e.Items.CopyAssembly.DepthLimit", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FDnd5eItemCopyDepthTest::RunTest(const FString& Parameters)
{
	TArray<FString> Rows{TEXT(R"({"name":"Row 0","source":"SYN","rarity":"none"})")};
	for (int32 Index = 1; Index <= 33; ++Index)
	{
		Rows.Add(FString::Printf(TEXT(R"({"name":"Row %d","source":"SYN","_copy":{"name":"Row %d","source":"SYN"}})"), Index, Index - 1));
	}
	const auto Document = CopyDocument(TEXT("{\"item\":[") + FString::Join(Rows, TEXT(",")) + TEXT("]}"));
	const auto Allowed = FDnd5eJsonItemCopyAssembler::AssembleAtIndex(Document, 32);
	TestTrue(TEXT("32 copy hops are accepted"), Allowed.Status == EDnd5eJsonReadStatus::Success && Allowed.SourceRowIndices.Num() == 33);
	const auto Rejected = FDnd5eJsonItemCopyAssembler::AssembleAtIndex(Document, 33);
	TestTrue(TEXT("A deeper chain fails before returning any preview"), Rejected.Status == EDnd5eJsonReadStatus::Failure &&
		Rejected.AssembledJson.IsEmpty() && Rejected.SourceRowIndices.IsEmpty() && Rejected.FieldSourceRows.IsEmpty() &&
		Rejected.Diagnostics.Num() == 1 && Rejected.Diagnostics[0].JsonPointer == TEXT("/item/1/_copy"));
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eItemCopyModifierOrderTest, "Dnd5e.Items.CopyAssembly.ModifierOrder", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FDnd5eItemCopyModifierOrderTest::RunTest(const FString& Parameters)
{
	const auto Document = CopyDocument(TEXT(R"json({"item":[
		{"name":"Base","source":"BAS","type":"M","rarity":"none","entries":["A","B"],"property":["P"]},
		{"name":"Middle","source":"MID","_copy":{"name":"Base","source":"BAS","_mod":{
			"entries":[{"mode":"insertArr","index":0,"items":"Start"},
				{"mode":"appendArr","items":[{"type":"entries","entries":["Middle note"]}]},
				{"mode":"replaceArr","replace":{"index":2},"items":["Replacement","After"]}],
			"property":{"mode":"appendIfNotExistsArr","items":["P","Q","p","Q"]}}}},
		{"name":"Final","source":"SYN","_copy":{"name":"Middle","source":"MID","_mod":{
			"entries":[{"mode":"insertArr","index":-1,"items":"Tail"},{"mode":"replaceArr","replace":{"index":1},"items":[]}],
			"property":{"mode":"appendIfNotExistsArr","items":"Q"}}}},
		{"name":"Blank","source":"BAS","entries":null},
		{"name":"New arrays","source":"SYN","_copy":{"name":"Blank","source":"BAS","_mod":{
			"entries":[{"mode":"appendArr","items":"Created"},{"mode":"insertArr","index":99,"items":"Clamped"}],
			"property":{"mode":"appendIfNotExistsArr","items":"P"}}}}
	]})json"));
	const auto ReadPreview = [&](int32 Index, TSharedPtr<FJsonObject>& Out, FDnd5eJsonItemCopyAssemblyResult& Result)
	{
		Result = FDnd5eJsonItemCopyAssembler::AssembleAtIndex(Document, Index);
		TSharedPtr<FJsonObject> Root;
		const TArray<TSharedPtr<FJsonValue>>* Rows = nullptr;
		if (!TestTrue(TEXT("Copy assembles a single preview row"), Result.Status == EDnd5eJsonReadStatus::Success &&
			Result.Diagnostics.IsEmpty() && FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Result.AssembledJson), Root) &&
			Root.IsValid() && Root->TryGetArrayField(TEXT("item"), Rows) && Rows && Rows->Num() == 1)) { return false; }
		Out = (*Rows)[0]->AsObject();
		return TestTrue(TEXT("Preview row is an object"), Out.IsValid());
	};
	FDnd5eJsonItemCopyAssemblyResult Result;
	TSharedPtr<FJsonObject> Row;
	if (!ReadPreview(2, Row, Result)) { return false; }
	const TArray<TSharedPtr<FJsonValue>>* Entries = nullptr;
	const TArray<TSharedPtr<FJsonValue>>* Properties = nullptr;
	if (!TestTrue(TEXT("Modified arrays are present"), Row->TryGetArrayField(TEXT("entries"), Entries) && Entries && Entries->Num() == 5 &&
		Row->TryGetArrayField(TEXT("property"), Properties) && Properties && Properties->Num() == 4)) { return false; }
	const TArray<TSharedPtr<FJsonValue>>* Nested = nullptr;
	TestTrue(TEXT("Directives run in order and preserve structured insertion"), (*Entries)[0]->AsString() == TEXT("Start") &&
		(*Entries)[1]->AsString() == TEXT("Replacement") && (*Entries)[2]->AsString() == TEXT("After") &&
		(*Entries)[3]->Type == EJson::Object && (*Entries)[3]->AsObject()->TryGetArrayField(TEXT("entries"), Nested) &&
		Nested && Nested->Num() == 1 && (*Nested)[0]->AsString() == TEXT("Middle note") && (*Entries)[4]->AsString() == TEXT("Tail"));
	TestTrue(TEXT("Conditional append is exact and compares against the pre-operation array"),
		(*Properties)[0]->AsString().Equals(TEXT("P"), ESearchCase::CaseSensitive) &&
		(*Properties)[1]->AsString().Equals(TEXT("Q"), ESearchCase::CaseSensitive) &&
		(*Properties)[2]->AsString().Equals(TEXT("p"), ESearchCase::CaseSensitive) &&
		(*Properties)[3]->AsString().Equals(TEXT("Q"), ESearchCase::CaseSensitive));
	const TArray<int32> ExpectedChain{2, 1, 0};
	TestTrue(TEXT("Modified origins follow the last effective operation, not a no-op"),
		Result.SourceRowIndices == ExpectedChain && Result.FieldSourceRows.FindRef(TEXT("entries"), INDEX_NONE) == 2 &&
		Result.FieldSourceRows.FindRef(TEXT("property"), INDEX_NONE) == 1);
	const auto Typed = FDnd5eJsonItemReader::ReadAtIndex(CopyDocument(Result.AssembledJson), 0);
	TestTrue(TEXT("The modified preview converts to an ordinary item"),
		Typed.Status == EDnd5eJsonReadStatus::Success && Typed.Item.Description.Num() == 5 && Typed.Item.Properties.Num() == 4);

	if (!ReadPreview(4, Row, Result)) { return false; }
	if (!TestTrue(TEXT("Append creates arrays from null and absence"),
		Row->TryGetArrayField(TEXT("entries"), Entries) && Entries && Entries->Num() == 2 &&
		Row->TryGetArrayField(TEXT("property"), Properties) && Properties && Properties->Num() == 1)) { return false; }
	TestTrue(TEXT("Out-of-range insertion clamps to end"), (*Entries)[0]->AsString() == TEXT("Created") &&
		(*Entries)[1]->AsString() == TEXT("Clamped") && (*Properties)[0]->AsString() == TEXT("P"));
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eItemCopyModifierFailureTest, "Dnd5e.Items.CopyAssembly.ModifierFailures", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FDnd5eItemCopyModifierFailureTest::RunTest(const FString& Parameters)
{
	const auto Document = CopyDocument(TEXT(R"json({"item":[
		{"name":"Base","source":"SYN","entries":["A"],"property":["P"]},
		{"name":"Late failure","source":"SYN","_copy":{"name":"Base","source":"SYN","_mod":{"entries":[
			{"mode":"appendArr","items":"Staged"},{"mode":"replaceArr","replace":{"index":9},"items":"Bad"}]}}},
		{"name":"Nested target","source":"SYN","_copy":{"name":"Base","source":"SYN","_mod":{"entries/0~bad":{}}}},
		{"name":"Variable","source":"SYN","_copy":{"name":"Base","source":"SYN","_mod":{
			"entries":{"mode":"appendArr","items":{"entries":["<$unknown$>"]}}}}},
		{"name":"Wrong property","source":"SYN","_copy":{"name":"Base","source":"SYN","_mod":{
			"property":{"mode":"appendIfNotExistsArr","items":5}}}},
		{"name":"Regex selector","source":"SYN","_copy":{"name":"Base","source":"SYN","_mod":{
			"entries":{"mode":"replaceArr","replace":{"regex":"A"},"items":"B"}}}},
		{"name":"Nonarray target","source":"SYN","entries":"Not an array","_copy":{"name":"Base","source":"SYN","_mod":{
			"entries":{"mode":"insertArr","index":-1,"items":"B"}}}},
		{"name":"Invalid modifiers","source":"SYN","_copy":{"name":"Base","source":"SYN","_mod":null}},
		{"name":"Invalid insert index","source":"SYN","_copy":{"name":"Base","source":"SYN","_mod":{
			"entries":{"mode":"insertArr","index":-2,"items":"B"}}}}
	]})json"));
	const TCHAR* Pointers[] = {
		TEXT("/item/1/_copy/_mod/entries/1/replace/index"), TEXT("/item/2/_copy/_mod/entries~10~0bad"),
		TEXT("/item/3/_copy/_mod/entries/items"), TEXT("/item/4/_copy/_mod/property/items"),
		TEXT("/item/5/_copy/_mod/entries/replace/regex"), TEXT("/item/6/_copy/_mod/entries"), TEXT("/item/7/_copy/_mod"), TEXT("/item/8/_copy/_mod/entries/index")
	};
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(Pointers); ++Index)
	{
		const auto Result = FDnd5eJsonItemCopyAssembler::AssembleAtIndex(Document, Index + 1);
		TestTrue(FString::Printf(TEXT("Modifier failure at %s is atomic"), Pointers[Index]),
			Result.Status == EDnd5eJsonReadStatus::Failure && Result.AssembledJson.IsEmpty() &&
			Result.SourceRowIndices.IsEmpty() && Result.FieldSourceRows.IsEmpty() &&
			Result.Diagnostics.Num() == 1 && Result.Diagnostics[0].JsonPointer == Pointers[Index]);
	}
	return !HasAnyErrors();
}

IMPLEMENT_COMPLEX_AUTOMATION_TEST(FDnd5eItemCopySnapshotTest, "Dnd5e.Items.CopyAssembly.External", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
void FDnd5eItemCopySnapshotTest::GetTests(TArray<FString>& Names, TArray<FString>& Commands) const
{
	FString Path;
	if (FParse::Value(FCommandLine::Get(), TEXT("Dnd5eJsonItemCopyFile="), Path))
	{
		Names.Add(TEXT("InspectedSnapshot"));
		Commands.Add(Path);
	}
}

bool FDnd5eItemCopySnapshotTest::RunTest(const FString& Parameters)
{
	FString Json;
	if (!TestTrue(TEXT("Read supplied ordinary item snapshot"), FFileHelper::LoadFileToString(Json, *Parameters))) { return false; }
	TSharedPtr<FJsonObject> Root;
	const TArray<TSharedPtr<FJsonValue>>* Rows = nullptr;
	if (!TestTrue(TEXT("Snapshot has an item array"), FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json), Root) &&
		Root.IsValid() && Root->TryGetArrayField(TEXT("item"), Rows) && Rows)) { return false; }

	const auto Document = CopyDocument(Json);
	int32 Copies = 0, Assembled = 0;
	for (int32 Index = 0; Index < Rows->Num(); ++Index)
	{
		const auto& Row = (*Rows)[Index]->AsObject();
		const TSharedPtr<FJsonObject>* Copy = nullptr;
		if (!Row.IsValid() || !Row->TryGetObjectField(TEXT("_copy"), Copy) || !Copy || !Copy->IsValid()) { continue; }
		++Copies;
		const auto Result = FDnd5eJsonItemCopyAssembler::AssembleAtIndex(Document, Index);
		if (Result.Status != EDnd5eJsonReadStatus::Success)
		{
			for (const auto& Diagnostic : Result.Diagnostics)
			{
				AddError(FString::Printf(TEXT("Row %d: %s %s"), Index, *Diagnostic.JsonPointer, *Diagnostic.Message));
			}
			continue;
		}
		TestTrue(TEXT("Preview retains the selected index and its own identity fields"), Result.SourceRowIndices.Num() >= 2 &&
			Result.SourceRowIndices[0] == Index && Result.FieldSourceRows.FindRef(TEXT("name"), INDEX_NONE) == Index &&
			Result.FieldSourceRows.FindRef(TEXT("source"), INDEX_NONE) == Index);
		++Assembled;
	}
	TestTrue(TEXT("All 67 copy chains assemble in the inspected snapshot"), Copies == 67 && Assembled == 67);
	return !HasAnyErrors();
}
#endif
