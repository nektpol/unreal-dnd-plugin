// Developed by Neko Creative Technologies
#if WITH_DEV_AUTOMATION_TESTS
#include "Dnd5eTestSupport.h"
using namespace Dnd5eTests;

namespace
{
	FDnd5eNumber EquipmentInteger(int32 Value) { FDnd5eNumber Out; Out.Kind = EDnd5eNumberKind::Integer; Out.Integer = Value; Out.SourceLiteral = FString::FromInt(Value); return Out; }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eEquipmentOwnershipTest, "Dnd5e.Items.Equipment.OwnershipAndDamageVariants", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FDnd5eEquipmentOwnershipTest::RunTest(const FString& Parameters)
{
	FDnd5eItem Item; Item.bHasWeaponData = true; Item.WeaponData.Category = EDnd5eWeaponCategory::Martial;
	Item.WeaponData.bHasPrimaryDamage = true; Item.WeaponData.PrimaryDamage.Amount.Expression = TEXT("1"); Item.WeaponData.PrimaryDamage.Amount.Syntax = TEXT("synthetic"); Item.WeaponData.PrimaryDamage.Type = Dnd5e::Damage::Type::Slashing;
	Item.WeaponData.bHasAlternateDamage = true; Item.WeaponData.AlternateDamage.Amount.Expression = TEXT("2d6+2"); Item.WeaponData.AlternateDamage.Amount.Syntax = TEXT("synthetic"); Item.WeaponData.AlternateDamage.Type = Dnd5e::Damage::Type::Fire;
	Item.WeaponData.bHasRange = true; Item.WeaponData.Range.Normal.Amount = EquipmentInteger(20); Item.WeaponData.Range.Normal.Unit = EDnd5eDistanceUnit::Feet;
	Item.WeaponData.Range.bHasLong = true; Item.WeaponData.Range.Long.Amount = EquipmentInteger(60); Item.WeaponData.Range.Long.Unit = EDnd5eDistanceUnit::Feet;
	Item.WeaponData.bHasReload = true; Item.WeaponData.Reload = 4;
	Item.bHasArmorData = true; Item.ArmorData.bHasArmorClass = true; Item.ArmorData.ArmorClass.Value = EquipmentInteger(14); Item.ArmorData.ArmorClass.Kind = EDnd5eArmorClassValueKind::Base;
	Item.ArmorData.ArmorClass.DexterityMode = EDnd5eArmorClassDexterityMode::Capped; Item.ArmorData.ArmorClass.DexterityModifierMaximum = 2;
	Item.ArmorData.StrengthPresence = EDnd5eValuePresence::Present; Item.ArmorData.StrengthMinimum.Ability = Dnd5e::Ability::Strength; Item.ArmorData.StrengthMinimum.Minimum = 12;
	Item.ArmorData.bHasStealthDisadvantage = true; Item.ArmorData.bStealthDisadvantage = false;
	const auto Before = Item; FDnd5eItem Out;
	if (!RoundTrip(*this, FDnd5eItem::StaticStruct(), &Item, &Out)) { return false; }
	TestTrue(TEXT("Independent weapon/armor data round trip together"), Equal(FDnd5eItem::StaticStruct(), &Item, &Out));
	TestTrue(TEXT("Constant and dice expressions keep their independent amount/type pairs"), Out.WeaponData.PrimaryDamage.Amount.Expression == TEXT("1") && Out.WeaponData.PrimaryDamage.Type == Dnd5e::Damage::Type::Slashing && Out.WeaponData.AlternateDamage.Amount.Expression == TEXT("2d6+2") && Out.WeaponData.AlternateDamage.Type == Dnd5e::Damage::Type::Fire);
	TestTrue(TEXT("Normal and long distances remain separate with their units"), Out.WeaponData.Range.Normal.Amount.Integer == 20 && Out.WeaponData.Range.Long.Amount.Integer == 60 && Out.WeaponData.Range.Normal.Unit == EDnd5eDistanceUnit::Feet && Out.WeaponData.Range.Long.Unit == EDnd5eDistanceUnit::Feet);
	Out.WeaponData.PrimaryDamage.Amount.Expression = TEXT("changed primary"); Out.WeaponData.AlternateDamage.Type = Dnd5e::Damage::Type::Cold;
	Out.WeaponData.Range.Normal.Amount.SourceLiteral = TEXT("changed normal"); Out.WeaponData.Range.Long.Unit = EDnd5eDistanceUnit::Miles; Out.WeaponData.Reload = 99;
	Out.ArmorData.ArmorClass.Value.Integer = 3; Out.ArmorData.ArmorClass.DexterityModifierMaximum = 7; Out.ArmorData.StrengthMinimum.Minimum = 20;
	TestTrue(TEXT("Nested equipment mutations leave original data intact"), Equal(FDnd5eItem::StaticStruct(), &Item, &Before));
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eEquipmentPresenceTest, "Dnd5e.Items.Equipment.PresenceAndArmorClassModes", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FDnd5eEquipmentPresenceTest::RunTest(const FString& Parameters)
{
	FDnd5eItemArmorData Missing, Null, Zero, ExplicitFalse; Null.StrengthPresence = EDnd5eValuePresence::ExplicitNull;
	Zero.StrengthPresence = EDnd5eValuePresence::Present; Zero.StrengthMinimum.Ability = Dnd5e::Ability::Strength; Zero.StrengthMinimum.Minimum = 0; ExplicitFalse.bHasStealthDisadvantage = true;
	const FDnd5eItemArmorData States[] = {Missing, Null, Zero, ExplicitFalse};
	for (const auto& Input : States) { FDnd5eItemArmorData Out; if (!RoundTrip(*this, FDnd5eItemArmorData::StaticStruct(), &Input, &Out)) { return false; } TestTrue(TEXT("Strength and stealth presence states survive serialization"), Equal(FDnd5eItemArmorData::StaticStruct(), &Input, &Out)); }
	TestFalse(TEXT("Missing Strength differs from explicit null"), Equal(FDnd5eItemArmorData::StaticStruct(), &Missing, &Null));
	TestFalse(TEXT("Null Strength differs from a supplied zero minimum"), Equal(FDnd5eItemArmorData::StaticStruct(), &Null, &Zero));
	TestFalse(TEXT("Stealth false differs from absence"), Equal(FDnd5eItemArmorData::StaticStruct(), &Missing, &ExplicitFalse));
	FDnd5eItemWeaponData NoReload, ZeroReload; ZeroReload.bHasReload = true;
	TestFalse(TEXT("Reload zero differs from absence"), Equal(FDnd5eItemWeaponData::StaticStruct(), &NoReload, &ZeroReload));
	FDnd5eAttackRange NoLong, ZeroLong; ZeroLong.bHasLong = true; ZeroLong.Long.Amount = EquipmentInteger(0); ZeroLong.Long.Unit = EDnd5eDistanceUnit::Feet;
	TestFalse(TEXT("A supplied long-distance zero differs from no long distance"), Equal(FDnd5eAttackRange::StaticStruct(), &NoLong, &ZeroLong));
	FDnd5eArmorClassDefinition Base; Base.Value = EquipmentInteger(2); Base.Kind = EDnd5eArmorClassValueKind::Base;
	auto Bonus = Base; Bonus.Kind = EDnd5eArmorClassValueKind::Bonus;
	TestFalse(TEXT("Same numeric AC does not collapse base and bonus roles"), Equal(FDnd5eArmorClassDefinition::StaticStruct(), &Base, &Bonus));
	const EDnd5eArmorClassDexterityMode Modes[] = {EDnd5eArmorClassDexterityMode::Unspecified, EDnd5eArmorClassDexterityMode::NotAdded, EDnd5eArmorClassDexterityMode::Uncapped, EDnd5eArmorClassDexterityMode::Capped};
	for (const auto Mode : Modes) { auto Input = Base; Input.DexterityMode = Mode; Input.DexterityModifierMaximum = 2; FDnd5eArmorClassDefinition Out; if (!RoundTrip(*this, FDnd5eArmorClassDefinition::StaticStruct(), &Input, &Out)) { return false; } TestTrue(TEXT("Dexterity mode and active/inactive cap are retained without evaluation"), Equal(FDnd5eArmorClassDefinition::StaticStruct(), &Input, &Out)); }
	FDnd5eItem Inactive; Inactive.WeaponData.bHasReload = true; Inactive.WeaponData.Reload = 4; Inactive.ArmorData = Zero; FDnd5eItem Restored;
	if (!RoundTrip(*this, FDnd5eItem::StaticStruct(), &Inactive, &Restored)) { return false; }
	TestTrue(TEXT("Inactive equipment payloads are retained without asserting presence"), !Restored.bHasWeaponData && Restored.WeaponData.Reload == 4 && !Restored.bHasArmorData && Restored.ArmorData.StrengthPresence == EDnd5eValuePresence::Present);
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eEquipmentConditionsTest, "Dnd5e.Items.Equipment.EditorConditions", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FDnd5eEquipmentConditionsTest::RunTest(const FString& Parameters)
{
	struct FCondition { UScriptStruct* Type; const TCHAR* Field; const TCHAR* Expected; };
	const FCondition Cases[] = {
		{FDnd5eAttackRange::StaticStruct(), TEXT("Long"), TEXT("bHasLong")},
		{FDnd5eArmorClassDefinition::StaticStruct(), TEXT("DexterityModifierMaximum"), TEXT("DexterityMode == EDnd5eArmorClassDexterityMode::Capped")},
		{FDnd5eItemWeaponData::StaticStruct(), TEXT("PrimaryDamage"), TEXT("bHasPrimaryDamage")},
		{FDnd5eItemWeaponData::StaticStruct(), TEXT("AlternateDamage"), TEXT("bHasAlternateDamage")},
		{FDnd5eItemWeaponData::StaticStruct(), TEXT("Range"), TEXT("bHasRange")},
		{FDnd5eItemWeaponData::StaticStruct(), TEXT("Reload"), TEXT("bHasReload")},
		{FDnd5eItemArmorData::StaticStruct(), TEXT("ArmorClass"), TEXT("bHasArmorClass")},
		{FDnd5eItemArmorData::StaticStruct(), TEXT("StrengthMinimum"), TEXT("StrengthPresence == EDnd5eValuePresence::Present")},
		{FDnd5eItemArmorData::StaticStruct(), TEXT("bStealthDisadvantage"), TEXT("bHasStealthDisadvantage")},
		{FDnd5eItem::StaticStruct(), TEXT("WeaponData"), TEXT("bHasWeaponData")},
		{FDnd5eItem::StaticStruct(), TEXT("ArmorData"), TEXT("bHasArmorData")}
	};
	for (const auto& Case : Cases) { const auto* Field = Case.Type->FindPropertyByName(Case.Field); if (TestNotNull(Case.Field, Field)) { TestEqual(Case.Field, Field->GetMetaData(TEXT("EditCondition")), FString(Case.Expected)); } }
	return !HasAnyErrors();
}
#endif
