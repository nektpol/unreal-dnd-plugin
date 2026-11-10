// Developed by Neko Creative Technologies

#include "Readers/Items/Dnd5eJsonItemEntryStrings.h"

bool Dnd5eJsonReader::TransformItemEntryStrings(const TSharedPtr<FJsonValue>& Value, const FString& Pointer,
	TSharedPtr<FJsonValue>& Out, FReadState& State,
	TFunctionRef<bool(const FString&, const FString&, FString&, FReadState&)> Render, int32 Depth)
{
	if (!Value.IsValid()) { return Fail(State, Codes::InvalidFieldValue, Pointer, TEXT("Missing item entry value.")); }
	if (Depth > 64) { return Fail(State, Codes::ResourceLimitExceeded, Pointer, TEXT("Item entry nesting exceeds its limit.")); }
	if (!ConsumeContentBudget(Pointer, State)) { return false; }
	if (Value->Type == EJson::String)
	{
		const FString Text = Value->AsString();
		FString Rendered;
		if (!Render(Text, Pointer, Rendered, State)) { return false; }
		Out = Value;
		if (!Rendered.Equals(Text, ESearchCase::CaseSensitive)) { Out = MakeShared<FJsonValueString>(MoveTemp(Rendered)); }
		return true;
	}
	if (Value->Type == EJson::Array)
	{
		TArray<TSharedPtr<FJsonValue>> Candidate;
		const auto& Entries = Value->AsArray();
		Candidate.Reserve(Entries.Num());
		for (int32 Index = 0; Index < Entries.Num(); ++Index)
		{
			TSharedPtr<FJsonValue> Child;
			if (!TransformItemEntryStrings(Entries[Index], AppendPointer(Pointer, FString::FromInt(Index)),
				Child, State, Render, Depth + 1)) { return false; }
			Candidate.Add(MoveTemp(Child));
		}
		Out = MakeShared<FJsonValueArray>(MoveTemp(Candidate));
		return true;
	}
	if (Value->Type == EJson::Object)
	{
		auto Candidate = MakeShared<FJsonObject>();
		for (const auto& Field : Value->AsObject()->Values)
		{
			const FString Key(Field.Key.Len(), *Field.Key);
			TSharedPtr<FJsonValue> Child;
			if (!TransformItemEntryStrings(Field.Value, AppendPointer(Pointer, Key), Child, State, Render, Depth + 1)) { return false; }
			Candidate->SetField(Key, Child);
		}
		Out = MakeShared<FJsonValueObject>(Candidate);
		return true;
	}
	Out = Value;
	return true;
}