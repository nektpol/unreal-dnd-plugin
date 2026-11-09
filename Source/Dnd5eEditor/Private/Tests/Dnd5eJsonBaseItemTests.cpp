// Developed by Neko Creative Technologies
#if WITH_DEV_AUTOMATION_TESTS
#include "Dnd5eTestSupport.h"
#include "Items/Dnd5eItemIdentity.h"
#include "Readers/Items/Dnd5eJsonBaseItemLibrary.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"

namespace
{
	FDnd5eJsonDocument BaseItemDocument(const FString& Json)
	{
		FDnd5eJsonDocument Document;
		Document.Context.DatasetNamespace = TEXT("items-test");
		Document.Context.RelativeFilePath = TEXT("fixtures/base-items.json");
		Document.JsonText = Json;
		return Document;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eBaseItemConversionTest, "Dnd5e.Items.BaseJson.Conversion", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FDnd5eBaseItemConversionTest::RunTest(const FString& Parameters)
{
	const auto Result = UDnd5eJsonBaseItemLibrary::ReadBaseItems(BaseItemDocument(TEXT(R"json({
		"baseitem":[{"name":"Invented Gear","source":"SYN-A","type":"T1",
		"property":[{"uid":"P9|SYN-A","note":"Invented note"}],
		"mastery":[{"uid":"Invented Mastery|SYN-A","note":"Invented mastery note"}],
		"rarity":"none","weight":0.25,"value":12.5,"page":0,"basicRules2024":false,
		"entries":["Invented description."],"ammoType":"SYN-AMMO"}]
	})json")));
	if (!TestTrue(TEXT("One item through the Blueprint library"), Result.Status == EDnd5eJsonReadStatus::Success && Result.Items.Num() == 1)) { return false; }
	const auto& Item = Result.Items[0];
	TestEqual(TEXT("Canonical key"), Item.Header.RecordId.SourceKey, FString(TEXT("dnd5e:item:v1:invented gear|syn-a")));
	TestTrue(TEXT("Type keeps omitted source and property keeps explicit source"), Item.bHasType && !Item.Type.bHasSource &&
		Item.Properties.Num() == 1 && Item.Properties[0].bHasSource && Item.Properties[0].SourceId == TEXT("SYN-A"));
	TestTrue(TEXT("Mastery note is converted"), Item.Masteries.Num() == 1 && Item.Masteries[0].Note.Num() == 1);
	TestTrue(TEXT("Fractional price and weight retained"), Item.Price.CostPresence == EDnd5eValuePresence::Present &&
		Item.Price.Cost.Num() == 1 && Item.Price.Cost[0].Currency == EDnd5eCurrency::Copper &&
		Item.Price.Cost[0].Amount.SourceLiteral == TEXT("12.5") && Item.bHasWeight &&
		Item.Weight.Unit == EDnd5eMassUnit::Pounds && Item.Weight.Amount.SourceLiteral == TEXT("0.25"));
	if (!TestEqual(TEXT("One primary origin"), Item.Header.Provenance.Num(), 1)) { return false; }
	const auto& Origin = Item.Header.Provenance[0];
	TestTrue(TEXT("Page zero and unconverted equipment and property note stay as evidence"),
		Origin.Page.Kind == EDnd5ePageReferenceKind::Unspecified &&
		Origin.AdditionalMetadataJson.Contains(TEXT("\"page\"")) &&
		Origin.AdditionalMetadataJson.Contains(TEXT("Invented note")) &&
		Origin.AdditionalMetadataJson.Contains(TEXT("SYN-AMMO")));
	TestTrue(TEXT("Explicit false membership stays excluded"), Origin.Memberships.Num() == 1 &&
		Origin.Memberships[0].Inclusion == EDnd5eSourceInclusion::Excluded);
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eBaseItemFailureTest, "Dnd5e.Items.BaseJson.FailureAndPresence", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FDnd5eBaseItemFailureTest::RunTest(const FString& Parameters)
{
	const auto Empty = FDnd5eJsonBaseItemReader::Read(BaseItemDocument(TEXT(R"({"baseitem":[]})")));
	TestTrue(TEXT("Explicit empty array succeeds"), Empty.Status == EDnd5eJsonReadStatus::Success && Empty.Items.IsEmpty());
	const auto Invalid = FDnd5eJsonBaseItemReader::Read(BaseItemDocument(TEXT(R"({"baseitem":[
		{"name":"Gear","source":"SYN","type":"T","rarity":"none"},
		{"name":"gear","source":"syn","type":"T","rarity":"none"}]})")));
	TestTrue(TEXT("Case-folded duplicate discards staged rows"), Invalid.Status == EDnd5eJsonReadStatus::Failure &&
		Invalid.Items.IsEmpty() && Invalid.Diagnostics.Num() == 1 && Invalid.Diagnostics[0].JsonPointer == TEXT("/baseitem/1"));
	const auto Price = FDnd5eJsonBaseItemReader::Read(BaseItemDocument(TEXT(R"({"baseitem":[
		{"name":"Null","source":"SYN","type":"T","rarity":"none","value":null},
		{"name":"Custom","source":"SYN","type":"T","rarity":"none","value":7,"currencyConversion":"SYN","weight":2}]})")));
	if (!TestTrue(TEXT("Both price states read"), Price.Status == EDnd5eJsonReadStatus::Success && Price.Items.Num() == 2)) { return false; }
	TestTrue(TEXT("Null and custom currency are distinct"), Price.Items[0].Price.CostPresence == EDnd5eValuePresence::ExplicitNull &&
		Price.Items[1].Price.CostPresence == EDnd5eValuePresence::Unspecified && Price.Items[1].bHasWeight);
	FDnd5eRecordId Id;
	TestTrue(TEXT("Item ID escapes delimiters"), Dnd5e::Items::TryMakeId(TEXT("scope"), TEXT("A|B"), TEXT("S:%"), Id) &&
		Id.SourceKey == TEXT("dnd5e:item:v1:a%7Cb|s%3A%25"));
	TestTrue(TEXT("Failed construction clears old ID"), !Dnd5e::Items::TryMakeId(TEXT("scope"), TEXT(""), TEXT("S"), Id) && Id == FDnd5eRecordId{});
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eBaseItemEquipmentTest, "Dnd5e.Items.BaseJson.Equipment", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FDnd5eBaseItemEquipmentTest::RunTest(const FString& Parameters)
{
	const auto Result = FDnd5eJsonBaseItemReader::Read(BaseItemDocument(TEXT(R"json({"baseitem":[
		{"name":"Launcher","source":"SYN","type":"T","rarity":"none","weaponCategory":"martial",
		 "dmg1":"1","dmg2":"1d8","dmgType":"P","range":"20/60","reload":2},
		{"name":"Armor","source":"SYN","type":"MA|SYN","rarity":"none","ac":14,"strength":"13","stealth":false},
		{"name":"Shield","source":"SYN","type":"S","rarity":"none","ac":2},
		{"name":"Custom","source":"SYN","type":"MA","rarity":"none","ac":14,"dexterityMax":1,
		 "dmg1":"1d6","dmgType":"custom","strength":null,"Range":"retained"}
	]})json")));
	if (!TestTrue(TEXT("Equipment fixture reads"), Result.Status == EDnd5eJsonReadStatus::Success && Result.Items.Num() == 4)) { return false; }
	const auto& Weapon = Result.Items[0].WeaponData;
	TestTrue(TEXT("Weapon category, damage and range are typed"), Result.Items[0].bHasWeaponData &&
		Weapon.Category == EDnd5eWeaponCategory::Martial && Weapon.bHasPrimaryDamage &&
		Weapon.PrimaryDamage.Amount.Expression == TEXT("1") && Weapon.PrimaryDamage.Type == Dnd5e::Damage::Type::Piercing &&
		Weapon.bHasAlternateDamage && Weapon.AlternateDamage.Amount.Expression == TEXT("1d8") &&
		Weapon.bHasRange && Weapon.Range.bHasLong && Weapon.Range.Normal.Amount.Integer == 20 &&
		Weapon.Range.Long.Amount.Integer == 60 && Weapon.Range.Long.Unit == EDnd5eDistanceUnit::Feet &&
		Weapon.bHasReload && Weapon.Reload == 2);
	const FString& Evidence = Result.Items[0].Header.Provenance[0].AdditionalMetadataJson;
	TestTrue(TEXT("Consume exact equipment fields"),
		!Evidence.Contains(TEXT("\"range\""), ESearchCase::CaseSensitive) &&
		!Evidence.Contains(TEXT("\"dmg1\""), ESearchCase::CaseSensitive));
	const auto& Armor = Result.Items[1].ArmorData;
	TestTrue(TEXT("Medium AC and explicit false stealth are represented"), Result.Items[1].bHasArmorData &&
		Armor.bHasArmorClass && Armor.ArmorClass.Kind == EDnd5eArmorClassValueKind::Base &&
		Armor.ArmorClass.Value.Integer == 14 && Armor.ArmorClass.DexterityMode == EDnd5eArmorClassDexterityMode::Capped &&
		Armor.ArmorClass.DexterityModifierMaximum == 2 && Armor.StrengthPresence == EDnd5eValuePresence::Present &&
		Armor.StrengthMinimum.Minimum == 13 && Armor.bHasStealthDisadvantage && !Armor.bStealthDisadvantage);
	const auto& Shield = Result.Items[2].ArmorData;
	TestTrue(TEXT("Shield is an AC bonus"), Shield.bHasArmorClass && Shield.ArmorClass.Kind == EDnd5eArmorClassValueKind::Bonus &&
		Shield.ArmorClass.DexterityMode == EDnd5eArmorClassDexterityMode::NotAdded);
	const auto& Custom = Result.Items[3];
	const auto& CustomEvidence = Custom.Header.Provenance[0].AdditionalMetadataJson;
	TestTrue(TEXT("Unknown damage and overridden AC remain raw with explicit null Strength"),
		!Custom.bHasWeaponData && !Custom.ArmorData.bHasArmorClass &&
		Custom.ArmorData.StrengthPresence == EDnd5eValuePresence::ExplicitNull &&
		CustomEvidence.Contains(TEXT("dexterityMax")) && CustomEvidence.Contains(TEXT("\"ac\"")) &&
		CustomEvidence.Contains(TEXT("custom")) && CustomEvidence.Contains(TEXT("1d6")) &&
		CustomEvidence.Contains(TEXT("\"Range\""), ESearchCase::CaseSensitive));
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eBaseItemEquipmentFailureTest, "Dnd5e.Items.BaseJson.EquipmentFailure", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FDnd5eBaseItemEquipmentFailureTest::RunTest(const FString& Parameters)
{
	const auto Result = FDnd5eJsonBaseItemReader::Read(BaseItemDocument(TEXT(R"json({"baseitem":[
		{"name":"Valid","source":"SYN","type":"T","rarity":"none"},
		{"name":"Invalid","source":"SYN","type":"T","rarity":"none","range":"60/20"}
	]})json")));
	TestTrue(TEXT("Invalid range discards all staged rows and locates the error"),
		Result.Status == EDnd5eJsonReadStatus::Failure && Result.Items.IsEmpty() &&
		Result.Diagnostics.Num() == 1 && Result.Diagnostics[0].JsonPointer == TEXT("/baseitem/1/range"));
	return !HasAnyErrors();
}

IMPLEMENT_COMPLEX_AUTOMATION_TEST(FDnd5eBaseItemSnapshotTest, "Dnd5e.Items.BaseJson.External", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
void FDnd5eBaseItemSnapshotTest::GetTests(TArray<FString>& Names, TArray<FString>& Commands) const
{
	FString Path;
	if (FParse::Value(FCommandLine::Get(), TEXT("Dnd5eJsonDefinitionFile="), Path))
	{
		Names.Add(TEXT("InspectedSnapshot"));
		Commands.Add(Path);
	}
}

bool FDnd5eBaseItemSnapshotTest::RunTest(const FString& Parameters)
{
	FString Text;
	if (!TestTrue(TEXT("Read supplied local snapshot"), FFileHelper::LoadFileToString(Text, *Parameters))) { return false; }
	const auto Result = FDnd5eJsonBaseItemReader::Read(BaseItemDocument(Text));
	for (const auto& Diagnostic : Result.Diagnostics)
	{
		if (Diagnostic.Severity == EDnd5eDiagnosticSeverity::Error)
		{
			AddError(Diagnostic.JsonPointer + TEXT(" ") + Diagnostic.Message);
		}
	}
	TestTrue(TEXT("Read all 230 base items"), Result.Status == EDnd5eJsonReadStatus::Success && Result.Items.Num() == 230);
	int32 Categories = 0, Primary = 0, Alternate = 0, Ranges = 0, Reloads = 0, AC = 0, Strength = 0, Stealth = 0;
	for (const auto& Item : Result.Items)
	{
		Categories += Item.WeaponData.Category != EDnd5eWeaponCategory::Unspecified;
		Primary += Item.WeaponData.bHasPrimaryDamage;
		Alternate += Item.WeaponData.bHasAlternateDamage;
		Ranges += Item.WeaponData.bHasRange;
		Reloads += Item.WeaponData.bHasReload;
		AC += Item.ArmorData.bHasArmorClass;
		Strength += Item.ArmorData.StrengthPresence == EDnd5eValuePresence::Present;
		Stealth += Item.ArmorData.bHasStealthDisadvantage;
	}
	TestTrue(TEXT("Typed weapon counts match snapshot"), Categories == 102 && Primary == 101 &&
		Alternate == 17 && Ranges == 52 && Reloads == 16);
	TestTrue(TEXT("Typed armor counts match snapshot"), AC == 27 && Strength == 6 && Stealth == 15);
	return !HasAnyErrors();
}
#endif
