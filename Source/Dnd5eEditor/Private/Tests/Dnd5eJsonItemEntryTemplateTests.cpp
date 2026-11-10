// Developed by Neko Creative Technologies
#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Content/Dnd5eContentLibrary.h"
#include "Dnd5eTags.h"
#include "Readers/Items/Dnd5eJsonItemLibrary.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"

namespace
{
	FDnd5eJsonDocument EntryTemplateDocument(const FString& Json, const TCHAR* Path)
	{
		FDnd5eJsonDocument Document;
		Document.Context.DatasetNamespace = TEXT("item-entry-test");
		Document.Context.RelativeFilePath = Path;
		Document.JsonText = Json;
		return Document;
	}

	bool TryGetItemEntryReference(const FDnd5eContentBlock& Block, FDnd5eReferenceContent& Reference)
	{
		FDnd5eParagraphContent Paragraph;
		return UDnd5eContentLibrary::TryGetParagraph(Block, Paragraph) && Paragraph.Inlines.Num() == 1 &&
			UDnd5eContentLibrary::TryGetReference(Paragraph.Inlines[0], Reference);
	}

	void CollectAuthoredItemEntryTokens(const TSharedPtr<FJsonValue>& Value, TArray<TSharedPtr<FJsonValue>>& Tokens)
	{
		if (Value->Type == EJson::String)
		{
			if (Value->AsString().Contains(TEXT("{#itemEntry "), ESearchCase::CaseSensitive)) { Tokens.Add(Value); }
		}
		else if (Value->Type == EJson::Array)
		{
			for (const auto& Child : Value->AsArray()) { CollectAuthoredItemEntryTokens(Child, Tokens); }
		}
		else if (Value->Type == EJson::Object)
		{
			for (const auto& Field : Value->AsObject()->Values) { CollectAuthoredItemEntryTokens(Field.Value, Tokens); }
		}
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eItemEntryTemplateReadTest, "Dnd5e.Items.EntryTemplate.ReadAndIdentity", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FDnd5eItemEntryTemplateReadTest::RunTest(const FString& Parameters)
{
	const auto Document = EntryTemplateDocument(TEXT(R"json({"itemEntry":[
		{"name":"Invented Ward","source":"SYN","page":4,"extra":true,
		 "entriesTemplate":["Before",{"type":"entries","name":"{{item.detail1}}",
		 "entries":["{{getFullImmRes item.resist}}",3.00]}]},
		{"name":"Broken","source":"SYN","entriesTemplate":"invalid"},
		{"name":"Bad Page","source":"SYN","page":[],"entriesTemplate":[]}
	]})json"), TEXT("fixtures/items-base.json"));
	const auto Read = UDnd5eJsonItemLibrary::ReadItemEntryTemplateAtIndex(Document, 0);
	if (!TestTrue(TEXT("The selected template converts"), Read.Status == EDnd5eJsonReadStatus::Success &&
		Read.Diagnostics.IsEmpty() && Read.EntryTemplate.Header.Provenance.Num() == 1)) { return false; }
	const auto& Template = Read.EntryTemplate;
	const auto& Origin = Template.Header.Provenance[0];
	TestTrue(TEXT("Identity and original metadata"),
		Template.Header.DisplayName == TEXT("Invented Ward") && Template.Header.RecordId.DatasetNamespace == Document.Context.DatasetNamespace &&
		Template.Header.RecordId.Category == Dnd5e::Category::ItemEntry.GetTag() &&
		Template.Header.RecordId.SourceKey == TEXT("dnd5e:item-entry:v1:invented ward|syn") &&
		Origin.SourceId == TEXT("SYN") && Origin.RelativeFilePath == Document.Context.RelativeFilePath &&
		Origin.JsonPointer == TEXT("/itemEntry/0") && Origin.Page.Number == 4 && Origin.AdditionalMetadataJson.Contains(TEXT("\"extra\"")));
	TArray<TSharedPtr<FJsonValue>> Values;
	if (TestTrue(TEXT("Template is an ordered JSON array with a nested object"),
		FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Template.EntriesTemplateJson), Values) &&
		Values.Num() == 2 && Values[0]->AsString() == TEXT("Before") && Values[1]->Type == EJson::Object))
	{
		const auto Object = Values[1]->AsObject();
		const auto& Entries = Object->GetArrayField(TEXT("entries"));
		TestTrue(TEXT("Both placeholder forms and the source numeric literal survive"),
			Object->GetStringField(TEXT("name")) == TEXT("{{item.detail1}}") && Entries.Num() == 2 &&
			Entries[0]->AsString() == TEXT("{{getFullImmRes item.resist}}") &&
			Template.EntriesTemplateJson.Contains(TEXT("3.00")));
	}
	struct FCase { int32 Index; const TCHAR* Pointer; };
	for (const auto& Case : {FCase{-1, TEXT("/itemEntry")}, {1, TEXT("/itemEntry/1/entriesTemplate")}, {2, TEXT("/itemEntry/2/page")}})
	{
		const auto Failed = FDnd5eJsonItemEntryTemplateReader::ReadAtIndex(Document, Case.Index);
		TestTrue(Case.Pointer, Failed.Status == EDnd5eJsonReadStatus::Failure &&
			Failed.EntryTemplate.Header.RecordId == FDnd5eRecordId{} && Failed.EntryTemplate.Header.Provenance.IsEmpty() &&
			Failed.EntryTemplate.EntriesTemplateJson.IsEmpty() && Failed.Diagnostics.Num() == 1 &&
			Failed.Diagnostics[0].RelativeFilePath == Document.Context.RelativeFilePath && Failed.Diagnostics[0].JsonPointer == Case.Pointer);
	}
	const auto* Node = UDnd5eJsonItemLibrary::StaticClass()->FindFunctionByName(TEXT("ReadItemEntryTemplateAtIndex"));
	const auto* Field = FDnd5eJsonItemEntryTemplateReadResult::StaticStruct()->FindPropertyByName(TEXT("EntryTemplate"));
	TestTrue(TEXT("The template and reader result are exposed to Blueprint"),
		Node && Node->HasAllFunctionFlags(FUNC_Static | FUNC_BlueprintCallable) && Field &&
		Field->HasAllPropertyFlags(CPF_BlueprintVisible | CPF_BlueprintReadOnly) &&
		FDnd5eItemEntryTemplate::StaticStruct()->GetBoolMetaData(TEXT("BlueprintType")));
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eItemEntryReferenceReadTest, "Dnd5e.Items.EntryTemplate.ContentReferences", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FDnd5eItemEntryReferenceReadTest::RunTest(const FString& Parameters)
{
	const auto Document = EntryTemplateDocument(TEXT(R"json({
		"itemEntry":[{"name":"Invented Ward","source":"SYN","entriesTemplate":[]}],
		"item":[{"name":"Invented Token","source":"SYN","rarity":"rare","entries":[
			"{#itemEntry invented ward|syn}",
			{"type":"refItemEntry","itemEntry":"Invented Ward|SYN"},
			"{#itemEntry Invented Ward}",
			"{#itemEntry Invalid|}",
			{"type":"refItemEntry","itemEntry":"Invented Ward|SYN","unknown":true},
			"Prose {#itemEntry Invented Ward|SYN}"]
		}]})json"), TEXT("fixtures/items.json"));
	const auto Template = FDnd5eJsonItemEntryTemplateReader::ReadAtIndex(Document, 0);
	const auto Item = FDnd5eJsonItemReader::ReadAtIndex(Document, 0);
	if (!TestTrue(TEXT("The template and item convert"), Template.Status == EDnd5eJsonReadStatus::Success &&
		Item.Status == EDnd5eJsonReadStatus::Success && Item.Item.Description.Num() == 6)) { return false; }
	const auto& Blocks = Item.Item.Description;
	for (int32 Index = 0; Index < 3; ++Index)
	{
		FDnd5eReferenceContent Reference;
		const bool bExplicit = Index != 2;
		TestTrue(FString::Printf(TEXT("Reference %d keeps its selector and original pointer"), Index),
			TryGetItemEntryReference(Blocks[Index], Reference) && Reference.bHasTargetSelector &&
			Reference.TargetSelector.Category == Dnd5e::Category::ItemEntry.GetTag() &&
			Reference.TargetSelector.Name.Equals(TEXT("Invented Ward"), ESearchCase::IgnoreCase) &&
			Reference.TargetSelector.bHasSource == bExplicit && Reference.bHasTargetRecordId == bExplicit &&
			(!bExplicit || Reference.TargetRecordId == Template.EntryTemplate.Header.RecordId) &&
			(bExplicit || Reference.TargetSelector.SourceId.IsEmpty()) && Reference.SourceFormat == TEXT("5etools") &&
			Blocks[Index].Metadata.JsonPointer == FString::Printf(TEXT("/item/0/entries/%d"), Index));
		TestTrue(TEXT("Source form is retained"), Index == 1
			? Blocks[Index].Metadata.SourceType == TEXT("refItemEntry") && Reference.SourceReference.Contains(TEXT("\"itemEntry\""))
			: Reference.SourceReference.StartsWith(TEXT("{#itemEntry ")));
	}
	FDnd5eParagraphContent Paragraph;
	FDnd5eUnsupportedContent Unsupported;
	FDnd5eTextContent Text;
	TestTrue(TEXT("Malformed reference string is retained as an unsupported inline"),
		UDnd5eContentLibrary::TryGetParagraph(Blocks[3], Paragraph) && Paragraph.Inlines.Num() == 1 &&
		UDnd5eContentLibrary::TryGetUnsupportedInline(Paragraph.Inlines[0], Unsupported) &&
		Unsupported.SourceFragment == TEXT("{#itemEntry Invalid|}"));
	TestTrue(TEXT("An extra field retains the complete structured source value"),
		UDnd5eContentLibrary::TryGetUnsupportedBlock(Blocks[4], Unsupported) && Unsupported.SourceFragment.Contains(TEXT("unknown")));
	TestTrue(TEXT("Embedded source syntax stays in prose"),
		UDnd5eContentLibrary::TryGetParagraph(Blocks[5], Paragraph) && Paragraph.Inlines.Num() == 1 &&
		UDnd5eContentLibrary::TryGetInlineText(Paragraph.Inlines[0], Text) && Text.Text.StartsWith(TEXT("Prose {#itemEntry ")));
	TestTrue(TEXT("Omitted and unsupported source forms emit located diagnostics"), Item.Diagnostics.Num() == 3 &&
		Item.Diagnostics[0].Code == FName(TEXT("Dnd5e.Json.UnresolvedReference")) &&
		Item.Diagnostics[0].JsonPointer == TEXT("/item/0/entries/2") &&
		Item.Diagnostics[1].JsonPointer == TEXT("/item/0/entries/3") && Item.Diagnostics[2].JsonPointer == TEXT("/item/0/entries/4"));
	return !HasAnyErrors();
}

IMPLEMENT_COMPLEX_AUTOMATION_TEST(FDnd5eItemEntryTemplateSnapshotTest, "Dnd5e.Items.EntryTemplate.External", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
void FDnd5eItemEntryTemplateSnapshotTest::GetTests(TArray<FString>& Names, TArray<FString>& Commands) const
{
	FString Definitions, Items, Variants;
	if (FParse::Value(FCommandLine::Get(), TEXT("Dnd5eJsonDefinitionFile="), Definitions) &&
		FParse::Value(FCommandLine::Get(), TEXT("Dnd5eJsonItemCopyFile="), Items) &&
		FParse::Value(FCommandLine::Get(), TEXT("Dnd5eJsonVariantFile="), Variants))
	{
		Names.Add(TEXT("InspectedSnapshot"));
		Commands.Add(Definitions);
	}
}

bool FDnd5eItemEntryTemplateSnapshotTest::RunTest(const FString& Parameters)
{
	FString Definitions;
	if (!TestTrue(TEXT("Read supplied template definitions"), FFileHelper::LoadFileToString(Definitions, *Parameters))) { return false; }
	const auto Document = EntryTemplateDocument(Definitions, TEXT("supplied/items-base.json"));
	TSharedPtr<FJsonObject> Root;
	const TArray<TSharedPtr<FJsonValue>>* Rows = nullptr;
	if (!TestTrue(TEXT("Snapshot contains 13 reusable templates"),
		FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Definitions), Root) && Root.IsValid() &&
		Root->TryGetArrayField(TEXT("itemEntry"), Rows) && Rows && Rows->Num() == 13)) { return false; }
	TSet<FDnd5eRecordId> TemplateIds;
	for (int32 Index = 0; Index < Rows->Num(); ++Index)
	{
		const auto Result = FDnd5eJsonItemEntryTemplateReader::ReadAtIndex(Document, Index);
		TArray<TSharedPtr<FJsonValue>> Entries;
		if (!TestTrue(TEXT("Each recipe keeps its JSON array and original location"),
			Result.Status == EDnd5eJsonReadStatus::Success && Result.Diagnostics.IsEmpty() &&
			Result.EntryTemplate.Header.Provenance.Num() == 1 &&
			Result.EntryTemplate.Header.Provenance[0].JsonPointer == FString::Printf(TEXT("/itemEntry/%d"), Index) &&
			FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Result.EntryTemplate.EntriesTemplateJson), Entries) &&
			Entries.Num() == (*Rows)[Index]->AsObject()->GetArrayField(TEXT("entriesTemplate")).Num())) { continue; }
		TemplateIds.Add(Result.EntryTemplate.Header.RecordId);
	}
	TestEqual(TEXT("All template IDs are unique"), TemplateIds.Num(), 13);

	TArray<TSharedPtr<FJsonValue>> Tokens;
	for (const TCHAR* Argument : {TEXT("Dnd5eJsonItemCopyFile="), TEXT("Dnd5eJsonVariantFile=")})
	{
		FString Path, Text;
		TSharedPtr<FJsonValue> Value;
		if (!TestTrue(TEXT("Read supplied item reference tokens"), FParse::Value(FCommandLine::Get(), Argument, Path) &&
			FFileHelper::LoadFileToString(Text, *Path) && FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Value) &&
			Value.IsValid())) { return false; }
		CollectAuthoredItemEntryTokens(Value, Tokens);
	}
	TestEqual(TEXT("All authored template references in the two supplied files"), Tokens.Num(), 122);
	auto ProbeItem = MakeShared<FJsonObject>();
	ProbeItem->SetStringField(TEXT("name"), TEXT("Invented Probe"));
	ProbeItem->SetStringField(TEXT("source"), TEXT("SYN"));
	ProbeItem->SetStringField(TEXT("rarity"), TEXT("none"));
	ProbeItem->SetArrayField(TEXT("entries"), Tokens);
	auto ProbeRoot = MakeShared<FJsonObject>();
	ProbeRoot->SetArrayField(TEXT("item"), {MakeShared<FJsonValueObject>(ProbeItem)});
	FString ProbeJson;
	auto Writer = TJsonWriterFactory<>::Create(&ProbeJson);
	if (!FJsonSerializer::Serialize(ProbeRoot, Writer)) { return false; }
	const auto Read = FDnd5eJsonItemReader::ReadAtIndex(EntryTemplateDocument(ProbeJson, TEXT("fixtures/token-probe.json")), 0);
	if (!TestTrue(TEXT("All authored tokens convert through the item Content reader"), Read.Status == EDnd5eJsonReadStatus::Success &&
		Read.Item.Description.Num() == Tokens.Num())) { return false; }
	int32 Explicit = 0, Omitted = 0;
	for (int32 Index = 0; Index < Tokens.Num(); ++Index)
	{
		FDnd5eReferenceContent Reference;
		if (!TestTrue(TEXT("Typed reference preserves the exact authored token"),
			TryGetItemEntryReference(Read.Item.Description[Index], Reference) && Reference.bHasTargetSelector &&
			Reference.SourceReference == Tokens[Index]->AsString())) { continue; }
		if (Reference.TargetSelector.bHasSource)
		{
			++Explicit;
			TestTrue(TEXT("Explicit identity matches one supplied definition"),
				Reference.bHasTargetRecordId && TemplateIds.Contains(Reference.TargetRecordId));
		}
		else
		{
			++Omitted;
			TestFalse(TEXT("Omitted publication stays unresolved"), Reference.bHasTargetRecordId);
		}
	}
	TestEqual(TEXT("Explicit references"), Explicit, 92);
	TestEqual(TEXT("Omitted references"), Omitted, 30);
	TestEqual(TEXT("Omitted-source diagnostics"), Read.Diagnostics.Num(), 30);
	return !HasAnyErrors();
}
#endif
