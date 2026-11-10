// Developed by Neko Creative Technologies
#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Dnd5eTags.h"
#include "Items/Dnd5eItemIdentity.h"
#include "Readers/Items/Dnd5eJsonItemLibrary.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"

namespace
{
	FDnd5eJsonDocument GroupDocument(const FString& Json)
	{
		FDnd5eJsonDocument Document;
		Document.Context.DatasetNamespace = TEXT("item-group-test");
		Document.Context.RelativeFilePath = TEXT("fixtures/items.json");
		Document.JsonText = Json;
		return Document;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eItemGroupReadTest, "Dnd5e.Items.Group.ReadAndIdentity", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FDnd5eItemGroupReadTest::RunTest(const FString& Parameters)
{
	const auto Document = GroupDocument(TEXT(R"({"item":[{"name":"Invented Group","source":"SYN","rarity":"none"}],
		"itemGroup":[{"name":"Invented Group","source":"SYN","rarity":"rare","page":12,"weight":3.00,
		"baseItem":"Invented Group|SYN","entries":["Invented description."],
		"items":["Invented Blade|BAS","Invented Pattern","Invented Blade|BAS"],"itemsHidden":true,"tier":"invented"}]})"));
	const auto Read = UDnd5eJsonItemLibrary::ReadItemGroupAtIndex(Document, 0);
	const auto Ordinary = UDnd5eJsonItemLibrary::ReadItemAtIndex(Document, 0);
	if (!TestTrue(TEXT("The named group and the same-name ordinary item both convert"),
		Read.Status == EDnd5eJsonReadStatus::Success && Ordinary.Status == EDnd5eJsonReadStatus::Success)) { return false; }

	const auto& Group = Read.Group;
	const auto& Details = Group.Details;
	FDnd5eRecordId Canonical;
	TestTrue(TEXT("The group has its own canonical identity and can link to the same-name base"),
		Dnd5e::Items::TryMakeGroupId(Document.Context.DatasetNamespace, TEXT("invented group"), TEXT("syn"), Canonical) &&
		Details.Header.RecordId == Canonical && Canonical.Category == Dnd5e::Category::ItemGroup.GetTag() &&
		Canonical.SourceKey == TEXT("dnd5e:item-group:v1:invented group|syn") &&
		Details.bHasBaseItemId && Details.BaseItemId == Ordinary.Item.Header.RecordId && Details.BaseItemId != Canonical);
	TestTrue(TEXT("Common fields and source locations use the existing typed reader"),
		Details.Header.DisplayName == TEXT("Invented Group") && Details.Rarity == Dnd5e::Item::Rarity::Rare.GetTag() &&
		Details.bHasWeight && Details.Weight.Amount.SourceLiteral == TEXT("3.00") && Details.Description.Num() == 1 &&
		Details.Description[0].Metadata.JsonPointer == TEXT("/itemGroup/0/entries/0") &&
		Details.Header.Provenance.Num() == 1 && Details.Header.Provenance[0].Page.Number == 12 &&
		Details.Header.Provenance[0].RelativeFilePath == Document.Context.RelativeFilePath &&
		Details.Header.Provenance[0].JsonPointer == TEXT("/itemGroup/0"));
	if (!TestEqual(TEXT("Hidden member declarations preserve order and repetitions"), Group.Members.Num(), 3)) { return false; }
	TestTrue(TEXT("Visibility and explicit source parsing"),
		Group.bItemsHidden && Group.Members[0].SourceUid == TEXT("Invented Blade|BAS") &&
		Group.Members[0].Name == TEXT("Invented Blade") && Group.Members[0].bHasSource && Group.Members[0].SourceId == TEXT("BAS") &&
		Group.Members[1].SourceUid == TEXT("Invented Pattern") && Group.Members[1].Name == TEXT("Invented Pattern") &&
		!Group.Members[1].bHasSource && Group.Members[1].SourceId.IsEmpty() &&
		Group.Members[2].SourceUid == Group.Members[0].SourceUid);
	TSharedPtr<FJsonObject> Metadata;
	TestTrue(TEXT("The typed group fields are consumed, and unrelated metadata survives"),
		Details.Header.Provenance.Num() == 1 && FJsonSerializer::Deserialize(
			TJsonReaderFactory<>::Create(Details.Header.Provenance[0].AdditionalMetadataJson), Metadata) &&
		Metadata.IsValid() && Metadata->HasField(TEXT("tier")) && !Metadata->HasField(TEXT("items")) &&
		!Metadata->HasField(TEXT("itemsHidden")));

	const auto* Node = UDnd5eJsonItemLibrary::StaticClass()->FindFunctionByName(TEXT("ReadItemGroupAtIndex"));
	const auto* GroupProperty = FDnd5eJsonItemGroupReadResult::StaticStruct()->FindPropertyByName(TEXT("Group"));
	TestTrue(TEXT("Blueprint can call the reader and inspect its result"),
		Node && Node->HasAllFunctionFlags(FUNC_Static | FUNC_BlueprintCallable) && GroupProperty &&
		GroupProperty->HasAllPropertyFlags(CPF_BlueprintVisible | CPF_BlueprintReadOnly) &&
		FDnd5eItemGroup::StaticStruct()->GetBoolMetaData(TEXT("BlueprintType")) &&
		FDnd5eItemGroupMember::StaticStruct()->GetBoolMetaData(TEXT("BlueprintType")));
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eItemGroupFailureTest, "Dnd5e.Items.Group.AtomicFailures", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FDnd5eItemGroupFailureTest::RunTest(const FString& Parameters)
{
	const auto Document = GroupDocument(TEXT(R"({"itemGroup":[
		{"name":"No Members","source":"SYN","rarity":"none","items":[]},
		{"name":"Empty Source","source":"SYN","rarity":"none","items":["Valid","Broken|"]},
		{"name":"Extra Separator","source":"SYN","rarity":"none","items":["Broken|SRC|Display"]},
		{"name":"Bad Flag","source":"SYN","rarity":"none","items":[],"itemsHidden":null},
		{"name":"Bad Member","source":"SYN","rarity":"none","items":[{}]},
		{"name":"Missing List","source":"SYN","rarity":"none"},
		{"name":"Bad Details","source":"SYN","rarity":"none","items":["Valid"],"weight":"invalid"}
	]})"));
	const auto Empty = FDnd5eJsonItemGroupReader::ReadAtIndex(Document, 0);
	TestTrue(TEXT("An explicit empty member list is valid and visible by default"),
		Empty.Status == EDnd5eJsonReadStatus::Success && Empty.Group.Members.IsEmpty() && !Empty.Group.bItemsHidden);
	struct FCase { int32 Index; const TCHAR* Pointer; };
	for (const FCase& Case : {FCase{1, TEXT("/itemGroup/1/items/1")}, {2, TEXT("/itemGroup/2/items/0")},
		{3, TEXT("/itemGroup/3/itemsHidden")}, {4, TEXT("/itemGroup/4/items/0")}, {5, TEXT("/itemGroup/5/items")},
		{6, TEXT("/itemGroup/6/weight")}, {-1, TEXT("/itemGroup")}})
	{
		const auto Read = FDnd5eJsonItemGroupReader::ReadAtIndex(Document, Case.Index);
		TestTrue(FString::Printf(TEXT("Failure withholds the group and reports %s"), Case.Pointer),
			Read.Status == EDnd5eJsonReadStatus::Failure && Read.Group.Members.IsEmpty() &&
			Read.Group.Details.Header.RecordId == FDnd5eRecordId{} && Read.Group.Details.Header.Provenance.IsEmpty() &&
			Read.Diagnostics.Num() == 1 && Read.Diagnostics[0].RelativeFilePath == Document.Context.RelativeFilePath &&
			Read.Diagnostics[0].JsonPointer == Case.Pointer);
	}
	return !HasAnyErrors();
}

IMPLEMENT_COMPLEX_AUTOMATION_TEST(FDnd5eItemGroupSnapshotTest, "Dnd5e.Items.Group.External", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
void FDnd5eItemGroupSnapshotTest::GetTests(TArray<FString>& Names, TArray<FString>& Commands) const
{
	FString Path;
	if (FParse::Value(FCommandLine::Get(), TEXT("Dnd5eJsonItemCopyFile="), Path))
	{
		Names.Add(TEXT("InspectedSnapshot"));
		Commands.Add(Path);
	}
}

bool FDnd5eItemGroupSnapshotTest::RunTest(const FString& Parameters)
{
	FString Json;
	if (!TestTrue(TEXT("Read the supplied items document"), FFileHelper::LoadFileToString(Json, *Parameters))) { return false; }
	TSharedPtr<FJsonObject> Root;
	const TArray<TSharedPtr<FJsonValue>>* Rows = nullptr;
	if (!TestTrue(TEXT("Snapshot has all 119 item group rows"),
		FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json), Root) && Root.IsValid() &&
		Root->TryGetArrayField(TEXT("itemGroup"), Rows) && Rows && Rows->Num() == 119)) { return false; }

	const auto Document = GroupDocument(Json);
	TSet<FDnd5eRecordId> Ids;
	int32 MemberCount = 0, ExplicitCount = 0, HiddenCount = 0;
	for (int32 Index = 0; Index < Rows->Num(); ++Index)
	{
		const auto Read = FDnd5eJsonItemGroupReader::ReadAtIndex(Document, Index);
		if (!TestTrue(FString::Printf(TEXT("Group row %d converts"), Index), Read.Status == EDnd5eJsonReadStatus::Success))
		{
			for (const auto& Diagnostic : Read.Diagnostics) { AddError(Diagnostic.JsonPointer + TEXT(" ") + Diagnostic.Message); }
			continue;
		}
		const auto& SourceMembers = (*Rows)[Index]->AsObject()->GetArrayField(TEXT("items"));
		const auto& Group = Read.Group;
		TestTrue(TEXT("Original group location and distinct group identity"),
			Group.Details.Header.Provenance.Num() == 1 &&
			Group.Details.Header.Provenance[0].JsonPointer == FString::Printf(TEXT("/itemGroup/%d"), Index) &&
			Group.Details.Header.RecordId.Category == Dnd5e::Category::ItemGroup.GetTag());
		Ids.Add(Group.Details.Header.RecordId);
		HiddenCount += Group.bItemsHidden;
		if (!TestEqual(TEXT("All group declarations survive"), Group.Members.Num(), SourceMembers.Num())) { continue; }
		MemberCount += Group.Members.Num();
		for (int32 MemberIndex = 0; MemberIndex < Group.Members.Num(); ++MemberIndex)
		{
			const auto& Member = Group.Members[MemberIndex];
			const FString Uid = SourceMembers[MemberIndex]->AsString();
			const FString RoundTrip = Member.bHasSource ? Member.Name + TEXT("|") + Member.SourceId : Member.Name;
			TestTrue(TEXT("Source UID and parsed member preserve order, case and omission"),
				Member.SourceUid == Uid && RoundTrip == Uid && Member.bHasSource == Uid.Contains(TEXT("|")) &&
				(Member.bHasSource || Member.SourceId.IsEmpty()));
			ExplicitCount += Member.bHasSource;
		}
	}
	TestEqual(TEXT("Unique group IDs"), Ids.Num(), 119);
	TestEqual(TEXT("Member declarations"), MemberCount, 669);
	TestEqual(TEXT("Source-qualified declarations"), ExplicitCount, 511);
	TestEqual(TEXT("Hidden groups"), HiddenCount, 12);
	return !HasAnyErrors();
}
#endif
