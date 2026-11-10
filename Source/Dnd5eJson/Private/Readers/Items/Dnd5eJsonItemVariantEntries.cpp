// Developed by Neko Creative Technologies

#include "Readers/Items/Dnd5eJsonItemVariantEntries.h"
#include "Containers/StringView.h"

namespace
{
	using namespace Dnd5eJsonReader;

	constexpr int32 MaxRenderedStringLength = 65536;
	constexpr int32 MaxSubstitutionsPerString = 256;
	constexpr int64 MaxRenderedEntryCharacters = 2 * 1024 * 1024;

	struct FVariantEntryInputs
	{
		const FJsonObject& Base;
		const FJsonObject& Inherits;
		const FString& BaseName;
		int64 RenderedCharacters = 0;
	};

	bool ResolveVariantDamageName(const FJsonObject& Base, const FString& Pointer, FString& Out, FReadState& State)
	{
		const auto Value = FindExactField(Base, TEXT("dmgType"));
		FString Code;
		if (!Value.IsValid() || Value->Type != EJson::String || !Value->TryGetString(Code))
		{
			return Fail(State, Codes::InvalidFieldValue, Pointer, TEXT("The damage type substitution needs a base damage code."));
		}
		struct FDamageName { const TCHAR* Code; const TCHAR* Name; };
		const FDamageName Names[] = {
			{TEXT("A"), TEXT("Acid")}, {TEXT("B"), TEXT("Bludgeoning")}, {TEXT("C"), TEXT("Cold")},
			{TEXT("F"), TEXT("Fire")}, {TEXT("O"), TEXT("Force")}, {TEXT("L"), TEXT("Lightning")},
			{TEXT("N"), TEXT("Necrotic")}, {TEXT("P"), TEXT("Piercing")}, {TEXT("I"), TEXT("Poison")},
			{TEXT("Y"), TEXT("Psychic")}, {TEXT("R"), TEXT("Radiant")}, {TEXT("S"), TEXT("Slashing")},
			{TEXT("T"), TEXT("Thunder")}
		};
		for (const auto& Name : Names)
		{
			if (Code.Equals(Name.Code, ESearchCase::CaseSensitive)) { Out = Name.Name; return true; }
		}
		return Fail(State, Codes::UnsupportedRequiredValue, Pointer, TEXT("Unsupported base damage code for variant text."));
	}

	bool ResolveVariantToken(const FString& Token, const FVariantEntryInputs& Inputs, const FString& Pointer,
		FString& Out, FReadState& State)
	{
		FString Key, Modifier;
		const bool bHasModifier = Token.Split(TEXT("/"), &Key, &Modifier, ESearchCase::CaseSensitive);
		if (!bHasModifier) { Key = Token; }
		if (Key.Equals(TEXT("baseName"), ESearchCase::CaseSensitive))
		{
			if (!bHasModifier) { Out = Inputs.BaseName; return true; }
			if (Modifier.Equals(TEXT("l"), ESearchCase::CaseSensitive)) { Out = Inputs.BaseName.ToLower(); return true; }
			if (Modifier.Equals(TEXT("a"), ESearchCase::CaseSensitive) ||
				Modifier.Equals(TEXT("at"), ESearchCase::CaseSensitive))
			{
				const TCHAR First = FChar::ToLower(Inputs.BaseName[0]);
				const bool bAn = First == TEXT('a') || First == TEXT('e') || First == TEXT('i') ||
					First == TEXT('o') || First == TEXT('u');
				Out = Modifier.Len() == 2 ? (bAn ? TEXT("An") : TEXT("A")) : (bAn ? TEXT("an") : TEXT("a"));
				return true;
			}
		}
		else if (!bHasModifier)
		{
			if (Key.Equals(TEXT("dmgType"), ESearchCase::CaseSensitive))
			{
				return ResolveVariantDamageName(Inputs.Base, Pointer, Out, State);
			}
			for (const TCHAR* Bonus : {TEXT("bonusAc"), TEXT("bonusWeapon"), TEXT("bonusWeaponAttack"),
				TEXT("bonusWeaponDamage"), TEXT("bonusWeaponCritDamage"), TEXT("bonusSpellAttack"),
				TEXT("bonusSpellSaveDc"), TEXT("bonusSavingThrow")})
			{
				if (!Key.Equals(Bonus, ESearchCase::CaseSensitive)) { continue; }
				const auto Value = FindExactField(Inputs.Inherits, Bonus);
				if (Value.IsValid() && (Value->Type == EJson::String || Value->Type == EJson::Number) &&
					Value->TryGetString(Out)) { return true; }
				return Fail(State, Codes::InvalidFieldValue, Pointer, TEXT("The bonus substitution needs a template string or number."));
			}
		}
		return Fail(State, Codes::UnsupportedRequiredValue, Pointer, TEXT("Unsupported item variant entry substitution."));
	}

	bool RenderVariantString(const FString& Text, FVariantEntryInputs& Inputs, const FString& Pointer,
		FString& Out, FReadState& State)
	{
		const FStringView View(Text);
		FString Candidate;
		int32 Cursor = 0;
		int32 Substitutions = 0;
		for (;;)
		{
			const int32 Open = View.Find(FStringView(TEXT("{=")), Cursor, ESearchCase::CaseSensitive);
			if (Open == INDEX_NONE)
			{
				if (Candidate.Len() + Text.Len() - Cursor > MaxRenderedStringLength)
				{
					return Fail(State, Codes::ResourceLimitExceeded, Pointer, TEXT("Rendered variant text exceeds its length limit."));
				}
				Candidate.AppendChars(*Text + Cursor, Text.Len() - Cursor);
				break;
			}

			const int32 Close = View.Find(FStringView(TEXT("}")), Open + 2, ESearchCase::CaseSensitive);
			if (Close == INDEX_NONE)
			{
				return Fail(State, Codes::InvalidFieldValue, Pointer, TEXT("Unclosed item variant substitution."));
			}
			if (++Substitutions > MaxSubstitutionsPerString || Close - Open > 128)
			{
				return Fail(State, Codes::ResourceLimitExceeded, Pointer, TEXT("Item variant substitution exceeds its token limits."));
			}
			FString Replacement;
			if (!ResolveVariantToken(Text.Mid(Open + 2, Close - Open - 2), Inputs, Pointer, Replacement, State)) { return false; }
			if (Candidate.Len() + Open - Cursor + Replacement.Len() > MaxRenderedStringLength)
			{
				return Fail(State, Codes::ResourceLimitExceeded, Pointer, TEXT("Rendered variant text exceeds its length limit."));
			}
			Candidate.AppendChars(*Text + Cursor, Open - Cursor);
			Candidate += Replacement;
			Cursor = Close + 1;
		}

		Inputs.RenderedCharacters += Candidate.Len();
		if (Inputs.RenderedCharacters > MaxRenderedEntryCharacters)
		{
			return Fail(State, Codes::ResourceLimitExceeded, Pointer, TEXT("Variant entry substitutions exceed their total output budget."));
		}
		Out = MoveTemp(Candidate);
		return true;
	}

	bool RenderVariantEntryValue(const TSharedPtr<FJsonValue>& Value, FVariantEntryInputs& Inputs,
		const FString& Pointer, TSharedPtr<FJsonValue>& Out, FReadState& State)
	{
		if (!ConsumeContentBudget(Pointer, State)) { return false; }
		if (Value->Type == EJson::String)
		{
			const FString Text = Value->AsString();
			if (Text.Contains(TEXT("{="), ESearchCase::CaseSensitive))
			{
				FString Rendered;
				if (!RenderVariantString(Text, Inputs, Pointer, Rendered, State)) { return false; }
				Out = MakeShared<FJsonValueString>(MoveTemp(Rendered));
				return true;
			}
		}
		else if (Value->Type == EJson::Array)
		{
			const auto& Source = Value->AsArray();
			TArray<TSharedPtr<FJsonValue>> Candidate;
			Candidate.Reserve(Source.Num());
			for (int32 Index = 0; Index < Source.Num(); ++Index)
			{
				TSharedPtr<FJsonValue> Child;
				if (!RenderVariantEntryValue(Source[Index], Inputs, AppendPointer(Pointer, FString::FromInt(Index)),
					Child, State)) { return false; }
				Candidate.Add(MoveTemp(Child));
			}
			Out = MakeShared<FJsonValueArray>(MoveTemp(Candidate));
			return true;
		}
		else if (Value->Type == EJson::Object)
		{
			const auto& Source = *Value->AsObject();
			auto Candidate = MakeShared<FJsonObject>();
			for (const auto& Pair : Source.Values)
			{
				const FString Key(Pair.Key.Len(), *Pair.Key);
				TSharedPtr<FJsonValue> Child;
				if (!RenderVariantEntryValue(Pair.Value, Inputs, AppendPointer(Pointer, Key), Child, State)) { return false; }
				Candidate->SetField(Key, Child);
			}
			Out = MakeShared<FJsonValueObject>(Candidate);
			return true;
		}
		Out = Value;
		return true;
	}
}

bool Dnd5eJsonReader::RenderItemVariantEntries(const TArray<TSharedPtr<FJsonValue>>& Entries, const FJsonObject& Base,
	const FJsonObject& Inherits, const FString& BaseName, const FString& Pointer,
	TArray<TSharedPtr<FJsonValue>>& Out, FReadState& State)
{
	FVariantEntryInputs Inputs{Base, Inherits, BaseName};
	TArray<TSharedPtr<FJsonValue>> Candidate;
	Candidate.Reserve(Entries.Num());
	for (int32 Index = 0; Index < Entries.Num(); ++Index)
	{
		TSharedPtr<FJsonValue> Entry;
		if (!RenderVariantEntryValue(Entries[Index], Inputs, AppendPointer(Pointer, FString::FromInt(Index)),
			Entry, State)) { return false; }
		Candidate.Add(MoveTemp(Entry));
	}
	Out = MoveTemp(Candidate);
	return true;
}
