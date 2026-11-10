// Developed by Neko Creative Technologies

#include "Dnd5eJsonItemCopyPolicy.h"


namespace Dnd5eJsonReader
{
	bool ReadPreserveMap(const FJsonObject& Copy, const FString& Pointer, TSharedPtr<FJsonObject>& Out, FReadState& State)
	{
		const auto Value = FindExactField(Copy, TEXT("_preserve"));
		if (!Value.IsValid()) { return true; }

		const FString PreservePointer = AppendPointer(Pointer, TEXT("_preserve"));
		if (!RequireObject(Value, PreservePointer, Out, State)) { return false; }
		for (const auto& Pair : Out->Values)
		{
			if (Pair.Value->Type != EJson::Boolean)
			{
				const FString Key(Pair.Key.Len(), *Pair.Key);
				return Fail(State, Codes::InvalidFieldValue, AppendPointer(PreservePointer, Key),
					TEXT("Expected a Boolean copy-preserve value."));
			}
		}
		return true;
	}

	bool IsProtectedCopyField(const FString& Key)
	{
		const TCHAR* const Protected[] = {
			TEXT("page"), TEXT("otherSources"), TEXT("referenceSources"),
			TEXT("srd"), TEXT("srd52"), TEXT("basicRules"), TEXT("basicRules2024"),
			TEXT("reprintedAs"), TEXT("hasFluff"), TEXT("hasFluffImages"),
			TEXT("hasToken"), TEXT("tokenCredit"), TEXT("tokenCustom"), TEXT("foundryTokenScale"),
			TEXT("altArt"), TEXT("_versions"), TEXT("lootTables"), TEXT("tier")
		};
		for (const TCHAR* Field : Protected)
		{
			if (Key.Equals(Field, ESearchCase::CaseSensitive)) { return true; }
		}
		return false;
	}

	bool IsPreserved(const TSharedPtr<FJsonObject>& Preserve, const TCHAR* Key)
	{
		if (!Preserve.IsValid()) { return false; }
		const auto IsTrue = [&Preserve](const TCHAR* Field)
		{
			const auto Value = FindExactField(*Preserve, Field);
			bool bValue = false;
			return Value.IsValid() && Value->TryGetBool(bValue) && bValue;
		};
		return IsTrue(TEXT("*")) || IsTrue(Key);
	}

	bool ContainsCopyVariable(const FJsonValue& Value)
	{
		if (Value.Type == EJson::String)
		{
			return Value.AsString().Contains(TEXT("<$"), ESearchCase::CaseSensitive);
		}
		if (Value.Type == EJson::Array)
		{
			for (const auto& Entry : Value.AsArray())
			{
				if (ContainsCopyVariable(*Entry)) { return true; }
			}
		}
		if (Value.Type == EJson::Object)
		{
			for (const auto& Pair : Value.AsObject()->Values)
			{
				if (ContainsCopyVariable(*Pair.Value)) { return true; }
			}
		}
		return false;
	}
}