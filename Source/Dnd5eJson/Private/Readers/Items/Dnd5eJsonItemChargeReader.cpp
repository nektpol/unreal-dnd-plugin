// Developed by Neko Creative Technologies

#include "Readers/Items/Dnd5eJsonItemChargeReader.h"
#include "Readers/Dnd5eJsonReaderInternal.h"
#include "Readers/Dnd5eJsonInlineSyntax.h"

namespace
{
	using namespace Dnd5eJsonReader;

	bool TryReadItemChargeAmount(const FJsonValue& Value, FDnd5eItemChargeAmount& Out)
	{
		FDnd5eItemChargeAmount Candidate;
		if (Value.Type == EJson::Null)
		{
			Candidate.Kind = EDnd5eItemChargeAmountKind::ExplicitNull;
			Out = MoveTemp(Candidate);
			return true;
		}

		FString Literal;
		if ((Value.Type != EJson::Number && Value.Type != EJson::String) || !Value.TryGetString(Literal)) { return false; }
		if (TryParseBoundedIntegerToken(Literal, 0, MAX_int32, Candidate.Fixed))
		{
			Candidate.Kind = EDnd5eItemChargeAmountKind::Fixed;
		}
		else
		{
			FString Tag;
			TArray<FString> Parts;
			if (Value.Type != EJson::String || !TryParseInlineToken(Literal, Tag, Parts) ||
				!Tag.Equals(TEXT("dice"), ESearchCase::CaseSensitive) || Parts.Num() != 1 ||
				Parts[0].TrimStartAndEnd().IsEmpty() || Parts[0].Contains(TEXT("{")) || Parts[0].Contains(TEXT("}")))
			{
				return false;
			}
			Candidate.Kind = EDnd5eItemChargeAmountKind::Dice;
			Candidate.Dice.Expression = MoveTemp(Parts[0]);
			Candidate.Dice.Syntax = TEXT("5etools");
		}
		Candidate.SourceLiteral = MoveTemp(Literal);
		Out = MoveTemp(Candidate);
		return true;
	}

	bool TryReadItemRechargeTrigger(const FJsonValue& Value, FDnd5eItemChargeData& Out)
	{
		if (Value.Type == EJson::Null)
		{
			Out.RechargePresence = EDnd5eValuePresence::ExplicitNull;
			Out.RechargeTrigger = EDnd5eItemRechargeTrigger::Unspecified;
			return true;
		}
		FString Token;
		if (Value.Type != EJson::String || !Value.TryGetString(Token)) { return false; }
		struct FTriggerMapping
		{
			const TCHAR* Token;
			EDnd5eItemRechargeTrigger Trigger;
		};
		const FTriggerMapping Mappings[] = {
			{TEXT("dawn"), EDnd5eItemRechargeTrigger::Dawn},
			{TEXT("dusk"), EDnd5eItemRechargeTrigger::Dusk},
			{TEXT("midnight"), EDnd5eItemRechargeTrigger::Midnight},
			{TEXT("restShort"), EDnd5eItemRechargeTrigger::ShortRest},
			{TEXT("restLong"), EDnd5eItemRechargeTrigger::LongRest},
			{TEXT("decade"), EDnd5eItemRechargeTrigger::Decade},
			{TEXT("special"), EDnd5eItemRechargeTrigger::Special}
		};
		for (const auto& Mapping : Mappings)
		{
			if (!Token.Equals(Mapping.Token, ESearchCase::CaseSensitive)) { continue; }
			Out.RechargePresence = EDnd5eValuePresence::Present;
			Out.RechargeTrigger = Mapping.Trigger;
			return true;
		}
		return false;
	}
}

void Dnd5eJsonReader::ReadItemChargeFields(const FJsonObject& Object, FDnd5eItemChargeData& Out, TSet<FString>& ConsumedFields)
{
	FDnd5eItemChargeData Candidate;
	const auto Charges = FindExactField(Object, TEXT("charges"));
	if (Charges.IsValid() && TryReadItemChargeAmount(*Charges, Candidate.Charges)) { ConsumedFields.Add(TEXT("charges")); }

	const auto Recharge = FindExactField(Object, TEXT("recharge"));
	if (Recharge.IsValid() && TryReadItemRechargeTrigger(*Recharge, Candidate)) { ConsumedFields.Add(TEXT("recharge")); }

	const auto RechargeAmount = FindExactField(Object, TEXT("rechargeAmount"));
	if (RechargeAmount.IsValid() && TryReadItemChargeAmount(*RechargeAmount, Candidate.RechargeAmount))
	{
		ConsumedFields.Add(TEXT("rechargeAmount"));
	}
	Out = MoveTemp(Candidate);
}