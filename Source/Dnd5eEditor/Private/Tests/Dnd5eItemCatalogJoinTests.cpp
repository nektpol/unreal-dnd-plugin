// Developed by Neko Creative Technologies
#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Catalog/Dnd5eItemCatalogObject.h"
#include "Dnd5eTags.h"
#include "Items/Dnd5eItemLibrary.h"
#include "Readers/Items/Dnd5eJsonItemDefinitionCatalogLibrary.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eDefinitionDocumentsTest, "Dnd5e.Items.Joins.DefinitionDocuments", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FDnd5eDefinitionDocumentsTest::RunTest(const FString& Parameters)
{
	FDnd5eJsonDocument Classic;
	Classic.Context.DatasetNamespace = TEXT("classic");
	Classic.Context.RelativeFilePath = TEXT("classic.json");
	Classic.JsonText = TEXT(R"({"itemType":[{"name":"Classic Blade","source":"SYN","abbreviation":"M"}]})");
	FDnd5eJsonDocument Modern = Classic;
	Modern.Context.DatasetNamespace = TEXT("modern");
	Modern.Context.RelativeFilePath = TEXT("modern.json");
	Modern.JsonText = TEXT(R"({"itemType":[{"name":"Modern Blade","source":"SYN","abbreviation":"M"}]})");
	auto* Catalog = NewObject<UDnd5eItemDefinitionCatalogObject>();
	TArray<FDnd5eDiagnostic> Diagnostics;
	using FLibrary = UDnd5eJsonItemDefinitionCatalogLibrary;
	if (!TestTrue(TEXT("Both namespaces publish"), FLibrary::BuildItemDefinitionCatalogFromDocuments(Catalog, {Classic, Modern}, Diagnostics) &&
		Catalog->GetDefinitionCount() == 2)) { return false; }
	FDnd5eItemDefinitionSelector Selector;
	Selector.Category = Dnd5e::Category::ItemType;
	Selector.Abbreviation = TEXT("M");
	Selector.bHasSource = true;
	Selector.SourceId = TEXT("SYN");
	TArray<FDnd5eRecordId> Ids;
	FDnd5eItemTypeDefinition Type;
	TestTrue(TEXT("Modern namespace resolves its own definition"),
		Catalog->MatchDefinitionSelector(TEXT("modern"), Selector, Ids, Diagnostics) == EDnd5eItemDefinitionMatchStatus::Matched &&
		Ids.Num() == 1 && Catalog->TryGetType(Ids[0], Type) && Type.Header.DisplayName == TEXT("Modern Blade"));
	TestTrue(TEXT("Duplicate documents preserve active definitions"),
		!FLibrary::BuildItemDefinitionCatalogFromDocuments(Catalog, {Classic, Classic}, Diagnostics) &&
		Catalog->GetDefinitionCount() == 2 && !Diagnostics.IsEmpty());
	Modern.JsonText = TEXT("[");
	TestTrue(TEXT("A later malformed document preserves active definitions"),
		!FLibrary::BuildItemDefinitionCatalogFromDocuments(Catalog, {Classic, Modern}, Diagnostics) &&
		Catalog->GetDefinitionCount() == 2 && Diagnostics.Num() == 1 && Diagnostics[0].RelativeFilePath == TEXT("modern.json"));
	TestTrue(TEXT("Empty build clears definitions"),
		FLibrary::BuildItemDefinitionCatalogFromDocuments(Catalog, {}, Diagnostics) && Catalog->GetDefinitionCount() == 0);
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eItemMemberMatchTest, "Dnd5e.Items.Joins.GroupMember", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FDnd5eItemMemberMatchTest::RunTest(const FString& Parameters)
{
	FDnd5eItem Item;
	Item.Header.RecordId.DatasetNamespace = TEXT("classic");
	Item.Header.RecordId.Category = Dnd5e::Category::Item;
	Item.Header.RecordId.SourceKey = TEXT("ordinary");
	Item.Header.DisplayName = TEXT("Invented Blade");
	Item.Header.Provenance.AddDefaulted_GetRef().SourceId = TEXT("SYN");
	FDnd5eItem Variant = Item;
	Variant.Header.RecordId.SourceKey = TEXT("generated-pair");
	FDnd5eItem Modern = Item;
	Modern.Header.RecordId.DatasetNamespace = TEXT("modern");
	FDnd5eItemGroup Group;
	Group.Details = Item;
	Group.Details.Header.RecordId.Category = Dnd5e::Category::ItemGroup;
	Group.Details.Header.RecordId.SourceKey = TEXT("group");
	auto* Catalog = NewObject<UDnd5eItemCatalogObject>();
	TArray<FDnd5eDiagnostic> Diagnostics;
	if (!TestTrue(TEXT("Fixture publishes"), Catalog->ReplaceRecords({Item, Variant, Modern}, {Group}, Diagnostics))) { return false; }
	FDnd5eItemGroupMember Selector;
	Selector.Name = TEXT("INVENTED BLADE");
	Selector.bHasSource = true;
	Selector.SourceId = TEXT("syn");
	TArray<FDnd5eRecordId> Ids;
	using EStatus = EDnd5eItemSelectorMatchStatus;
	TestTrue(TEXT("Same authored identity returns stored IDs in order"),
		Catalog->MatchItemSelector(TEXT("classic"), Selector, Ids) == EStatus::Ambiguous &&
		Ids == TArray<FDnd5eRecordId>{Item.Header.RecordId, Variant.Header.RecordId});
	TestTrue(TEXT("Modern match ignores classic records"),
		Catalog->MatchItemSelector(TEXT("modern"), Selector, Ids) == EStatus::Matched && Ids == TArray<FDnd5eRecordId>{Modern.Header.RecordId});
	Selector.bHasSource = false;
	TestTrue(TEXT("A single name candidate still requires a publication"),
		Catalog->MatchItemSelector(TEXT("modern"), Selector, Ids) == EStatus::MissingSource && Ids.Num() == 1);
	Selector.bHasSource = true;
	Selector.SourceId.Reset();
	TestTrue(TEXT("Empty explicit source clears candidates"),
		Catalog->MatchItemSelector(TEXT("modern"), Selector, Ids) == EStatus::InvalidSelector && Ids.IsEmpty());
	Selector.SourceId = TEXT("OTHER");
	TestTrue(TEXT("Wrong publication returns no match"),
		Catalog->MatchItemSelector(TEXT("classic"), Selector, Ids) == EStatus::NoMatch && Ids.IsEmpty());
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eAttachedSpellMatchTest, "Dnd5e.Items.Joins.AttachedSpell", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FDnd5eAttachedSpellMatchTest::RunTest(const FString& Parameters)
{
	FDnd5eSpell Spell;
	Spell.Header.RecordId.DatasetNamespace = TEXT("classic");
	Spell.Header.RecordId.Category = Dnd5e::Category::Spell;
	Spell.Header.RecordId.SourceKey = TEXT("stored-beacon-id");
	Spell.Header.DisplayName = TEXT("Invented Beacon");
	Spell.Level = 0;
	Spell.Header.Provenance.AddDefaulted_GetRef().SourceId = TEXT("SYN");
	auto* Catalog = NewObject<UDnd5eSpellCatalogObject>();
	TArray<FDnd5eDiagnostic> Diagnostics;
	if (!TestTrue(TEXT("Spell fixture publishes"), Catalog->ReplaceSpells({Spell}, Diagnostics))) { return false; }
	FDnd5eItemAttachedSpell Attached;
	Attached.Selector.Category = Dnd5e::Category::Spell;
	Attached.Selector.Name = TEXT("invented beacon");
	Attached.Selector.bHasSource = true;
	Attached.Selector.SourceId = TEXT("syn");
	TArray<FDnd5eRecordId> Ids;
	using EStatus = EDnd5eSpellSelectorMatchStatus;
	using FLibrary = UDnd5eItemLibrary;
	TestTrue(TEXT("Authored selector resolves to the loaded canonical ID"),
		FLibrary::MatchItemAttachedSpell(TEXT("classic"), Attached, Catalog, Ids, Diagnostics) == EStatus::Matched &&
		Ids == TArray<FDnd5eRecordId>{Spell.Header.RecordId} && Diagnostics.IsEmpty());
	TestTrue(TEXT("Unloaded namespace clears candidates"),
		FLibrary::MatchItemAttachedSpell(TEXT("modern"), Attached, Catalog, Ids, Diagnostics) == EStatus::NoMatch && Ids.IsEmpty());
	Attached.Selector.bHasSource = false;
	TestTrue(TEXT("Source flag controls the lookup even with a populated string"),
		FLibrary::MatchItemAttachedSpell(TEXT("classic"), Attached, Catalog, Ids, Diagnostics) == EStatus::MissingSource && Ids.Num() == 1);
	TestTrue(TEXT("Null catalog clears outputs and reports invalid selector"),
		FLibrary::MatchItemAttachedSpell(TEXT("classic"), Attached, nullptr, Ids, Diagnostics) == EStatus::InvalidSelector &&
		Ids.IsEmpty() && Diagnostics.Num() == 1);
	return !HasAnyErrors();
}
#endif
