// Developed by Neko Creative Technologies

#include "Readers/Bestiary/Dnd5eJsonCreatureReader.h"
#include "Readers/Dnd5eJsonReaderInternal.h"
#include "Readers/Bestiary/Dnd5eJsonCreatureCopyResolver.h"
#include "Algo/Find.h"
#include "Dnd5eTags.h"

namespace
{
	using namespace Dnd5eJsonReader;

	bool ReadCreatureDocument(TSharedPtr<FJsonObject>& OutRoot,
		const TArray<TSharedPtr<FJsonValue>>*& OutRows, FReadState& State)
	{
		TSharedPtr<FJsonValue> RootValue;
		if (!ValidateReadContext(State) || !ValidateDocumentText(State) ||
			!PreflightJson(State) || !ParseJsonDocument(RootValue, State) ||
			!RequireObject(RootValue, TEXT(""), OutRoot, State) ||
			!ReadRequiredArray(*OutRoot, TEXT("monster"), TEXT(""), OutRows, State))
		{
			return false;
		}

		if (OutRows->Num() > 100000)
		{
			return Fail(State, Codes::ResourceLimitExceeded, TEXT("/monster"), TEXT("Too many creature rows."));
		}
		return true;
	}

	bool ReadStatInteger(const FJsonObject& Object, const TCHAR* Key, const FString& Pointer,
		int32& Out, FReadState& State)
	{
		FString Literal;
		return ReadRequiredInteger(Object, Key, Pointer, 0, MAX_int32, Out, Literal, State);
	}

	bool ReadStrings(const FJsonObject& Object, const TCHAR* Key, const FString& Pointer,
		TArray<FString>& Out, FReadState& State, bool bRequired = false)
	{
		const auto Value = FindExactField(Object, Key);
		if (!Value.IsValid() && !bRequired) { return true; }

		const FString ArrayPointer = AppendPointer(Pointer, Key);
		const TArray<TSharedPtr<FJsonValue>>* Rows = nullptr;
		if (!ReadRequiredArray(Object, Key, Pointer, Rows, State)) { return false; }

		for (int32 Index = 0; Index < Rows->Num(); ++Index)
		{
			const auto& Row = (*Rows)[Index];
			FString Text;
			if (!Row.IsValid() || Row->Type != EJson::String || !Row->TryGetString(Text))
			{
				return Fail(State, Codes::InvalidFieldValue,
					AppendPointer(ArrayPointer, FString::FromInt(Index)), TEXT("Expected a string."));
			}
			Out.Add(MoveTemp(Text));
		}
		return true;
	}

	bool FindCreatureRow(const TArray<TSharedPtr<FJsonValue>>& Rows, const FString& Name,
		const FString& SourceId, TSharedPtr<FJsonObject>& OutRow, FString& OutPointer, FReadState& State)
	{
		for (int32 Index = 0; Index < Rows.Num(); ++Index)
		{
			const auto& Value = Rows[Index];
			if (!Value.IsValid() || Value->Type != EJson::Object) { continue; }

			const TSharedPtr<FJsonObject> Object = Value->AsObject();
			const auto NameValue = FindExactField(*Object, TEXT("name"));
			const auto SourceValue = FindExactField(*Object, TEXT("source"));
			if (!NameValue.IsValid() || NameValue->Type != EJson::String ||
				!SourceValue.IsValid() || SourceValue->Type != EJson::String ||
				!NameValue->AsString().Equals(Name, ESearchCase::CaseSensitive) ||
				!SourceValue->AsString().Equals(SourceId, ESearchCase::CaseSensitive))
			{
				continue;
			}

			const FString Pointer = AppendPointer(TEXT("/monster"), FString::FromInt(Index));
			if (OutRow.IsValid())
			{
				return Fail(State, Codes::InvalidFieldValue, Pointer, TEXT("Duplicate creature name and source."));
			}
			OutRow = Object;
			OutPointer = Pointer;
		}
		return OutRow.IsValid() ||
			Fail(State, Codes::InvalidFieldValue, TEXT("/monster"), TEXT("Creature name and source were not found."));
	}

	FDnd5eRecordId MakeCreatureRecordId(const FString& Name, const FString& SourceId, const FString& DatasetNamespace)
	{
		FDnd5eRecordId Id;
		Id.DatasetNamespace = DatasetNamespace;
		Id.Category = Dnd5e::Category::Creature;
		Id.SourceKey = TEXT("5etools:creature:v1:") + EscapeIdentityComponent(Name) +
			TEXT("|") + EscapeIdentityComponent(SourceId);
		return Id;
	}

	bool ReadCreatureHeader(const FJsonObject& Object, const FString& Pointer,
		FDnd5eRecordHeader& Header, FReadState& State)
	{
		FDnd5eRecordProvenance Origin;
		if (!ReadRequiredString(Object, TEXT("name"), Pointer, Header.DisplayName, State) ||
			!ReadRequiredString(Object, TEXT("source"), Pointer, Origin.SourceId, State) ||
			!ValidateIdentityText(Header.DisplayName, AppendPointer(Pointer, TEXT("name")), State) ||
			!ValidateIdentityText(Origin.SourceId, AppendPointer(Pointer, TEXT("source")), State))
		{
			return false;
		}
		State.SourceId = Origin.SourceId;
		Header.RecordId = MakeCreatureRecordId(Header.DisplayName, Origin.SourceId, State.Context.DatasetNamespace);

		Origin.RelativeFilePath = State.Context.RelativeFilePath;
		Origin.JsonPointer = Pointer;
		if (const auto Page = FindExactField(Object, TEXT("page")))
		{
			FString Literal;
			const bool bPageZero = Page->Type == EJson::Number && Page->TryGetString(Literal) && Literal == TEXT("0");
			if (!bPageZero && !ReadPage(Page, AppendPointer(Pointer, TEXT("page")), Origin.Page, State))
			{
				return false;
			}
		}
		Header.Provenance.Add(MoveTemp(Origin));
		return true;
	}

	bool ReadCreatureTypeTag(const FString& Value, const FString& Pointer,
		FGameplayTag& Out, FReadState& State)
	{
		static const TMap<FString, FGameplayTag> Types{
			{TEXT("aberration"), Dnd5e::Creature::Type::Aberration},
			{TEXT("beast"), Dnd5e::Creature::Type::Beast},
			{TEXT("celestial"), Dnd5e::Creature::Type::Celestial},
			{TEXT("construct"), Dnd5e::Creature::Type::Construct},
			{TEXT("dragon"), Dnd5e::Creature::Type::Dragon},
			{TEXT("elemental"), Dnd5e::Creature::Type::Elemental},
			{TEXT("fey"), Dnd5e::Creature::Type::Fey},
			{TEXT("fiend"), Dnd5e::Creature::Type::Fiend},
			{TEXT("giant"), Dnd5e::Creature::Type::Giant},
			{TEXT("humanoid"), Dnd5e::Creature::Type::Humanoid},
			{TEXT("monstrosity"), Dnd5e::Creature::Type::Monstrosity},
			{TEXT("ooze"), Dnd5e::Creature::Type::Ooze},
			{TEXT("plant"), Dnd5e::Creature::Type::Plant},
			{TEXT("undead"), Dnd5e::Creature::Type::Undead},
		};

		const FGameplayTag* Tag = Types.Find(Value);
		if (!Tag)
		{
			return Fail(State, Codes::UnsupportedRequiredValue, Pointer, TEXT("Unsupported creature type."));
		}
		Out = *Tag;
		return true;
	}

	const FGameplayTag* FindSimpleAlignmentTag(const FString& Code)
	{
		static const TMap<FString, FGameplayTag> Tags{
			{TEXT("L"), Dnd5e::Alignment::Lawful},
			{TEXT("C"), Dnd5e::Alignment::Chaotic},
			{TEXT("N"), Dnd5e::Alignment::Neutral},
			{TEXT("G"), Dnd5e::Alignment::Good},
			{TEXT("E"), Dnd5e::Alignment::Evil},
			{TEXT("U"), Dnd5e::Alignment::Unaligned},
		};
		return Tags.Find(Code);
	}

	bool ReadAlignmentCodes(const TArray<TSharedPtr<FJsonValue>>& Values, const FString& ArrayPointer,
		TArray<FString>& Out, FReadState& State)
	{
		for (int32 Index = 0; Index < Values.Num(); ++Index)
		{
			const auto& Value = Values[Index];
			const FString Pointer = AppendPointer(ArrayPointer, FString::FromInt(Index));
			FString Code;
			if (!Value.IsValid() || Value->Type != EJson::String || !Value->TryGetString(Code))
			{
				return Fail(State, Codes::InvalidFieldValue, Pointer, TEXT("Expected an alignment code string."));
			}
			if (!FindSimpleAlignmentTag(Code) && Code != TEXT("A") && Code != TEXT("NX") && Code != TEXT("NY"))
			{
				return Fail(State, Codes::UnsupportedRequiredValue, Pointer, TEXT("Unsupported alignment code."));
			}
			Out.Add(MoveTemp(Code));
		}
		return true;
	}

	void AddSimpleAlignmentTags(const TArray<FString>& Codes, FGameplayTagContainer& Tags)
	{
		if (Codes.Num() == 1 && (Codes[0] == TEXT("N") || Codes[0] == TEXT("U")))
		{
			Tags.AddTag(*FindSimpleAlignmentTag(Codes[0]));
			return;
		}
		if (Codes.Num() != 2) { return; }

		const auto IsLawAxis = [](const FString& Code)
		{
			return Code == TEXT("L") || Code == TEXT("C") || Code == TEXT("N");
		};
		const auto IsMoralAxis = [](const FString& Code)
		{
			return Code == TEXT("G") || Code == TEXT("E") || Code == TEXT("N");
		};
		if ((IsLawAxis(Codes[0]) && IsMoralAxis(Codes[1])) ||
			(IsLawAxis(Codes[1]) && IsMoralAxis(Codes[0])))
		{
			Tags.AddTag(*FindSimpleAlignmentTag(Codes[0]));
			Tags.AddTag(*FindSimpleAlignmentTag(Codes[1]));
		}
	}

	bool ReadAlignmentOption(const FJsonObject& Object, const FString& Pointer,
		FDnd5eCreatureAlignmentOption& Out, FReadState& State)
	{
		if (!CheckAllowedKeys(Object, {TEXT("alignment"), TEXT("chance"), TEXT("note"), TEXT("special")},
			Pointer, State))
		{
			return false;
		}

		const auto Alignment = FindExactField(Object, TEXT("alignment"));
		const bool bHasSpecial = FindExactField(Object, TEXT("special")).IsValid();
		if (!Alignment.IsValid() && !bHasSpecial)
		{
			return Fail(State, Codes::MissingRequiredField, Pointer, TEXT("Expected alignment codes or special text."));
		}
		if (Alignment.IsValid())
		{
			const FString CodesPointer = AppendPointer(Pointer, TEXT("alignment"));
			const TArray<TSharedPtr<FJsonValue>>* Codes = nullptr;
			if (!RequireArray(Alignment, CodesPointer, Codes, State) ||
				!ReadAlignmentCodes(*Codes, CodesPointer, Out.Codes, State))
			{
				return false;
			}
		}
		if (FindExactField(Object, TEXT("chance")).IsValid())
		{
			FString Literal;
			if (!ReadRequiredInteger(Object, TEXT("chance"), Pointer, 0, 100, Out.ChancePercent, Literal, State))
			{
				return false;
			}
			Out.bHasChance = true;
		}

		return (!FindExactField(Object, TEXT("note")).IsValid() ||
			ReadRequiredString(Object, TEXT("note"), Pointer, Out.Note, State)) &&
			(!bHasSpecial || ReadRequiredString(Object, TEXT("special"), Pointer, Out.Special, State));
	}

	bool ReadCreatureAlignment(const FJsonObject& Object, const FString& Pointer,
		FDnd5eCreature& Creature, FReadState& State)
	{
		const auto Value = FindExactField(Object, TEXT("alignment"));
		if (!Value.IsValid()) { return true; }

		const FString AlignmentPointer = AppendPointer(Pointer, TEXT("alignment"));
		const TArray<TSharedPtr<FJsonValue>>* Values = nullptr;
		if (!RequireArray(Value, AlignmentPointer, Values, State)) { return false; }

		Creature.bHasAlignment = true;
		if (Values->IsEmpty()) { return true; }

		const auto& First = (*Values)[0];
		if (First.IsValid() && First->Type == EJson::String)
		{
			FDnd5eCreatureAlignmentOption Option;
			if (!ReadAlignmentCodes(*Values, AlignmentPointer, Option.Codes, State)) { return false; }
			AddSimpleAlignmentTags(Option.Codes, Creature.Alignment);
			Creature.AlignmentOptions.Add(MoveTemp(Option));
			return true;
		}

		for (int32 Index = 0; Index < Values->Num(); ++Index)
		{
			const FString OptionPointer = AppendPointer(AlignmentPointer, FString::FromInt(Index));
			TSharedPtr<FJsonObject> Entry;
			FDnd5eCreatureAlignmentOption Option;
			if (!RequireObject((*Values)[Index], OptionPointer, Entry, State) ||
				!ReadAlignmentOption(*Entry, OptionPointer, Option, State))
			{
				return false;
			}
			Creature.AlignmentOptions.Add(MoveTemp(Option));
		}
		return true;
	}

	bool ReadCreatureSubtypes(const FJsonObject& TypeObject, const FString& TypePointer,
		TArray<FDnd5eCreatureSubtype>& Out, FReadState& State)
	{
		const auto Value = FindExactField(TypeObject, TEXT("tags"));
		if (!Value.IsValid()) { return true; }

		const FString TagsPointer = AppendPointer(TypePointer, TEXT("tags"));
		const TArray<TSharedPtr<FJsonValue>>* Rows = nullptr;
		if (!RequireArray(Value, TagsPointer, Rows, State)) { return false; }

		for (int32 Index = 0; Index < Rows->Num(); ++Index)
		{
			const auto& Row = (*Rows)[Index];
			const FString RowPointer = AppendPointer(TagsPointer, FString::FromInt(Index));
			FDnd5eCreatureSubtype Subtype;
			if (Row.IsValid() && Row->Type == EJson::String)
			{
				Subtype.Name = Row->AsString();
			}
			else
			{
				TSharedPtr<FJsonObject> Entry;
				if (!RequireObject(Row, RowPointer, Entry, State) ||
					!CheckAllowedKeys(*Entry, {TEXT("tag"), TEXT("prefix"), TEXT("prefixHidden")}, RowPointer, State) ||
					!ReadRequiredString(*Entry, TEXT("tag"), RowPointer, Subtype.Name, State) ||
					!ReadRequiredString(*Entry, TEXT("prefix"), RowPointer, Subtype.Prefix, State) ||
					!ReadOptionalBool(*Entry, TEXT("prefixHidden"), RowPointer, Subtype.bPrefixHidden, State))
				{
					return false;
				}
			}
			Out.Add(MoveTemp(Subtype));
		}
		return true;
	}

	bool ReadCreatureTypeChoices(const TSharedPtr<FJsonValue>& Value, const FString& Pointer,
		TArray<FGameplayTag>& Out, FReadState& State)
	{
		TSharedPtr<FJsonObject> Object;
		TArray<FString> Choices;
		if (!RequireObject(Value, Pointer, Object, State) ||
			!CheckAllowedKeys(*Object, {TEXT("choose")}, Pointer, State) ||
			!ReadStrings(*Object, TEXT("choose"), Pointer, Choices, State, true))
		{
			return false;
		}

		const FString ChoicePointer = AppendPointer(Pointer, TEXT("choose"));
		if (Choices.IsEmpty())
		{
			return Fail(State, Codes::InvalidFieldValue, ChoicePointer, TEXT("Expected at least one creature type choice."));
		}
		for (int32 Index = 0; Index < Choices.Num(); ++Index)
		{
			FGameplayTag Tag;
			if (!ReadCreatureTypeTag(Choices[Index], AppendPointer(ChoicePointer, FString::FromInt(Index)), Tag, State))
			{
				return false;
			}
			Out.Add(Tag);
		}
		return true;
	}

	bool ReadCreatureClassification(const FJsonObject& Object, const FString& Pointer,
		FDnd5eCreature& Creature, FReadState& State)
	{
		const TMap<FString, EDnd5eSize> SizeMap{
			{TEXT("T"), EDnd5eSize::Tiny},
			{TEXT("S"), EDnd5eSize::Small},
			{TEXT("M"), EDnd5eSize::Medium},
			{TEXT("L"), EDnd5eSize::Large},
			{TEXT("H"), EDnd5eSize::Huge},
			{TEXT("G"), EDnd5eSize::Gargantuan},
		};
		TArray<FString> Sizes;
		if (!ReadStrings(Object, TEXT("size"), Pointer, Sizes, State, true)) { return false; }

		for (int32 Index = 0; Index < Sizes.Num(); ++Index)
		{
			const auto* Size = SizeMap.Find(Sizes[Index]);
			if (!Size)
			{
				return Fail(State, Codes::UnsupportedRequiredValue,
					AppendPointer(AppendPointer(Pointer, TEXT("size")), FString::FromInt(Index)), TEXT("Unsupported size."));
			}
			Creature.Sizes.Add(*Size);
		}
		if (!ReadCreatureAlignment(Object, Pointer, Creature, State)) { return false; }

		FString Type;
		const FString TypePointer = AppendPointer(Pointer, TEXT("type"));
		const auto TypeValue = FindExactField(Object, TEXT("type"));
		if (TypeValue.IsValid() && TypeValue->Type == EJson::Object)
		{
			const auto TypeObject = TypeValue->AsObject();
			const auto InnerType = FindExactField(*TypeObject, TEXT("type"));
			const FString InnerPointer = AppendPointer(TypePointer, TEXT("type"));
			if (InnerType.IsValid() && InnerType->Type == EJson::Object)
			{
				if (!ReadCreatureTypeChoices(InnerType, InnerPointer, Creature.CreatureTypeChoices, State)) { return false; }
			}
			else if (!ReadRequiredString(*TypeObject, TEXT("type"), TypePointer, Type, State) ||
				!ReadCreatureTypeTag(Type, InnerPointer, Creature.CreatureType, State))
			{
				return false;
			}
			return ReadCreatureSubtypes(*TypeObject, TypePointer, Creature.CreatureSubtypes, State);
		}

		return ReadRequiredString(Object, TEXT("type"), Pointer, Type, State) &&
			ReadCreatureTypeTag(Type, TypePointer, Creature.CreatureType, State);
	}

	bool ReadOptionalArmorInlines(const FJsonObject& Object, const TCHAR* Key, const FString& Pointer,
		TArray<FDnd5eInlineContent>& Out, FReadState& State)
	{
		if (!FindExactField(Object, Key).IsValid()) { return true; }

		FString Text;
		return ReadRequiredString(Object, Key, Pointer, Text, State) &&
			ReadInlines(Text, AppendPointer(Pointer, Key), Out, State);
	}

	bool ReadCreatureArmorObject(const FJsonObject& Object, const FString& Pointer,
		FDnd5eCreatureArmorClass& Out, FReadState& State)
	{
		if (FindExactField(Object, TEXT("special")).IsValid())
		{
			return CheckAllowedKeys(Object, {TEXT("special")}, Pointer, State) &&
				ReadOptionalArmorInlines(Object, TEXT("special"), Pointer, Out.Special, State);
		}

		if (!CheckAllowedKeys(Object, {TEXT("ac"), TEXT("from"), TEXT("condition"), TEXT("braces")}, Pointer, State) ||
			!ReadStatInteger(Object, TEXT("ac"), Pointer, Out.Value, State))
		{
			return false;
		}
		Out.bHasValue = true;

		if (const auto From = FindExactField(Object, TEXT("from")))
		{
			if (!ReadBlocks(From, AppendPointer(Pointer, TEXT("from")), Out.Sources, State)) { return false; }
		}
		return ReadOptionalArmorInlines(Object, TEXT("condition"), Pointer, Out.Condition, State) &&
			ReadOptionalBool(Object, TEXT("braces"), Pointer, Out.bWrapInParentheses, State);
	}

	bool ReadArmorClasses(const FJsonObject& Object, const FString& Pointer,
		TArray<FDnd5eCreatureArmorClass>& Out, EDnd5eValuePresence& Presence, FReadState& State)
	{
		const auto Value = FindExactField(Object, TEXT("ac"));
		if (!Value.IsValid()) { return true; }
		if (Value->Type == EJson::Null)
		{
			Presence = EDnd5eValuePresence::ExplicitNull;
			return true;
		}

		const TArray<TSharedPtr<FJsonValue>>* Rows = nullptr;
		if (!RequireArray(Value, AppendPointer(Pointer, TEXT("ac")), Rows, State)) { return false; }
		for (int32 Index = 0; Index < Rows->Num(); ++Index)
		{
			const auto& Row = (*Rows)[Index];
			const FString RowPointer = AppendPointer(AppendPointer(Pointer, TEXT("ac")), FString::FromInt(Index));
			FDnd5eCreatureArmorClass Armor;
			if (Row.IsValid() && Row->Type == EJson::Number)
			{
				FString Literal;
				if (!Row->TryGetString(Literal) || !TryParseBoundedIntegerToken(Literal, 0, MAX_int32, Armor.Value))
				{
					return Fail(State, Codes::InvalidFieldValue, RowPointer, TEXT("Expected integer armor class."));
				}
				Armor.bHasValue = true;
			}
			else
			{
				TSharedPtr<FJsonObject> Entry;
				if (!RequireObject(Row, RowPointer, Entry, State) ||
					!ReadCreatureArmorObject(*Entry, RowPointer, Armor, State))
				{
					return false;
				}
			}
			Out.Add(MoveTemp(Armor));
		}
		Presence = EDnd5eValuePresence::Present;
		return true;
	}

	bool ReadHitPoints(const FJsonObject& Object, const FString& Pointer,
		FDnd5eCreatureHitPoints& Out, FReadState& State)
	{
		const auto Value = FindExactField(Object, TEXT("hp"));
		if (!Value.IsValid()) { return true; }
		if (Value->Type == EJson::Null)
		{
			Out.Presence = EDnd5eValuePresence::ExplicitNull;
			return true;
		}

		const FString HpPointer = AppendPointer(Pointer, TEXT("hp"));
		TSharedPtr<FJsonObject> Hp;
		if (!RequireObject(Value, HpPointer, Hp, State)) { return false; }

		if (FindExactField(*Hp, TEXT("special")).IsValid())
		{
			FString Text;
			if (!CheckAllowedKeys(*Hp, {TEXT("special")}, HpPointer, State) ||
				!ReadRequiredString(*Hp, TEXT("special"), HpPointer, Text, State) ||
				!ReadInlines(Text, AppendPointer(HpPointer, TEXT("special")), Out.Special, State))
			{
				return false;
			}
		}
		else
		{
			if (!CheckAllowedKeys(*Hp, {TEXT("average"), TEXT("formula")}, HpPointer, State) ||
				!ReadStatInteger(*Hp, TEXT("average"), HpPointer, Out.Average, State) ||
				!ReadRequiredString(*Hp, TEXT("formula"), HpPointer, Out.Formula.Expression, State))
			{
				return false;
			}
			Out.bHasAverage = true;
			Out.Formula.Syntax = TEXT("5etools");
		}

		Out.Presence = EDnd5eValuePresence::Present;
		return true;
	}

	struct FCreatureMovementKey
	{
		const TCHAR* Key;
		EDnd5eCreatureMovementMode Mode;
	};

	const FCreatureMovementKey CreatureMovementKeys[] = {
		{TEXT("walk"), EDnd5eCreatureMovementMode::Walk},
		{TEXT("burrow"), EDnd5eCreatureMovementMode::Burrow},
		{TEXT("climb"), EDnd5eCreatureMovementMode::Climb},
		{TEXT("fly"), EDnd5eCreatureMovementMode::Fly},
		{TEXT("swim"), EDnd5eCreatureMovementMode::Swim},
	};

	FDnd5eDistance MakeCreatureSpeedDistance(int32 Feet)
	{
		FDnd5eDistance Distance;
		Distance.Unit = EDnd5eDistanceUnit::Feet;
		Distance.Amount.Kind = EDnd5eNumberKind::Integer;
		Distance.Amount.Integer = Feet;
		return Distance;
	}

	bool ReadCreatureSpeed(const TSharedPtr<FJsonValue>& Value, const FString& Pointer,
		EDnd5eCreatureMovementMode Mode, FDnd5eCreatureSpeed& Out, FReadState& State)
	{
		int32 Feet = 0;
		if (Value.IsValid() && Value->Type == EJson::Number)
		{
			FString Literal;
			if (!Value->TryGetString(Literal) || !TryParseBoundedIntegerToken(Literal, 0, MAX_int32, Feet))
			{
				return Fail(State, Codes::InvalidFieldValue, Pointer, TEXT("Expected a nonnegative integer speed."));
			}
		}
		else
		{
			TSharedPtr<FJsonObject> Object;
			FString Condition;
			if (!RequireObject(Value, Pointer, Object, State) ||
				!CheckAllowedKeys(*Object, {TEXT("number"), TEXT("condition")}, Pointer, State) ||
				!ReadStatInteger(*Object, TEXT("number"), Pointer, Feet, State))
			{
				return false;
			}
			if (FindExactField(*Object, TEXT("condition")).IsValid() &&
				(!ReadRequiredString(*Object, TEXT("condition"), Pointer, Condition, State) ||
				!ReadInlines(Condition, AppendPointer(Pointer, TEXT("condition")), Out.Condition, State)))
			{
				return false;
			}
		}
		Out.Mode = Mode;
		Out.Distance = MakeCreatureSpeedDistance(Feet);
		return true;
	}

	bool ReadCreatureMovementAlternates(const FJsonObject& Speed, const FString& Pointer,
		TArray<FDnd5eCreatureSpeed>& Out, FReadState& State)
	{
		const auto Value = FindExactField(Speed, TEXT("alternate"));
		if (!Value.IsValid()) { return true; }

		const FString AlternatePointer = AppendPointer(Pointer, TEXT("alternate"));
		TSharedPtr<FJsonObject> Alternate;
		if (!RequireObject(Value, AlternatePointer, Alternate, State) ||
			!CheckAllowedKeys(*Alternate, {TEXT("walk"), TEXT("burrow"), TEXT("climb"), TEXT("fly"), TEXT("swim")},
				AlternatePointer, State))
		{
			return false;
		}
		for (const auto& Key : CreatureMovementKeys)
		{
			const auto Entries = FindExactField(*Alternate, Key.Key);
			if (!Entries.IsValid()) { continue; }

			const FString ModePointer = AppendPointer(AlternatePointer, Key.Key);
			const TArray<TSharedPtr<FJsonValue>>* Rows = nullptr;
			if (!RequireArray(Entries, ModePointer, Rows, State)) { return false; }
			for (int32 Index = 0; Index < Rows->Num(); ++Index)
			{
				FDnd5eCreatureSpeed Entry;
				if (!ReadCreatureSpeed((*Rows)[Index], AppendPointer(ModePointer, FString::FromInt(Index)),
					Key.Mode, Entry, State))
				{
					return false;
				}
				Out.Add(MoveTemp(Entry));
			}
		}
		return true;
	}

	bool ReadCreatureMovementChoice(const FJsonObject& Speed, const FString& Pointer,
		FDnd5eCreatureMovement& Out, FReadState& State)
	{
		const auto Value = FindExactField(Speed, TEXT("choose"));
		if (!Value.IsValid()) { return true; }

		const FString ChoicePointer = AppendPointer(Pointer, TEXT("choose"));
		TSharedPtr<FJsonObject> Choice;
		int32 Amount = 0;
		TArray<FString> Modes;
		if (!RequireObject(Value, ChoicePointer, Choice, State) ||
			!CheckAllowedKeys(*Choice, {TEXT("from"), TEXT("amount"), TEXT("note")}, ChoicePointer, State) ||
			!ReadStrings(*Choice, TEXT("from"), ChoicePointer, Modes, State, true) ||
			!ReadStatInteger(*Choice, TEXT("amount"), ChoicePointer, Amount, State))
		{
			return false;
		}

		const FString FromPointer = AppendPointer(ChoicePointer, TEXT("from"));
		if (Modes.IsEmpty())
		{
			return Fail(State, Codes::InvalidFieldValue, FromPointer, TEXT("Expected at least one movement choice."));
		}
		for (int32 Index = 0; Index < Modes.Num(); ++Index)
		{
			const auto* Key = Algo::FindByPredicate(CreatureMovementKeys, [&Modes, Index](const FCreatureMovementKey& Entry)
			{
				return Modes[Index] == Entry.Key;
			});
			if (!Key)
			{
				return Fail(State, Codes::UnsupportedRequiredValue, AppendPointer(FromPointer, FString::FromInt(Index)),
					TEXT("Unsupported movement mode."));
			}
			Out.ChoiceModes.Add(Key->Mode);
		}

		Out.ChoiceDistance = MakeCreatureSpeedDistance(Amount);
		FString Note;
		return !FindExactField(*Choice, TEXT("note")).IsValid() ||
			(ReadRequiredString(*Choice, TEXT("note"), ChoicePointer, Note, State) &&
				ReadInlines(Note, AppendPointer(ChoicePointer, TEXT("note")), Out.ChoiceNote, State));
	}

	bool ReadCreatureMovement(const FJsonObject& Object, const FString& Pointer,
		FDnd5eCreature& Creature, FReadState& State)
	{
		const auto Value = FindExactField(Object, TEXT("speed"));
		if (!Value.IsValid()) { return true; }

		auto& Out = Creature.Movement;
		if (Value->Type == EJson::Null)
		{
			Out.Presence = EDnd5eValuePresence::ExplicitNull;
			return true;
		}

		const FString SpeedPointer = AppendPointer(Pointer, TEXT("speed"));
		TSharedPtr<FJsonObject> Speed;
		if (!RequireObject(Value, SpeedPointer, Speed, State) ||
			!CheckAllowedKeys(*Speed, {TEXT("walk"), TEXT("burrow"), TEXT("climb"), TEXT("fly"), TEXT("swim"),
				TEXT("canHover"), TEXT("alternate"), TEXT("choose")}, SpeedPointer, State))
		{
			return false;
		}
		for (const auto& Key : CreatureMovementKeys)
		{
			const auto Field = FindExactField(*Speed, Key.Key);
			if (!Field.IsValid()) { continue; }

			FDnd5eCreatureSpeed Entry;
			if (!ReadCreatureSpeed(Field, AppendPointer(SpeedPointer, Key.Key), Key.Mode, Entry, State)) { return false; }
			if (Key.Mode == EDnd5eCreatureMovementMode::Walk) { Creature.WalkSpeed = Entry.Distance; }
			Out.Speeds.Add(MoveTemp(Entry));
		}
		if (!ReadOptionalBool(*Speed, TEXT("canHover"), SpeedPointer, Out.bCanHover, State) ||
			!ReadCreatureMovementAlternates(*Speed, SpeedPointer, Out.AlternateSpeeds, State) ||
			!ReadCreatureMovementChoice(*Speed, SpeedPointer, Out, State))
		{
			return false;
		}

		Out.Presence = EDnd5eValuePresence::Present;
		return true;
	}

	bool ReadMovementAndScores(const FJsonObject& Object, const FString& Pointer,
		FDnd5eCreature& Creature, FReadState& State)
	{
		if (!ReadCreatureMovement(Object, Pointer, Creature, State)) { return false; }

		if (!FindExactField(Object, TEXT("str")).IsValid() &&
			!FindExactField(Object, TEXT("dex")).IsValid() &&
			!FindExactField(Object, TEXT("con")).IsValid() &&
			!FindExactField(Object, TEXT("int")).IsValid() &&
			!FindExactField(Object, TEXT("wis")).IsValid() &&
			!FindExactField(Object, TEXT("cha")).IsValid())
		{
			return true;
		}

		auto& Scores = Creature.AbilityScores;
		if (!ReadStatInteger(Object, TEXT("str"), Pointer, Scores.Strength, State) ||
			!ReadStatInteger(Object, TEXT("dex"), Pointer, Scores.Dexterity, State) ||
			!ReadStatInteger(Object, TEXT("con"), Pointer, Scores.Constitution, State) ||
			!ReadStatInteger(Object, TEXT("int"), Pointer, Scores.Intelligence, State) ||
			!ReadStatInteger(Object, TEXT("wis"), Pointer, Scores.Wisdom, State) ||
			!ReadStatInteger(Object, TEXT("cha"), Pointer, Scores.Charisma, State))
		{
			return false;
		}

		Scores.bHasScores = true;
		return true;
	}

	bool ReadCreatureBonusString(const TSharedPtr<FJsonValue>& Value, const FString& Pointer,
		int32& OutBonus, bool& bOutHasNumericBonus, FString& OutExpression,
		const TCHAR* ExpectedMessage, const TCHAR* RangeMessage, FReadState& State)
	{
		FString Token;
		if (!Value.IsValid() || Value->Type != EJson::String || !Value->TryGetString(Token) ||
			Token.TrimStartAndEnd().IsEmpty())
		{
			return Fail(State, Codes::InvalidFieldValue, Pointer, ExpectedMessage);
		}

		const bool bNegative = Token.StartsWith(TEXT("-"));
		const FString Digits = bNegative || Token.StartsWith(TEXT("+")) ? Token.Mid(1) : Token;
		bool bAllDigits = !Digits.IsEmpty();
		for (const TCHAR Character : Digits)
		{
			bAllDigits = bAllDigits && Character >= TEXT('0') && Character <= TEXT('9');
		}

		if (!bAllDigits)
		{
			OutExpression = MoveTemp(Token);
			return true;
		}

		int32 Magnitude = 0;
		if (!TryParseBoundedIntegerToken(Digits, 0, MAX_int32, Magnitude))
		{
			return Fail(State, Codes::InvalidFieldValue, Pointer, RangeMessage);
		}
		bOutHasNumericBonus = true;
		OutBonus = bNegative ? -Magnitude : Magnitude;
		return true;
	}

	bool ReadCreatureSkillBonus(const FString& Name, const TSharedPtr<FJsonValue>& Value,
		const FString& Pointer, FDnd5eCreatureSkillBonus& Out, FReadState& State)
	{
		if (!ReadCreatureBonusString(Value, Pointer, Out.Bonus, Out.bHasNumericBonus, Out.BonusExpression,
			TEXT("Expected a nonempty skill bonus string."),
			TEXT("Skill bonus is outside the supported integer format."), State))
		{
			return false;
		}
		Out.Skill = Name;
		return true;
	}

	bool ReadCreatureSkillChoices(const TSharedPtr<FJsonValue>& Value, const FString& Pointer,
		TArray<FDnd5eCreatureSkillChoice>& Out, FReadState& State)
	{
		const TArray<TSharedPtr<FJsonValue>>* Rows = nullptr;
		if (!RequireArray(Value, Pointer, Rows, State)) { return false; }

		for (int32 Index = 0; Index < Rows->Num(); ++Index)
		{
			const FString GroupPointer = AppendPointer(Pointer, FString::FromInt(Index));
			const FString ChoicePointer = AppendPointer(GroupPointer, TEXT("oneOf"));
			TSharedPtr<FJsonObject> Group;
			TSharedPtr<FJsonObject> Options;
			if (!RequireObject((*Rows)[Index], GroupPointer, Group, State) ||
				!CheckAllowedKeys(*Group, {TEXT("oneOf")}, GroupPointer, State) ||
				!ReadRequiredObject(*Group, TEXT("oneOf"), GroupPointer, Options, State))
			{
				return false;
			}
			if (Options->Values.IsEmpty())
			{
				return Fail(State, Codes::InvalidFieldValue, ChoicePointer, TEXT("Expected at least one skill option."));
			}

			FDnd5eCreatureSkillChoice Choice;
			for (const auto& Pair : Options->Values)
			{
				const FString Name(Pair.Key.Len(), *Pair.Key);
				FDnd5eCreatureSkillBonus Bonus;
				if (!ReadCreatureSkillBonus(Name, Pair.Value, AppendPointer(ChoicePointer, Name), Bonus, State))
				{
					return false;
				}
				Choice.Options.Add(MoveTemp(Bonus));
			}
			Out.Add(MoveTemp(Choice));
		}
		return true;
	}

	bool ReadSkills(const FJsonObject& Object, const FString& Pointer,
		FDnd5eCreature& Creature, FReadState& State)
	{
		const auto Value = FindExactField(Object, TEXT("skill"));
		if (!Value.IsValid()) { return true; }

		const FString SkillsPointer = AppendPointer(Pointer, TEXT("skill"));
		TSharedPtr<FJsonObject> Skills;
		if (!RequireObject(Value, SkillsPointer, Skills, State)) { return false; }

		for (const auto& Pair : Skills->Values)
		{
			const FString Name(Pair.Key.Len(), *Pair.Key);
			const FString SkillPointer = AppendPointer(SkillsPointer, Name);
			if (Name.Equals(TEXT("other"), ESearchCase::CaseSensitive))
			{
				if (!ReadCreatureSkillChoices(Pair.Value, SkillPointer, Creature.SkillChoices, State)) { return false; }
				continue;
			}

			FDnd5eCreatureSkillBonus Bonus;
			if (!ReadCreatureSkillBonus(Name, Pair.Value, SkillPointer, Bonus, State)) { return false; }
			Creature.Skills.Add(MoveTemp(Bonus));
		}
		return true;
	}

	bool ReadCreatureChallengeRating(const FJsonObject& Object, const FString& Pointer,
		FDnd5eCreatureChallengeRating& Out, FReadState& State)
	{
		const auto Value = FindExactField(Object, TEXT("cr"));
		if (!Value.IsValid()) { return true; }

		if (Value->Type == EJson::Null)
		{
			Out.Presence = EDnd5eValuePresence::ExplicitNull;
			return true;
		}
		if (Value->Type == EJson::String)
		{
			if (!ReadRequiredString(Object, TEXT("cr"), Pointer, Out.Base, State)) { return false; }
			Out.Presence = EDnd5eValuePresence::Present;
			return true;
		}

		const FString RatingPointer = AppendPointer(Pointer, TEXT("cr"));
		TSharedPtr<FJsonObject> Rating;
		if (!RequireObject(Value, RatingPointer, Rating, State) ||
			!CheckAllowedKeys(*Rating, {TEXT("cr"), TEXT("lair"), TEXT("coven"), TEXT("xp"), TEXT("xpLair")},
				RatingPointer, State) ||
			!ReadRequiredString(*Rating, TEXT("cr"), RatingPointer, Out.Base, State))
		{
			return false;
		}
		if (FindExactField(*Rating, TEXT("lair")).IsValid() &&
			!ReadRequiredString(*Rating, TEXT("lair"), RatingPointer, Out.Lair, State))
		{
			return false;
		}
		if (FindExactField(*Rating, TEXT("coven")).IsValid() &&
			!ReadRequiredString(*Rating, TEXT("coven"), RatingPointer, Out.Coven, State))
		{
			return false;
		}
		if (FindExactField(*Rating, TEXT("xp")).IsValid())
		{
			if (!ReadStatInteger(*Rating, TEXT("xp"), RatingPointer, Out.Xp, State)) { return false; }
			Out.bHasXp = true;
		}
		if (FindExactField(*Rating, TEXT("xpLair")).IsValid())
		{
			if (!ReadStatInteger(*Rating, TEXT("xpLair"), RatingPointer, Out.LairXp, State)) { return false; }
			Out.bHasLairXp = true;
		}

		Out.Presence = EDnd5eValuePresence::Present;
		return true;
	}

	bool ReadCreaturePassivePerception(const FJsonObject& Object, const FString& Pointer,
		FDnd5eCreature& Creature, FReadState& State)
	{
		const auto Value = FindExactField(Object, TEXT("passive"));
		if (!Value.IsValid()) { return true; }
		if (Value->Type == EJson::Null)
		{
			Creature.PassivePerceptionPresence = EDnd5eValuePresence::ExplicitNull;
			return true;
		}
		if (Value->Type == EJson::String)
		{
			if (!ReadRequiredString(Object, TEXT("passive"), Pointer, Creature.PassivePerceptionFormula, State))
			{
				return false;
			}
			if (Creature.PassivePerceptionFormula.TrimStartAndEnd().IsEmpty())
			{
				return Fail(State, Codes::InvalidFieldValue, AppendPointer(Pointer, TEXT("passive")),
					TEXT("Expected a nonempty passive Perception formula."));
			}
		}
		else if (!ReadStatInteger(Object, TEXT("passive"), Pointer, Creature.PassivePerception, State))
		{
			return false;
		}

		Creature.PassivePerceptionPresence = EDnd5eValuePresence::Present;
		return true;
	}

	bool ReadCreatureSavingThrows(const FJsonObject& Object, const FString& Pointer,
		FDnd5eCreature& Creature, FReadState& State)
	{
		const auto Value = FindExactField(Object, TEXT("save"));
		if (!Value.IsValid()) { return true; }
		if (Value->Type == EJson::Null)
		{
			Creature.SavingThrowsPresence = EDnd5eValuePresence::ExplicitNull;
			return true;
		}

		const FString SavesPointer = AppendPointer(Pointer, TEXT("save"));
		TSharedPtr<FJsonObject> Saves;
		if (!RequireObject(Value, SavesPointer, Saves, State) ||
			!CheckAllowedKeys(*Saves, {TEXT("str"), TEXT("dex"), TEXT("con"), TEXT("int"), TEXT("wis"), TEXT("cha")},
				SavesPointer, State))
		{
			return false;
		}

		const TPair<const TCHAR*, FGameplayTag> Abilities[] = {
			{TEXT("str"), Dnd5e::Ability::Strength},
			{TEXT("dex"), Dnd5e::Ability::Dexterity},
			{TEXT("con"), Dnd5e::Ability::Constitution},
			{TEXT("int"), Dnd5e::Ability::Intelligence},
			{TEXT("wis"), Dnd5e::Ability::Wisdom},
			{TEXT("cha"), Dnd5e::Ability::Charisma},
		};
		TArray<FDnd5eCreatureSavingThrowBonus> Bonuses;
		Bonuses.Reserve(Saves->Values.Num());
		for (const auto& Ability : Abilities)
		{
			const auto BonusValue = FindExactField(*Saves, Ability.Key);
			if (!BonusValue.IsValid()) { continue; }

			FDnd5eCreatureSavingThrowBonus Bonus;
			Bonus.Ability = Ability.Value;
			if (!ReadCreatureBonusString(BonusValue, AppendPointer(SavesPointer, Ability.Key),
				Bonus.Bonus, Bonus.bHasNumericBonus, Bonus.BonusExpression,
				TEXT("Expected a nonempty saving throw bonus string."),
				TEXT("Saving throw bonus is outside the supported integer format."), State))
			{
				return false;
			}
			Bonuses.Add(MoveTemp(Bonus));
		}

		Creature.SavingThrows = MoveTemp(Bonuses);
		Creature.SavingThrowsPresence = EDnd5eValuePresence::Present;
		return true;
	}

	FGameplayTag FindCreatureDefenseTag(const FString& Token, bool bCondition)
	{
		static const TMap<FString, FGameplayTag> DamageTags{
			{TEXT("acid"), Dnd5e::Damage::Type::Acid}, {TEXT("bludgeoning"), Dnd5e::Damage::Type::Bludgeoning},
			{TEXT("cold"), Dnd5e::Damage::Type::Cold}, {TEXT("fire"), Dnd5e::Damage::Type::Fire},
			{TEXT("force"), Dnd5e::Damage::Type::Force}, {TEXT("lightning"), Dnd5e::Damage::Type::Lightning},
			{TEXT("necrotic"), Dnd5e::Damage::Type::Necrotic}, {TEXT("piercing"), Dnd5e::Damage::Type::Piercing},
			{TEXT("poison"), Dnd5e::Damage::Type::Poison}, {TEXT("psychic"), Dnd5e::Damage::Type::Psychic},
			{TEXT("radiant"), Dnd5e::Damage::Type::Radiant}, {TEXT("slashing"), Dnd5e::Damage::Type::Slashing},
			{TEXT("thunder"), Dnd5e::Damage::Type::Thunder},
		};
		static const TMap<FString, FGameplayTag> ConditionTags{
			{TEXT("blinded"), Dnd5e::Condition::Type::Blinded}, {TEXT("charmed"), Dnd5e::Condition::Type::Charmed},
			{TEXT("deafened"), Dnd5e::Condition::Type::Deafened}, {TEXT("exhaustion"), Dnd5e::Condition::Type::Exhaustion},
			{TEXT("frightened"), Dnd5e::Condition::Type::Frightened}, {TEXT("grappled"), Dnd5e::Condition::Type::Grappled},
			{TEXT("incapacitated"), Dnd5e::Condition::Type::Incapacitated}, {TEXT("invisible"), Dnd5e::Condition::Type::Invisible},
			{TEXT("paralyzed"), Dnd5e::Condition::Type::Paralyzed}, {TEXT("petrified"), Dnd5e::Condition::Type::Petrified},
			{TEXT("poisoned"), Dnd5e::Condition::Type::Poisoned}, {TEXT("prone"), Dnd5e::Condition::Type::Prone},
			{TEXT("restrained"), Dnd5e::Condition::Type::Restrained}, {TEXT("stunned"), Dnd5e::Condition::Type::Stunned},
			{TEXT("unconscious"), Dnd5e::Condition::Type::Unconscious},
		};
		const auto& Tags = bCondition ? ConditionTags : DamageTags;
		const auto* Tag = Tags.Find(Token);
		return Tag ? *Tag : FGameplayTag();
	}

	bool ReadCreatureDefenseEntries(const TArray<TSharedPtr<FJsonValue>>& Values, const TCHAR* Key,
		bool bCondition, const FString& Pointer, int32 ParentIndex, int32 Depth,
		TArray<FDnd5eCreatureDefenseEntry>& Out, FReadState& State)
	{
		if (Depth > 32)
		{
			return Fail(State, Codes::ResourceLimitExceeded, Pointer, TEXT("Creature defense groups are nested too deeply."));
		}

		for (int32 Index = 0; Index < Values.Num(); ++Index)
		{
			const FString EntryPointer = AppendPointer(Pointer, FString::FromInt(Index));
			const auto& Value = Values[Index];
			FDnd5eCreatureDefenseEntry Entry;
			Entry.ParentIndex = ParentIndex;
			if (Value.IsValid() && Value->Type == EJson::String)
			{
				Entry.SourceToken = Value->AsString();
				if (Entry.SourceToken.TrimStartAndEnd().IsEmpty())
				{
					return Fail(State, Codes::InvalidFieldValue, EntryPointer, TEXT("Expected a nonempty defense token."));
				}
				Entry.TypeTag = FindCreatureDefenseTag(Entry.SourceToken, bCondition);
				Out.Add(MoveTemp(Entry));
				continue;
			}

			TSharedPtr<FJsonObject> Object;
			if (!RequireObject(Value, EntryPointer, Object, State)) { return false; }
			if (FindExactField(*Object, TEXT("special")).IsValid())
			{
				FString Text;
				if (!CheckAllowedKeys(*Object, {TEXT("special")}, EntryPointer, State) ||
					!ReadRequiredString(*Object, TEXT("special"), EntryPointer, Text, State) ||
					!ReadInlines(Text, AppendPointer(EntryPointer, TEXT("special")), Entry.Special, State))
				{
					return false;
				}
				Entry.Kind = EDnd5eCreatureDefenseEntryKind::Special;
				Out.Add(MoveTemp(Entry));
				continue;
			}

			const TArray<TSharedPtr<FJsonValue>>* Children = nullptr;
			if (!CheckAllowedKeys(*Object, {Key, TEXT("cond"), TEXT("preNote"), TEXT("note")}, EntryPointer, State) ||
				!ReadRequiredArray(*Object, Key, EntryPointer, Children, State) ||
				!ReadOptionalBool(*Object, TEXT("cond"), EntryPointer, Entry.bConditional, State))
			{
				return false;
			}
			const auto ReadNote = [&](const TCHAR* NoteKey, TArray<FDnd5eInlineContent>& Inlines)
			{
				if (!FindExactField(*Object, NoteKey).IsValid()) { return true; }
				FString Text;
				return ReadRequiredString(*Object, NoteKey, EntryPointer, Text, State) &&
					ReadInlines(Text, AppendPointer(EntryPointer, NoteKey), Inlines, State);
			};
			if (!ReadNote(TEXT("preNote"), Entry.PreNote) || !ReadNote(TEXT("note"), Entry.Note))
			{
				return false;
			}

			Entry.Kind = EDnd5eCreatureDefenseEntryKind::Group;
			const int32 GroupIndex = Out.Add(MoveTemp(Entry));
			if (!ReadCreatureDefenseEntries(*Children, Key, bCondition, AppendPointer(EntryPointer, Key),
				GroupIndex, Depth + 1, Out, State))
			{
				return false;
			}
		}
		return true;
	}

	bool ReadCreatureDefenseList(const FJsonObject& Object, const TCHAR* Key, bool bCondition,
		const FString& Pointer, FDnd5eCreatureDefenseList& Out, FReadState& State)
	{
		const auto Value = FindExactField(Object, Key);
		if (!Value.IsValid()) { return true; }
		if (Value->Type == EJson::Null)
		{
			Out.Presence = EDnd5eValuePresence::ExplicitNull;
			return true;
		}

		const FString ListPointer = AppendPointer(Pointer, Key);
		const TArray<TSharedPtr<FJsonValue>>* Values = nullptr;
		FDnd5eCreatureDefenseList List;
		if (!RequireArray(Value, ListPointer, Values, State) ||
			!ReadCreatureDefenseEntries(*Values, Key, bCondition, ListPointer, INDEX_NONE, 0, List.Entries, State))
		{
			return false;
		}
		List.Presence = EDnd5eValuePresence::Present;
		Out = MoveTemp(List);
		return true;
	}

	bool ReadCreatureProficiencyNote(const FJsonObject& Object, const FString& Pointer,
		FDnd5eCreature& Creature, FReadState& State)
	{
		const auto Note = FindExactField(Object, TEXT("pbNote"));
		return !Note || ReadRequiredString(Object, TEXT("pbNote"), Pointer, Creature.ProficiencyNote, State);
	}

	bool ReadCreatureObservations(const FJsonObject& Object, const FString& Pointer,
		FDnd5eCreature& Creature, FReadState& State)
	{
		return ReadCreatureSavingThrows(Object, Pointer, Creature, State) &&
			ReadSkills(Object, Pointer, Creature, State) &&
			ReadStrings(Object, TEXT("senses"), Pointer, Creature.Senses, State) &&
			ReadStrings(Object, TEXT("languages"), Pointer, Creature.Languages, State) &&
			ReadStrings(Object, TEXT("environment"), Pointer, Creature.Environments, State) &&
			ReadStrings(Object, TEXT("referenceSources"), Pointer, Creature.ReferenceSources, State) &&
			ReadCreatureProficiencyNote(Object, Pointer, Creature, State) &&
			ReadCreaturePassivePerception(Object, Pointer, Creature, State) &&
			ReadCreatureChallengeRating(Object, Pointer, Creature.ChallengeRating, State) &&
			ReadCreatureDefenseList(Object, TEXT("vulnerable"), false, Pointer, Creature.DamageVulnerabilities, State) &&
			ReadCreatureDefenseList(Object, TEXT("resist"), false, Pointer, Creature.DamageResistances, State) &&
			ReadCreatureDefenseList(Object, TEXT("immune"), false, Pointer, Creature.DamageImmunities, State) &&
			ReadCreatureDefenseList(Object, TEXT("conditionImmune"), true, Pointer, Creature.ConditionImmunities, State);
	}

	bool ReadCreatureEntries(const FJsonObject& Object, const TCHAR* Key, const FString& Pointer,
		TArray<FDnd5eCreatureEntry>& Out, FReadState& State)
	{
		const auto Value = FindExactField(Object, Key);
		if (!Value.IsValid()) { return true; }

		const FString ArrayPointer = AppendPointer(Pointer, Key);
		const TArray<TSharedPtr<FJsonValue>>* Rows = nullptr;
		if (!RequireArray(Value, ArrayPointer, Rows, State)) { return false; }

		for (int32 Index = 0; Index < Rows->Num(); ++Index)
		{
			const FString RowPointer = AppendPointer(ArrayPointer, FString::FromInt(Index));
			TSharedPtr<FJsonObject> Row;
			FDnd5eCreatureEntry Entry;
			TSharedPtr<FJsonValue> EntriesValue;
			if (!RequireObject((*Rows)[Index], RowPointer, Row, State) ||
				!ReadRequiredString(*Row, TEXT("name"), RowPointer, Entry.Name, State) ||
				!ReadRequiredValue(*Row, TEXT("entries"), EJson::Array, RowPointer, EntriesValue, State) ||
				!ReadBlocks(EntriesValue, AppendPointer(RowPointer, TEXT("entries")), Entry.Entries, State))
			{
				return false;
			}
			Out.Add(MoveTemp(Entry));
		}
		return true;
	}

	bool ReadCreatureActionGroup(const FJsonObject& Object, const TCHAR* Key, const TCHAR* HeaderKey,
		const FString& Pointer, FDnd5eCreatureActionGroup& Out, FReadState& State)
	{
		FDnd5eCreatureActionGroup Group;
		const auto Entries = FindExactField(Object, Key);
		if (Entries.IsValid())
		{
			if (Entries->Type == EJson::Null)
			{
				Group.Presence = EDnd5eValuePresence::ExplicitNull;
			}
			else
			{
				if (!ReadCreatureEntries(Object, Key, Pointer, Group.Entries, State)) { return false; }
				Group.Presence = EDnd5eValuePresence::Present;
			}
		}

		const auto Header = FindExactField(Object, HeaderKey);
		if (Header.IsValid() && Header->Type != EJson::Null)
		{
			const FString HeaderPointer = AppendPointer(Pointer, HeaderKey);
			if (!ReadBlocks(Header, HeaderPointer, Group.Header, State)) { return false; }
		}

		Out = MoveTemp(Group);
		return true;
	}

	bool ReadCreatureLegendaryActionCounts(const FJsonObject& Object, const FString& Pointer,
		FDnd5eCreature& Creature, FReadState& State)
	{
		if (FindExactField(Object, TEXT("legendaryActions")).IsValid())
		{
			if (!ReadStatInteger(Object, TEXT("legendaryActions"), Pointer, Creature.LegendaryActionCount, State))
			{
				return false;
			}
			Creature.bHasLegendaryActionCount = true;
		}
		if (FindExactField(Object, TEXT("legendaryActionsLair")).IsValid())
		{
			if (!ReadStatInteger(Object, TEXT("legendaryActionsLair"), Pointer, Creature.LegendaryActionCountInLair, State))
			{
				return false;
			}
			Creature.bHasLegendaryActionCountInLair = true;
		}
		return true;
	}

	bool ReadCreatureSpellcastingEntries(const TSharedPtr<FJsonValue>& Value, const FString& Pointer,
		TArray<FDnd5eCreatureSpellcastingEntry>& Out, FReadState& State)
	{
		const TArray<TSharedPtr<FJsonValue>>* Values = nullptr;
		if (!RequireArray(Value, Pointer, Values, State)) { return false; }

		Out.Reserve(Values->Num());
		for (int32 Index = 0; Index < Values->Num(); ++Index)
		{
			const FString EntryPointer = AppendPointer(Pointer, FString::FromInt(Index));
			const auto& Item = (*Values)[Index];
			FDnd5eCreatureSpellcastingEntry Entry;
			FString Text;
			FString TextPointer = EntryPointer;
			if (Item.IsValid() && Item->Type == EJson::String)
			{
				Text = Item->AsString();
			}
			else
			{
				TSharedPtr<FJsonObject> Object;
				if (!RequireObject(Item, EntryPointer, Object, State) ||
					!CheckAllowedKeys(*Object, {TEXT("entry"), TEXT("hidden")}, EntryPointer, State) ||
					!ReadRequiredString(*Object, TEXT("entry"), EntryPointer, Text, State) ||
					!ReadOptionalBool(*Object, TEXT("hidden"), EntryPointer, Entry.bHidden, State))
				{
					return false;
				}
				TextPointer = AppendPointer(EntryPointer, TEXT("entry"));
			}

			if (!ReadInlines(Text, TextPointer, Entry.Content, State)) { return false; }
			Out.Add(MoveTemp(Entry));
		}
		return true;
	}

	bool ReadCreatureSpellcastingLists(const FJsonObject& Object, const FString& Pointer,
		TArray<FDnd5eCreatureSpellcastingList>& Out, FReadState& State)
	{
		for (const TCHAR* Group : {TEXT("constant"), TEXT("will"), TEXT("ritual")})
		{
			const auto Value = FindExactField(Object, Group);
			if (!Value.IsValid()) { continue; }

			FDnd5eCreatureSpellcastingList List;
			List.Group = Group;
			if (!ReadCreatureSpellcastingEntries(Value, AppendPointer(Pointer, Group), List.Spells, State))
			{
				return false;
			}
			Out.Add(MoveTemp(List));
		}

		for (const TCHAR* Group : {TEXT("spells"), TEXT("daily"), TEXT("rest"), TEXT("restLong"),
			TEXT("recharge"), TEXT("legendary"), TEXT("charges"), TEXT("weekly"), TEXT("monthly"), TEXT("yearly")})
		{
			const auto Value = FindExactField(Object, Group);
			if (!Value.IsValid()) { continue; }

			const FString GroupPointer = AppendPointer(Pointer, Group);
			TSharedPtr<FJsonObject> Lists;
			if (!RequireObject(Value, GroupPointer, Lists, State)) { return false; }

			const bool bLevels = FCString::Strcmp(Group, TEXT("spells")) == 0;
			for (const auto& Pair : Lists->Values)
			{
				FDnd5eCreatureSpellcastingList List;
				List.Group = Group;
				List.Key = FString(Pair.Key.Len(), *Pair.Key);
				const FString ListPointer = AppendPointer(GroupPointer, List.Key);
				if (bLevels)
				{
					if (!TryParseBoundedIntegerToken(List.Key, 0, 9, List.Level))
					{
						return Fail(State, Codes::InvalidFieldValue, ListPointer, TEXT("Invalid spell level."));
					}

					TSharedPtr<FJsonObject> Level;
					TSharedPtr<FJsonValue> Spells;
					if (!RequireObject(Pair.Value, ListPointer, Level, State) ||
						!CheckAllowedKeys(*Level, {TEXT("spells"), TEXT("slots"), TEXT("lower")}, ListPointer, State) ||
						!ReadRequiredValue(*Level, TEXT("spells"), EJson::Array, ListPointer, Spells, State))
					{
						return false;
					}
					if ((FindExactField(*Level, TEXT("slots")).IsValid() &&
							!ReadStatInteger(*Level, TEXT("slots"), ListPointer, List.Slots, State)) ||
						(FindExactField(*Level, TEXT("lower")).IsValid() &&
							!ReadStatInteger(*Level, TEXT("lower"), ListPointer, List.LowerLevel, State)) ||
						!ReadCreatureSpellcastingEntries(Spells, AppendPointer(ListPointer, TEXT("spells")), List.Spells, State))
					{
						return false;
					}
				}
				else
				{
					List.bEach = List.Key.EndsWith(TEXT("e"), ESearchCase::CaseSensitive);
					const FString Count = List.bEach ? List.Key.LeftChop(1) : List.Key;
					if (!TryParseBoundedIntegerToken(Count, 1, 99, List.UseCount))
					{
						return Fail(State, Codes::InvalidFieldValue, ListPointer, TEXT("Invalid spell use key."));
					}
					if (!ReadCreatureSpellcastingEntries(Pair.Value, ListPointer, List.Spells, State)) { return false; }
				}
				Out.Add(MoveTemp(List));
			}
		}
		return true;
	}

	bool ReadCreatureSpellcasting(const FJsonObject& Object, const FString& Pointer,
		FDnd5eCreature& Creature, FReadState& State)
	{
		const auto Value = FindExactField(Object, TEXT("spellcasting"));
		if (!Value.IsValid()) { return true; }
		if (Value->Type == EJson::Null)
		{
			Creature.SpellcastingPresence = EDnd5eValuePresence::ExplicitNull;
			return true;
		}

		const FString CastingPointer = AppendPointer(Pointer, TEXT("spellcasting"));
		const TArray<TSharedPtr<FJsonValue>>* Blocks = nullptr;
		if (!RequireArray(Value, CastingPointer, Blocks, State)) { return false; }

		TArray<FDnd5eCreatureSpellcasting> Casting;
		Casting.Reserve(Blocks->Num());
		for (int32 Index = 0; Index < Blocks->Num(); ++Index)
		{
			const FString BlockPointer = AppendPointer(CastingPointer, FString::FromInt(Index));
			TSharedPtr<FJsonObject> Block;
			if (!RequireObject((*Blocks)[Index], BlockPointer, Block, State)) { return false; }

			FDnd5eCreatureSpellcasting Entry;
			const auto ReadText = [&](const TCHAR* Key, FString& Out)
			{
				return !FindExactField(*Block, Key).IsValid() ||
					ReadRequiredString(*Block, Key, BlockPointer, Out, State);
			};
			const auto ReadContent = [&](const TCHAR* Key, TArray<FDnd5eContentBlock>& Out)
			{
				const auto Content = FindExactField(*Block, Key);
				return !Content.IsValid() || ReadBlocks(Content, AppendPointer(BlockPointer, Key), Out, State);
			};

			FString ChargesItem;
			if (!ReadText(TEXT("name"), Entry.Name) ||
				!ReadText(TEXT("type"), Entry.SourceType) ||
				!ReadText(TEXT("ability"), Entry.Ability) ||
				!ReadText(TEXT("displayAs"), Entry.DisplayAs) ||
				!ReadText(TEXT("chargesItem"), ChargesItem) ||
				!ReadStrings(*Block, TEXT("hidden"), BlockPointer, Entry.HiddenGroups, State) ||
				!ReadContent(TEXT("headerEntries"), Entry.Header) ||
				!ReadContent(TEXT("footerEntries"), Entry.Footer) ||
				!ReadCreatureSpellcastingLists(*Block, BlockPointer, Entry.Lists, State))
			{
				return false;
			}
			if (!ChargesItem.IsEmpty() && !ReadInlines(ChargesItem,
				AppendPointer(BlockPointer, TEXT("chargesItem")), Entry.ChargesItem, State))
			{
				return false;
			}
			Casting.Add(MoveTemp(Entry));
		}
		Creature.Spellcasting = MoveTemp(Casting);
		Creature.SpellcastingPresence = EDnd5eValuePresence::Present;
		return true;
	}

	const TCHAR* DescribeJsonType(const TSharedPtr<FJsonValue>& Value)
	{
		if (!Value.IsValid()) { return TEXT("Invalid"); }
		switch (Value->Type)
		{
		case EJson::Null: return TEXT("Null");
		case EJson::String: return TEXT("String");
		case EJson::Number: return TEXT("Number");
		case EJson::Boolean: return TEXT("Boolean");
		case EJson::Array: return TEXT("Array");
		case EJson::Object: return TEXT("Object");
		default: return TEXT("Unknown");
		}
	}

	void InventoryCreatureValue(const TSharedPtr<FJsonValue>& Value, const FString& Path,
		const FString& Pointer, int32 Depth, TArray<FDnd5eCreatureFieldShape>& Out)
	{
		Out.Add({Path, DescribeJsonType(Value), Pointer});
		if (!Value.IsValid() || Depth >= 4) { return; }

		if (Value->Type == EJson::Object)
		{
			for (const auto& Pair : Value->AsObject()->Values)
			{
				const FString Key(Pair.Key.Len(), *Pair.Key);
				InventoryCreatureValue(Pair.Value, Path + TEXT(".") + Key,
					AppendPointer(Pointer, Key), Depth + 1, Out);
			}
		}
		else if (Value->Type == EJson::Array)
		{
			const auto& Elements = Value->AsArray();
			for (int32 Index = 0; Index < Elements.Num(); ++Index)
			{
				InventoryCreatureValue(Elements[Index], Path + TEXT("[]"),
					AppendPointer(Pointer, FString::FromInt(Index)), Depth + 1, Out);
			}
		}
	}

	void InventoryCreatureFields(const FJsonObject& Object, const FString& RowPointer,
		TArray<FDnd5eCreatureFieldShape>& Out)
	{
		for (const auto& Pair : Object.Values)
		{
			const FString Key(Pair.Key.Len(), *Pair.Key);
			InventoryCreatureValue(Pair.Value, Key, AppendPointer(RowPointer, Key), 0, Out);
		}
	}

    void SetCreatureTokenReference(const FString& Kind, const FString& Value, const FString& Pointer,
        FDnd5eMediaReference& Token, FReadState& State)
    {
        if (Kind == TEXT("internal") && Dnd5eMedia::IsDatasetRelativePath(Value))
        {
            Token.Kind = EDnd5eMediaReferenceKind::DatasetRelative;
            Token.RelativePath = Value;
        }
        else if (Kind == TEXT("external") &&
            (Value.StartsWith(TEXT("https://")) || Value.StartsWith(TEXT("http://"))))
        {
            Token.Kind = EDnd5eMediaReferenceKind::ExternalUrl;
            Token.Url = Value;
        }
        else
        {
            AddDiagnostic(State, EDnd5eDiagnosticSeverity::Warning, Codes::UnsupportedRequiredValue,
                Pointer, TEXT("Unsupported creature token reference."));
        }
    }

    bool ReadCreatureToken(const FJsonObject& Object, const FString& Pointer, FDnd5eCreature& Creature, FReadState& State)
    {
        if (const auto Value = FindExactField(Object, TEXT("tokenUrl")); Value && Value->Type != EJson::Null)
        {
            FString Url;
            if (!ReadRequiredString(Object, TEXT("tokenUrl"), Pointer, Url, State)) { return false; }
            SetCreatureTokenReference(TEXT("external"), Url, AppendPointer(Pointer, TEXT("tokenUrl")), Creature.Token, State);
            return true;
        }

        FString Name = Creature.Header.DisplayName;
        FString Source = Creature.Header.Provenance[0].SourceId;
        FString Location = AppendPointer(Pointer, TEXT("hasToken"));
        if (const auto Value = FindExactField(Object, TEXT("token")); Value && Value->Type != EJson::Null)
        {
            Location = AppendPointer(Pointer, TEXT("token"));
            TSharedPtr<FJsonObject> Selector;
            if (!RequireObject(Value, Location, Selector, State) ||
                !ReadRequiredString(*Selector, TEXT("name"), Location, Name, State) ||
                !ReadRequiredString(*Selector, TEXT("source"), Location, Source, State))
            {
                return false;
            }
        }
        else if (const auto Href = FindExactField(Object, TEXT("tokenHref")); Href && Href->Type != EJson::Null)
        {
            Location = AppendPointer(Pointer, TEXT("tokenHref"));
            TSharedPtr<FJsonObject> Reference;
            FString Kind;
            FString ValueText;
            if (!RequireObject(Href, Location, Reference, State) ||
                !ReadRequiredString(*Reference, TEXT("type"), Location, Kind, State))
            {
                return false;
            }

            const TCHAR* Field = Kind == TEXT("external") ? TEXT("url") : TEXT("path");
            if (!ReadRequiredString(*Reference, Field, Location, ValueText, State)) { return false; }
            SetCreatureTokenReference(Kind, ValueText, Location, Creature.Token, State);
            return true;
        }
        else
        {
            bool bHasToken = false;
            if (!ReadOptionalBool(Object, TEXT("hasToken"), Pointer, bHasToken, State)) { return false; }
            if (!bHasToken) { return true; }
        }

        Name.ReplaceInline(TEXT("\""), TEXT(""));
        if (Name.IsEmpty() || Source.IsEmpty() || Name.Contains(TEXT("/")) || Source.Contains(TEXT("/")))
        {
            AddDiagnostic(State, EDnd5eDiagnosticSeverity::Warning, Codes::UnsupportedRequiredValue,
                Location, TEXT("Creature token name or source cannot form a local filename."));
            return true;
        }
        for (TCHAR Character : Name)
        {
            if (Character > 127)
            {
                AddDiagnostic(State, EDnd5eDiagnosticSeverity::Warning, Codes::UnsupportedRequiredValue,
                    Location, TEXT("Supply an explicit token path for this non-ASCII name."));
                return true;
            }
        }

        const FString Path = TEXT("bestiary/tokens/") + Source + TEXT("/") + Name + TEXT(".webp");
        SetCreatureTokenReference(TEXT("internal"), Path, Location, Creature.Token, State);
        return true;
    }

	bool ReadCreatureRelationships(const FJsonObject& Object, const FString& Pointer,
		TArray<FDnd5eRecordRelationship>& Out, FReadState& State)
	{
		const auto Value = FindExactField(Object, TEXT("reprintedAs"));
		if (!Value.IsValid()) { return true; }

		const FString ArrayPointer = AppendPointer(Pointer, TEXT("reprintedAs"));
		const TArray<TSharedPtr<FJsonValue>>* Entries = nullptr;
		if (!RequireArray(Value, ArrayPointer, Entries, State)) { return false; }

		for (int32 Index = 0; Index < Entries->Num(); ++Index)
		{
			const FString EntryPointer = AppendPointer(ArrayPointer, FString::FromInt(Index));
			const auto& Entry = (*Entries)[Index];
			FDnd5eRecordRelationship Link;
			if (!Entry.IsValid() || Entry->Type != EJson::String || !Entry->TryGetString(Link.SourceReference))
			{
				return Fail(State, Codes::InvalidFieldValue, EntryPointer, TEXT("Expected a creature reprint reference string."));
			}
			if (!ValidateIdentityText(Link.SourceReference, EntryPointer, State)) { return false; }

			Link.Kind = Dnd5e::Record::Relationship::ReprintedAs;
			Link.SourceFormat = TEXT("5etools");

			FString Name, SourceId;
			if (Link.SourceReference.Split(TEXT("|"), &Name, &SourceId) &&
				!Name.IsEmpty() && !SourceId.IsEmpty() && !SourceId.Contains(TEXT("|")))
			{
				if (!ValidateIdentityText(Name, EntryPointer, State) ||
					!ValidateIdentityText(SourceId, EntryPointer, State))
				{
					return false;
				}
				Link.TargetRecordId = MakeCreatureRecordId(Name, SourceId, State.Context.DatasetNamespace);
				Link.bHasTargetRecord = true;
			}
			else
			{
				AddDiagnostic(State, EDnd5eDiagnosticSeverity::Warning, Codes::UnresolvedReference,
					EntryPointer, TEXT("Creature reprint retained without an explicit name and source."));
			}

			Out.Add(MoveTemp(Link));
		}
		return true;
	}

	bool ReadCreatureRow(const FJsonObject& Object, const FString& Pointer,
		FDnd5eCreature& Creature, FReadState& State)
	{
		if (!ReadCreatureHeader(Object, Pointer, Creature.Header, State)) { return false; }
		if (FindExactField(Object, TEXT("_copy")).IsValid())
		{
			return Fail(State, Codes::UnsupportedRequiredValue, AppendPointer(Pointer, TEXT("_copy")),
				TEXT("Creature copy assembly is pending."));
		}

		return ReadCreatureRelationships(Object, Pointer, Creature.Relationships, State) &&
			ReadCreatureToken(Object, Pointer, Creature, State) &&
			ReadCreatureClassification(Object, Pointer, Creature, State) &&
			ReadArmorClasses(Object, Pointer, Creature.ArmorClasses, Creature.ArmorClassPresence, State) &&
			ReadHitPoints(Object, Pointer, Creature.HitPoints, State) &&
			ReadMovementAndScores(Object, Pointer, Creature, State) &&
			ReadCreatureObservations(Object, Pointer, Creature, State) &&
			ReadCreatureEntries(Object, TEXT("trait"), Pointer, Creature.Traits, State) &&
			ReadCreatureEntries(Object, TEXT("action"), Pointer, Creature.Actions, State) &&
			ReadCreatureEntries(Object, TEXT("bonus"), Pointer, Creature.BonusActions, State) &&
			ReadCreatureActionGroup(Object, TEXT("reaction"), TEXT("reactionHeader"), Pointer, Creature.Reactions, State) &&
			ReadCreatureActionGroup(Object, TEXT("legendary"), TEXT("legendaryHeader"), Pointer, Creature.LegendaryActions, State) &&
			ReadCreatureActionGroup(Object, TEXT("mythic"), TEXT("mythicHeader"), Pointer, Creature.MythicActions, State) &&
			ReadCreatureLegendaryActionCounts(Object, Pointer, Creature, State) &&
			ReadCreatureSpellcasting(Object, Pointer, Creature, State);
	}

	void AuditCreatureRows(const FDnd5eJsonDocument& Document,
		const TArray<TSharedPtr<FJsonValue>>& Rows, const FDnd5eCreatureCopyIndex* CopyIndex,
		FDnd5eCreatureDocumentAudit& Report, TArray<FDnd5eCreature>* OutCreatures = nullptr)
	{
		Report.Rows.Reserve(Rows.Num());
		for (int32 Index = 0; Index < Rows.Num(); ++Index)
		{
			auto& Row = Report.Rows.AddDefaulted_GetRef();
			Row.Status = EDnd5eJsonReadStatus::Failure;
			Row.JsonPointer = AppendPointer(TEXT("/monster"), FString::FromInt(Index));

			FReadState RowState{Document.Context, Document.JsonText, Row.Diagnostics};
			TSharedPtr<FJsonObject> Object;
			if (!RequireObject(Rows[Index], Row.JsonPointer, Object, RowState)) { continue; }

			if (const auto Name = FindExactField(*Object, TEXT("name")); Name.IsValid() && Name->Type == EJson::String)
			{
				Row.Name = Name->AsString();
			}
			if (const auto Source = FindExactField(*Object, TEXT("source")); Source.IsValid() && Source->Type == EJson::String)
			{
				Row.SourceId = Source->AsString();
			}
			Row.bIsCopy = FindExactField(*Object, TEXT("_copy")).IsValid();
			if (!OutCreatures)
			{
				InventoryCreatureFields(*Object, Row.JsonPointer, Row.FieldShapes);
			}

			TSharedPtr<FJsonObject> Assembled = Object;
			TArray<FDnd5eCreatureCopyRow> Ancestors;
			if (CopyIndex && Row.bIsCopy &&
				!ResolveCreatureCopy(*CopyIndex, {Object, Document.Context.RelativeFilePath, Row.JsonPointer},
					Assembled, Ancestors, RowState))
			{
				continue;
			}

			FDnd5eCreature Candidate;
			if (ReadCreatureRow(*Assembled, Row.JsonPointer, Candidate, RowState))
			{
				Row.Status = EDnd5eJsonReadStatus::Success;
				if (OutCreatures)
				{
					OutCreatures->Add(MoveTemp(Candidate));
				}
			}
		}
	}
}

FDnd5eJsonCreatureReadResult FDnd5eJsonCreatureReader::ReadCreature(const FDnd5eJsonDocument& Document,
	const FString& Name, const FString& SourceId)
{
	using namespace Dnd5eJsonReader;

	FDnd5eJsonCreatureReadResult Result;
	Result.Status = EDnd5eJsonReadStatus::Failure;
	FReadState State{Document.Context, Document.JsonText, Result.Diagnostics};

	TSharedPtr<FJsonObject> Root;
	const TArray<TSharedPtr<FJsonValue>>* Rows = nullptr;
	if (!ValidateIdentityText(Name, TEXT(""), State) ||
		!ValidateIdentityText(SourceId, TEXT(""), State) ||
		!ReadCreatureDocument(Root, Rows, State))
	{
		return Result;
	}

	TSharedPtr<FJsonObject> Row;
	FString Pointer;
	FDnd5eCreature Candidate;
	if (!FindCreatureRow(*Rows, Name, SourceId, Row, Pointer, State) ||
		!ReadCreatureRow(*Row, Pointer, Candidate, State))
	{
		return Result;
	}

	Result.Creature = MoveTemp(Candidate);
	Result.Status = EDnd5eJsonReadStatus::Success;
	return Result;
}

FDnd5eCreatureDocumentAudit FDnd5eJsonCreatureReader::AuditDocument(const FDnd5eJsonDocument& Document)
{
	using namespace Dnd5eJsonReader;

	FDnd5eCreatureDocumentAudit Report;
	Report.Status = EDnd5eJsonReadStatus::Failure;
	FReadState DocumentState{Document.Context, Document.JsonText, Report.Diagnostics};

	TSharedPtr<FJsonObject> Root;
	const TArray<TSharedPtr<FJsonValue>>* Rows = nullptr;
	if (!ReadCreatureDocument(Root, Rows, DocumentState)) { return Report; }

	AuditCreatureRows(Document, *Rows, nullptr, Report);

	Report.Status = EDnd5eJsonReadStatus::Success;
	return Report;
}

static bool IndexCreatureTemplates(const FDnd5eJsonDocument& Document, const FString& DatasetNamespace,
	FDnd5eCreatureCopyIndex& Out, TArray<FDnd5eDiagnostic>& Diagnostics)
{
	using namespace Dnd5eJsonReader;

	FReadState State{Document.Context, Document.JsonText, Diagnostics};
	if (!Document.Context.DatasetNamespace.Equals(DatasetNamespace, ESearchCase::CaseSensitive))
	{
		return Fail(State, Codes::InvalidFieldValue, TEXT(""), TEXT("Template and creature namespaces differ."));
	}
	TSharedPtr<FJsonValue> RootValue;
	TSharedPtr<FJsonObject> Root;
	const TArray<TSharedPtr<FJsonValue>>* Rows = nullptr;
	if (!ValidateReadContext(State) || !ValidateDocumentText(State) ||
		!PreflightJson(State) || !ParseJsonDocument(RootValue, State) ||
		!RequireObject(RootValue, TEXT(""), Root, State) ||
		!ReadRequiredArray(*Root, TEXT("monsterTemplate"), TEXT(""), Rows, State))
	{
		return false;
	}

	FDnd5eCreatureCopyIndex Templates;
	for (int32 Index = 0; Index < Rows->Num(); ++Index)
	{
		const FString Pointer = AppendPointer(TEXT("/monsterTemplate"), FString::FromInt(Index));
		TSharedPtr<FJsonObject> Object;
		FString Name;
		FString Source;
		if (!RequireObject((*Rows)[Index], Pointer, Object, State) ||
			!ReadRequiredString(*Object, TEXT("name"), Pointer, Name, State) ||
			!ValidateIdentityText(Name, AppendPointer(Pointer, TEXT("name")), State) ||
			!ReadRequiredString(*Object, TEXT("source"), Pointer, Source, State) ||
			!ValidateIdentityText(Source, AppendPointer(Pointer, TEXT("source")), State))
		{
			return false;
		}
		IndexCreatureCopyRow(Templates, {Object, Document.Context.RelativeFilePath, Pointer});
	}

	Out.TemplateRows = MoveTemp(Templates.Rows);
	Out.AmbiguousTemplateKeys = MoveTemp(Templates.AmbiguousKeys);
	Out.bHasTemplateDocument = true;
	return true;
}

static TArray<FDnd5eCreatureDocumentAudit> ProcessCreatureDocuments(
	TArrayView<const FDnd5eJsonDocument> Documents, const FDnd5eJsonDocument* TemplateDocument, TArray<FDnd5eCreature>* OutCreatures)
{
	using namespace Dnd5eJsonReader;
	using FCreatureRows = TArray<TSharedPtr<FJsonValue>>;

	TArray<FDnd5eCreatureDocumentAudit> Reports;
	Reports.SetNum(Documents.Num());
	TArray<TSharedPtr<FJsonObject>> Roots;
	Roots.SetNum(Documents.Num());
	TArray<const FCreatureRows*> RowsByDocument;
	RowsByDocument.SetNumZeroed(Documents.Num());
	FDnd5eCreatureCopyIndex CopyIndex;
	if (TemplateDocument && !Documents.IsEmpty())
	{
		IndexCreatureTemplates(*TemplateDocument, Documents[0].Context.DatasetNamespace, CopyIndex, Reports[0].Diagnostics);
	}

	for (int32 DocumentIndex = 0; DocumentIndex < Documents.Num(); ++DocumentIndex)
	{
		const auto& Document = Documents[DocumentIndex];
		auto& Report = Reports[DocumentIndex];
		Report.Status = EDnd5eJsonReadStatus::Failure;
		FReadState State{Document.Context, Document.JsonText, Report.Diagnostics};
		if (!Document.Context.DatasetNamespace.Equals(Documents[0].Context.DatasetNamespace, ESearchCase::CaseSensitive))
		{
			Fail(State, Codes::InvalidFieldValue, TEXT(""), TEXT("Expected one dataset namespace per creature audit batch."));
			continue;
		}

		const FCreatureRows* Rows = nullptr;
		if (!ReadCreatureDocument(Roots[DocumentIndex], Rows, State)) { continue; }
		RowsByDocument[DocumentIndex] = Rows;
		for (int32 RowIndex = 0; RowIndex < Rows->Num(); ++RowIndex)
		{
			const auto& Value = (*Rows)[RowIndex];
			const TSharedPtr<FJsonObject>* Object = nullptr;
			if (Value.IsValid() && Value->Type == EJson::Object && Value->TryGetObject(Object) && Object)
			{
				IndexCreatureCopyRow(CopyIndex, {*Object, Document.Context.RelativeFilePath,
					AppendPointer(TEXT("/monster"), FString::FromInt(RowIndex))});
			}
		}
	}

	for (int32 DocumentIndex = 0; DocumentIndex < Documents.Num(); ++DocumentIndex)
	{
		if (const FCreatureRows* Rows = RowsByDocument[DocumentIndex])
		{
			auto& Report = Reports[DocumentIndex];
			AuditCreatureRows(Documents[DocumentIndex], *Rows, &CopyIndex, Report, OutCreatures);
			Report.Status = EDnd5eJsonReadStatus::Success;
		}
	}
	return Reports;
}

TArray<FDnd5eCreatureDocumentAudit> FDnd5eJsonCreatureReader::AuditDocuments(
	TArrayView<const FDnd5eJsonDocument> Documents)
{
	return ProcessCreatureDocuments(Documents, nullptr, nullptr);
}

TArray<FDnd5eCreatureDocumentAudit> FDnd5eJsonCreatureReader::AuditDocuments(
	TArrayView<const FDnd5eJsonDocument> Documents, const FDnd5eJsonDocument& TemplateDocument)
{
	return ProcessCreatureDocuments(Documents, &TemplateDocument, nullptr);
}

FDnd5eCreatureBatchReadResult FDnd5eJsonCreatureReader::ReadDocuments(
	TArrayView<const FDnd5eJsonDocument> Documents, const FDnd5eJsonDocument* TemplateDocument)
{
	FDnd5eCreatureBatchReadResult Result;
	Result.Documents = ProcessCreatureDocuments(Documents, TemplateDocument, &Result.Creatures);
	return Result;
}
