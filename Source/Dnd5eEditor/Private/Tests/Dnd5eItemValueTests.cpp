// Developed by Neko Creative Technologies
#if WITH_DEV_AUTOMATION_TESTS
#include "Dnd5eTestSupport.h"
using namespace Dnd5eTests;

namespace
{
	FDnd5eNumber Integer(int32 Value) { FDnd5eNumber N; N.Kind = EDnd5eNumberKind::Integer; N.Integer = Value; N.SourceLiteral = FString::FromInt(Value); return N; }
	FDnd5eMoney Money(int32 Value, EDnd5eCurrency Currency) { FDnd5eMoney M; M.Amount = Integer(Value); M.Currency = Currency; return M; }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5ePriceStateTest, "Dnd5e.Items.Values.PricePresenceAndCurrencyBasket", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FDnd5ePriceStateTest::RunTest(const FString& Parameters)
{
	FDnd5ePrice Missing, Null, Zero, Multiplier, Basket;
	Null.CostPresence = EDnd5eValuePresence::ExplicitNull;
	Zero.CostPresence = EDnd5eValuePresence::Present; Zero.Cost.Add(Money(0, EDnd5eCurrency::Copper));
	Multiplier.bHasMultiplier = true; Multiplier.Multiplier = Integer(2);
	Basket.CostPresence = EDnd5eValuePresence::Present; Basket.Cost = {Money(5, EDnd5eCurrency::Gold), Money(1, EDnd5eCurrency::Copper)};
	const FDnd5ePrice Inputs[] = {Missing, Null, Zero, Multiplier, Basket};
	TArray<FDnd5ePrice> Restored;
	for (const auto& Input : Inputs) { FDnd5ePrice Out; RoundTrip(*this, FDnd5ePrice::StaticStruct(), &Input, &Out); TestTrue(TEXT("Presence, zero and multiplier data survive serialization"), Equal(FDnd5ePrice::StaticStruct(), &Input, &Out)); Restored.Add(Out); }
	TestFalse(TEXT("Absence differs from explicit null"), Equal(FDnd5ePrice::StaticStruct(), &Restored[0], &Restored[1]));
	TestFalse(TEXT("Explicit null differs from zero cost"), Equal(FDnd5ePrice::StaticStruct(), &Restored[1], &Restored[2]));
	TestTrue(TEXT("Multiplier-only price does not acquire a fixed cost"), Restored[3].Cost.IsEmpty() && Restored[3].CostPresence == EDnd5eValuePresence::Unspecified && Restored[3].Multiplier.Integer == 2);
	TestTrue(TEXT("Mixed denominations remain separate and ordered"), Restored[4].Cost.Num() == 2 && Restored[4].Cost[0].Currency == EDnd5eCurrency::Gold && Restored[4].Cost[0].Amount.Integer == 5 && Restored[4].Cost[1].Currency == EDnd5eCurrency::Copper && Restored[4].Cost[1].Amount.Integer == 1);
	Restored[4].Cost[0].Amount.Integer = 99;
	TestEqual(TEXT("Returned basket owns its nested amounts"), Basket.Cost[0].Amount.Integer, 5);
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eItemValuesOwnershipTest, "Dnd5e.Items.Values.MassAndAttunementOwnership", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FDnd5eItemValuesOwnershipTest::RunTest(const FString& Parameters)
{
	FDnd5eItem Item; Item.bHasWeight = true; Item.Weight.Unit = EDnd5eMassUnit::Pounds;
	Item.Weight.Amount.Kind = EDnd5eNumberKind::Decimal; Item.Weight.Amount.Decimal = 0.25; Item.Weight.Amount.SourceLiteral = TEXT("0.25");
	Item.bHasWeightMultiplier = true; Item.WeightMultiplier = Integer(2); Item.WeightNote.Add(Text(TEXT("Invented weight note")));
	Item.Attunement.Mode = EDnd5eAttunementMode::Required;
	FDnd5eFormattedContent Condition; Condition.Formatting.AddTag(Dnd5e::Content::Formatting::Italic); Condition.Inlines.Add(Text(TEXT("by an invented guild member")));
	Item.Attunement.Conditions.Add(Inline(Dnd5e::Content::Inline::Formatted, Condition));
	Item.bHasAlternativeAttunement = true; Item.AlternativeAttunement.Mode = EDnd5eAttunementMode::Optional;
	Item.AlternativeAttunement.Conditions.Add(Text(TEXT("Independent alternative wording")));
	Item.Price.CostPresence = EDnd5eValuePresence::Present; Item.Price.Cost = {Money(5, EDnd5eCurrency::Gold), Money(1, EDnd5eCurrency::Copper)};
	FDnd5eItem Restored; RoundTrip(*this, FDnd5eItem::StaticStruct(), &Item, &Restored);
	TestTrue(TEXT("The full item retains all shared values"), Equal(FDnd5eItem::StaticStruct(), &Item, &Restored));
	TestTrue(TEXT("Fraction and source unit are not normalized during storage"), Restored.Weight.Unit == EDnd5eMassUnit::Pounds && Restored.Weight.Amount.Decimal == 0.25 && Restored.Weight.Amount.SourceLiteral == TEXT("0.25"));
	TestTrue(TEXT("Primary and alternative declarations remain distinct"), Restored.Attunement.Mode == EDnd5eAttunementMode::Required && Restored.AlternativeAttunement.Mode == EDnd5eAttunementMode::Optional);
	const auto Before = Item;
	Restored.Weight.Amount.Decimal = 3; Restored.Price.Cost[0].Amount.Integer = 42;
	Restored.WeightNote[0].Payload.GetMutable<FDnd5eTextContent>().Text = TEXT("Changed note");
	Restored.Attunement.Conditions[0].Payload.GetMutable<FDnd5eFormattedContent>().Inlines[0].Payload.GetMutable<FDnd5eTextContent>().Text = TEXT("Changed primary");
	Restored.AlternativeAttunement.Conditions[0].Payload.GetMutable<FDnd5eTextContent>().Text = TEXT("Changed alternative");
	TestTrue(TEXT("Nested changes preserve the original item"), Equal(FDnd5eItem::StaticStruct(), &Item, &Before));
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eItemValueConditionsTest, "Dnd5e.Items.Values.EditorPresenceConditions", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FDnd5eItemValueConditionsTest::RunTest(const FString& Parameters)
{
	struct FCondition { UScriptStruct* Type; const TCHAR* Field; const TCHAR* Expected; };
	const FCondition Cases[] = {
		{FDnd5ePrice::StaticStruct(), TEXT("Cost"), TEXT("CostPresence == EDnd5eValuePresence::Present")},
		{FDnd5ePrice::StaticStruct(), TEXT("Multiplier"), TEXT("bHasMultiplier")},
		{FDnd5eItem::StaticStruct(), TEXT("Weight"), TEXT("bHasWeight")},
		{FDnd5eItem::StaticStruct(), TEXT("WeightMultiplier"), TEXT("bHasWeightMultiplier")},
		{FDnd5eItem::StaticStruct(), TEXT("AlternativeAttunement"), TEXT("bHasAlternativeAttunement")}
	};
	for (const auto& Case : Cases) { const auto* Field = Case.Type->FindPropertyByName(Case.Field); if (TestNotNull(Case.Field, Field)) { TestEqual(Case.Field, Field->GetMetaData(TEXT("EditCondition")), FString(Case.Expected)); } }
	return !HasAnyErrors();
}
#endif
