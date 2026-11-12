// Developed by Neko Creative Technologies

#include "Readers/Bestiary/Dnd5eJsonCreatureCopyResolver.h"

namespace
{
	using namespace Dnd5eJsonReader;

	constexpr int32 MaxCreatureCopyLinks = 32;

	FString MakeLookupKey(const FString& Name, const FString& Source)
	{
		return EscapeIdentityComponent(Name.ToLower()) + TEXT("|") +
			EscapeIdentityComponent(Source.ToLower());
	}

	bool TryReadLookupKey(const FJsonObject& Object, FString& Out)
	{
		const auto NameValue = FindExactField(Object, TEXT("name"));
		const auto SourceValue = FindExactField(Object, TEXT("source"));
		FString Name;
		FString Source;
		if (!NameValue.IsValid() || NameValue->Type != EJson::String || !NameValue->TryGetString(Name) ||
			!SourceValue.IsValid() || SourceValue->Type != EJson::String || !SourceValue->TryGetString(Source) ||
			!IsValidIdentityText(Name) || !IsValidIdentityText(Source))
		{
			return false;
		}
		Out = MakeLookupKey(Name, Source);
		return true;
	}

	bool RequiresPreserve(const FString& Key)
	{
		static const TCHAR* Fields[] = {
			TEXT("page"), TEXT("otherSources"), TEXT("referenceSources"), TEXT("srd"), TEXT("srd52"),
			TEXT("basicRules"), TEXT("basicRules2024"), TEXT("reprintedAs"), TEXT("hasFluff"),
			TEXT("hasFluffImages"), TEXT("hasToken"), TEXT("tokenCredit"), TEXT("tokenCustom"),
			TEXT("foundryTokenScale"), TEXT("altArt"), TEXT("_versions"), TEXT("legendaryGroup"),
			TEXT("environment"), TEXT("soundClip"), TEXT("variant"), TEXT("dragonCastingColor"), TEXT("familiar"),
		};
		for (const TCHAR* Field : Fields)
		{
			if (Key.Equals(Field, ESearchCase::CaseSensitive)) { return true; }
		}
		return false;
	}

	bool HasPreservePermission(const TSharedPtr<FJsonObject>& Preserve, const TCHAR* Field)
	{
		if (!Preserve.IsValid()) { return false; }
		const auto Value = FindExactField(*Preserve, Field);
		return Value.IsValid() && Value->Type == EJson::Boolean && Value->AsBool();
	}

	struct FCreatureCopyStep
	{
		FDnd5eCreatureCopyRow Row;
		TSharedPtr<FJsonObject> Preserve;
		TSharedPtr<FJsonObject> Modifiers;
	};

	TSharedPtr<FJsonObject> MergeCreatureCopy(const FCreatureCopyStep& Child, const FJsonObject& Parent)
	{
		const auto Merged = MakeShared<FJsonObject>();
		for (const auto& Pair : Child.Row.Object->Values)
		{
			const FString Key(Pair.Key.Len(), *Pair.Key);
			if (!Key.Equals(TEXT("_copy"), ESearchCase::CaseSensitive))
			{
				Merged->SetField(Key, Pair.Value);
			}
		}
		const bool bPreserveAll = HasPreservePermission(Child.Preserve, TEXT("*"));
		for (const auto& Pair : Parent.Values)
		{
			const FString Key(Pair.Key.Len(), *Pair.Key);
			const auto OwnValue = FindExactField(*Child.Row.Object, *Key);
			if (OwnValue.IsValid())
			{
				if (OwnValue->Type == EJson::Null) { Merged->RemoveField(Key); }
			}
			else if (bPreserveAll || !RequiresPreserve(Key) || HasPreservePermission(Child.Preserve, *Key))
			{
				Merged->SetField(Key, Pair.Value);
			}
		}
		return Merged;
	}

	int32 FindCreatureArrayEntry(const TArray<TSharedPtr<FJsonValue>>& Entries, const FString& Name)
	{
		return Entries.IndexOfByPredicate([&Name](const TSharedPtr<FJsonValue>& Entry)
		{
			if (!Entry.IsValid()) { return false; }
			if (Entry->Type == EJson::String)
			{
				return Entry->AsString().Equals(Name, ESearchCase::CaseSensitive);
			}
			if (Entry->Type != EJson::Object) { return false; }
			const auto Value = FindExactField(*Entry->AsObject(), TEXT("name"));
			return Value.IsValid() && Value->Type == EJson::String &&
				Value->AsString().Equals(Name, ESearchCase::CaseSensitive);
		});
	}

	bool RemoveCreatureArrayEntries(const FJsonObject& Operation, const FString& Pointer,
		const FString& ModPointer, TArray<TSharedPtr<FJsonValue>>& Entries, FReadState& State)
	{
		const FString NamesPointer = AppendPointer(Pointer, TEXT("names"));
		if (FindExactField(Operation, TEXT("items")).IsValid())
		{
			return Fail(State, Codes::UnsupportedRequiredValue, ModPointer,
				TEXT("Creature copy removal by literal item is deferred."));
		}
		if (!CheckAllowedKeys(Operation, {TEXT("mode"), TEXT("names"), TEXT("force")}, Pointer, State)) { return false; }

		bool bForce = false;
		if (!ReadOptionalBool(Operation, TEXT("force"), Pointer, bForce, State)) { return false; }

		const auto Value = FindExactField(Operation, TEXT("names"));
		if (!Value.IsValid())
		{
			return Fail(State, Codes::MissingRequiredField, NamesPointer, TEXT("Expected removal names."));
		}
		TArray<TSharedPtr<FJsonValue>> Names;
		if (Value->Type == EJson::Array) { Names = Value->AsArray(); }
		else { Names.Add(Value); }

		for (int32 Index = 0; Index < Names.Num(); ++Index)
		{
			const FString NamePointer = Value->Type == EJson::Array
				? AppendPointer(NamesPointer, FString::FromInt(Index)) : NamesPointer;
			if (!Names[Index].IsValid() || Names[Index]->Type != EJson::String)
			{
				return Fail(State, Codes::InvalidFieldValue, NamePointer, TEXT("Expected a removal name string."));
			}

			const int32 Match = FindCreatureArrayEntry(Entries, Names[Index]->AsString());
			if (Match != INDEX_NONE) { Entries.RemoveAt(Match); }
			else if (!bForce)
			{
				return Fail(State, Codes::UnresolvedReference, NamePointer, TEXT("Creature array entry was not found."));
			}
		}
		return true;
	}

	bool ApplyCreatureArrayOperation(FJsonObject& Merged, const FString& Field,
		const FJsonObject& Operation, const FString& Pointer, const FString& ModPointer, FReadState& State)
	{
		FString Mode;
		if (!ReadRequiredString(Operation, TEXT("mode"), Pointer, Mode, State)) { return false; }

		const bool bPrepend = Mode.Equals(TEXT("prependArr"), ESearchCase::CaseSensitive);
		const bool bAppend = Mode.Equals(TEXT("appendArr"), ESearchCase::CaseSensitive);
		const bool bReplace = Mode.Equals(TEXT("replaceArr"), ESearchCase::CaseSensitive);
		const bool bRemove = Mode.Equals(TEXT("removeArr"), ESearchCase::CaseSensitive);
		if (!bPrepend && !bAppend && !bReplace && !bRemove)
		{
			return Fail(State, Codes::UnsupportedRequiredValue, ModPointer,
				TEXT("Creature copy modifier mode is deferred: ") + Mode);
		}

		const auto Existing = FindExactField(Merged, *Field);
		const bool bHasTarget = Existing.IsValid() && Existing->Type != EJson::Null;
		if (!bHasTarget && (bReplace || bRemove))
		{
			return Fail(State, Codes::InvalidFieldValue, Pointer, TEXT("Expected an existing creature array."));
		}
		const TArray<TSharedPtr<FJsonValue>>* ExistingItems = nullptr;
		if (bHasTarget && !RequireArray(Existing, Pointer, ExistingItems, State)) { return false; }
		TArray<TSharedPtr<FJsonValue>> Entries;
		if (ExistingItems) { Entries = *ExistingItems; }

		if (bRemove)
		{
			if (!RemoveCreatureArrayEntries(Operation, Pointer, ModPointer, Entries, State)) { return false; }
		}
		else
		{
			if (bReplace)
			{
				if (!CheckAllowedKeys(Operation, {TEXT("mode"), TEXT("replace"), TEXT("items")}, Pointer, State))
				{
					return false;
				}
			}
			else if (!CheckAllowedKeys(Operation, {TEXT("mode"), TEXT("items")}, Pointer, State))
			{
				return false;
			}

			const auto Items = FindExactField(Operation, TEXT("items"));
			const FString ItemsPointer = AppendPointer(Pointer, TEXT("items"));
			if (!Items.IsValid())
			{
				return Fail(State, Codes::MissingRequiredField, ItemsPointer, TEXT("Expected modifier items."));
			}
			TArray<TSharedPtr<FJsonValue>> Added;
			if (Items->Type == EJson::Array) { Added = Items->AsArray(); }
			else if (Items->Type == EJson::Object) { Added.Add(Items); }
			else { return Fail(State, Codes::InvalidFieldValue, ItemsPointer, TEXT("Expected an entry object or array.")); }

			int32 InsertAt = bPrepend ? 0 : Entries.Num();
			if (bReplace)
			{
				const auto Selector = FindExactField(Operation, TEXT("replace"));
				const FString ReplacePointer = AppendPointer(Pointer, TEXT("replace"));
				if (Selector.IsValid() && Selector->Type == EJson::Object)
				{
					return Fail(State, Codes::UnsupportedRequiredValue, ModPointer,
						TEXT("Creature copy replacement by index or regex is deferred."));
				}
				FString Name;
				if (!ReadRequiredString(Operation, TEXT("replace"), Pointer, Name, State)) { return false; }
				InsertAt = FindCreatureArrayEntry(Entries, Name);
				if (InsertAt == INDEX_NONE)
				{
					return Fail(State, Codes::UnresolvedReference, ReplacePointer, TEXT("Creature array entry was not found."));
				}
				Entries.RemoveAt(InsertAt);
			}

			if (!Added.IsEmpty()) { Entries.Insert(Added.GetData(), Added.Num(), InsertAt); }
		}

		Merged.SetField(Field, MakeShared<FJsonValueArray>(MoveTemp(Entries)));
		return true;
	}

	bool ApplyCreatureCopyModifiers(const FJsonObject& Modifiers, const FString& ModPointer,
		FJsonObject& Merged, FReadState& State)
	{
		for (const auto& Pair : Modifiers.Values)
		{
			const FString Field(Pair.Key.Len(), *Pair.Key);
			if (!Field.Equals(TEXT("trait"), ESearchCase::CaseSensitive) &&
				!Field.Equals(TEXT("action"), ESearchCase::CaseSensitive) &&
				!Field.Equals(TEXT("bonus"), ESearchCase::CaseSensitive))
			{
				return Fail(State, Codes::UnsupportedRequiredValue, ModPointer,
					TEXT("Creature copy modifier target is deferred: ") + Field);
			}

			const FString FieldPointer = AppendPointer(ModPointer, Field);
			const bool bArray = Pair.Value.IsValid() && Pair.Value->Type == EJson::Array;
			TArray<TSharedPtr<FJsonValue>> Operations;
			if (bArray) { Operations = Pair.Value->AsArray(); }
			else { Operations.Add(Pair.Value); }

			for (int32 Index = 0; Index < Operations.Num(); ++Index)
			{
				const FString Pointer = bArray ? AppendPointer(FieldPointer, FString::FromInt(Index)) : FieldPointer;
				const auto& Value = Operations[Index];
				if (Value.IsValid() && Value->Type == EJson::String &&
					Value->AsString().Equals(TEXT("remove"), ESearchCase::CaseSensitive))
				{
					Merged.RemoveField(Field);
					continue;
				}

				TSharedPtr<FJsonObject> Operation;
				if (!RequireObject(Value, Pointer, Operation, State) ||
					!ApplyCreatureArrayOperation(Merged, Field, *Operation, Pointer, ModPointer, State))
				{
					return false;
				}
			}
		}
		return true;
	}

	bool CollectCreatureCopyChain(const FDnd5eCreatureCopyIndex& Index,
		const FDnd5eCreatureCopyRow& Child, TArray<FCreatureCopyStep>& Chain, FReadState& State)
	{
		TSet<FString> Visited;
		const FDnd5eCreatureCopyRow* Current = &Child;
		for (;;)
		{
			auto Context = State.Context;
			Context.RelativeFilePath = Current->RelativeFilePath;
			FReadState RowState{Context, State.JsonText, State.Diagnostics};
			const FString CopyPointer = AppendPointer(Current->JsonPointer, TEXT("_copy"));

			FString Key;
			if (!Current->Object.IsValid() || !TryReadLookupKey(*Current->Object, Key))
			{
				return Fail(RowState, Codes::InvalidFieldValue, Current->JsonPointer,
					TEXT("Expected a creature name and explicit source."));
			}
			if (Index.AmbiguousKeys.Contains(Key))
			{
				return Fail(RowState, Codes::InvalidFieldValue, Current->JsonPointer,
					TEXT("Creature identity is ambiguous."));
			}
			Visited.Add(Key);

			FCreatureCopyStep Step{*Current, nullptr, nullptr};
			const auto CopyValue = FindExactField(*Current->Object, TEXT("_copy"));
			if (!CopyValue.IsValid())
			{
				Chain.Add(MoveTemp(Step));
				return true;
			}
			if (Chain.Num() >= MaxCreatureCopyLinks)
			{
				return Fail(RowState, Codes::ResourceLimitExceeded, CopyPointer,
					TEXT("Too many links in the creature copy chain."));
			}

			TSharedPtr<FJsonObject> Copy;
			FString ParentName;
			FString ParentSource;
			if (!RequireObject(CopyValue, CopyPointer, Copy, RowState) ||
				!CheckAllowedKeys(*Copy, {TEXT("name"), TEXT("source"), TEXT("_preserve"), TEXT("_mod"), TEXT("_templates")},
					CopyPointer, RowState) ||
				!ReadRequiredString(*Copy, TEXT("name"), CopyPointer, ParentName, RowState) ||
				!ValidateIdentityText(ParentName, AppendPointer(CopyPointer, TEXT("name")), RowState) ||
				!ReadRequiredString(*Copy, TEXT("source"), CopyPointer, ParentSource, RowState) ||
				!ValidateIdentityText(ParentSource, AppendPointer(CopyPointer, TEXT("source")), RowState))
			{
				return false;
			}
			if (FindExactField(*Copy, TEXT("_templates")).IsValid())
			{
				return Fail(RowState, Codes::UnsupportedRequiredValue, AppendPointer(CopyPointer, TEXT("_templates")),
					TEXT("Creature copy templates are deferred."));
			}
			if (const auto Modifiers = FindExactField(*Copy, TEXT("_mod")))
			{
				if (!RequireObject(Modifiers, AppendPointer(CopyPointer, TEXT("_mod")), Step.Modifiers, RowState))
				{
					return false;
				}
			}
			if (const auto Preserve = FindExactField(*Copy, TEXT("_preserve")))
			{
				const FString PreservePointer = AppendPointer(CopyPointer, TEXT("_preserve"));
				if (!RequireObject(Preserve, PreservePointer, Step.Preserve, RowState)) { return false; }
				for (const auto& Pair : Step.Preserve->Values)
				{
					if (!Pair.Value.IsValid() || Pair.Value->Type != EJson::Boolean)
					{
						const FString Field(Pair.Key.Len(), *Pair.Key);
						return Fail(RowState, Codes::InvalidFieldValue, AppendPointer(PreservePointer, Field),
							TEXT("Expected a boolean preserve flag."));
					}
				}
			}

			const FString ParentKey = MakeLookupKey(ParentName, ParentSource);
			if (Visited.Contains(ParentKey))
			{
				return Fail(RowState, Codes::InvalidFieldValue, CopyPointer, TEXT("Creature copy cycle."));
			}
			if (Index.AmbiguousKeys.Contains(ParentKey))
			{
				return Fail(RowState, Codes::UnresolvedReference, CopyPointer, TEXT("Creature copy parent is ambiguous."));
			}
			const FDnd5eCreatureCopyRow* Parent = Index.Rows.Find(ParentKey);
			if (!Parent)
			{
				return Fail(RowState, Codes::UnresolvedReference, CopyPointer, TEXT("Creature copy parent was not found."));
			}
			Chain.Add(MoveTemp(Step));
			Current = Parent;
		}
	}
}

void IndexCreatureCopyRow(FDnd5eCreatureCopyIndex& Index, const FDnd5eCreatureCopyRow& Row)
{
	FString Key;
	if (!Row.Object.IsValid() || !TryReadLookupKey(*Row.Object, Key) || Index.AmbiguousKeys.Contains(Key)) { return; }
	if (Index.Rows.Remove(Key) != 0)
	{
		Index.AmbiguousKeys.Add(Key);
		return;
	}
	Index.Rows.Add(MoveTemp(Key), Row);
}

bool ResolveCreatureCopy(const FDnd5eCreatureCopyIndex& Index, const FDnd5eCreatureCopyRow& Child,
	TSharedPtr<FJsonObject>& OutObject, TArray<FDnd5eCreatureCopyRow>& OutAncestors,
	Dnd5eJsonReader::FReadState& State)
{
	TArray<FCreatureCopyStep> Chain;
	if (!CollectCreatureCopyChain(Index, Child, Chain, State)) { return false; }

	TSharedPtr<FJsonObject> Assembled = Chain.Last().Row.Object;
	for (int32 IndexInChain = Chain.Num() - 2; IndexInChain >= 0; --IndexInChain)
	{
		const auto& Step = Chain[IndexInChain];
		Assembled = MergeCreatureCopy(Step, *Assembled);
		if (Step.Modifiers.IsValid())
		{
			auto Context = State.Context;
			Context.RelativeFilePath = Step.Row.RelativeFilePath;
			Dnd5eJsonReader::FReadState RowState{Context, State.JsonText, State.Diagnostics};
			const FString ModPointer = Dnd5eJsonReader::AppendPointer(
				Dnd5eJsonReader::AppendPointer(Step.Row.JsonPointer, TEXT("_copy")), TEXT("_mod"));
			if (!ApplyCreatureCopyModifiers(*Step.Modifiers, ModPointer, *Assembled, RowState)) { return false; }
		}
	}

	TArray<FDnd5eCreatureCopyRow> Ancestors;
	for (int32 IndexInChain = 1; IndexInChain < Chain.Num(); ++IndexInChain)
	{
		Ancestors.Add(Chain[IndexInChain].Row);
	}
	OutObject = MoveTemp(Assembled);
	OutAncestors = MoveTemp(Ancestors);
	return true;
}