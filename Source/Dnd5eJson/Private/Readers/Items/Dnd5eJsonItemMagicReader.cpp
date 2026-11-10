// Developed by Neko Creative Technologies

#include "Readers/Items/Dnd5eJsonItemMagicReader.h"
#include "Readers/Dnd5eJsonReaderInternal.h"
#include "Items/Dnd5eItem.h"
#include "Dnd5eTags.h"

namespace
{
	using namespace Dnd5eJsonReader;

	bool TryReadItemBonus(const FJsonValue& Value, EDnd5eItemBonusKind Kind, FDnd5eItemBonus& Out)
	{
		FString Literal;
		if ((Value.Type != EJson::String && Value.Type != EJson::Number) ||
			!Value.TryGetString(Literal) || Literal.IsEmpty()) { return false; }
		const bool bNegative = Literal[0] == TEXT('-');
		const bool bHasSign = bNegative || Literal[0] == TEXT('+');
		int32 Magnitude = 0;
		if (!TryParseBoundedIntegerToken(bHasSign ? Literal.Mid(1) : Literal, 0, MAX_int32, Magnitude)) { return false; }
		Out.Kind = Kind;
		Out.Amount = bNegative ? -Magnitude : Magnitude;
		Out.SourceLiteral = MoveTemp(Literal);
		return true;
	}

	void ReadItemBonuses(const FJsonObject& Object, TArray<FDnd5eItemBonus>& Out, TSet<FString>& ConsumedFields)
	{
		struct FBonusField
		{
			const TCHAR* Key;
			EDnd5eItemBonusKind Kind;
		};
		const FBonusField Fields[] = {
			{TEXT("bonusAc"), EDnd5eItemBonusKind::ArmorClass},
			{TEXT("bonusWeapon"), EDnd5eItemBonusKind::WeaponAttackAndDamage},
			{TEXT("bonusWeaponAttack"), EDnd5eItemBonusKind::WeaponAttack},
			{TEXT("bonusWeaponDamage"), EDnd5eItemBonusKind::WeaponDamage},
			{TEXT("bonusSpellAttack"), EDnd5eItemBonusKind::SpellAttack},
			{TEXT("bonusSpellDamage"), EDnd5eItemBonusKind::SpellDamage},
			{TEXT("bonusSpellSaveDc"), EDnd5eItemBonusKind::SpellSaveDc},
			{TEXT("bonusSavingThrow"), EDnd5eItemBonusKind::SavingThrow},
			{TEXT("bonusSavingThrowConcentration"), EDnd5eItemBonusKind::ConcentrationSavingThrow},
			{TEXT("bonusAbilityCheck"), EDnd5eItemBonusKind::AbilityCheck},
			{TEXT("bonusProficiencyBonus"), EDnd5eItemBonusKind::ProficiencyBonus}
		};
		TArray<FDnd5eItemBonus> Candidate;
		for (const auto& Field : Fields)
		{
			const auto Value = FindExactField(Object, Field.Key);
			FDnd5eItemBonus Bonus;
			if (Value.IsValid() && TryReadItemBonus(*Value, Field.Kind, Bonus))
			{
				ConsumedFields.Add(Field.Key);
				Candidate.Add(MoveTemp(Bonus));
			}
		}
		Out = MoveTemp(Candidate);
	}

	bool TryReadItemSpellUid(const FString& Uid, const FString& DatasetNamespace, FDnd5eItemAttachedSpell& Out)
	{
		FString Reference = Uid;
		FString LevelToken;
		if (Uid.Split(TEXT("#"), &Reference, &LevelToken, ESearchCase::CaseSensitive))
		{
			if (!TryParseBoundedIntegerToken(LevelToken, 0, 9, Out.CastLevel)) { return false; }
		}
		TArray<FString> Parts;
		Reference.ParseIntoArray(Parts, TEXT("|"), false);
		if (Parts.IsEmpty() || Parts.Num() > 2 || (Parts.Num() == 2 && Parts[1].TrimStartAndEnd().IsEmpty())) { return false; }
		FString Display;
		if (!TryReadRecordSelector(TEXT("spell"), Parts, Out.Selector, Display)) { return false; }
		Out.SourceUid = Uid;
		Out.bHasTargetId = Out.Selector.bHasSource;
		if (Out.bHasTargetId)
		{
			Out.TargetId = MakeSpellRecordId(Out.Selector.Name, Out.Selector.SourceId, DatasetNamespace);
		}
		return true;
	}

	bool ReadItemSpellList(const FJsonValue& Value, const FString& DatasetNamespace, EDnd5eItemSpellUse Use,
		bool bHasCount, int32 Count, bool bEach, TArray<FDnd5eItemAttachedSpell>& Out)
	{
		if (Value.Type != EJson::Array) { return false; }
		for (const auto& Entry : Value.AsArray())
		{
			FString Uid;
			FDnd5eItemAttachedSpell Spell;
			if (Entry->Type != EJson::String || !Entry->TryGetString(Uid) || !TryReadItemSpellUid(Uid, DatasetNamespace, Spell))
			{
				return false;
			}
			Spell.Use = Use;
			Spell.bHasCount = bHasCount;
			Spell.Count = Count;
			Spell.bEach = bEach;
			Out.Add(MoveTemp(Spell));
		}
		return true;
	}

	bool ReadCountedItemSpellLists(const FJsonValue& Value, const FString& DatasetNamespace, EDnd5eItemSpellUse Use,
		TArray<FDnd5eItemAttachedSpell>& Out)
	{
		if (Value.Type != EJson::Object) { return false; }
		struct FCountedBucket
		{
			TSharedPtr<FJsonValue> Values;
			int32 Count;
			bool bEach;
		};
		TArray<FCountedBucket> Buckets;
		for (const auto& Field : Value.AsObject()->Values)
		{
			const FString Key(Field.Key.Len(), *Field.Key);
			const bool bEach = Key.EndsWith(TEXT("e"), ESearchCase::CaseSensitive);
			int32 Count = 0;
			if (!TryParseBoundedIntegerToken(bEach ? Key.LeftChop(1) : Key, 0, MAX_int32, Count)) { return false; }
			Buckets.Add({Field.Value, Count, bEach});
		}
		Buckets.Sort([](const FCountedBucket& Left, const FCountedBucket& Right)
		{
			if (Left.bEach != Right.bEach) { return !Left.bEach; }
			return Left.Count < Right.Count;
		});
		for (const auto& Bucket : Buckets)
		{
			if (!ReadItemSpellList(*Bucket.Values, DatasetNamespace, Use, true, Bucket.Count, Bucket.bEach, Out))
			{
				return false;
			}
		}
		return true;
	}

	bool TryReadItemSpellAbility(const FJsonValue& Value, FGameplayTag& Out)
	{
		FString Token;
		if (Value.Type != EJson::String || !Value.TryGetString(Token)) { return false; }
		using namespace Dnd5e::Ability;
		if (Token.Equals(TEXT("str"), ESearchCase::CaseSensitive)) { Out = Strength; }
		else if (Token.Equals(TEXT("dex"), ESearchCase::CaseSensitive)) { Out = Dexterity; }
		else if (Token.Equals(TEXT("con"), ESearchCase::CaseSensitive)) { Out = Constitution; }
		else if (Token.Equals(TEXT("int"), ESearchCase::CaseSensitive)) { Out = Intelligence; }
		else if (Token.Equals(TEXT("wis"), ESearchCase::CaseSensitive)) { Out = Wisdom; }
		else if (Token.Equals(TEXT("cha"), ESearchCase::CaseSensitive)) { Out = Charisma; }
		else { return false; }
		return true;
	}

	bool TryReadItemAttachedSpells(const FJsonValue& Value, const FString& DatasetNamespace, FDnd5eItemAttachedSpellData& Out)
	{
		FDnd5eItemAttachedSpellData Candidate;
		if (Value.Type == EJson::Null)
		{
			Candidate.Presence = EDnd5eValuePresence::ExplicitNull;
			Out = MoveTemp(Candidate);
			return true;
		}
		Candidate.Presence = EDnd5eValuePresence::Present;
		if (Value.Type == EJson::Array)
		{
			if (!ReadItemSpellList(Value, DatasetNamespace, EDnd5eItemSpellUse::Unspecified, false, 0, false, Candidate.Spells))
			{
				return false;
			}
		}
		else if (Value.Type == EJson::Object)
		{
			const auto Object = Value.AsObject();
			struct FUseField
			{
				const TCHAR* Key;
				EDnd5eItemSpellUse Use;
				bool bCounted;
			};
			const FUseField Fields[] = {
				{TEXT("will"), EDnd5eItemSpellUse::AtWill, false},
				{TEXT("charges"), EDnd5eItemSpellUse::Charges, true},
				{TEXT("rest"), EDnd5eItemSpellUse::Rest, true},
				{TEXT("daily"), EDnd5eItemSpellUse::Daily, true},
				{TEXT("limited"), EDnd5eItemSpellUse::Limited, true},
				{TEXT("ritual"), EDnd5eItemSpellUse::Ritual, false},
				{TEXT("other"), EDnd5eItemSpellUse::Other, false}
			};
			int32 FieldsRead = 0;
			if (const auto Ability = FindExactField(*Object, TEXT("ability")); Ability.IsValid())
			{
				if (!TryReadItemSpellAbility(*Ability, Candidate.Ability)) { return false; }
				++FieldsRead;
			}
			for (const auto& Field : Fields)
			{
				const auto Values = FindExactField(*Object, Field.Key);
				if (!Values.IsValid()) { continue; }
				const bool bRead = Field.bCounted
					? ReadCountedItemSpellLists(*Values, DatasetNamespace, Field.Use, Candidate.Spells)
					: ReadItemSpellList(*Values, DatasetNamespace, Field.Use, false, 0, false, Candidate.Spells);
				if (!bRead) { return false; }
				++FieldsRead;
			}
			if (FieldsRead != Object->Values.Num()) { return false; }
		}
		else { return false; }
		Out = MoveTemp(Candidate);
		return true;
	}
}

void Dnd5eJsonReader::ReadItemMagicFields(const FJsonObject& Object, const FString& DatasetNamespace, FDnd5eItem& Out,
	TSet<FString>& ConsumedFields)
{
	ReadItemBonuses(Object, Out.Bonuses, ConsumedFields);
	FDnd5eItemAttachedSpellData Candidate;
	const auto Attached = FindExactField(Object, TEXT("attachedSpells"));
	if (Attached.IsValid() && TryReadItemAttachedSpells(*Attached, DatasetNamespace, Candidate))
	{
		ConsumedFields.Add(TEXT("attachedSpells"));
	}
	Out.AttachedSpells = MoveTemp(Candidate);
}
