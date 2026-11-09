// Developed by Neko Creative Technologies
#if WITH_DEV_AUTOMATION_TESTS
#include "Dnd5eTestSupport.h"
#include "Readers/Items/Dnd5eJsonItemLibrary.h"

namespace
{
	FDnd5eJsonDocument StandaloneItemDocument(const TCHAR* Text)
	{
		FDnd5eJsonDocument Document;
		Document.Context.DatasetNamespace = TEXT("ordinary-test");
		Document.Context.RelativeFilePath = TEXT("fixtures/items.json");
		Document.JsonText = Text;
		return Document;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eOrdinaryItemConversionTest, "Dnd5e.Items.OrdinaryJson.Conversion", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FDnd5eOrdinaryItemConversionTest::RunTest(const FString& Parameters)
{
	const auto Document = StandaloneItemDocument(TEXT(R"json({"item":[
		{"name":"Charm","source":"SYN","rarity":"rare","reqAttune":"Invented condition",
		 "reqAttuneTags":[{"background":"SYN"}]},
		{"name":"Thrower","source":"SYN","rarity":"uncommon","type":"T1","range":"20",
		 "dmg1":"1d6","dmgType":"O","reqAttune":false,"reqAttuneAlt":"optional"},
		{"name":"Plain","source":"SYN","rarity":"unknown (magic)"}
	]})json"));
	const auto Charm = UDnd5eJsonItemLibrary::ReadItemAtIndex(Document, 0);
	if (!TestTrue(TEXT("Read selected row without a type"), Charm.Status == EDnd5eJsonReadStatus::Success &&
		!Charm.Item.bHasType && Charm.Item.Rarity == Dnd5e::Item::Rarity::Rare &&
		Charm.Item.Header.Provenance.Num() == 1)) { return false; }
	TestTrue(TEXT("Readable condition and raw index are separate"), Charm.Item.Attunement.Mode == EDnd5eAttunementMode::Required &&
		Charm.Item.Attunement.Conditions.Num() == 1 && !Charm.Item.Attunement.bHasIndex &&
		Charm.Item.Header.Provenance[0].AdditionalMetadataJson.Contains(TEXT("reqAttuneTags")));
	TestEqual(TEXT("Selected row pointer"), Charm.Item.Header.Provenance[0].JsonPointer, FString(TEXT("/item/0")));

	const auto Weapon = FDnd5eJsonItemReader::ReadAtIndex(Document, 1);
	TestTrue(TEXT("Single range and additional damage code are typed"), Weapon.Status == EDnd5eJsonReadStatus::Success &&
		Weapon.Item.WeaponData.bHasRange && !Weapon.Item.WeaponData.Range.bHasLong &&
		Weapon.Item.WeaponData.Range.Normal.Amount.Integer == 20 &&
		Weapon.Item.WeaponData.PrimaryDamage.Type == Dnd5e::Damage::Type::Force);
	TestTrue(TEXT("False primary and optional alternative keep independent presence"),
		Weapon.Item.Attunement.Mode == EDnd5eAttunementMode::NotRequired && Weapon.Item.bHasAlternativeAttunement &&
		Weapon.Item.AlternativeAttunement.Mode == EDnd5eAttunementMode::Optional);
	const auto Plain = FDnd5eJsonItemReader::ReadAtIndex(Document, 2);
	TestTrue(TEXT("Omitted attunement and explicit rarity stay distinct"), Plain.Status == EDnd5eJsonReadStatus::Success &&
		Plain.Item.Attunement.Mode == EDnd5eAttunementMode::Unspecified && !Plain.Item.bHasAlternativeAttunement &&
		Plain.Item.Rarity == Dnd5e::Item::Rarity::UnknownMagic);
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eOrdinaryItemFailureTest, "Dnd5e.Items.OrdinaryJson.SelectionAndFailure", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FDnd5eOrdinaryItemFailureTest::RunTest(const FString& Parameters)
{
	const auto Document = StandaloneItemDocument(TEXT(R"json({"item":[
		{"name":"Dependent","source":"SYN","rarity":"none","baseItem":"Base|SYN"},
		{"name":"Copy","source":"SYN","rarity":"none","_copy":{}},
		{"name":"Bad condition","source":"SYN","rarity":"none","reqAttune":7},
		{"name":"Valid","source":"SYN","rarity":"none"}
	]})json"));
	struct FCase { int32 Index; const TCHAR* Pointer; };
	for (const FCase& Case : {FCase{-1, TEXT("/item")}, {4, TEXT("/item")},
		{0, TEXT("/item/0/baseItem")}, {1, TEXT("/item/1/_copy")}, {2, TEXT("/item/2/reqAttune")}})
	{
		const auto Result = FDnd5eJsonItemReader::ReadAtIndex(Document, Case.Index);
		TestTrue(FString::Printf(TEXT("Failure clears output and identifies %s"), Case.Pointer),
			Result.Status == EDnd5eJsonReadStatus::Failure && Result.Item.Header.RecordId == FDnd5eRecordId{} &&
			Result.Diagnostics.Num() == 1 && Result.Diagnostics[0].JsonPointer == Case.Pointer);
	}
	TestTrue(TEXT("A valid row can be selected after unsupported rows"),
		FDnd5eJsonItemReader::ReadAtIndex(Document, 3).Status == EDnd5eJsonReadStatus::Success);
	return !HasAnyErrors();
}
#endif
