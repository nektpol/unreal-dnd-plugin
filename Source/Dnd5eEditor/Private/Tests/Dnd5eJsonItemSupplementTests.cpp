// Developed by Neko Creative Technologies
#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Content/Dnd5eContentLibrary.h"
#include "Readers/Items/Dnd5eJsonItemLibrary.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"

namespace
{
	FDnd5eJsonDocument ItemSupplementDocument(const FString& Json)
	{
		FDnd5eJsonDocument Document;
		Document.Context.DatasetNamespace = TEXT("item-supplement-test");
		Document.Context.RelativeFilePath = TEXT("fixtures/item-supplements.json");
		Document.JsonText = Json;
		return Document;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eItemSupplementTest, "Dnd5e.Items.Supplement.InheritanceAndAttachment", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FDnd5eItemSupplementTest::RunTest(const FString& Parameters)
{
	const auto Document = ItemSupplementDocument(TEXT(R"json({
		"item":[{"name":"Invented Compass","source":"SYN","rarity":"rare","entries":["Primary."]},
		        {"name":"Invented Compass","source":"OTHER","rarity":"rare"}],
		"itemFluff":[
		 {"name":"Invented Compass","source":"SYN","_copy":{"name":"Invented Middle","source":"MID"},"entries":["Child."]},
		 {"name":"Invented Middle","source":"MID","_copy":{"name":"invented parent","source":"art"}},
		 {"name":"Invented Parent","source":"ART","entries":["Parent."],"images":[
		  {"type":"image","href":{"type":"internal","path":"items/invented-compass.webp"},"width":640,"height":480,"credit":"Invented credit"}]},
		 {"name":"Invented Cleared","source":"SYN","_copy":{"name":"Invented Parent","source":"ART"},"entries":null,"images":[]}
		]})json"));
	const auto Result = UDnd5eJsonItemLibrary::ReadItemSupplements(Document);
	if (!TestTrue(TEXT("All rows convert in source order"), Result.Status == EDnd5eJsonReadStatus::Success &&
		Result.Diagnostics.IsEmpty() && Result.Rows.Num() == 4)) { return false; }
	const auto& Child = Result.Rows[0];
	const auto& Middle = Result.Rows[1];
	FDnd5eImageContent Image;
	TestTrue(TEXT("The copy chain preserves each original row in order"),
		Child.Provenance.Num() == 3 && Child.Provenance[0].JsonPointer == TEXT("/itemFluff/0") &&
		Child.Provenance[1].JsonPointer == TEXT("/itemFluff/1") && Child.Provenance[2].JsonPointer == TEXT("/itemFluff/2"));
	TestTrue(TEXT("Child entries and inherited images retain different owners"),
		Child.Content.Entries.Num() == 1 && Child.Content.Entries[0].Metadata.SourceId == TEXT("SYN") &&
		Child.Content.Entries[0].Metadata.JsonPointer == TEXT("/itemFluff/0/entries/0") &&
		Child.Content.Images.Num() == 1 && Child.Content.Images[0].Metadata.SourceId == TEXT("ART") &&
		Child.Content.Images[0].Metadata.JsonPointer == TEXT("/itemFluff/2/images/0") &&
		Child.Content.Images[0].Metadata.RelativeFilePath == Document.Context.RelativeFilePath &&
		UDnd5eContentLibrary::TryGetImage(Child.Content.Images[0], Image) && Image.bHasWidth && Image.Width == 640 &&
		Image.bHasHeight && Image.Height == 480 && Image.Credit.Num() == 1);
	TestTrue(TEXT("Omission inherits the parent entry, and null and empty arrays clear fields"),
		Middle.Content.Entries.Num() == 1 && Middle.Content.Entries[0].Metadata.JsonPointer == TEXT("/itemFluff/2/entries/0") &&
		Result.Rows[3].Content.Entries.IsEmpty() && Result.Rows[3].Content.Images.IsEmpty());

	const auto Primary = FDnd5eJsonItemReader::ReadAtIndex(Document, 0);
	const auto Other = FDnd5eJsonItemReader::ReadAtIndex(Document, 1);
	FDnd5eItem Attached;
	TestTrue(TEXT("The Blueprint attachment node keeps the primary identity and description"),
		Primary.Status == EDnd5eJsonReadStatus::Success && UDnd5eJsonItemLibrary::TryApplyItemSupplement(Primary.Item, Child, Attached) &&
		Attached.bHasSupplementaryContent && Attached.Header.RecordId == Primary.Item.Header.RecordId &&
		Attached.Description.Num() == 1 && Attached.SupplementaryContent.Images.Num() == 1 &&
		!Primary.Item.bHasSupplementaryContent);
	TestTrue(TEXT("The same name in a different publication fails the join"),
		Other.Status == EDnd5eJsonReadStatus::Success && !FDnd5eJsonItemSupplementReader::TryApply(Other.Item, Child, Attached) &&
		Attached.Header.RecordId == FDnd5eRecordId{} && !Attached.bHasSupplementaryContent);
	auto Generated = Primary.Item;
	Generated.Header.RecordId.SourceKey = TEXT("generated-pair");
	TestTrue(TEXT("The join uses the authored name and publication for a generated item"),
		FDnd5eJsonItemSupplementReader::TryApply(Generated, Child, Attached) &&
		Attached.Header.RecordId == Generated.Header.RecordId && Attached.bHasSupplementaryContent);
	Generated.Header.RecordId.DatasetNamespace = TEXT("other-dataset");
	TestFalse(TEXT("A different dataset cannot join"), FDnd5eJsonItemSupplementReader::TryApply(Generated, Child, Attached));
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eItemSupplementFailureTest, "Dnd5e.Items.Supplement.AtomicFailures", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FDnd5eItemSupplementFailureTest::RunTest(const FString& Parameters)
{
	const auto Check = [&](const TCHAR* Json, const TCHAR* Pointer)
	{
		const auto Document = ItemSupplementDocument(Json);
		const auto Result = FDnd5eJsonItemSupplementReader::ReadDocument(Document);
		TestTrue(Pointer, Result.Status == EDnd5eJsonReadStatus::Failure && Result.Rows.IsEmpty() &&
			Result.Diagnostics.Num() == 1 && Result.Diagnostics[0].RelativeFilePath == Document.Context.RelativeFilePath &&
			Result.Diagnostics[0].JsonPointer == Pointer);
	};
	Check(TEXT(R"({"itemFluff":[{"name":"A","source":"SYN"},{"name":"a","source":"syn"}]})"), TEXT("/itemFluff/1"));
	Check(TEXT(R"({"itemFluff":[{"name":"A","source":"SYN"},{"name":"B","source":"SYN","_copy":{"name":"Missing","source":"SYN"}}]})"),
		TEXT("/itemFluff/1/_copy"));
	Check(TEXT(R"({"itemFluff":[{"name":"A","source":"SYN","_copy":{"name":"B","source":"SYN"}},
		{"name":"B","source":"SYN","_copy":{"name":"A","source":"SYN"}}]})"), TEXT("/itemFluff/1/_copy"));
	Check(TEXT(R"({"itemFluff":[{"name":"A","source":"SYN","_copy":{"name":"B","source":"SYN"}},
		{"name":"B","source":"SYN","images":["This is a paragraph."]}]})"), TEXT("/itemFluff/1/images/0"));
	Check(TEXT(R"({"itemFluff":[{"name":"A","source":"SYN"},{"name":"B","source":"SYN","entries":{}}]})"), TEXT("/itemFluff/1/entries"));
	return !HasAnyErrors();
}

IMPLEMENT_COMPLEX_AUTOMATION_TEST(FDnd5eItemSupplementSnapshotTest, "Dnd5e.Items.Supplement.External", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
void FDnd5eItemSupplementSnapshotTest::GetTests(TArray<FString>& Names, TArray<FString>& Commands) const
{
	FString Path;
	if (FParse::Value(FCommandLine::Get(), TEXT("Dnd5eJsonItemSupplementFile="), Path))
	{
		Names.Add(TEXT("InspectedSnapshot"));
		Commands.Add(Path);
	}
}

bool FDnd5eItemSupplementSnapshotTest::RunTest(const FString& Parameters)
{
	FString Json;
	if (!TestTrue(TEXT("Load the supplied supplementary item document"), FFileHelper::LoadFileToString(Json, *Parameters))) { return false; }
	const auto Result = FDnd5eJsonItemSupplementReader::ReadDocument(ItemSupplementDocument(Json));
	if (!TestTrue(TEXT("The snapshot converts atomically"), Result.Status == EDnd5eJsonReadStatus::Success && Result.Rows.Num() == 980))
	{
		for (const auto& Diagnostic : Result.Diagnostics) { AddError(Diagnostic.JsonPointer + TEXT(" ") + Diagnostic.Message); }
		return false;
	}
	int32 Copied = 0, Entries = 0, Images = 0, TypedImages = 0;
	TSet<FDnd5eRecordId> Targets;
	for (int32 Index = 0; Index < Result.Rows.Num(); ++Index)
	{
		const auto& Row = Result.Rows[Index];
		if (!TestTrue(TEXT("Selected row origin remains in source order"), !Row.Provenance.IsEmpty() &&
			Row.Provenance[0].JsonPointer == FString::Printf(TEXT("/itemFluff/%d"), Index))) { continue; }
		Targets.Add(Row.TargetItemId);
		Copied += Row.Provenance.Num() > 1;
		Entries += Row.Content.Entries.Num();
		Images += Row.Content.Images.Num();
		for (const auto& Block : Row.Content.Images)
		{
			FDnd5eImageContent Image;
			TypedImages += UDnd5eContentLibrary::TryGetImage(Block, Image);
		}
	}
	TestEqual(TEXT("Unique target identities"), Targets.Num(), 980);
	TestEqual(TEXT("Copy chains"), Copied, 149);
	TestEqual(TEXT("Resolved text entries"), Entries, 26);
	TestEqual(TEXT("Resolved image entries"), Images, 984);
	TestEqual(TEXT("Resolved typed images"), TypedImages, 984);
	return !HasAnyErrors();
}
#endif
