// Developed by Neko Creative Technologies
#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Catalog/Dnd5eItemCatalogObject.h"
#include "Dnd5eTags.h"

namespace
{
	FDnd5eItem MakeCatalogItem(const TCHAR* Dataset)
	{
		FDnd5eItem Item;
		Item.Header.DisplayName = TEXT("Invented Compass");
		Item.Header.Aliases.Add(TEXT("Wayfinder"));
		Item.Header.RecordId.DatasetNamespace = Dataset;
		Item.Header.RecordId.Category = Dnd5e::Category::Item;
		Item.Header.RecordId.SourceKey = TEXT("invented-compass");
		auto& Origin = Item.Header.Provenance.AddDefaulted_GetRef();
		Origin.SourceId = TEXT("SYN");
		Origin.RelativeFilePath = TEXT("fixtures/items.json");
		Origin.JsonPointer = TEXT("/item/0");
		Item.Rarity = Dnd5e::Item::Rarity::Rare;
		Item.ChargeData.Charges.Fixed = 3;
		return Item;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eItemCatalogQueryTest, "Dnd5e.Items.Catalog.QueryAndLookup", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FDnd5eItemCatalogQueryTest::RunTest(const FString& Parameters)
{
	auto Classic = MakeCatalogItem(TEXT("guide-2014"));
	const auto Modern = MakeCatalogItem(TEXT("guide-2024"));
	const auto ClassicId = Classic.Header.RecordId;
	FDnd5eItemGroup Group;
	Group.Details = Classic;
	Group.Details.Header.RecordId.Category = Dnd5e::Category::ItemGroup;
	Group.Members.AddDefaulted_GetRef().Name = TEXT("Invented Compass");
	const auto GroupId = Group.Details.Header.RecordId;
	auto* Catalog = NewObject<UDnd5eItemCatalogObject>();
	TArray<FDnd5eDiagnostic> Diagnostics;
	if (!TestTrue(TEXT("Mixed typed records enter the reflected catalog"),
		Catalog->ReplaceRecords({Classic, Modern}, {Group}, Diagnostics) && Diagnostics.IsEmpty())) { return false; }
	Classic.Header.DisplayName.Reset();
	Group.Members.Reset();
	TArray<FDnd5eRecordId> Ids;
	FDnd5eItemCatalogQuery Query;
	const TArray<FDnd5eRecordId> Ordered{ClassicId, Modern.Header.RecordId, GroupId};
	TestTrue(TEXT("Unfiltered query retains item and group input order"),
		Catalog->QueryRecordIds(Query, Ids, Diagnostics) && Ids == Ordered && Catalog->GetRecordCount() == 3);
	Query.DatasetNamespace = TEXT("guide-2014");
	Query.SourceId = TEXT("SYN");
	Query.Rarity = Dnd5e::Item::Rarity::Rare;
	Query.NameContains = TEXT("WAYFIND");
	TestTrue(TEXT("Combined filters match aliases case-insensitively within one dataset"),
		Catalog->QueryRecordIds(Query, Ids, Diagnostics) && Ids.Num() == 2 && Ids[0] == ClassicId && Ids[1] == GroupId);
	Query.bIncludeAliases = false;
	TestTrue(TEXT("Aliases can be excluded"), Catalog->QueryRecordIds(Query, Ids, Diagnostics) && Ids.IsEmpty());
	Query.NameContains = TEXT("compass");
	Query.Category = Dnd5e::Category::ItemGroup;
	TestTrue(TEXT("Category selects the group"), Catalog->QueryRecordIds(Query, Ids, Diagnostics) && Ids.Num() == 1 && Ids[0] == GroupId);
	Query.SourceId = TEXT("syn");
	TestTrue(TEXT("Publication filter uses exact spelling"), Catalog->QueryRecordIds(Query, Ids, Diagnostics) && Ids.IsEmpty());
	FDnd5eItemGroup FoundGroup;
	TestTrue(TEXT("Group lookup retains independently owned members"),
		Catalog->TryGetGroup(GroupId, FoundGroup) && FoundGroup.Members.Num() == 1);
	FDnd5eItem Found;
	TestTrue(TEXT("Item lookup retains the typed data"), Catalog->TryGetItem(ClassicId, Found) &&
		Found.Header.DisplayName == TEXT("Invented Compass") && Found.ChargeData.Charges.Fixed == 3);
	Found.ChargeData.Charges.Fixed = 42;
	TestTrue(TEXT("Mutating a lookup result leaves the stored record intact"),
		Catalog->TryGetItem(ClassicId, Found) && Found.ChargeData.Charges.Fixed == 3);
	TestTrue(TEXT("Wrong category lookup resets output"), !Catalog->TryGetItem(GroupId, Found) && Found.Header.RecordId == FDnd5eRecordId{});
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eItemCatalogReplacementTest, "Dnd5e.Items.Catalog.AtomicReplacement", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FDnd5eItemCatalogReplacementTest::RunTest(const FString& Parameters)
{
	FDnd5eItemCatalog Catalog;
	const auto Item = MakeCatalogItem(TEXT("guide-2014"));
	TArray<FDnd5eDiagnostic> Diagnostics;
	if (!TestTrue(TEXT("Initial record loads"), Catalog.ReplaceRecords({Item}, {}, Diagnostics))) { return false; }
	TestTrue(TEXT("Duplicate item rejects replacement and identifies the source row"),
		!Catalog.ReplaceRecords({Item, Item}, {}, Diagnostics) && Diagnostics.Num() == 1 &&
		Diagnostics[0].Code == FName(TEXT("Dnd5e.Catalog.DuplicateRecordId")) &&
		Diagnostics[0].RelativeFilePath == TEXT("fixtures/items.json") && Diagnostics[0].JsonPointer == TEXT("/item/0"));
	FDnd5eItemGroup Group;
	Group.Details = Item;
	TestTrue(TEXT("Group category is validated before replacement"),
		!Catalog.ReplaceRecords({}, {Group}, Diagnostics) && Diagnostics.Num() == 1 &&
		Diagnostics[0].Code == FName(TEXT("Dnd5e.Catalog.InvalidItemRecord")));
	FDnd5eItem Found;
	TestTrue(TEXT("Previous record remains queryable"), Catalog.GetRecordCount() == 1 &&
		Catalog.TryGetItem(Item.Header.RecordId, Found));
	FDnd5eItemCatalogQuery Query;
	Query.Category = Dnd5e::Category::Spell;
	TArray<FDnd5eRecordId> Ids{Item.Header.RecordId};
	TestTrue(TEXT("Invalid query clears stale IDs"), !Catalog.QueryRecordIds(Query, Ids, Diagnostics) &&
		Ids.IsEmpty() && Diagnostics.Num() == 1);
	TestTrue(TEXT("Empty replacement clears the catalog"), Catalog.ReplaceRecords({}, {}, Diagnostics) &&
		Diagnostics.IsEmpty() && Catalog.GetRecordCount() == 0);
	return !HasAnyErrors();
}
#endif
