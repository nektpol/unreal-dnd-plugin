// Developed by Neko Creative Technologies
#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Readers/Items/Dnd5eJsonItemCatalogBuilder.h"
#include "Dnd5eTags.h"

namespace
{
	FDnd5eJsonDocument CatalogBuildDocument(const TCHAR* Json, const TCHAR* Path, const TCHAR* Dataset = TEXT("guide-2014"))
	{
		FDnd5eJsonDocument Document;
		Document.JsonText = Json;
		Document.Context.DatasetNamespace = Dataset;
		Document.Context.RelativeFilePath = Path;
		return Document;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eItemCatalogBuildTest, "Dnd5e.Items.CatalogBuilder.SuppliedDocuments", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FDnd5eItemCatalogBuildTest::RunTest(const FString& Parameters)
{
	FDnd5eJsonItemCatalogBuildRequest Request;
	Request.BaseDocuments.Add(CatalogBuildDocument(TEXT(R"({"baseitem":[{"name":"Blade","source":"SYN","type":"M|SYN","rarity":"none"}]})"), TEXT("base.json")));
	Request.ItemDocuments.Add(CatalogBuildDocument(TEXT(R"({"item":[
		{"name":"Compass","source":"SYN","rarity":"rare","charges":3},
		{"name":"Copy","source":"SYN","_copy":{"name":"Compass","source":"SYN"}}],
		"itemGroup":[{"name":"Family","source":"SYN","rarity":"rare","items":["Compass|SYN"]}]})"), TEXT("items.json")));
	Request.ItemDocuments.Add(CatalogBuildDocument(TEXT(R"({"item":[{"name":"Compass","source":"SYN","rarity":"rare","charges":5}]})"),
		TEXT("items-modern.json"), TEXT("guide-2024")));
	auto* Catalog = NewObject<UDnd5eItemCatalogObject>();
	const auto Result = UDnd5eJsonItemCatalogBuilder::BuildItemCatalog(Request, Catalog);
	if (!TestTrue(TEXT("Both namespaces and every supplied record kind are published"), Result.Status == EDnd5eJsonReadStatus::Success &&
		Result.RecordCount == 5 && Catalog->GetRecordCount() == 5 && Result.Diagnostics.IsEmpty())) { return false; }
	FDnd5eItemCatalogQuery Query;
	Query.DatasetNamespace = TEXT("guide-2014");
	TArray<FDnd5eRecordId> Ids;
	TArray<FDnd5eDiagnostic> Diagnostics;
	if (!TestTrue(TEXT("Classic namespace keeps base, standalone, copy and group in order"),
		Catalog->QueryRecordIds(Query, Ids, Diagnostics) && Ids.Num() == 4)) { return false; }
	FDnd5eItem Item;
	FDnd5eItemGroup Group;
	TestTrue(TEXT("Base record comes first"), Catalog->TryGetItem(Ids[0], Item) && Item.Header.DisplayName == TEXT("Blade"));
	TestTrue(TEXT("Copy retains typed data and both source rows"), Catalog->TryGetItem(Ids[2], Item) &&
		Item.Header.DisplayName == TEXT("Copy") && Item.ChargeData.Charges.Fixed == 3 && Item.Header.Provenance.Num() == 2 &&
		Item.Header.Provenance[0].JsonPointer == TEXT("/item/1") && Item.Header.Provenance[1].JsonPointer == TEXT("/item/0"));
	TestTrue(TEXT("Group retains its member selector"), Catalog->TryGetGroup(Ids[3], Group) && Group.Members.Num() == 1 &&
		Group.Members[0].SourceUid == TEXT("Compass|SYN"));
	Query.DatasetNamespace = TEXT("guide-2024");
	TestTrue(TEXT("Modern record has an independent value"), Catalog->QueryRecordIds(Query, Ids, Diagnostics) &&
		Ids.Num() == 1 && Catalog->TryGetItem(Ids[0], Item) && Item.ChargeData.Charges.Fixed == 5);
	const auto* Function = UDnd5eJsonItemCatalogBuilder::StaticClass()->FindFunctionByName(TEXT("BuildItemCatalog"));
	TestTrue(TEXT("Builder exposes its Blueprint entry point"), Function && Function->HasAllFunctionFlags(FUNC_BlueprintCallable | FUNC_Static));
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eItemCatalogBuildFailureTest, "Dnd5e.Items.CatalogBuilder.AtomicFailure", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FDnd5eItemCatalogBuildFailureTest::RunTest(const FString& Parameters)
{
	FDnd5eJsonItemCatalogBuildRequest Request;
	const auto Valid = CatalogBuildDocument(TEXT(R"({"item":[{"name":"Old","source":"SYN","rarity":"rare"}]})"), TEXT("old.json"));
	Request.ItemDocuments = {Valid};
	auto* Catalog = NewObject<UDnd5eItemCatalogObject>();
	if (!TestTrue(TEXT("Initial build succeeds"),
		UDnd5eJsonItemCatalogBuilder::BuildItemCatalog(Request, Catalog).Status == EDnd5eJsonReadStatus::Success)) { return false; }
	const auto New = CatalogBuildDocument(TEXT(R"({"item":[{"name":"New","source":"SYN","rarity":"rare","unknownField":true}]})"), TEXT("new.json"));
	const auto Broken = CatalogBuildDocument(TEXT(R"({"item":[{"name":"Copy","source":"SYN","_copy":{"name":"Missing","source":"SYN"}}]})"), TEXT("broken.json"));
	Request.ItemDocuments = {New, Broken};
	const auto CopyFailure = UDnd5eJsonItemCatalogBuilder::BuildItemCatalog(Request, Catalog);
	TestTrue(TEXT("Copy failure retains earlier warnings and original diagnostics"), CopyFailure.Status == EDnd5eJsonReadStatus::Failure &&
		CopyFailure.Diagnostics.Num() == 2 && CopyFailure.Diagnostics[0].RelativeFilePath == TEXT("new.json") &&
		CopyFailure.Diagnostics[0].JsonPointer == TEXT("/item/0/unknownField") &&
		CopyFailure.Diagnostics[1].RelativeFilePath == TEXT("broken.json") && CopyFailure.Diagnostics[1].JsonPointer == TEXT("/item/0/_copy"));
	Request.ItemDocuments = {New, New};
	const auto Duplicate = UDnd5eJsonItemCatalogBuilder::BuildItemCatalog(Request, Catalog);
	TestTrue(TEXT("Replacement diagnostics append after reader warnings"), Duplicate.Status == EDnd5eJsonReadStatus::Failure &&
		Duplicate.Diagnostics.Num() == 3 && Duplicate.Diagnostics[2].Code == FName(TEXT("Dnd5e.Catalog.DuplicateRecordId")));
	Request.ItemDocuments = {CatalogBuildDocument(TEXT("["), TEXT("malformed.json"))};
	const auto Malformed = UDnd5eJsonItemCatalogBuilder::BuildItemCatalog(Request, Catalog);
	TestTrue(TEXT("Malformed JSON retains the document location"), Malformed.Status == EDnd5eJsonReadStatus::Failure &&
		Malformed.Diagnostics.Num() == 1 && Malformed.Diagnostics[0].RelativeFilePath == TEXT("malformed.json"));
	FDnd5eItemCatalogQuery Query;
	Query.NameContains = TEXT("Old");
	TArray<FDnd5eRecordId> Ids;
	TArray<FDnd5eDiagnostic> Diagnostics;
	TestTrue(TEXT("Every failed rebuild preserves the original record"), Catalog->GetRecordCount() == 1 &&
		Catalog->QueryRecordIds(Query, Ids, Diagnostics) && Ids.Num() == 1);
	const auto Empty = UDnd5eJsonItemCatalogBuilder::BuildItemCatalog({}, Catalog);
	TestTrue(TEXT("An empty request clears the catalog"), Empty.Status == EDnd5eJsonReadStatus::Success && Catalog->GetRecordCount() == 0);
	const auto Missing = UDnd5eJsonItemCatalogBuilder::BuildItemCatalog({}, nullptr);
	TestTrue(TEXT("A null target returns a diagnostic"), Missing.Status == EDnd5eJsonReadStatus::Failure && Missing.Diagnostics.Num() == 1);
	return !HasAnyErrors();
}
#endif
