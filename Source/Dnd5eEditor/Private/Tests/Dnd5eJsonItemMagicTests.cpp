// Developed by Neko Creative Technologies
#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Dnd5eTags.h"
#include "Readers/Items/Dnd5eJsonBaseItemReader.h"
#include "Readers/Items/Dnd5eJsonItemLibrary.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"

namespace
{
	using EBonus = EDnd5eItemBonusKind;
	using EUse = EDnd5eItemSpellUse;

	const TCHAR* BonusKeys[] = {TEXT("bonusAc"), TEXT("bonusWeapon"), TEXT("bonusWeaponAttack"), TEXT("bonusWeaponDamage"),
		TEXT("bonusSpellAttack"), TEXT("bonusSpellDamage"), TEXT("bonusSpellSaveDc"), TEXT("bonusSavingThrow"),
		TEXT("bonusSavingThrowConcentration"), TEXT("bonusAbilityCheck"), TEXT("bonusProficiencyBonus")};

	FDnd5eJsonDocument ItemMagicDocument(const FString& Json, const TCHAR* Path = TEXT("fixtures/items.json"))
	{
		FDnd5eJsonDocument Document;
		Document.Context.DatasetNamespace = TEXT("item-magic-test");
		Document.Context.RelativeFilePath = Path;
		Document.JsonText = Json;
		return Document;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eItemMagicFieldsTest, "Dnd5e.Items.Magic.TypedFields", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FDnd5eItemMagicFieldsTest::RunTest(const FString& Parameters)
{
	const auto Document = ItemMagicDocument(TEXT(R"({"item":[{
		"name":"Invented Wand","source":"SYN","rarity":"rare",
		"bonusAc":"+1","bonusWeapon":"+2","bonusWeaponAttack":"+3","bonusWeaponDamage":4,
		"bonusSpellAttack":"+5","bonusSpellDamage":"+6","bonusSpellSaveDc":"+7","bonusSavingThrow":"-2",
		"bonusSavingThrowConcentration":"+9","bonusAbilityCheck":"0","bonusProficiencyBonus":"+1",
		"attachedSpells":{"ability":"int","will":["Invented Spark"],"charges":{"10":["Invented Cost"],"3":["Invented Ward|SYN#5"],"0":["Invented Light"]},
		"daily":{"1e":["Invented Mist|SYN"]},"rest":{"1e":["Invented Rest"]},"limited":{"2":["Invented Limit"]},
		"ritual":["Invented Ritual"],"other":["Invented Other"]}
	}]})"));
	const auto Result = UDnd5eJsonItemLibrary::ReadItemAtIndex(Document, 0);
	if (!TestTrue(TEXT("All authored fields convert without metadata warnings"), Result.Status == EDnd5eJsonReadStatus::Success &&
		Result.Diagnostics.IsEmpty() && Result.Item.Header.Provenance.Num() == 1 &&
		Result.Item.Header.Provenance[0].AdditionalMetadataJson.IsEmpty())) { return false; }

	const auto& Bonuses = Result.Item.Bonuses;
	const EBonus Kinds[] = {EBonus::ArmorClass, EBonus::WeaponAttackAndDamage, EBonus::WeaponAttack, EBonus::WeaponDamage,
		EBonus::SpellAttack, EBonus::SpellDamage, EBonus::SpellSaveDc, EBonus::SavingThrow, EBonus::ConcentrationSavingThrow,
		EBonus::AbilityCheck, EBonus::ProficiencyBonus};
	const int32 Amounts[] = {1, 2, 3, 4, 5, 6, 7, -2, 9, 0, 1};
	const TCHAR* Literals[] = {TEXT("+1"), TEXT("+2"), TEXT("+3"), TEXT("4"), TEXT("+5"), TEXT("+6"), TEXT("+7"),
		TEXT("-2"), TEXT("+9"), TEXT("0"), TEXT("+1")};
	if (!TestEqual(TEXT("All eleven bonus kinds are represented"), Bonuses.Num(), 11)) { return false; }
	for (int32 Index = 0; Index < Bonuses.Num(); ++Index)
	{
		TestTrue(BonusKeys[Index], Bonuses[Index].Kind == Kinds[Index] && Bonuses[Index].Amount == Amounts[Index] &&
			Bonuses[Index].SourceLiteral == Literals[Index]);
	}
	const auto& Attached = Result.Item.AttachedSpells;
	if (!TestTrue(TEXT("Spell declaration stores the ability and all usage lists"), Attached.Presence == EDnd5eValuePresence::Present &&
		Attached.Ability == Dnd5e::Ability::Intelligence.GetTag() && Attached.Spells.Num() == 9)) { return false; }
	const auto& Spells = Attached.Spells;
	TestTrue(TEXT("Omitted source remains a selector without an inferred target"),
		Spells[0].Use == EUse::AtWill && Spells[0].Selector.Category == Dnd5e::Category::Spell.GetTag() &&
		Spells[0].Selector.Name == TEXT("Invented Spark") && !Spells[0].Selector.bHasSource &&
		!Spells[0].bHasTargetId && !Spells[0].bHasCount && Spells[0].CastLevel == INDEX_NONE);
	TestTrue(TEXT("Count buckets are numeric and zero is a present count"),
		Spells[1].Use == EUse::Charges && Spells[1].bHasCount && Spells[1].Count == 0 &&
		Spells[2].Count == 3 && Spells[3].Count == 10);
	TestTrue(TEXT("Explicit publication produces the canonical ID without the cast-level suffix"),
		Spells[2].bHasTargetId && Spells[2].TargetId.DatasetNamespace == Document.Context.DatasetNamespace &&
		Spells[2].TargetId.SourceKey.Equals(TEXT("5etools:spell:v1:Invented%20Ward|SYN"), ESearchCase::CaseSensitive) &&
		Spells[2].Selector.SourceId == TEXT("SYN") && Spells[2].CastLevel == 5 &&
		Spells[2].SourceUid == TEXT("Invented Ward|SYN#5"));
	TestTrue(TEXT("Rest, daily, limited, ritual and other declarations retain their uses"),
		Spells[4].Use == EUse::Rest && Spells[4].bEach && Spells[5].Use == EUse::Daily && Spells[5].Count == 1 &&
		Spells[5].bEach && Spells[6].Use == EUse::Limited && Spells[6].Count == 2 &&
		Spells[7].Use == EUse::Ritual && !Spells[7].bHasCount && Spells[8].Use == EUse::Other && !Spells[8].bHasCount);
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eItemMagicFallbackTest, "Dnd5e.Items.Magic.FallbackAndSharedPaths", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FDnd5eItemMagicFallbackTest::RunTest(const FString& Parameters)
{
	const auto Document = ItemMagicDocument(TEXT(R"({"item":[
		{"name":"Invented Odd","source":"SYN","rarity":"rare","bonusAc":"varies","bonusWeapon":"+2",
		 "attachedSpells":{"will":["Invented Spark"],"daily":{"1e":["Invented Mist|SYN#12"]}}},
		{"name":"Invented Legacy","source":"SYN","rarity":"rare","attachedSpells":["Invented Spark|SYN"]},
		{"name":"Invented Null","source":"SYN","rarity":"rare","attachedSpells":null},
		{"name":"Invented Empty","source":"SYN","rarity":"rare","attachedSpells":[]},
		{"name":"Invented Omitted","source":"SYN","rarity":"rare"},
		{"name":"Invented Copy","source":"SYN","_copy":{"name":"Invented Legacy","source":"SYN"},"bonusSpellSaveDc":"+1"}],
		"itemGroup":[{"name":"Family","source":"SYN","rarity":"rare","items":["Invented Copy|SYN"],"bonusAc":"+1","attachedSpells":{"other":["Invented Spark"]}}],
		"baseitem":[{"name":"Invented Blade","source":"BAS","rarity":"none","type":"M|BAS"}]})"));
	const auto Odd = FDnd5eJsonItemReader::ReadAtIndex(Document, 0);
	TSharedPtr<FJsonObject> Metadata;
	TestTrue(TEXT("Unsupported members preserve the entire attached declaration while valid bonuses convert"),
		Odd.Status == EDnd5eJsonReadStatus::Success && Odd.Item.Bonuses.Num() == 1 &&
		Odd.Item.AttachedSpells.Presence == EDnd5eValuePresence::Unspecified && Odd.Item.AttachedSpells.Spells.IsEmpty() &&
		Odd.Item.Header.Provenance.Num() == 1 && FJsonSerializer::Deserialize(
			TJsonReaderFactory<>::Create(Odd.Item.Header.Provenance[0].AdditionalMetadataJson), Metadata) &&
		Metadata.IsValid() && Metadata->HasTypedField<EJson::Object>(TEXT("attachedSpells")) &&
		Metadata->GetObjectField(TEXT("attachedSpells"))->HasField(TEXT("will")) &&
		Metadata->HasTypedField<EJson::String>(TEXT("bonusAc")) && Odd.Diagnostics.Num() == 2 &&
		Odd.Diagnostics[0].JsonPointer == TEXT("/item/0/attachedSpells") && Odd.Diagnostics[1].JsonPointer == TEXT("/item/0/bonusAc"));
	const auto Legacy = FDnd5eJsonItemReader::ReadAtIndex(Document, 1);
	const auto Null = FDnd5eJsonItemReader::ReadAtIndex(Document, 2);
	const auto Empty = FDnd5eJsonItemReader::ReadAtIndex(Document, 3);
	const auto Omitted = FDnd5eJsonItemReader::ReadAtIndex(Document, 4);
	TestTrue(TEXT("Legacy list has no inferred use, count or cast level"), Legacy.Status == EDnd5eJsonReadStatus::Success &&
		Legacy.Item.AttachedSpells.Spells.Num() == 1 && Legacy.Item.AttachedSpells.Spells[0].Use == EUse::Unspecified &&
		!Legacy.Item.AttachedSpells.Spells[0].bHasCount && Legacy.Item.AttachedSpells.Spells[0].CastLevel == INDEX_NONE);
	TestTrue(TEXT("Null, empty and omitted declarations stay distinct"),
		Null.Status == EDnd5eJsonReadStatus::Success && Null.Item.AttachedSpells.Presence == EDnd5eValuePresence::ExplicitNull &&
		Empty.Status == EDnd5eJsonReadStatus::Success && Empty.Item.AttachedSpells.Presence == EDnd5eValuePresence::Present &&
		Empty.Item.AttachedSpells.Spells.IsEmpty() && Omitted.Status == EDnd5eJsonReadStatus::Success &&
		Omitted.Item.AttachedSpells.Presence == EDnd5eValuePresence::Unspecified);

	const auto Variants = ItemMagicDocument(TEXT(R"({"magicvariant":[{"name":"Invented Pattern","requires":[{"type":"M|BAS"}],
		"inherits":{"source":"SYN","rarity":"rare","namePrefix":"Enchanted ","bonusWeapon":"+3","attachedSpells":{"charges":{"1":["Invented Ward|SYN#5"]}}}}]})"),
		TEXT("fixtures/magicvariants.json"));
	const auto Copy = FDnd5eJsonItemCopyAssembler::ReadCopyAtIndex(Document, 5);
	const auto Group = FDnd5eJsonItemGroupReader::ReadAtIndex(Document, 0);
	const auto Generated = UDnd5eJsonItemLibrary::ReadMagicItemVariantAtIndex(Variants, 0, Document, 0, EDnd5eJsonItemVariantEditionPolicy::SourceDefault);
	TestTrue(TEXT("Typed copies and groups reach the shared reader"),
		Copy.Status == EDnd5eJsonReadStatus::Success && Copy.Item.Bonuses.Num() == 1 && Copy.Item.AttachedSpells.Spells.Num() == 1 &&
		Group.Status == EDnd5eJsonReadStatus::Success && Group.Group.Details.Bonuses.Num() == 1 &&
		Group.Group.Details.AttachedSpells.Spells.Num() == 1);
	TestTrue(TEXT("Generated items convert the inherited declarations"),
		Generated.Status == EDnd5eJsonReadStatus::Success && Generated.Item.Bonuses.Num() == 1 &&
		Generated.Item.Bonuses[0].Amount == 3 && Generated.Item.AttachedSpells.Spells.Num() == 1 &&
		Generated.Item.AttachedSpells.Spells[0].CastLevel == 5);
	return !HasAnyErrors();
}

IMPLEMENT_COMPLEX_AUTOMATION_TEST(FDnd5eItemMagicSnapshotTest, "Dnd5e.Items.Magic.External", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
void FDnd5eItemMagicSnapshotTest::GetTests(TArray<FString>& Names, TArray<FString>& Commands) const
{
	FString Items, Variants;
	if (FParse::Value(FCommandLine::Get(), TEXT("Dnd5eJsonItemCopyFile="), Items) &&
		FParse::Value(FCommandLine::Get(), TEXT("Dnd5eJsonVariantFile="), Variants))
	{
		Names.Add(TEXT("InspectedSnapshot"));
		Commands.Add(Items);
	}
}

bool FDnd5eItemMagicSnapshotTest::RunTest(const FString& Parameters)
{
	FString VariantPath;
	if (!FParse::Value(FCommandLine::Get(), TEXT("Dnd5eJsonVariantFile="), VariantPath)) { return false; }
	TArray<TSharedPtr<FJsonValue>> Projected;
	for (const auto& Path : {Parameters, VariantPath})
	{
		FString Text;
		TSharedPtr<FJsonObject> Root;
		if (!TestTrue(TEXT("Read supplied item or variant document"), FFileHelper::LoadFileToString(Text, *Path) &&
			FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Root) && Root.IsValid())) { return false; }
		for (const TCHAR* Key : {TEXT("item"), TEXT("itemGroup"), TEXT("magicvariant")})
		{
			const TArray<TSharedPtr<FJsonValue>>* Rows = nullptr;
			if (!Root->TryGetArrayField(Key, Rows)) { continue; }
			for (const auto& Value : *Rows)
			{
				if (Value->Type != EJson::Object) { continue; }
				const auto Row = Value->AsObject();
				const TSharedPtr<FJsonObject>* Inherits = nullptr;
				const auto Source = Row->TryGetObjectField(TEXT("inherits"), Inherits) ? *Inherits : Row;
				auto Candidate = MakeShared<FJsonObject>();
				for (const TCHAR* Field : BonusKeys)
				{
					if (const auto* Authored = Source->Values.Find(Field)) { Candidate->SetField(Field, *Authored); }
				}
				if (const auto* Authored = Source->Values.Find(TEXT("attachedSpells"))) { Candidate->SetField(TEXT("attachedSpells"), *Authored); }
				if (Candidate->Values.IsEmpty()) { continue; }
				Candidate->SetStringField(TEXT("name"), FString::Printf(TEXT("Magic probe %d"), Projected.Num()));
				Candidate->SetStringField(TEXT("source"), TEXT("SYN"));
				Candidate->SetStringField(TEXT("rarity"), TEXT("rare"));
				Candidate->SetStringField(TEXT("type"), TEXT("G|SYN"));
				Projected.Add(MakeShared<FJsonValueObject>(Candidate));
			}
		}
	}
	auto Root = MakeShared<FJsonObject>();
	Root->SetArrayField(TEXT("baseitem"), Projected);
	FString Json;
	if (!FJsonSerializer::Serialize(Root, TJsonWriterFactory<>::Create(&Json))) { return false; }
	const auto Result = FDnd5eJsonBaseItemReader::Read(ItemMagicDocument(Json));
	if (!TestTrue(TEXT("All projected declarations convert through the shared reader"), Result.Status == EDnd5eJsonReadStatus::Success &&
		Result.Items.Num() == Projected.Num() && Result.Diagnostics.IsEmpty()))
	{
		for (const auto& Diagnostic : Result.Diagnostics) { AddError(Diagnostic.JsonPointer + TEXT(" ") + Diagnostic.Message); }
		return false;
	}
	int32 Bonuses = 0, Attached = 0, Spells = 0, Levels = 0;
	for (const auto& Item : Result.Items)
	{
		Bonuses += Item.Bonuses.Num();
		Attached += Item.AttachedSpells.Presence == EDnd5eValuePresence::Present;
		Spells += Item.AttachedSpells.Spells.Num();
		for (const auto& Spell : Item.AttachedSpells.Spells) { Levels += Spell.CastLevel != INDEX_NONE; }
	}
	TestEqual(TEXT("Authored bonuses"), Bonuses, 519);
	TestEqual(TEXT("Authored spell declarations"), Attached, 462);
	TestEqual(TEXT("Authored spell references"), Spells, 1196);
	TestEqual(TEXT("Authored cast levels"), Levels, 18);
	return !HasAnyErrors();
}
#endif
