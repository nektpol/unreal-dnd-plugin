// Developed by Neko Creative Technologies

#include "Readers/Bestiary/Dnd5eJsonCreatureReader.h"
#include "Readers/Dnd5eJsonReaderInternal.h"
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
		Header.RecordId.DatasetNamespace = State.Context.DatasetNamespace;
		Header.RecordId.Category = Dnd5e::Category::Creature;
		Header.RecordId.SourceKey = TEXT("5etools:creature:v1:") + EscapeIdentityComponent(Header.DisplayName) +
			TEXT("|") + EscapeIdentityComponent(Origin.SourceId);

		Origin.RelativeFilePath = State.Context.RelativeFilePath;
		Origin.JsonPointer = Pointer;
		if (const auto Page = FindExactField(Object, TEXT("page")))
		{
			if (!ReadPage(Page, AppendPointer(Pointer, TEXT("page")), Origin.Page, State)) { return false; }
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
			if (!ReadRequiredString(*TypeObject, TEXT("type"), TypePointer, Type, State) ||
				!ReadCreatureTypeTag(Type, AppendPointer(TypePointer, TEXT("type")), Creature.CreatureType, State))
			{
				return false;
			}
			return ReadStrings(*TypeObject, TEXT("tags"), TypePointer, Creature.CreatureSubtypes, State);
		}

		return ReadRequiredString(Object, TEXT("type"), Pointer, Type, State) &&
			ReadCreatureTypeTag(Type, TypePointer, Creature.CreatureType, State);
	}

	bool ReadArmorClasses(const FJsonObject& Object, const FString& Pointer,
		TArray<FDnd5eCreatureArmorClass>& Out, FReadState& State)
	{
		const TArray<TSharedPtr<FJsonValue>>* Rows = nullptr;
		if (!ReadRequiredArray(Object, TEXT("ac"), Pointer, Rows, State)) { return false; }
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
			}
			else
			{
				TSharedPtr<FJsonObject> Entry;
				if (!RequireObject(Row, RowPointer, Entry, State) ||
					!CheckAllowedKeys(*Entry, {TEXT("ac"), TEXT("from")}, RowPointer, State) ||
					!ReadStatInteger(*Entry, TEXT("ac"), RowPointer, Armor.Value, State))
				{
					return false;
				}
				if (const auto From = FindExactField(*Entry, TEXT("from")))
				{
					if (!ReadBlocks(From, AppendPointer(RowPointer, TEXT("from")), Armor.Sources, State)) { return false; }
				}
			}
			Out.Add(MoveTemp(Armor));
		}
		return true;
	}

	bool ReadHitPoints(const FJsonObject& Object, const FString& Pointer,
		FDnd5eCreatureHitPoints& Out, FReadState& State)
	{
		TSharedPtr<FJsonObject> Hp;
		const FString HpPointer = AppendPointer(Pointer, TEXT("hp"));
		if (!ReadRequiredObject(Object, TEXT("hp"), Pointer, Hp, State) ||
			!ReadStatInteger(*Hp, TEXT("average"), HpPointer, Out.Average, State) ||
			!ReadRequiredString(*Hp, TEXT("formula"), HpPointer, Out.Formula.Expression, State))
		{
			return false;
		}
		Out.Formula.Syntax = TEXT("5etools");
		return true;
	}

	bool ReadMovementAndScores(const FJsonObject& Object, const FString& Pointer,
		FDnd5eCreature& Creature, FReadState& State)
	{
		TSharedPtr<FJsonObject> Speed;
		const FString SpeedPointer = AppendPointer(Pointer, TEXT("speed"));
		int32 Walk = 0;
		if (!ReadRequiredObject(Object, TEXT("speed"), Pointer, Speed, State) ||
			!ReadStatInteger(*Speed, TEXT("walk"), SpeedPointer, Walk, State))
		{
			return false;
		}
		Creature.WalkSpeed.Unit = EDnd5eDistanceUnit::Feet;
		Creature.WalkSpeed.Amount.Kind = EDnd5eNumberKind::Integer;
		Creature.WalkSpeed.Amount.Integer = Walk;

		auto& Scores = Creature.AbilityScores;
		return ReadStatInteger(Object, TEXT("str"), Pointer, Scores.Strength, State) &&
			ReadStatInteger(Object, TEXT("dex"), Pointer, Scores.Dexterity, State) &&
			ReadStatInteger(Object, TEXT("con"), Pointer, Scores.Constitution, State) &&
			ReadStatInteger(Object, TEXT("int"), Pointer, Scores.Intelligence, State) &&
			ReadStatInteger(Object, TEXT("wis"), Pointer, Scores.Wisdom, State) &&
			ReadStatInteger(Object, TEXT("cha"), Pointer, Scores.Charisma, State);
	}

	bool ReadSkills(const FJsonObject& Object, const FString& Pointer,
		TArray<FDnd5eCreatureSkillBonus>& Out, FReadState& State)
	{
		const auto Value = FindExactField(Object, TEXT("skill"));
		if (!Value.IsValid()) { return true; }

		const FString SkillsPointer = AppendPointer(Pointer, TEXT("skill"));
		TSharedPtr<FJsonObject> Skills;
		if (!RequireObject(Value, SkillsPointer, Skills, State)) { return false; }

		for (const auto& Pair : Skills->Values)
		{
			const FString SkillName(Pair.Key.Len(), *Pair.Key);
			FString Token;
			int32 Magnitude = 0;
			if (!Pair.Value.IsValid() || Pair.Value->Type != EJson::String || !Pair.Value->TryGetString(Token))
			{
				return Fail(State, Codes::InvalidFieldValue,
					AppendPointer(SkillsPointer, SkillName), TEXT("Expected a skill bonus string."));
			}

			const bool bNegative = Token.StartsWith(TEXT("-"));
			const bool bHasSign = bNegative || Token.StartsWith(TEXT("+"));
			const FString Digits = bHasSign ? Token.Mid(1) : Token;
			if (!TryParseBoundedIntegerToken(Digits, 0, MAX_int32, Magnitude))
			{
				return Fail(State, Codes::InvalidFieldValue,
					AppendPointer(SkillsPointer, SkillName), TEXT("Expected an integer skill bonus."));
			}

			FDnd5eCreatureSkillBonus Skill;
			Skill.Skill = SkillName;
			Skill.Bonus = bNegative ? -Magnitude : Magnitude;
			Out.Add(MoveTemp(Skill));
		}
		return true;
	}

	bool ReadCreatureObservations(const FJsonObject& Object, const FString& Pointer,
		FDnd5eCreature& Creature, FReadState& State)
	{
		return ReadSkills(Object, Pointer, Creature.Skills, State) &&
			ReadStrings(Object, TEXT("senses"), Pointer, Creature.Senses, State) &&
			ReadStrings(Object, TEXT("languages"), Pointer, Creature.Languages, State) &&
			ReadStatInteger(Object, TEXT("passive"), Pointer, Creature.PassivePerception, State) &&
			ReadRequiredString(Object, TEXT("cr"), Pointer, Creature.ChallengeRating, State);
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

	bool ReadCreatureRow(const FJsonObject& Object, const FString& Pointer,
		FDnd5eCreature& Creature, FReadState& State)
	{
		if (!ReadCreatureHeader(Object, Pointer, Creature.Header, State)) { return false; }
		if (FindExactField(Object, TEXT("_copy")).IsValid())
		{
			return Fail(State, Codes::UnsupportedRequiredValue, AppendPointer(Pointer, TEXT("_copy")),
				TEXT("Creature copy assembly is pending."));
		}

		return ReadCreatureClassification(Object, Pointer, Creature, State) &&
			ReadArmorClasses(Object, Pointer, Creature.ArmorClasses, State) &&
			ReadHitPoints(Object, Pointer, Creature.HitPoints, State) &&
			ReadMovementAndScores(Object, Pointer, Creature, State) &&
			ReadCreatureObservations(Object, Pointer, Creature, State) &&
			ReadCreatureEntries(Object, TEXT("trait"), Pointer, Creature.Traits, State) &&
			ReadCreatureEntries(Object, TEXT("action"), Pointer, Creature.Actions, State) &&
			ReadCreatureEntries(Object, TEXT("bonus"), Pointer, Creature.BonusActions, State);
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

	Report.Rows.Reserve(Rows->Num());
	for (int32 Index = 0; Index < Rows->Num(); ++Index)
	{
		auto& Row = Report.Rows.AddDefaulted_GetRef();
		Row.Status = EDnd5eJsonReadStatus::Failure;
		Row.JsonPointer = AppendPointer(TEXT("/monster"), FString::FromInt(Index));

		FReadState RowState{Document.Context, Document.JsonText, Row.Diagnostics};
		TSharedPtr<FJsonObject> Object;
		if (!RequireObject((*Rows)[Index], Row.JsonPointer, Object, RowState)) { continue; }

		if (const auto Name = FindExactField(*Object, TEXT("name")); Name.IsValid() && Name->Type == EJson::String)
		{
			Row.Name = Name->AsString();
		}
		if (const auto Source = FindExactField(*Object, TEXT("source")); Source.IsValid() && Source->Type == EJson::String)
		{
			Row.SourceId = Source->AsString();
		}
		Row.bIsCopy = FindExactField(*Object, TEXT("_copy")).IsValid();
		InventoryCreatureFields(*Object, Row.JsonPointer, Row.FieldShapes);

		FDnd5eCreature Candidate;
		if (ReadCreatureRow(*Object, Row.JsonPointer, Candidate, RowState))
		{
			Row.Status = EDnd5eJsonReadStatus::Success;
		}
	}

	Report.Status = EDnd5eJsonReadStatus::Success;
	return Report;
}
