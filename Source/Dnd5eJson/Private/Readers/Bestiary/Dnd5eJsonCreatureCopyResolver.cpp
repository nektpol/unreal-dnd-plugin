// Developed by Neko Creative Technologies

#include "Readers/Bestiary/Dnd5eJsonCreatureCopyResolver.h"
#include "Containers/StringConv.h"

#if UE_ENABLE_ICU
THIRD_PARTY_INCLUDES_START
#include <string>
#include <unicode/regex.h>
THIRD_PARTY_INCLUDES_END
#endif

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
		TArray<FDnd5eCreatureCopyRow> Templates;
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
		const bool bAppendUnique = Mode.Equals(TEXT("appendIfNotExistsArr"), ESearchCase::CaseSensitive);
		const bool bReplace = Mode.Equals(TEXT("replaceArr"), ESearchCase::CaseSensitive);
		const bool bRemove = Mode.Equals(TEXT("removeArr"), ESearchCase::CaseSensitive);
		if (!bPrepend && !bAppend && !bAppendUnique && !bReplace && !bRemove)
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
			if (Items->Type == EJson::Array)
			{
				Added = Items->AsArray();
			}
			else if (Items->Type == EJson::Object || (bAppendUnique && Items->Type == EJson::String))
			{
				Added.Add(Items);
			}
			else
			{
				return Fail(State, Codes::InvalidFieldValue, ItemsPointer,
					TEXT("Expected modifier items in a supported form."));
			}

			if (bAppendUnique && ExistingItems)
			{
				Added.RemoveAll([ExistingItems](const TSharedPtr<FJsonValue>& Candidate)
				{
					return Candidate.IsValid() && ExistingItems->ContainsByPredicate(
						[&Candidate](const TSharedPtr<FJsonValue>& ExistingEntry)
						{
							return ExistingEntry.IsValid() && FJsonValue::CompareEqual(*Candidate, *ExistingEntry);
						});
				});
			}

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
					return Fail(State, Codes::UnresolvedReference, ReplacePointer,
						TEXT("Creature array entry was not found."));
				}
				Entries.RemoveAt(InsertAt);
			}

			if (!Added.IsEmpty()) { Entries.Insert(Added.GetData(), Added.Num(), InsertAt); }
		}

		Merged.SetField(Field, MakeShared<FJsonValueArray>(MoveTemp(Entries)));
		return true;
	}

#if UE_ENABLE_ICU
	icu::UnicodeString CreatureTextToIcu(const FString& Text)
	{
		const FTCHARToUTF8 Utf8(*Text, Text.Len());
		return icu::UnicodeString::fromUTF8(icu::StringPiece(Utf8.Get(), Utf8.Length()));
	}

	struct FCreatureTextReplacement
	{
		const icu::RegexPattern& Pattern;
		const icu::UnicodeString& Replacement;
		const FString& OperationPointer;
		bool bTagInsensitive;

		bool ReplaceSegment(const FString& Input, FString& Out, FReadState& State) const
		{
			const auto Text = CreatureTextToIcu(Input);
			UErrorCode Status = U_ZERO_ERROR;
			TUniquePtr<icu::RegexMatcher> Matcher(Pattern.matcher(Text, Status));
			if (U_FAILURE(Status) || !Matcher.IsValid())
			{
				return Fail(State, Codes::InvalidFieldValue, OperationPointer, TEXT("Could not create text matcher."));
			}
			Matcher->setTimeLimit(50, Status);
			Matcher->setStackLimit(256 * 1024, Status);
			const auto Replaced = Matcher->replaceAll(Replacement, Status);
			if (U_FAILURE(Status))
			{
				return Fail(State, Codes::InvalidFieldValue, OperationPointer,
					TEXT("Creature text replacement failed or exceeded the matcher limits."));
			}

			std::string Utf8;
			Replaced.toUTF8String(Utf8);
			const FUTF8ToTCHAR Converted(Utf8.data(), static_cast<int32>(Utf8.size()));
			Out.AppendChars(Converted.Get(), Converted.Length());
			return true;
		}

		bool RewriteString(const FString& Input, FString& Out, FReadState& State) const
		{
			if (bTagInsensitive) { return ReplaceSegment(Input, Out, State); }

			int32 Cursor = 0;
			while (Cursor < Input.Len())
			{
				const int32 Start = Input.Find(TEXT("{@"), ESearchCase::CaseSensitive, ESearchDir::FromStart, Cursor);
				if (Start == INDEX_NONE) { break; }

				int32 Depth = 1;
				int32 End = Start + 2;
				for (; End < Input.Len() && Depth > 0; ++End)
				{
					if (Input[End] == TEXT('{')) { ++Depth; }
					else if (Input[End] == TEXT('}')) { --Depth; }
				}
				if (Depth != 0) { break; }

				if (Start > Cursor && !ReplaceSegment(Input.Mid(Cursor, Start - Cursor), Out, State)) { return false; }
				Out += Input.Mid(Start, End - Start);
				Cursor = End;
			}
			return Cursor < Input.Len() ? ReplaceSegment(Input.Mid(Cursor), Out, State) : true;
		}
	};

	bool RewriteCreatureTextValue(const TSharedPtr<FJsonValue>& Value, const FCreatureTextReplacement& Rule,
		int32 Depth, TSharedPtr<FJsonValue>& Out, FReadState& State)
	{
		if (Depth > 64)
		{
			return Fail(State, Codes::ResourceLimitExceeded, Rule.OperationPointer, TEXT("Creature text nesting is too deep."));
		}
		Out = Value;
		if (!Value.IsValid()) { return true; }

		if (Value->Type == EJson::String)
		{
			FString Text;
			if (!Rule.RewriteString(Value->AsString(), Text, State)) { return false; }
			Out = MakeShared<FJsonValueString>(MoveTemp(Text));
		}
		else if (Value->Type == EJson::Array)
		{
			TArray<TSharedPtr<FJsonValue>> Children;
			for (const auto& Child : Value->AsArray())
			{
				TSharedPtr<FJsonValue> Rewritten;
				if (!RewriteCreatureTextValue(Child, Rule, Depth + 1, Rewritten, State)) { return false; }
				Children.Add(MoveTemp(Rewritten));
			}
			Out = MakeShared<FJsonValueArray>(MoveTemp(Children));
		}
		else if (Value->Type == EJson::Object)
		{
			const auto Object = MakeShared<FJsonObject>();
			for (const auto& Pair : Value->AsObject()->Values)
			{
				const FString Key(Pair.Key.Len(), *Pair.Key);
				TSharedPtr<FJsonValue> Rewritten;
				if (!RewriteCreatureTextValue(Pair.Value, Rule, Depth + 1, Rewritten, State)) { return false; }
				Object->SetField(Key, MoveTemp(Rewritten));
			}
			Out = MakeShared<FJsonValueObject>(Object);
		}
		return true;
	}

	bool RewriteCreatureTextField(FJsonObject& Merged, const TCHAR* Field, bool bRewriteStrings,
		const TArray<FString>& Properties, const FCreatureTextReplacement& Rule, FReadState& State)
	{
		const auto Value = FindExactField(Merged, Field);
		if (!Value.IsValid() || Value->Type == EJson::Null) { return true; }

		const TArray<TSharedPtr<FJsonValue>>* Entries = nullptr;
		if (!RequireArray(Value, Rule.OperationPointer, Entries, State)) { return false; }

		TArray<TSharedPtr<FJsonValue>> RewrittenEntries;
		RewrittenEntries.Reserve(Entries->Num());
		for (const auto& Entry : *Entries)
		{
			TSharedPtr<FJsonValue> Rewritten = Entry;
			if (Entry.IsValid() && Entry->Type == EJson::String && bRewriteStrings)
			{
				if (!RewriteCreatureTextValue(Entry, Rule, 0, Rewritten, State)) { return false; }
			}
			else if (Entry.IsValid() && Entry->Type == EJson::Object && !Properties.IsEmpty())
			{
				const auto Object = MakeShared<FJsonObject>();
				for (const auto& Pair : Entry->AsObject()->Values)
				{
					const FString Key(Pair.Key.Len(), *Pair.Key);
					TSharedPtr<FJsonValue> Property = Pair.Value;
					const bool bSelected = Properties.ContainsByPredicate([&Key](const FString& Name)
					{
						return Name.Equals(Key, ESearchCase::CaseSensitive);
					});
					if (bSelected && !RewriteCreatureTextValue(Pair.Value, Rule, 0, Property, State)) { return false; }
					Object->SetField(Key, MoveTemp(Property));
				}
				Rewritten = MakeShared<FJsonValueObject>(Object);
			}
			RewrittenEntries.Add(MoveTemp(Rewritten));
		}

		Merged.SetField(Field, MakeShared<FJsonValueArray>(MoveTemp(RewrittenEntries)));
		return true;
	}
#endif

	bool ApplyCreatureReplaceText(const FJsonObject& Operation, const FString& Pointer,
		const FString& ModPointer, FJsonObject& Merged, FReadState& State)
	{
#if UE_ENABLE_ICU
		FString PatternText, Replacement, Flags;
		bool bTagInsensitive = false;
		if (!CheckAllowedKeys(Operation, {TEXT("mode"), TEXT("replace"), TEXT("with"), TEXT("flags"),
				TEXT("props"), TEXT("tagInsensitive")}, Pointer, State) ||
			!ReadRequiredString(Operation, TEXT("replace"), Pointer, PatternText, State) ||
			!ReadRequiredString(Operation, TEXT("with"), Pointer, Replacement, State) ||
			!ReadOptionalBool(Operation, TEXT("tagInsensitive"), Pointer, bTagInsensitive, State))
		{
			return false;
		}
		if (FindExactField(Operation, TEXT("flags")).IsValid() &&
			!ReadRequiredString(Operation, TEXT("flags"), Pointer, Flags, State))
		{
			return false;
		}
		if (!Flags.IsEmpty() && !Flags.Equals(TEXT("i"), ESearchCase::CaseSensitive))
		{
			return Fail(State, Codes::UnsupportedRequiredValue, ModPointer, TEXT("Creature text regex flags are deferred."));
		}
		for (int32 Index = 0; Index < Replacement.Len(); ++Index)
		{
			if (Replacement[Index] == TEXT('\\'))
			{
				return Fail(State, Codes::UnsupportedRequiredValue, ModPointer, TEXT("Creature replacement escapes are deferred."));
			}
			if (Replacement[Index] != TEXT('$')) { continue; }
			if (++Index >= Replacement.Len() || Replacement[Index] < TEXT('1') || Replacement[Index] > TEXT('9') ||
				(Index + 1 < Replacement.Len() && FChar::IsDigit(Replacement[Index + 1])))
			{
				return Fail(State, Codes::UnsupportedRequiredValue, ModPointer, TEXT("Creature replacement token is deferred."));
			}
		}

		bool bRewriteStrings = true;
		TArray<FString> Properties{TEXT("entries"), TEXT("headerEntries"), TEXT("footerEntries")};
		if (const auto Props = FindExactField(Operation, TEXT("props")))
		{
			const FString PropsPointer = AppendPointer(Pointer, TEXT("props"));
			const TArray<TSharedPtr<FJsonValue>>* Values = nullptr;
			if (!RequireArray(Props, PropsPointer, Values, State)) { return false; }
			Properties.Reset();
			bRewriteStrings = false;
			for (int32 Index = 0; Index < Values->Num(); ++Index)
			{
				const auto& Value = (*Values)[Index];
				if (Value.IsValid() && Value->Type == EJson::Null) { bRewriteStrings = true; }
				else if (Value.IsValid() && Value->Type == EJson::String) { Properties.Add(Value->AsString()); }
				else
				{
					return Fail(State, Codes::InvalidFieldValue, AppendPointer(PropsPointer, FString::FromInt(Index)),
						TEXT("Expected a property name or null."));
				}
			}
		}

		UErrorCode Status = U_ZERO_ERROR;
		const uint32 RegexFlags = Flags.IsEmpty() ? 0u : static_cast<uint32>(UREGEX_CASE_INSENSITIVE);
		const TUniquePtr<icu::RegexPattern> Pattern(icu::RegexPattern::compile(CreatureTextToIcu(PatternText), RegexFlags, Status));
		if (U_FAILURE(Status) || !Pattern.IsValid())
		{
			return Fail(State, Codes::InvalidFieldValue, AppendPointer(Pointer, TEXT("replace")), TEXT("Invalid creature regex pattern."));
		}
		if (!bRewriteStrings && Properties.IsEmpty()) { return true; }

		const auto IcuReplacement = CreatureTextToIcu(Replacement);
		const FCreatureTextReplacement Rule{*Pattern, IcuReplacement, Pointer, bTagInsensitive};
		static const TCHAR* Fields[] = {
			TEXT("action"), TEXT("bonus"), TEXT("reaction"), TEXT("trait"), TEXT("legendary"),
			TEXT("mythic"), TEXT("variant"), TEXT("spellcasting"), TEXT("actionHeader"), TEXT("bonusHeader"),
			TEXT("reactionHeader"), TEXT("legendaryHeader"), TEXT("mythicHeader"),
		};
		for (const TCHAR* Field : Fields)
		{
			if (!RewriteCreatureTextField(Merged, Field, bRewriteStrings, Properties, Rule, State)) { return false; }
		}
		return true;
#else
		return Fail(State, Codes::UnsupportedRequiredValue, ModPointer, TEXT("Creature regex replacement requires ICU."));
#endif
	}

	bool ApplyCreatureWildcardOperations(const TSharedPtr<FJsonValue>& Value, const FString& ModPointer,
		FJsonObject& Merged, FReadState& State)
	{
		const FString FieldPointer = AppendPointer(ModPointer, TEXT("*"));
		const bool bArray = Value.IsValid() && Value->Type == EJson::Array;
		TArray<TSharedPtr<FJsonValue>> Operations;
		if (bArray) { Operations = Value->AsArray(); }
		else { Operations.Add(Value); }

		for (int32 Index = 0; Index < Operations.Num(); ++Index)
		{
			const FString Pointer = bArray ? AppendPointer(FieldPointer, FString::FromInt(Index)) : FieldPointer;
			TSharedPtr<FJsonObject> Operation;
			FString Mode;
			if (!RequireObject(Operations[Index], Pointer, Operation, State) ||
				!ReadRequiredString(*Operation, TEXT("mode"), Pointer, Mode, State))
			{
				return false;
			}
			if (!Mode.Equals(TEXT("replaceTxt"), ESearchCase::CaseSensitive))
			{
				return Fail(State, Codes::UnsupportedRequiredValue, ModPointer,
					TEXT("Creature wildcard modifier mode is deferred: ") + Mode);
			}
			if (!ApplyCreatureReplaceText(*Operation, Pointer, ModPointer, Merged, State)) { return false; }
		}
		return true;
	}

	bool ApplyCreatureAddSenses(FJsonObject& Merged, const FJsonObject& Operation,
		const FString& Pointer, FReadState& State)
	{
		if (!CheckAllowedKeys(Operation, {TEXT("mode"), TEXT("senses")}, Pointer, State))
		{
			return false;
		}

		const FString SensesPointer = AppendPointer(Pointer, TEXT("senses"));
		const auto Value = FindExactField(Operation, TEXT("senses"));
		if (!Value.IsValid())
		{
			return Fail(State, Codes::MissingRequiredField, SensesPointer, TEXT("Expected senses."));
		}

		const bool bArray = Value->Type == EJson::Array;
		const TArray<TSharedPtr<FJsonValue>> Added = bArray
			? Value->AsArray() : TArray<TSharedPtr<FJsonValue>>{Value};
		TArray<TSharedPtr<FJsonValue>> Senses;
		if (const auto Existing = FindExactField(Merged, TEXT("senses")); Existing.IsValid() && Existing->Type != EJson::Null)
		{
			const TArray<TSharedPtr<FJsonValue>>* ExistingItems = nullptr;
			if (!RequireArray(Existing, SensesPointer, ExistingItems, State)) { return false; }
			Senses = *ExistingItems;
		}

		for (int32 Index = 0; Index < Added.Num(); ++Index)
		{
			const FString ItemPointer = bArray ? AppendPointer(SensesPointer, FString::FromInt(Index)) : SensesPointer;
			TSharedPtr<FJsonObject> Sense;
			FString Type;
			FString Literal;
			int32 Range = 0;
			if (!RequireObject(Added[Index], ItemPointer, Sense, State) ||
				!CheckAllowedKeys(*Sense, {TEXT("type"), TEXT("range")}, ItemPointer, State) ||
				!ReadRequiredString(*Sense, TEXT("type"), ItemPointer, Type, State) ||
				!ValidateIdentityText(Type, AppendPointer(ItemPointer, TEXT("type")), State) ||
				!ReadRequiredInteger(*Sense, TEXT("range"), ItemPointer, 0, MAX_int32, Range, Literal, State))
			{
				return false;
			}

			const FString Prefix = Type + TEXT(" ");
			const FString Formatted = FString::Printf(TEXT("%s %d ft."), *Type, Range);
			bool bMatched = false;
			for (auto& ExistingSense : Senses)
			{
				if (!ExistingSense.IsValid() || ExistingSense->Type != EJson::String) { continue; }
				const FString Text = ExistingSense->AsString();
				if (!Text.StartsWith(Prefix, ESearchCase::IgnoreCase)) { continue; }

				int32 End = Prefix.Len();
				while (End < Text.Len() && Text[End] >= TEXT('0') && Text[End] <= TEXT('9'))
				{
					++End;
				}
				int32 ExistingRange = 0;
				if (End == Prefix.Len() ||
					!LexTryParseString(ExistingRange, *Text.Mid(Prefix.Len(), End - Prefix.Len())))
				{
					continue;
				}
				if (ExistingRange < Range)
				{
					ExistingSense = MakeShared<FJsonValueString>(Formatted);
				}
				bMatched = true;
				break;
			}
			if (!bMatched)
			{
				Senses.Add(MakeShared<FJsonValueString>(Formatted));
			}
		}

		Merged.SetField(TEXT("senses"), MakeShared<FJsonValueArray>(MoveTemp(Senses)));
		return true;
	}

	bool ApplyCreatureAddSkills(FJsonObject& Merged, const FJsonObject& Operation,
		const FString& Pointer, const FString& ModPointer, FReadState& State)
	{
		if (!CheckAllowedKeys(Operation, {TEXT("mode"), TEXT("skills")}, Pointer, State))
		{
			return false;
		}

		const FString SkillsPointer = AppendPointer(Pointer, TEXT("skills"));
		TSharedPtr<FJsonValue> GrantsValue;
		TSharedPtr<FJsonObject> Grants;
		if (!ReadRequiredValue(Operation, TEXT("skills"), EJson::Object, Pointer, GrantsValue, State) ||
			!RequireObject(GrantsValue, SkillsPointer, Grants, State))
		{
			return false;
		}
		if (Grants->Values.IsEmpty()) { return true; }

		auto CrValue = FindExactField(Merged, TEXT("cr"));
		const TSharedPtr<FJsonObject>* CrObject = nullptr;
		if (CrValue.IsValid() && CrValue->Type == EJson::Object &&
			CrValue->TryGetObject(CrObject) && CrObject && CrObject->IsValid())
		{
			CrValue = FindExactField(**CrObject, TEXT("cr"));
		}

		FString CrToken;
		int32 WholeCr = 0;
		if (!CrValue.IsValid() || !CrValue->TryGetString(CrToken) ||
			(!(CrToken == TEXT("1/8") || CrToken == TEXT("1/4") || CrToken == TEXT("1/2")) &&
				!TryParseBoundedIntegerToken(CrToken, 0, 30, WholeCr)))
		{
			return Fail(State, Codes::UnsupportedRequiredValue, ModPointer,
				TEXT("Skill grants require a standard creature challenge rating."));
		}
		const int32 ProficiencyBonus = WholeCr < 5 ? 2 : (WholeCr + 3) / 4 + 1;

		const auto SkillValue = FindExactField(Merged, TEXT("skill"));
		const auto Skills = MakeShared<FJsonObject>();
		if (SkillValue.IsValid() && SkillValue->Type != EJson::Null)
		{
			TSharedPtr<FJsonObject> ExistingSkills;
			if (!RequireObject(SkillValue, SkillsPointer, ExistingSkills, State)) { return false; }
			Skills->Values = ExistingSkills->Values;
		}

		static const TMap<FString, const TCHAR*> AbilityBySkill{
			{TEXT("athletics"), TEXT("str")},
			{TEXT("acrobatics"), TEXT("dex")}, {TEXT("sleight of hand"), TEXT("dex")}, {TEXT("stealth"), TEXT("dex")},
			{TEXT("arcana"), TEXT("int")}, {TEXT("history"), TEXT("int")}, {TEXT("investigation"), TEXT("int")},
			{TEXT("nature"), TEXT("int")}, {TEXT("religion"), TEXT("int")},
			{TEXT("animal handling"), TEXT("wis")}, {TEXT("insight"), TEXT("wis")}, {TEXT("medicine"), TEXT("wis")},
			{TEXT("perception"), TEXT("wis")}, {TEXT("survival"), TEXT("wis")},
			{TEXT("deception"), TEXT("cha")}, {TEXT("intimidation"), TEXT("cha")},
			{TEXT("performance"), TEXT("cha")}, {TEXT("persuasion"), TEXT("cha")}
		};

		for (const auto& Pair : Grants->Values)
		{
			const FString Skill(Pair.Key.Len(), *Pair.Key);
			const FString SkillPointer = AppendPointer(SkillsPointer, Skill);
			const TCHAR* const* Ability = AbilityBySkill.Find(Skill);
			if (!Ability)
			{
				return Fail(State, Codes::InvalidFieldValue, SkillPointer, TEXT("Unknown skill in copy grant."));
			}

			int32 Rank = 0;
			int32 Score = 0;
			FString Literal;
			if (!ReadRequiredInteger(*Grants, *Skill, SkillsPointer, 1, 2, Rank, Literal, State) ||
				!ReadRequiredInteger(Merged, *Ability, Pointer, 0, MAX_int32, Score, Literal, State))
			{
				return false;
			}
			const int32 Bonus = Score / 2 - 5 + Rank * ProficiencyBonus;
			if (const auto Existing = FindExactField(*Skills, *Skill); Existing.IsValid() && Existing->Type != EJson::Null)
			{
				FString Token;
				if (!Existing->TryGetString(Token)) { continue; }

				const bool bNegative = Token.StartsWith(TEXT("-"));
				const FString Digits = bNegative || Token.StartsWith(TEXT("+")) ? Token.Mid(1) : Token;
				int32 Magnitude = 0;
				if (!TryParseBoundedIntegerToken(Digits, 0, MAX_int32, Magnitude) ||
					(bNegative ? -Magnitude : Magnitude) >= Bonus)
				{
					continue;
				}
			}
			Skills->SetStringField(Skill, FString::Printf(TEXT("%+d"), Bonus));
		}

		Merged.SetObjectField(TEXT("skill"), Skills);
		return true;
	}

	bool ReplaceCreatureSpellList(FJsonObject& Target, const FString& Field,
		const TSharedPtr<FJsonValue>& InstructionsValue, const FString& Pointer,
		const FString& ModPointer, FReadState& State)
	{
		const TArray<TSharedPtr<FJsonValue>>* Instructions = nullptr;
		const TArray<TSharedPtr<FJsonValue>>* Existing = nullptr;
		if (!RequireArray(InstructionsValue, Pointer, Instructions, State) ||
			!RequireArray(FindExactField(Target, *Field), Pointer, Existing, State))
		{
			return false;
		}
		if (Instructions->IsEmpty()) { return true; }

		TArray<FString> Spells;
		Spells.Reserve(Existing->Num());
		for (const auto& Entry : *Existing)
		{
			if (!Entry.IsValid() || Entry->Type != EJson::String)
			{
				return Fail(State, Codes::UnsupportedRequiredValue, ModPointer,
					TEXT("Replacing spells in a list containing non-string entries is deferred."));
			}
			Spells.Add(Entry->AsString());
		}

		for (int32 Index = 0; Index < Instructions->Num(); ++Index)
		{
			const FString ItemPointer = AppendPointer(Pointer, FString::FromInt(Index));
			TSharedPtr<FJsonObject> Instruction;
			FString Selector;
			if (!RequireObject((*Instructions)[Index], ItemPointer, Instruction, State) ||
				!CheckAllowedKeys(*Instruction, {TEXT("replace"), TEXT("with")}, ItemPointer, State) ||
				!ReadRequiredString(*Instruction, TEXT("replace"), ItemPointer, Selector, State))
			{
				return false;
			}

			const FString WithPointer = AppendPointer(ItemPointer, TEXT("with"));
			const auto With = FindExactField(*Instruction, TEXT("with"));
			if (!With.IsValid())
			{
				return Fail(State, Codes::MissingRequiredField, WithPointer, TEXT("Expected replacement spells."));
			}
			const bool bWithArray = With->Type == EJson::Array;
			const TArray<TSharedPtr<FJsonValue>> Replacements = bWithArray
				? With->AsArray() : TArray<TSharedPtr<FJsonValue>>{With};
			TArray<FString> Added;
			for (int32 ReplacementIndex = 0; ReplacementIndex < Replacements.Num(); ++ReplacementIndex)
			{
				const auto& Entry = Replacements[ReplacementIndex];
				if (!Entry.IsValid() || Entry->Type != EJson::String)
				{
					const FString EntryPointer = bWithArray
						? AppendPointer(WithPointer, FString::FromInt(ReplacementIndex)) : WithPointer;
					return Fail(State, Codes::InvalidFieldValue, EntryPointer, TEXT("Expected a spell string."));
				}
				Added.Add(Entry->AsString());
			}

			const int32 Found = Spells.IndexOfByPredicate([&Selector](const FString& Spell)
			{
				return Spell.Equals(Selector, ESearchCase::CaseSensitive);
			});
			if (Found == INDEX_NONE)
			{
				return Fail(State, Codes::UnresolvedReference, AppendPointer(ItemPointer, TEXT("replace")),
					TEXT("Spell to replace was not found."));
			}
			Spells.RemoveAt(Found);
			if (!Added.IsEmpty()) { Spells.Insert(Added.GetData(), Added.Num(), Found); }
			Spells.StableSort([](const FString& Left, const FString& Right)
			{
				return Left.Compare(Right, ESearchCase::IgnoreCase) < 0;
			});
		}

		TArray<TSharedPtr<FJsonValue>> Values;
		Values.Reserve(Spells.Num());
		for (auto& Spell : Spells)
		{
			Values.Add(MakeShared<FJsonValueString>(MoveTemp(Spell)));
		}
		Target.SetArrayField(Field, MoveTemp(Values));
		return true;
	}

	bool RemoveCreatureSpellList(FJsonObject& Target, const FString& Field,
		const TSharedPtr<FJsonValue>& InstructionsValue, const FString& Pointer, FReadState& State)
	{
		const TArray<TSharedPtr<FJsonValue>>* Instructions = nullptr;
		const TArray<TSharedPtr<FJsonValue>>* Existing = nullptr;
		if (!RequireArray(InstructionsValue, Pointer, Instructions, State) ||
			!RequireArray(FindExactField(Target, *Field), Pointer, Existing, State))
		{
			return false;
		}

		TArray<FString> Selectors;
		Selectors.Reserve(Instructions->Num());
		for (int32 Index = 0; Index < Instructions->Num(); ++Index)
		{
			const auto& Selector = (*Instructions)[Index];
			if (!Selector.IsValid() || Selector->Type != EJson::String)
			{
				return Fail(State, Codes::InvalidFieldValue, AppendPointer(Pointer, FString::FromInt(Index)),
					TEXT("Expected a spell string to remove."));
			}
			Selectors.Add(Selector->AsString());
		}

		TArray<TSharedPtr<FJsonValue>> Remaining;
		Remaining.Reserve(Existing->Num());
		for (const auto& Entry : *Existing)
		{
			if (Entry.IsValid() && Entry->Type == EJson::String)
			{
				const FString Spell = Entry->AsString();
				if (Selectors.ContainsByPredicate([&Spell](const FString& Selector)
				{
					return Spell.Equals(Selector, ESearchCase::CaseSensitive);
				}))
				{
					continue;
				}
			}
			Remaining.Add(Entry);
		}
		Target.SetArrayField(Field, MoveTemp(Remaining));
		return true;
	}

	bool ApplyCreatureSpellListEdits(FJsonObject& Merged, const FJsonObject& Operation, bool bRemove,
		const FString& Pointer, const FString& ModPointer, FReadState& State)
	{
		if (!CheckAllowedKeys(Operation, {TEXT("mode"), TEXT("spells"), TEXT("daily")}, Pointer, State))
		{
			return false;
		}

		const auto EditList = [bRemove, &ModPointer, &State](FJsonObject& Target, const FString& Field,
			const TSharedPtr<FJsonValue>& Instructions, const FString& EditPointer)
		{
			return bRemove ? RemoveCreatureSpellList(Target, Field, Instructions, EditPointer, State)
				: ReplaceCreatureSpellList(Target, Field, Instructions, EditPointer, ModPointer, State);
		};

		const FString CastingPointer = AppendPointer(Pointer, TEXT("spellcasting"));
		const TArray<TSharedPtr<FJsonValue>>* ExistingCasting = nullptr;
		if (!RequireArray(FindExactField(Merged, TEXT("spellcasting")), CastingPointer, ExistingCasting, State))
		{
			return false;
		}
		if (ExistingCasting->IsEmpty())
		{
			return Fail(State, Codes::InvalidFieldValue, CastingPointer, TEXT("Expected a spellcasting block."));
		}

		TSharedPtr<FJsonObject> FirstBlock;
		if (!RequireObject((*ExistingCasting)[0], AppendPointer(CastingPointer, TEXT("0")), FirstBlock, State))
		{
			return false;
		}
		const auto Block = MakeShared<FJsonObject>();
		Block->Values = FirstBlock->Values;

		for (const TCHAR* Group : {TEXT("spells"), TEXT("daily")})
		{
			const auto Authored = FindExactField(Operation, Group);
			if (!Authored.IsValid()) { continue; }
			const bool bDaily = FCString::Strcmp(Group, TEXT("daily")) == 0;
			const FString GroupPointer = AppendPointer(Pointer, Group);
			TSharedPtr<FJsonObject> Instructions;
			if (!RequireObject(Authored, GroupPointer, Instructions, State)) { return false; }
			if (Instructions->Values.IsEmpty()) { continue; }

			TSharedPtr<FJsonObject> ExistingGroup;
			if (!RequireObject(FindExactField(*Block, Group), GroupPointer, ExistingGroup, State)) { return false; }
			const auto GroupCopy = MakeShared<FJsonObject>();
			GroupCopy->Values = ExistingGroup->Values;

			for (const auto& Pair : Instructions->Values)
			{
				const FString Key(Pair.Key.Len(), *Pair.Key);
				const FString KeyPointer = AppendPointer(GroupPointer, Key);
				const FString CountToken = bDaily && Key.EndsWith(TEXT("e"), ESearchCase::CaseSensitive)
					? Key.LeftChop(1) : Key;
				int32 Count = 0;
				if (!TryParseBoundedIntegerToken(CountToken, bDaily ? 1 : 0, bDaily ? 99 : 9, Count))
				{
					return Fail(State, Codes::InvalidFieldValue, KeyPointer, TEXT("Invalid spell level or daily use key."));
				}

				if (bDaily)
				{
					if (!EditList(*GroupCopy, Key, Pair.Value, KeyPointer))
					{
						return false;
					}
					continue;
				}

				const auto LevelValue = FindExactField(*GroupCopy, *Key);
				if (!LevelValue.IsValid()) { continue; }
				TSharedPtr<FJsonObject> ExistingLevel;
				if (!RequireObject(LevelValue, KeyPointer, ExistingLevel, State)) { return false; }
				if (bRemove)
				{
					const auto ExistingSpells = FindExactField(*ExistingLevel, TEXT("spells"));
					if (!ExistingSpells.IsValid() || ExistingSpells->Type == EJson::Null) { continue; }
				}
				const auto Level = MakeShared<FJsonObject>();
				Level->Values = ExistingLevel->Values;
				if (!EditList(*Level, TEXT("spells"), Pair.Value, KeyPointer))
				{
					return false;
				}
				GroupCopy->SetObjectField(Key, Level);
			}
			Block->SetObjectField(Group, GroupCopy);
		}

		TArray<TSharedPtr<FJsonValue>> Casting = *ExistingCasting;
		Casting[0] = MakeShared<FJsonValueObject>(Block);
		Merged.SetArrayField(TEXT("spellcasting"), MoveTemp(Casting));
		return true;
	}

	bool AppendCreatureSpellList(FJsonObject& Target, const FString& Field,
		const TSharedPtr<FJsonValue>& AdditionsValue, bool bSortExistingLevel,
		const FString& Pointer, const FString& ModPointer, FReadState& State)
	{
		const TArray<TSharedPtr<FJsonValue>>* Additions = nullptr;
		if (!RequireArray(AdditionsValue, Pointer, Additions, State)) { return false; }

		TArray<TSharedPtr<FJsonValue>> Spells;
		const auto ExistingValue = FindExactField(Target, *Field);
		const bool bHasExisting = ExistingValue.IsValid() && ExistingValue->Type != EJson::Null;
		if (bHasExisting)
		{
			const TArray<TSharedPtr<FJsonValue>>* Existing = nullptr;
			if (!RequireArray(ExistingValue, Pointer, Existing, State)) { return false; }
			Spells = *Existing;
		}

		for (int32 Index = 0; Index < Additions->Num(); ++Index)
		{
			const auto& Spell = (*Additions)[Index];
			if (!Spell.IsValid() || Spell->Type != EJson::String)
			{
				return Fail(State, Codes::InvalidFieldValue, AppendPointer(Pointer, FString::FromInt(Index)),
					TEXT("Expected a spell string."));
			}
			Spells.Add(Spell);
		}

		if (bSortExistingLevel && bHasExisting)
		{
			for (const auto& Spell : Spells)
			{
				if (!Spell.IsValid() || Spell->Type != EJson::String)
				{
					return Fail(State, Codes::UnsupportedRequiredValue, ModPointer,
						TEXT("Sorting a spell list containing non-string entries is deferred."));
				}
			}
			Spells.StableSort([](const TSharedPtr<FJsonValue>& Left, const TSharedPtr<FJsonValue>& Right)
			{
				return Left->AsString().Compare(Right->AsString(), ESearchCase::IgnoreCase) < 0;
			});
		}

		Target.SetArrayField(Field, MoveTemp(Spells));
		return true;
	}

	bool ApplyCreatureAddSpells(FJsonObject& Merged, const FJsonObject& Operation,
		const FString& Pointer, const FString& ModPointer, FReadState& State)
	{
		if (!CheckAllowedKeys(Operation, {TEXT("mode"), TEXT("will"), TEXT("daily"), TEXT("spells")}, Pointer, State))
		{
			return false;
		}

		const FString CastingPointer = AppendPointer(Pointer, TEXT("spellcasting"));
		const TArray<TSharedPtr<FJsonValue>>* ExistingCasting = nullptr;
		if (!RequireArray(FindExactField(Merged, TEXT("spellcasting")), CastingPointer, ExistingCasting, State))
		{
			return false;
		}
		if (ExistingCasting->IsEmpty())
		{
			return Fail(State, Codes::InvalidFieldValue, CastingPointer, TEXT("Expected a spellcasting block."));
		}
		TSharedPtr<FJsonObject> FirstBlock;
		if (!RequireObject((*ExistingCasting)[0], AppendPointer(CastingPointer, TEXT("0")), FirstBlock, State))
		{
			return false;
		}
		const auto Block = MakeShared<FJsonObject>();
		Block->Values = FirstBlock->Values;

		if (const auto Will = FindExactField(Operation, TEXT("will")))
		{
			if (!AppendCreatureSpellList(*Block, TEXT("will"), Will, false,
				AppendPointer(Pointer, TEXT("will")), ModPointer, State))
			{
				return false;
			}
		}

		for (const TCHAR* Group : {TEXT("daily"), TEXT("spells")})
		{
			const auto GroupValue = FindExactField(Operation, Group);
			if (!GroupValue.IsValid()) { continue; }
			const bool bDaily = FCString::Strcmp(Group, TEXT("daily")) == 0;
			const FString GroupPointer = AppendPointer(Pointer, Group);
			TSharedPtr<FJsonObject> Additions;
			if (!RequireObject(GroupValue, GroupPointer, Additions, State)) { return false; }

			const auto GroupCopy = MakeShared<FJsonObject>();
			const auto ExistingGroupValue = FindExactField(*Block, Group);
			if (ExistingGroupValue.IsValid() && ExistingGroupValue->Type != EJson::Null)
			{
				TSharedPtr<FJsonObject> ExistingGroup;
				if (!RequireObject(ExistingGroupValue, GroupPointer, ExistingGroup, State)) { return false; }
				GroupCopy->Values = ExistingGroup->Values;
			}

			for (const auto& Pair : Additions->Values)
			{
				const FString Key(Pair.Key.Len(), *Pair.Key);
				const FString KeyPointer = AppendPointer(GroupPointer, Key);
				const FString CountToken = bDaily && Key.EndsWith(TEXT("e"), ESearchCase::CaseSensitive)
					? Key.LeftChop(1) : Key;
				int32 Count = 0;
				if (!TryParseBoundedIntegerToken(CountToken, bDaily ? 1 : 0, bDaily ? 99 : 9, Count))
				{
					return Fail(State, Codes::InvalidFieldValue, KeyPointer, TEXT("Invalid spell level or daily use key."));
				}

				if (bDaily)
				{
					if (!AppendCreatureSpellList(*GroupCopy, Key, Pair.Value, false, KeyPointer, ModPointer, State))
					{
						return false;
					}
					continue;
				}

				TSharedPtr<FJsonObject> AddedLevel;
				if (!RequireObject(Pair.Value, KeyPointer, AddedLevel, State) ||
					!CheckAllowedKeys(*AddedLevel, {TEXT("spells")}, KeyPointer, State))
				{
					return false;
				}
				const FString ListPointer = AppendPointer(KeyPointer, TEXT("spells"));
				const auto AddedList = FindExactField(*AddedLevel, TEXT("spells"));
				if (!AddedList.IsValid())
				{
					return Fail(State, Codes::MissingRequiredField, ListPointer, TEXT("Expected spells to add."));
				}

				const auto Level = MakeShared<FJsonObject>();
				const auto ExistingLevelValue = FindExactField(*GroupCopy, *Key);
				if (ExistingLevelValue.IsValid() && ExistingLevelValue->Type != EJson::Null)
				{
					TSharedPtr<FJsonObject> ExistingLevel;
					if (!RequireObject(ExistingLevelValue, KeyPointer, ExistingLevel, State)) { return false; }
					Level->Values = ExistingLevel->Values;
				}
				if (!AppendCreatureSpellList(*Level, TEXT("spells"), AddedList, true, ListPointer, ModPointer, State))
				{
					return false;
				}
				GroupCopy->SetObjectField(Key, Level);
			}
			Block->SetObjectField(Group, GroupCopy);
		}

		TArray<TSharedPtr<FJsonValue>> Casting = *ExistingCasting;
		Casting[0] = MakeShared<FJsonValueObject>(Block);
		Merged.SetArrayField(TEXT("spellcasting"), MoveTemp(Casting));
		return true;
	}

	bool ApplyCreatureFieldModifiers(const FString& Field, const TSharedPtr<FJsonValue>& FieldValue,
		const FString& ModPointer, FJsonObject& Merged, FReadState& State)
	{
		if (Field.Equals(TEXT("*"), ESearchCase::CaseSensitive))
		{
			return ApplyCreatureWildcardOperations(FieldValue, ModPointer, Merged, State);
		}

		const bool bRoot = Field.Equals(TEXT("_"), ESearchCase::CaseSensitive);
		const bool bEntryArray = Field.Equals(TEXT("trait"), ESearchCase::CaseSensitive) ||
			Field.Equals(TEXT("action"), ESearchCase::CaseSensitive) ||
			Field.Equals(TEXT("bonus"), ESearchCase::CaseSensitive);
		const bool bUniqueArray = Field.Equals(TEXT("languages"), ESearchCase::CaseSensitive) ||
			Field.Equals(TEXT("resist"), ESearchCase::CaseSensitive) ||
			Field.Equals(TEXT("immune"), ESearchCase::CaseSensitive) ||
			Field.Equals(TEXT("conditionImmune"), ESearchCase::CaseSensitive) ||
			Field.Equals(TEXT("miscTags"), ESearchCase::CaseSensitive) ||
			Field.Equals(TEXT("vulnerable"), ESearchCase::CaseSensitive);

		if (!bRoot && !bEntryArray && !bUniqueArray)
		{
			return Fail(State, Codes::UnsupportedRequiredValue, ModPointer,
				TEXT("Creature copy modifier target is deferred: ") + Field);
		}

		const FString FieldPointer = AppendPointer(ModPointer, Field);
		const bool bArray = FieldValue.IsValid() && FieldValue->Type == EJson::Array;
		TArray<TSharedPtr<FJsonValue>> Operations;
		if (bArray)
		{
			Operations = FieldValue->AsArray();
		}
		else
		{
			Operations.Add(FieldValue);
		}

		for (int32 Index = 0; Index < Operations.Num(); ++Index)
		{
			const FString Pointer = bArray ? AppendPointer(FieldPointer, FString::FromInt(Index)) : FieldPointer;
			const auto& Value = Operations[Index];

			if (bEntryArray && Value.IsValid() && Value->Type == EJson::String &&
				Value->AsString().Equals(TEXT("remove"), ESearchCase::CaseSensitive))
			{
				Merged.RemoveField(Field);
				continue;
			}

			TSharedPtr<FJsonObject> Operation;
			if (!RequireObject(Value, Pointer, Operation, State)) { return false; }

			if (bRoot || bUniqueArray)
			{
				FString Mode;
				if (!ReadRequiredString(*Operation, TEXT("mode"), Pointer, Mode, State)) { return false; }

				if (bRoot)
				{
					if (Mode.Equals(TEXT("addSkills"), ESearchCase::CaseSensitive))
					{
						if (!ApplyCreatureAddSkills(Merged, *Operation, Pointer, ModPointer, State)) { return false; }
						continue;
					}
					if (Mode.Equals(TEXT("addSpells"), ESearchCase::CaseSensitive))
					{
						if (!ApplyCreatureAddSpells(Merged, *Operation, Pointer, ModPointer, State)) { return false; }
						continue;
					}
					const bool bRemoveSpells = Mode.Equals(TEXT("removeSpells"), ESearchCase::CaseSensitive);
					if (bRemoveSpells || Mode.Equals(TEXT("replaceSpells"), ESearchCase::CaseSensitive))
					{
						if (!ApplyCreatureSpellListEdits(Merged, *Operation, bRemoveSpells, Pointer, ModPointer, State))
						{
							return false;
						}
						continue;
					}
					if (!Mode.Equals(TEXT("addSenses"), ESearchCase::CaseSensitive))
					{
						return Fail(State, Codes::UnsupportedRequiredValue, ModPointer,
							TEXT("Creature root modifier mode is deferred: ") + Mode);
					}

					if (!ApplyCreatureAddSenses(Merged, *Operation, Pointer, State)) { return false; }
					continue;
				}

				if (!Mode.Equals(TEXT("appendIfNotExistsArr"), ESearchCase::CaseSensitive))
				{
					return Fail(State, Codes::UnsupportedRequiredValue, ModPointer,
						TEXT("Creature array modifier mode is deferred: ") + Mode);
				}
			}

			if (!ApplyCreatureArrayOperation(Merged, Field, *Operation, Pointer, ModPointer, State))
			{
				return false;
			}
		}

		return true;
	}

	struct FCreatureModifierSource
	{
		TSharedPtr<FJsonObject> Modifiers;
		FString RelativeFilePath;
		FString JsonPointer;
	};

	bool ApplyCreatureTemplate(const FDnd5eCreatureCopyRow& Template, const FJsonObject& OriginalChild,
		FJsonObject& Merged, TArray<FCreatureModifierSource>& Sources, FReadState& State)
	{
		const auto Value = FindExactField(*Template.Object, TEXT("apply"));
		if (!Value.IsValid()) { return true; }

		auto Context = State.Context;
		Context.RelativeFilePath = Template.RelativeFilePath;
		FReadState TemplateState{Context, State.JsonText, State.Diagnostics};
		const FString ApplyPointer = AppendPointer(Template.JsonPointer, TEXT("apply"));
		TSharedPtr<FJsonObject> Apply;
		if (!RequireObject(Value, ApplyPointer, Apply, TemplateState) ||
			!CheckAllowedKeys(*Apply, {TEXT("_root"), TEXT("_mod")}, ApplyPointer, TemplateState))
		{
			return false;
		}

		if (const auto RootValue = FindExactField(*Apply, TEXT("_root")))
		{
			TSharedPtr<FJsonObject> Root;
			if (!RequireObject(RootValue, AppendPointer(ApplyPointer, TEXT("_root")), Root, TemplateState)) { return false; }
			for (const auto& Pair : Root->Values)
			{
				const FString Field(Pair.Key.Len(), *Pair.Key);
				if (!FindExactField(OriginalChild, *Field).IsValid())
				{
					Merged.SetField(Field, Pair.Value);
				}
			}
		}
		if (const auto ModValue = FindExactField(*Apply, TEXT("_mod")))
		{
			const FString ModPointer = AppendPointer(ApplyPointer, TEXT("_mod"));
			TSharedPtr<FJsonObject> Modifiers;
			if (!RequireObject(ModValue, ModPointer, Modifiers, TemplateState)) { return false; }
			Sources.Add({Modifiers, Template.RelativeFilePath, ModPointer});
		}
		return true;
	}

	bool ApplyCreatureCopyStep(const FCreatureCopyStep& Step, FJsonObject& Merged, FReadState& State)
	{
		TArray<FCreatureModifierSource> Sources;
		if (Step.Modifiers.IsValid())
		{
			Sources.Add({Step.Modifiers, Step.Row.RelativeFilePath,
				AppendPointer(AppendPointer(Step.Row.JsonPointer, TEXT("_copy")), TEXT("_mod"))});
		}
		for (const auto& Template : Step.Templates)
		{
			if (!ApplyCreatureTemplate(Template, *Step.Row.Object, Merged, Sources, State)) { return false; }
		}

		TArray<FString> Fields;
		bool bHasRootOperations = false;
		bool bHasWildcard = false;
		for (const auto& Source : Sources)
		{
			for (const auto& Pair : Source.Modifiers->Values)
			{
				const FString Field(Pair.Key.Len(), *Pair.Key);
				if (Field.Equals(TEXT("_"), ESearchCase::CaseSensitive)) { bHasRootOperations = true; }
				else if (Field.Equals(TEXT("*"), ESearchCase::CaseSensitive)) { bHasWildcard = true; }
				else if (!Fields.ContainsByPredicate([&Field](const FString& Existing)
				{
					return Existing.Equals(Field, ESearchCase::CaseSensitive);
				}))
				{
					Fields.Add(Field);
				}
			}
		}
		if (bHasRootOperations) { Fields.Add(TEXT("_")); }
		if (bHasWildcard) { Fields.Add(TEXT("*")); }

		for (const FString& Field : Fields)
		{
			for (const auto& Source : Sources)
			{
				const auto Value = FindExactField(*Source.Modifiers, *Field);
				if (!Value.IsValid()) { continue; }

				auto Context = State.Context;
				Context.RelativeFilePath = Source.RelativeFilePath;
				FReadState OperationState{Context, State.JsonText, State.Diagnostics};
				if (!ApplyCreatureFieldModifiers(Field, Value, Source.JsonPointer, Merged, OperationState)) { return false; }
			}
		}
		return true;
	}

	bool CollectCreatureTemplates(const TSharedPtr<FJsonValue>& Value, const FString& Pointer,
		const FDnd5eCreatureCopyIndex& Index, TArray<FDnd5eCreatureCopyRow>& Out, FReadState& State)
	{
		const TArray<TSharedPtr<FJsonValue>>* References = nullptr;
		if (!RequireArray(Value, Pointer, References, State)) { return false; }
		if (!References->IsEmpty() && !Index.bHasTemplateDocument)
		{
			return Fail(State, Codes::UnsupportedRequiredValue, Pointer, TEXT("Creature copy templates are deferred."));
		}

		for (int32 ReferenceIndex = 0; ReferenceIndex < References->Num(); ++ReferenceIndex)
		{
			const FString ReferencePointer = AppendPointer(Pointer, FString::FromInt(ReferenceIndex));
			TSharedPtr<FJsonObject> Reference;
			FString Name;
			FString Source;
			if (!RequireObject((*References)[ReferenceIndex], ReferencePointer, Reference, State) ||
				!CheckAllowedKeys(*Reference, {TEXT("name"), TEXT("source")}, ReferencePointer, State) ||
				!ReadRequiredString(*Reference, TEXT("name"), ReferencePointer, Name, State) ||
				!ValidateIdentityText(Name, AppendPointer(ReferencePointer, TEXT("name")), State) ||
				!ReadRequiredString(*Reference, TEXT("source"), ReferencePointer, Source, State) ||
				!ValidateIdentityText(Source, AppendPointer(ReferencePointer, TEXT("source")), State))
			{
				return false;
			}

			const FString Key = MakeLookupKey(Name, Source);
			if (Index.AmbiguousTemplateKeys.Contains(Key))
			{
				return Fail(State, Codes::UnresolvedReference, ReferencePointer, TEXT("Creature template is ambiguous."));
			}
			const auto* Template = Index.TemplateRows.Find(Key);
			if (!Template)
			{
				return Fail(State, Codes::UnresolvedReference, ReferencePointer, TEXT("Creature template was not found."));
			}
			Out.Add(*Template);
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

			FCreatureCopyStep Step{*Current, nullptr, nullptr, {}};
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
			if (const auto Templates = FindExactField(*Copy, TEXT("_templates")))
			{
				if (!CollectCreatureTemplates(Templates, AppendPointer(CopyPointer, TEXT("_templates")),
					Index, Step.Templates, RowState))
				{
					return false;
				}
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
		if (!ApplyCreatureCopyStep(Step, *Assembled, State)) { return false; }
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