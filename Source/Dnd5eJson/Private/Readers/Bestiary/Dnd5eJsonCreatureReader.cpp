// Developed by Neko Creative Technologies

#include "Readers/Bestiary/Dnd5eJsonCreatureReader.h"
#include "Readers/Dnd5eJsonReaderInternal.h"
#include "Dnd5eTags.h"

namespace
{
	using namespace Dnd5eJsonReader;

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
			!ReadRequiredString(Object, TEXT("source"), Pointer, Origin.SourceId, State))
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
		const TMap<FString, FGameplayTag> AlignmentMap{
			{TEXT("L"), Dnd5e::Alignment::Lawful},
			{TEXT("C"), Dnd5e::Alignment::Chaotic},
			{TEXT("N"), Dnd5e::Alignment::Neutral},
			{TEXT("G"), Dnd5e::Alignment::Good},
			{TEXT("E"), Dnd5e::Alignment::Evil},
			{TEXT("U"), Dnd5e::Alignment::Unaligned},
		};
		TArray<FString> Sizes, Alignments;
		if (!ReadStrings(Object, TEXT("size"), Pointer, Sizes, State, true) ||
			!ReadStrings(Object, TEXT("alignment"), Pointer, Alignments, State, true))
		{
			return false;
		}
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
		for (int32 Index = 0; Index < Alignments.Num(); ++Index)
		{
			const auto* Tag = AlignmentMap.Find(Alignments[Index]);
			if (!Tag)
			{
				return Fail(State, Codes::UnsupportedRequiredValue,
					AppendPointer(AppendPointer(Pointer, TEXT("alignment")), FString::FromInt(Index)), TEXT("Unsupported alignment."));
			}
			Creature.Alignment.AddTag(*Tag);
		}

		FString Type;
		const FString TypePointer = AppendPointer(Pointer, TEXT("type"));
		const auto TypeValue = FindExactField(Object, TEXT("type"));
		if (TypeValue.IsValid() && TypeValue->Type == EJson::Object)
		{
			const auto TypeObject = TypeValue->AsObject();
			if (!ReadRequiredString(*TypeObject, TEXT("type"), TypePointer, Type, State) ||
				!ReadStrings(*TypeObject, TEXT("tags"), TypePointer, Creature.CreatureSubtypes, State))
			{
				return false;
			}
		}
		else if (!ReadRequiredString(Object, TEXT("type"), Pointer, Type, State))
		{
			return false;
		}

		if (Type == TEXT("humanoid")) { Creature.CreatureType = Dnd5e::Creature::Type::Humanoid; }
		else if (Type == TEXT("fey")) { Creature.CreatureType = Dnd5e::Creature::Type::Fey; }
		else { return Fail(State, Codes::UnsupportedRequiredValue, TypePointer, TEXT("Creature type outside the first reader slice.")); }
		return true;
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

	TSharedPtr<FJsonValue> RootValue;
	TSharedPtr<FJsonObject> Root;
	if (!ValidateReadContext(State) || !ValidateDocumentText(State) ||
		!ValidateIdentityText(Name, TEXT(""), State) || !ValidateIdentityText(SourceId, TEXT(""), State) ||
		!PreflightJson(State) || !ParseJsonDocument(RootValue, State) ||
		!RequireObject(RootValue, TEXT(""), Root, State))
	{
		return Result;
	}

	const TArray<TSharedPtr<FJsonValue>>* Rows = nullptr;
	if (!ReadRequiredArray(*Root, TEXT("monster"), TEXT(""), Rows, State)) { return Result; }
	if (Rows->Num() > 100000)
	{
		Fail(State, Codes::ResourceLimitExceeded, TEXT("/monster"), TEXT("Too many creature rows."));
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