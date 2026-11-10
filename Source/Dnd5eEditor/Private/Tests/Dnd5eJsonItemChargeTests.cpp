// Developed by Neko Creative Technologies
#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Readers/Items/Dnd5eJsonBaseItemReader.h"
#include "Readers/Items/Dnd5eJsonItemLibrary.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"

namespace
{
	using EAmount = EDnd5eItemChargeAmountKind;
	using ETrigger = EDnd5eItemRechargeTrigger;

	FDnd5eJsonDocument ChargeDocument(const FString& Json, const TCHAR* Path = TEXT("fixtures/items.json"))
	{
		FDnd5eJsonDocument Document;
		Document.Context.DatasetNamespace = TEXT("item-charge-test");
		Document.Context.RelativeFilePath = Path;
		Document.JsonText = Json;
		return Document;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eItemChargeReadTest, "Dnd5e.Items.Charges.TypedAndRetainedValues", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FDnd5eItemChargeReadTest::RunTest(const FString& Parameters)
{
	const auto Document = ChargeDocument(TEXT(R"json({"item":[
		{"name":"Invented Lamp","source":"SYN","rarity":"rare","charges":7,"recharge":"dawn","rechargeAmount":"{@dice 1d6 + 1}"},
		{"name":"Invented Key","source":"SYN","rarity":"rare","charges":"{@dice 1d4 - 1}","recharge":"restLong","rechargeAmount":"5"},
		{"name":"Invented Empty","source":"SYN","rarity":"rare","charges":0,"recharge":null,"rechargeAmount":null},
		{"name":"Invented Missing","source":"SYN","rarity":"rare","charges":null},
		{"name":"Invented Odd","source":"SYN","rarity":"rare","charges":-1,"recharge":"unmapped","rechargeAmount":{}}
	]})json"));
	const auto Lamp = UDnd5eJsonItemLibrary::ReadItemAtIndex(Document, 0);
	const auto Key = FDnd5eJsonItemReader::ReadAtIndex(Document, 1);
	const auto Empty = FDnd5eJsonItemReader::ReadAtIndex(Document, 2);
	const auto Missing = FDnd5eJsonItemReader::ReadAtIndex(Document, 3);
	const auto Odd = FDnd5eJsonItemReader::ReadAtIndex(Document, 4);
	for (const auto* Result : {&Lamp, &Key, &Empty, &Missing, &Odd})
	{
		if (!TestTrue(TEXT("Item converts with its source provenance"), Result->Status == EDnd5eJsonReadStatus::Success &&
			Result->Item.Header.Provenance.Num() == 1)) { return false; }
	}
	const auto& Fixed = Lamp.Item.ChargeData;
	TestTrue(TEXT("Fixed capacity, dawn and dice recharge convert independently"),
		Fixed.Charges.Kind == EAmount::Fixed && Fixed.Charges.Fixed == 7 && Fixed.Charges.SourceLiteral == TEXT("7") &&
		Fixed.RechargePresence == EDnd5eValuePresence::Present && Fixed.RechargeTrigger == ETrigger::Dawn &&
		Fixed.RechargeAmount.Kind == EAmount::Dice && Fixed.RechargeAmount.Dice.Expression == TEXT("1d6 + 1") &&
		Fixed.RechargeAmount.Dice.Syntax == TEXT("5etools") && Fixed.RechargeAmount.SourceLiteral == TEXT("{@dice 1d6 + 1}") &&
		Lamp.Diagnostics.IsEmpty() && Lamp.Item.Header.Provenance[0].AdditionalMetadataJson.IsEmpty());
	const auto& Rolled = Key.Item.ChargeData;
	TestTrue(TEXT("Dice capacity and numeric text recharge preserve their literals"),
		Rolled.Charges.Kind == EAmount::Dice && Rolled.Charges.Dice.Expression == TEXT("1d4 - 1") &&
		Rolled.RechargeTrigger == ETrigger::LongRest && Rolled.RechargeAmount.Kind == EAmount::Fixed &&
		Rolled.RechargeAmount.Fixed == 5 && Rolled.RechargeAmount.SourceLiteral == TEXT("5") && Key.Diagnostics.IsEmpty());
	TestTrue(TEXT("Zero, null and omission stay distinct"),
		Empty.Item.ChargeData.Charges.Kind == EAmount::Fixed && Empty.Item.ChargeData.Charges.Fixed == 0 &&
		Empty.Item.ChargeData.RechargePresence == EDnd5eValuePresence::ExplicitNull &&
		Empty.Item.ChargeData.RechargeAmount.Kind == EAmount::ExplicitNull &&
		Missing.Item.ChargeData.Charges.Kind == EAmount::ExplicitNull &&
		Missing.Item.ChargeData.RechargePresence == EDnd5eValuePresence::Unspecified &&
		Missing.Item.ChargeData.RechargeAmount.Kind == EAmount::Unspecified && Empty.Diagnostics.IsEmpty() && Missing.Diagnostics.IsEmpty());
	TSharedPtr<FJsonObject> Metadata;
	TestTrue(TEXT("Unsupported fields remain in metadata and have exact diagnostic locations"),
		Odd.Item.ChargeData.Charges.Kind == EAmount::Unspecified &&
		Odd.Item.ChargeData.RechargePresence == EDnd5eValuePresence::Unspecified && Odd.Item.ChargeData.RechargeAmount.Kind == EAmount::Unspecified &&
		FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Odd.Item.Header.Provenance[0].AdditionalMetadataJson), Metadata) &&
		Metadata.IsValid() && Metadata->GetNumberField(TEXT("charges")) == -1 && Metadata->GetStringField(TEXT("recharge")) == TEXT("unmapped") &&
		Metadata->HasTypedField<EJson::Object>(TEXT("rechargeAmount")) && Odd.Diagnostics.Num() == 3 &&
		Odd.Diagnostics[0].JsonPointer == TEXT("/item/4/charges") && Odd.Diagnostics[1].JsonPointer == TEXT("/item/4/recharge") &&
		Odd.Diagnostics[2].JsonPointer == TEXT("/item/4/rechargeAmount"));
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eItemChargeSharedPathsTest, "Dnd5e.Items.Charges.SharedReaderPaths", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FDnd5eItemChargeSharedPathsTest::RunTest(const FString& Parameters)
{
	const auto Items = ChargeDocument(TEXT(R"({"item":[
		{"name":"Parent","source":"SYN","rarity":"rare","charges":4,"recharge":"midnight"},
		{"name":"Child","source":"SYN","_copy":{"name":"Parent","source":"SYN"},"rechargeAmount":2}],
		"itemGroup":[{"name":"Family","source":"SYN","rarity":"rare","items":["Child|SYN"],"recharge":"special"}],
		"baseitem":[{"name":"Invented Blade","source":"BAS","rarity":"none","type":"M|BAS","charges":1,"recharge":"decade"}]})"));
	const auto Variants = ChargeDocument(TEXT(R"({"magicvariant":[{"name":"Invented Pattern","requires":[{"type":"M|BAS"}],
		"inherits":{"source":"SYN","rarity":"rare","namePrefix":"Enchanted ","charges":3,"recharge":"dusk","rechargeAmount":2}}]})"),
		TEXT("fixtures/magicvariants.json"));
	const auto Copy = FDnd5eJsonItemCopyAssembler::ReadCopyAtIndex(Items, 1);
	const auto Group = FDnd5eJsonItemGroupReader::ReadAtIndex(Items, 0);
	const auto Base = FDnd5eJsonBaseItemReader::Read(Items);
	const auto Generated = UDnd5eJsonItemLibrary::ReadMagicItemVariantAtIndex(Variants, 0, Items, 0, EDnd5eJsonItemVariantEditionPolicy::SourceDefault);
	TestTrue(TEXT("Ordinary copies convert inherited and overridden fields"),
		Copy.Status == EDnd5eJsonReadStatus::Success && Copy.Item.ChargeData.Charges.Fixed == 4 &&
		Copy.Item.ChargeData.RechargeTrigger == ETrigger::Midnight && Copy.Item.ChargeData.RechargeAmount.Fixed == 2);
	TestTrue(TEXT("Groups keep a trigger with both amounts unspecified"),
		Group.Status == EDnd5eJsonReadStatus::Success && Group.Group.Details.ChargeData.RechargeTrigger == ETrigger::Special &&
		Group.Group.Details.ChargeData.Charges.Kind == EAmount::Unspecified && Group.Group.Details.ChargeData.RechargeAmount.Kind == EAmount::Unspecified);
	TestTrue(TEXT("Base items reach the shared reader"), Base.Status == EDnd5eJsonReadStatus::Success && Base.Items.Num() == 1 &&
		Base.Items[0].ChargeData.Charges.Fixed == 1 && Base.Items[0].ChargeData.RechargeTrigger == ETrigger::Decade);
	TestTrue(TEXT("Typed generated items convert the final inherited charge fields"),
		Generated.Status == EDnd5eJsonReadStatus::Success && Generated.Item.ChargeData.Charges.Fixed == 3 &&
		Generated.Item.ChargeData.RechargeTrigger == ETrigger::Dusk && Generated.Item.ChargeData.RechargeAmount.Fixed == 2);
	return !HasAnyErrors();
}

IMPLEMENT_COMPLEX_AUTOMATION_TEST(FDnd5eItemChargeSnapshotTest, "Dnd5e.Items.Charges.External", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
void FDnd5eItemChargeSnapshotTest::GetTests(TArray<FString>& Names, TArray<FString>& Commands) const
{
	FString Items, Variants;
	if (FParse::Value(FCommandLine::Get(), TEXT("Dnd5eJsonItemCopyFile="), Items) &&
		FParse::Value(FCommandLine::Get(), TEXT("Dnd5eJsonVariantFile="), Variants))
	{
		Names.Add(TEXT("InspectedSnapshot"));
		Commands.Add(Items);
	}
}

bool FDnd5eItemChargeSnapshotTest::RunTest(const FString& Parameters)
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
				for (const TCHAR* Field : {TEXT("charges"), TEXT("recharge"), TEXT("rechargeAmount")})
				{
					if (const auto* Authored = Source->Values.Find(Field)) { Candidate->SetField(Field, *Authored); }
				}
				if (Candidate->Values.IsEmpty()) { continue; }
				Candidate->SetStringField(TEXT("name"), FString::Printf(TEXT("Charge probe %d"), Projected.Num()));
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
	const auto Result = FDnd5eJsonBaseItemReader::Read(ChargeDocument(Json));
	if (!TestTrue(TEXT("All projected fields convert through the shared reader"), Result.Status == EDnd5eJsonReadStatus::Success &&
		Result.Items.Num() == Projected.Num() && Result.Diagnostics.IsEmpty()))
	{
		for (const auto& Diagnostic : Result.Diagnostics) { AddError(Diagnostic.JsonPointer + TEXT(" ") + Diagnostic.Message); }
		return false;
	}
	int32 Charges = 0, Triggers = 0, Amounts = 0, DiceCharges = 0, DiceRecharge = 0;
	for (const auto& Item : Result.Items)
	{
		const auto& Data = Item.ChargeData;
		Charges += Data.Charges.Kind != EAmount::Unspecified;
		Triggers += Data.RechargePresence == EDnd5eValuePresence::Present;
		Amounts += Data.RechargeAmount.Kind != EAmount::Unspecified;
		DiceCharges += Data.Charges.Kind == EAmount::Dice;
		DiceRecharge += Data.RechargeAmount.Kind == EAmount::Dice;
	}
	TestEqual(TEXT("Authored capacity fields"), Charges, 326);
	TestEqual(TEXT("Authored trigger fields"), Triggers, 325);
	TestEqual(TEXT("Authored recharge amounts"), Amounts, 293);
	TestEqual(TEXT("Dice capacity fields"), DiceCharges, 5);
	TestEqual(TEXT("Dice recharge fields"), DiceRecharge, 252);
	return !HasAnyErrors();
}
#endif
