// Developed by Neko Creative Technologies

#include "Readers/Items/Dnd5eJsonItemCopyModifiers.h"
#include "Algo/BinarySearch.h"

namespace
{
	using namespace Dnd5eJsonReader;

	constexpr int32 MaxDirectivesPerField = 64;
	constexpr int32 MaxModifiedArrayValues = 4096;

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

	bool ReadCopyItems(const FJsonObject& Operation, const FString& Pointer,
		TArray<TSharedPtr<FJsonValue>>& Items, FReadState& State)
	{
		const FString ItemsPointer = AppendPointer(Pointer, TEXT("items"));
		const auto Value = FindExactField(Operation, TEXT("items"));
		if (!Value.IsValid())
		{
			return Fail(State, Codes::MissingRequiredField, ItemsPointer, TEXT("A copy modifier needs items."));
		}
		if (Value->Type == EJson::Array) { Items = Value->AsArray(); }
		else { Items.Add(Value); }
		if (Items.Num() > MaxModifiedArrayValues)
		{
			return Fail(State, Codes::ResourceLimitExceeded, ItemsPointer, TEXT("Too many copy modifier items."));
		}
		if (ContainsCopyVariable(*Value))
		{
			return Fail(State, Codes::UnsupportedRequiredValue, ItemsPointer, TEXT("Copy variables are unsupported."));
		}
		return true;
	}

	bool SelectPropertyAdditions(const TArray<TSharedPtr<FJsonValue>>& Before,
		TArray<TSharedPtr<FJsonValue>>& Items, const FString& Pointer, FReadState& State)
	{
		TArray<FString> Original;
		for (const auto& Value : Before)
		{
			if (Value->Type != EJson::String)
			{
				return Fail(State, Codes::UnsupportedRequiredValue, Pointer, TEXT("Conditional property append requires string properties."));
			}
			Original.Add(Value->AsString());
		}
		const auto Less = [](const FString& A, const FString& B)
		{
			return A.Compare(B, ESearchCase::CaseSensitive) < 0;
		};
		Original.Sort(Less);

		for (int32 Index = 0; Index < Items.Num(); ++Index)
		{
			if (Items[Index]->Type != EJson::String)
			{
				return Fail(State, Codes::UnsupportedRequiredValue, AppendPointer(Pointer, TEXT("items")),
					TEXT("Conditional property append requires string items."));
			}
		}
		Items.RemoveAll([&](const TSharedPtr<FJsonValue>& Value)
		{
			const FString Text = Value->AsString();
			const int32 Found = Algo::LowerBound(Original, Text, Less);
			return Original.IsValidIndex(Found) && Original[Found].Equals(Text, ESearchCase::CaseSensitive);
		});
		return true;
	}

	bool ApplyArrayOperation(const FJsonObject& Operation, const TCHAR* Field, const FString& Pointer,
		int32 SourceIndex, FJsonObject& Merged, TMap<FString, int32>& FieldSources, FReadState& State)
	{
		FString Mode;
		if (!ReadRequiredString(Operation, TEXT("mode"), Pointer, Mode, State)) { return false; }
		const bool bEntries = FCString::Strcmp(Field, TEXT("entries")) == 0;
		const bool bAppend = bEntries && Mode.Equals(TEXT("appendArr"), ESearchCase::CaseSensitive);
		const bool bInsert = bEntries && Mode.Equals(TEXT("insertArr"), ESearchCase::CaseSensitive);
		const bool bReplace = bEntries && Mode.Equals(TEXT("replaceArr"), ESearchCase::CaseSensitive);
		const bool bUnique = !bEntries && Mode.Equals(TEXT("appendIfNotExistsArr"), ESearchCase::CaseSensitive);
		if (!bAppend && !bInsert && !bReplace && !bUnique)
		{
			return Fail(State, Codes::UnsupportedRequiredValue, AppendPointer(Pointer, TEXT("mode")),
				TEXT("Unsupported modifier for this item field."));
		}

		if (bInsert)
		{
			if (!CheckAllowedKeys(Operation, {TEXT("mode"), TEXT("items"), TEXT("index")}, Pointer, State)) { return false; }
		}
		else if (bReplace)
		{
			if (!CheckAllowedKeys(Operation, {TEXT("mode"), TEXT("items"), TEXT("replace")}, Pointer, State)) { return false; }
		}
		else if (!CheckAllowedKeys(Operation, {TEXT("mode"), TEXT("items")}, Pointer, State)) { return false; }

		const auto Existing = FindExactField(Merged, Field);
		if (!Existing.IsValid() && FieldSources.Contains(Field))
		{
			return Fail(State, Codes::InvalidFieldValue, Pointer, TEXT("The target collides with a differently cased field."));
		}
		const bool bHasArray = Existing.IsValid() && Existing->Type == EJson::Array;
		if (!bHasArray && ((Existing.IsValid() && Existing->Type != EJson::Null) || bInsert || bReplace))
		{
			return Fail(State, Codes::InvalidFieldValue, Pointer, TEXT("This operation requires an array target."));
		}

		TArray<TSharedPtr<FJsonValue>> Values;
		if (bHasArray)
		{
			const auto& Current = Existing->AsArray();
			if (Current.Num() > MaxModifiedArrayValues)
			{
				return Fail(State, Codes::ResourceLimitExceeded, Pointer, TEXT("The modifier target array exceeds the size limit."));
			}
			Values = Current;
		}
		TArray<TSharedPtr<FJsonValue>> Items;
		if (!ReadCopyItems(Operation, Pointer, Items, State)) { return false; }
		if (bUnique && !SelectPropertyAdditions(Values, Items, Pointer, State)) { return false; }

		FString IndexLiteral;
		int32 Position = Values.Num();
		if (bInsert)
		{
			TSharedPtr<FJsonValue> IndexValue;
			if (!ReadRequiredValue(Operation, TEXT("index"), EJson::Number, Pointer, IndexValue, State)) { return false; }
			int32 Index = 0;
			if (!IndexValue->TryGetString(IndexLiteral) ||
				(IndexLiteral != TEXT("-1") && !TryParseBoundedIntegerToken(IndexLiteral, 0, MAX_int32, Index)))
			{
				return Fail(State, Codes::InvalidFieldValue, AppendPointer(Pointer, TEXT("index")),
					TEXT("Expected -1 or a nonnegative integer insert index."));
			}
			if (IndexLiteral != TEXT("-1")) { Position = FMath::Min(Index, Values.Num()); }
		}
		if (bReplace)
		{
			TSharedPtr<FJsonObject> Replace;
			const FString ReplacePointer = AppendPointer(Pointer, TEXT("replace"));
			if (!ReadRequiredObject(Operation, TEXT("replace"), Pointer, Replace, State) ||
				!CheckAllowedKeys(*Replace, {TEXT("index")}, ReplacePointer, State) ||
				!ReadRequiredInteger(*Replace, TEXT("index"), ReplacePointer, 0, MAX_int32, Position, IndexLiteral, State)) { return false; }
			if (!Values.IsValidIndex(Position))
			{
				return Fail(State, Codes::InvalidFieldValue, AppendPointer(ReplacePointer, TEXT("index")),
					TEXT("The replace index is outside the target array."));
			}
		}

		const int32 RemoveCount = bReplace ? 1 : 0;
		if (Values.Num() - RemoveCount + Items.Num() > MaxModifiedArrayValues)
		{
			return Fail(State, Codes::ResourceLimitExceeded, Pointer, TEXT("The modified array exceeds the size limit."));
		}
		if (bHasArray && RemoveCount == 0 && Items.IsEmpty()) { return true; }
		if (bReplace) { Values.RemoveAt(Position); }
		Values.Insert(Items, Position);
		Merged.SetField(Field, MakeShared<FJsonValueArray>(MoveTemp(Values)));
		FieldSources.Add(Field, SourceIndex);
		return true;
	}

	bool ApplyFieldOperations(const FJsonObject& Modifiers, const TCHAR* Field, const FString& Pointer,
		int32 SourceIndex, FJsonObject& Merged, TMap<FString, int32>& FieldSources, FReadState& State)
	{
		const auto Value = FindExactField(Modifiers, Field);
		if (!Value.IsValid()) { return true; }
		const FString FieldPointer = AppendPointer(Pointer, Field);
		const auto Apply = [&](const TSharedPtr<FJsonValue>& Entry, const FString& EntryPointer)
		{
			TSharedPtr<FJsonObject> Operation;
			return RequireObject(Entry, EntryPointer, Operation, State) &&
				ApplyArrayOperation(*Operation, Field, EntryPointer, SourceIndex, Merged, FieldSources, State);
		};

		if (Value->Type != EJson::Array) { return Apply(Value, FieldPointer); }
		const auto& Directives = Value->AsArray();
		if (Directives.Num() > MaxDirectivesPerField)
		{
			return Fail(State, Codes::ResourceLimitExceeded, FieldPointer, TEXT("Too many modifiers for one field."));
		}
		for (int32 Index = 0; Index < Directives.Num(); ++Index)
		{
			if (!Apply(Directives[Index], AppendPointer(FieldPointer, FString::FromInt(Index)))) { return false; }
		}
		return true;
	}
}

bool Dnd5eJsonReader::ApplyItemCopyModifiers(const FJsonObject& Modifiers, const FString& Pointer, int32 SourceIndex,
	FJsonObject& Merged, TMap<FString, int32>& FieldSources, FReadState& State)
{
	return CheckAllowedKeys(Modifiers, {TEXT("entries"), TEXT("property")}, Pointer, State) &&
		ApplyFieldOperations(Modifiers, TEXT("entries"), Pointer, SourceIndex, Merged, FieldSources, State) &&
		ApplyFieldOperations(Modifiers, TEXT("property"), Pointer, SourceIndex, Merged, FieldSources, State);
}
