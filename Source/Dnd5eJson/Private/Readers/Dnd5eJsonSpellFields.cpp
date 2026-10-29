#include "Readers/Dnd5eJsonReaderInternal.h"

#include "Dnd5eTags.h"
#include "String/LexFromString.h"

namespace Dnd5eJsonReader
{
	bool ReadNumberValue(const TSharedPtr<FJsonValue>& Value, const FString& Pointer, FDnd5eNumber& OutNumber, FReadState& State)
	{
		FDnd5eNumber Number;
		if (!Value.IsValid() || Value->Type != EJson::Number || !Value->TryGetString(Number.SourceLiteral))
		{
			return Fail(State, Codes::InvalidFieldValue, Pointer, TEXT("Expected a JSON number with its original token retained."));
		}
		const FString& Token = Number.SourceLiteral;
		const bool bNegative = Token.StartsWith(TEXT("-"));
		const FString Magnitude = bNegative ? Token.Mid(1) : Token;
		int32 Integer = 0;
		if (bNegative && Token.Equals(TEXT("-2147483648"), ESearchCase::CaseSensitive))
		{
			Number.Kind = EDnd5eNumberKind::Integer;
			Number.Integer = MIN_int32;
		}
		else if (TryParseBoundedIntegerToken(Magnitude, 0, MAX_int32, Integer))
		{
			Number.Kind = EDnd5eNumberKind::Integer;
			Number.Integer = bNegative ? -Integer : Integer;
		}
		else if (!Token.Contains(TEXT(".")) && !Token.Contains(TEXT("e"), ESearchCase::IgnoreCase))
		{
			Number.Kind = EDnd5eNumberKind::Literal;
		}
		else if (LexTryParseString(Number.Decimal, *Token) && FMath::IsFinite(Number.Decimal))
		{
			Number.Kind = EDnd5eNumberKind::Decimal;
			if (Number.Decimal == 0.0)
			{
				for (TCHAR Character : Token)
				{
					if (Character == TEXT('e') || Character == TEXT('E')) { break; }
					if (Character >= TEXT('1') && Character <= TEXT('9')) { Number.Kind = EDnd5eNumberKind::Literal; break; }
				}
			}
		}
		else { Number.Kind = EDnd5eNumberKind::Literal; Number.Decimal = 0.0; }
		OutNumber = MoveTemp(Number);
		return true;
	}

	bool ReadNonnegativeNumber(const TSharedPtr<FJsonValue>& Value, const FString& Pointer, FDnd5eNumber& OutNumber, FReadState& State)
	{
		FDnd5eNumber Number;
		if (!ReadNumberValue(Value, Pointer, Number, State)) { return false; }
		if (Number.SourceLiteral.StartsWith(TEXT("-"))) { return Fail(State, Codes::InvalidFieldValue, Pointer, TEXT("This measurement/cost cannot be negative.")); }
		OutNumber = MoveTemp(Number);
		return true;
	}

	bool ReadDistanceUnit(const FString& Text, const FString& Pointer, EDnd5eDistanceUnit& OutUnit, FReadState& State)
	{
		struct FUnit { const TCHAR* Name; EDnd5eDistanceUnit Unit; };
		const FUnit Units[] = {{TEXT("inches"), EDnd5eDistanceUnit::Inches}, {TEXT("feet"), EDnd5eDistanceUnit::Feet}, {TEXT("yards"), EDnd5eDistanceUnit::Yards}, {TEXT("miles"), EDnd5eDistanceUnit::Miles}};
		for (const auto& Unit : Units)
		{
			if (Text.Equals(Unit.Name, ESearchCase::CaseSensitive)) { OutUnit = Unit.Unit; return true; }
		}
		return Fail(State, Codes::UnsupportedRequiredValue, Pointer, TEXT("Unknown distance unit."));
	}

	bool ReadMeasurement(const FJsonObject& Object, const FString& Pointer, const TCHAR* TypeKey, const TCHAR* AmountKey, FDnd5eDistance& OutDistance, FReadState& State)
	{
		FString Type;
		TSharedPtr<FJsonValue> Amount;
		FDnd5eDistance Distance;
		if (!ReadRequiredString(Object, TypeKey, Pointer, Type, State) || !ReadDistanceUnit(Type, AppendPointer(Pointer, TypeKey), Distance.Unit, State)) { return false; }
		if (!ReadRequiredValue(Object, AmountKey, EJson::Number, Pointer, Amount, State) || !ReadNonnegativeNumber(Amount, AppendPointer(Pointer, AmountKey), Distance.Amount, State)) { return false; }
		OutDistance = MoveTemp(Distance);
		return true;
	}

	bool ReadRange(const FJsonObject& Object, const FString& RecordPointer, FDnd5eSpell& Candidate, FReadState& State)
	{
		const FString Pointer = AppendPointer(RecordPointer, TEXT("range"));
		TSharedPtr<FJsonObject> RangeObject;
		FString Type;
		if (!ReadRequiredObject(Object, TEXT("range"), RecordPointer, RangeObject, State) || !ReadRequiredString(*RangeObject, TEXT("type"), Pointer, Type, State)) { return false; }
		FDnd5eRange Range;
		if (Type.Equals(TEXT("special"), ESearchCase::CaseSensitive))
		{
			if (!CheckAllowedKeys(*RangeObject, {TEXT("type")}, Pointer, State)) { return false; }
			Range.Kind = Dnd5e::Range::Kind::Special;
			Candidate.Range = MoveTemp(Range);
			return true;
		}
		if (!CheckAllowedKeys(*RangeObject, {TEXT("type"), TEXT("distance")}, Pointer, State)) { return false; }
		const FString DistancePointer = AppendPointer(Pointer, TEXT("distance"));
		TSharedPtr<FJsonObject> Distance;
		if (!ReadRequiredObject(*RangeObject, TEXT("distance"), Pointer, Distance, State)) { return false; }
		if (Type.Equals(TEXT("point"), ESearchCase::CaseSensitive))
		{
			FString DistanceType;
			if (!ReadRequiredString(*Distance, TEXT("type"), DistancePointer, DistanceType, State)) { return false; }
			struct FSymbol { const TCHAR* Name; FGameplayTag Tag; };
			const FSymbol Symbols[] = {
				{TEXT("self"), Dnd5e::Range::Kind::Self}, {TEXT("touch"), Dnd5e::Range::Kind::Touch}, {TEXT("sight"), Dnd5e::Range::Kind::Sight},
				{TEXT("unlimited"), Dnd5e::Range::Kind::Unlimited}, {TEXT("plane"), Dnd5e::Range::Kind::UnlimitedSamePlane}, {TEXT("special"), Dnd5e::Range::Kind::Special}
			};
			for (const auto& Symbol : Symbols)
			{
				if (!DistanceType.Equals(Symbol.Name, ESearchCase::CaseSensitive)) { continue; }
				if (!CheckAllowedKeys(*Distance, {TEXT("type")}, DistancePointer, State)) { return false; }
				Range.Kind = Symbol.Tag;
				Candidate.Range = MoveTemp(Range);
				return true;
			}
			if (!CheckAllowedKeys(*Distance, {TEXT("type"), TEXT("amount")}, DistancePointer, State) || !ReadMeasurement(*Distance, DistancePointer, TEXT("type"), TEXT("amount"), Range.Distance, State)) { return false; }
			Range.Kind = Dnd5e::Range::Kind::Distance;
			Range.bHasDistance = true;
		}
		else
		{
			struct FShape { const TCHAR* Name; FGameplayTag Tag; EDnd5eAreaDimensionKind Dimension; };
			const FShape Shapes[] = {
				{TEXT("cone"), Dnd5e::Area::Shape::Cone, EDnd5eAreaDimensionKind::Length}, {TEXT("line"), Dnd5e::Area::Shape::Line, EDnd5eAreaDimensionKind::Length},
				{TEXT("cube"), Dnd5e::Area::Shape::Cube, EDnd5eAreaDimensionKind::EdgeLength}, {TEXT("sphere"), Dnd5e::Area::Shape::Sphere, EDnd5eAreaDimensionKind::Radius},
				{TEXT("hemisphere"), Dnd5e::Area::Shape::Hemisphere, EDnd5eAreaDimensionKind::Radius}, {TEXT("radius"), Dnd5e::Area::Shape::Radius, EDnd5eAreaDimensionKind::Radius},
				{TEXT("emanation"), Dnd5e::Area::Shape::Emanation, EDnd5eAreaDimensionKind::Extent}, {TEXT("cylinder"), Dnd5e::Area::Shape::Cylinder, EDnd5eAreaDimensionKind::Radius}
			};
			const FShape* Shape = nullptr;
			for (const auto& Entry : Shapes) { if (Type.Equals(Entry.Name, ESearchCase::CaseSensitive)) { Shape = &Entry; break; } }
			if (!Shape) { return Fail(State, Codes::UnsupportedRequiredValue, AppendPointer(Pointer, TEXT("type")), TEXT("Unknown range shape.")); }
			const bool bCylinder = Type.Equals(TEXT("cylinder"), ESearchCase::CaseSensitive);
			if (bCylinder)
			{
				if (!CheckAllowedKeys(*Distance, {TEXT("type"), TEXT("amount"), TEXT("typeSecondary"), TEXT("amountSecondary")}, DistancePointer, State)) { return false; }
			}
			else if (!CheckAllowedKeys(*Distance, {TEXT("type"), TEXT("amount")}, DistancePointer, State)) { return false; }
			FDnd5eSpellArea Area;
			Area.Origin = Dnd5e::Area::Origin::Self;
			Area.Geometry.Shape = Shape->Tag;
			FDnd5eAreaDimension Dimension;
			Dimension.Kind = Shape->Dimension;
			if (!ReadMeasurement(*Distance, DistancePointer, TEXT("type"), TEXT("amount"), Dimension.Measurement, State)) { return false; }
			Area.Geometry.Dimensions.Add(MoveTemp(Dimension));
			if (bCylinder && (FindExactField(*Distance, TEXT("typeSecondary")).IsValid() || FindExactField(*Distance, TEXT("amountSecondary")).IsValid()))
			{
				FDnd5eAreaDimension Height;
				Height.Kind = EDnd5eAreaDimensionKind::Height;
				if (!ReadMeasurement(*Distance, DistancePointer, TEXT("typeSecondary"), TEXT("amountSecondary"), Height.Measurement, State)) { return false; }
				Area.Geometry.Dimensions.Add(MoveTemp(Height));
			}
			Range.Kind = Dnd5e::Range::Kind::Self;
			Candidate.Areas.Add(MoveTemp(Area));
		}
		Candidate.Range = MoveTemp(Range);
		return true;
	}

	bool ReadComponents(const FJsonObject& Object, const FString& RecordPointer, FDnd5eSpell& Candidate, FReadState& State)
	{
		const FString Pointer = AppendPointer(RecordPointer, TEXT("components"));
		TSharedPtr<FJsonObject> Components;
		if (!ReadRequiredObject(Object, TEXT("components"), RecordPointer, Components, State) || !CheckAllowedKeys(*Components, {TEXT("v"), TEXT("s"), TEXT("m"), TEXT("r")}, Pointer, State)) { return false; }
		FDnd5eComponents Result;
		bool bVerbal = false, bSomatic = false, bRoyalty = false;
		if (!ReadOptionalBool(*Components, TEXT("v"), Pointer, bVerbal, State) || !ReadOptionalBool(*Components, TEXT("s"), Pointer, bSomatic, State) || !ReadOptionalBool(*Components, TEXT("r"), Pointer, bRoyalty, State)) { return false; }
		if (bVerbal) { Result.Types.AddTag(Dnd5e::Component::Type::Verbal); }
		if (bSomatic) { Result.Types.AddTag(Dnd5e::Component::Type::Somatic); }
		if (bRoyalty) { Result.Types.AddTag(Dnd5e::Component::Type::Royalty); }
		if (const auto Material = FindExactField(*Components, TEXT("m")))
		{
			const FString MaterialPointer = AppendPointer(Pointer, TEXT("m"));
			if (Material->Type == EJson::Boolean)
			{
				bool bRequired = false;
				Material->TryGetBool(bRequired);
				if (bRequired) { Result.Types.AddTag(Dnd5e::Component::Type::Material); }
			}
			else
			{
				Result.Types.AddTag(Dnd5e::Component::Type::Material);
				Result.bHasMaterial = true;
				if (Material->Type == EJson::String)
				{
					FDnd5eContentBlock Block;
					if (!ReadContent(Material, MaterialPointer, Block, State)) { return false; }
					Result.Material.Description.Add(MoveTemp(Block));
				}
				else
				{
					TSharedPtr<FJsonObject> Details;
					FString Text;
					if (!RequireObject(Material, MaterialPointer, Details, State) || !CheckAllowedKeys(*Details, {TEXT("text"), TEXT("cost"), TEXT("consume")}, MaterialPointer, State)) { return false; }
					if (!ReadRequiredString(*Details, TEXT("text"), MaterialPointer, Text, State)) { return false; }
					FDnd5eContentBlock Block;
					if (!ReadContent(FindExactField(*Details, TEXT("text")), AppendPointer(MaterialPointer, TEXT("text")), Block, State)) { return false; }
					Result.Material.Description.Add(MoveTemp(Block));
					if (const auto Cost = FindExactField(*Details, TEXT("cost")))
					{
						FDnd5eMoney Money;
						Money.Currency = EDnd5eCurrency::Copper;
						if (!ReadNonnegativeNumber(Cost, AppendPointer(MaterialPointer, TEXT("cost")), Money.Amount, State)) { return false; }
						Result.Material.bHasCost = true;
						Result.Material.Cost.Add(MoveTemp(Money));
					}
					if (const auto Consume = FindExactField(*Details, TEXT("consume")))
					{
						FString Choice;
						bool bConsumed = false;
						if (Consume->Type == EJson::Boolean && Consume->TryGetBool(bConsumed)) { Result.Material.Consumption = bConsumed ? EDnd5eMaterialConsumption::Consumed : EDnd5eMaterialConsumption::NotConsumed; }
						else if (Consume->Type == EJson::String && Consume->TryGetString(Choice) && Choice.Equals(TEXT("optional"), ESearchCase::CaseSensitive)) { Result.Material.Consumption = EDnd5eMaterialConsumption::Optional; }
						else { return Fail(State, Codes::InvalidFieldValue, AppendPointer(MaterialPointer, TEXT("consume")), TEXT("Consumption must be Boolean or the exact string optional.")); }
					}
				}
			}
		}
		Candidate.Components = MoveTemp(Result);
		return true;
	}

	bool ReadDurations(const FJsonObject& Object, const FString& RecordPointer, FDnd5eSpell& Candidate, FReadState& State)
	{
		const FString Pointer = AppendPointer(RecordPointer, TEXT("duration"));
		const TArray<TSharedPtr<FJsonValue>>* Values = nullptr;
		if (!ReadRequiredArray(Object, TEXT("duration"), RecordPointer, Values, State)) { return false; }
		if (Values->IsEmpty()) { return Fail(State, Codes::InvalidFieldValue, Pointer, TEXT("At least one duration is required.")); }
		TArray<FDnd5eSpellDuration> Durations;
		bool bAnyConcentration = false;
		for (int32 Index = 0; Index < Values->Num(); ++Index)
		{
			const FString EntryPointer = AppendPointer(Pointer, FString::FromInt(Index));
			TSharedPtr<FJsonObject> Entry;
			FString Type;
			if (!RequireObject((*Values)[Index], EntryPointer, Entry, State) || !ReadRequiredString(*Entry, TEXT("type"), EntryPointer, Type, State)) { return false; }
			FDnd5eSpellDuration Duration;
			if (Type.Equals(TEXT("instant"), ESearchCase::CaseSensitive) || Type.Equals(TEXT("special"), ESearchCase::CaseSensitive))
			{
				if (!CheckAllowedKeys(*Entry, {TEXT("type")}, EntryPointer, State)) { return false; }
				Duration.Duration.Kind = Type.Equals(TEXT("instant"), ESearchCase::CaseSensitive) ? EDnd5eDurationKind::Instantaneous : EDnd5eDurationKind::Special;
			}
			else if (Type.Equals(TEXT("permanent"), ESearchCase::CaseSensitive))
			{
				if (!CheckAllowedKeys(*Entry, {TEXT("type"), TEXT("ends")}, EntryPointer, State)) { return false; }
				Duration.Duration.Kind = EDnd5eDurationKind::Permanent;
				if (const auto Ends = FindExactField(*Entry, TEXT("ends")))
				{
					const TArray<TSharedPtr<FJsonValue>>* Endings = nullptr;
					if (!RequireArray(Ends, AppendPointer(EntryPointer, TEXT("ends")), Endings, State)) { return false; }
					for (int32 EndingIndex = 0; EndingIndex < Endings->Num(); ++EndingIndex)
					{
						FString Ending;
						const auto& Value = (*Endings)[EndingIndex];
						const FString EndPointer = AppendPointer(AppendPointer(EntryPointer, TEXT("ends")), FString::FromInt(EndingIndex));
						if (Value->Type != EJson::String || !Value->TryGetString(Ending)) { return Fail(State, Codes::InvalidFieldValue, EndPointer, TEXT("Expected an ending-condition string.")); }
						if (Ending.Equals(TEXT("dispel"), ESearchCase::CaseSensitive)) { Duration.Duration.EndingConditions.AddTag(Dnd5e::Duration::Ending::Dispelled); }
						else if (Ending.Equals(TEXT("trigger"), ESearchCase::CaseSensitive)) { Duration.Duration.EndingConditions.AddTag(Dnd5e::Duration::Ending::Triggered); }
						else if (Ending.Equals(TEXT("discharge"), ESearchCase::CaseSensitive)) { Duration.Duration.EndingConditions.AddTag(Dnd5e::Duration::Ending::Discharged); }
						else { return Fail(State, Codes::UnsupportedRequiredValue, EndPointer, TEXT("Unknown ending condition.")); }
					}
				}
			}
			else if (Type.Equals(TEXT("timed"), ESearchCase::CaseSensitive))
			{
				if (!CheckAllowedKeys(*Entry, {TEXT("type"), TEXT("duration"), TEXT("concentration")}, EntryPointer, State)) { return false; }
				const FString TimePointer = AppendPointer(EntryPointer, TEXT("duration"));
				TSharedPtr<FJsonObject> Time;
				if (!ReadRequiredObject(*Entry, TEXT("duration"), EntryPointer, Time, State) || !CheckAllowedKeys(*Time, {TEXT("type"), TEXT("amount"), TEXT("upTo")}, TimePointer, State)) { return false; }
				if (!ReadRequiredString(*Time, TEXT("type"), TimePointer, Type, State) || !ReadTimeUnit(Type, AppendPointer(TimePointer, TEXT("type")), Duration.Duration.Time.Unit, State)) { return false; }
				TSharedPtr<FJsonValue> Amount;
				if (!ReadRequiredValue(*Time, TEXT("amount"), EJson::Number, TimePointer, Amount, State) || !ReadNonnegativeNumber(Amount, AppendPointer(TimePointer, TEXT("amount")), Duration.Duration.Time.Amount, State)) { return false; }
				bool bPositive = false;
				for (TCHAR Character : Duration.Duration.Time.Amount.SourceLiteral)
				{
					if (Character == TEXT('e') || Character == TEXT('E')) { break; }
					if (Character >= TEXT('1') && Character <= TEXT('9')) { bPositive = true; break; }
				}
				if (!bPositive) { return Fail(State, Codes::InvalidFieldValue, AppendPointer(TimePointer, TEXT("amount")), TEXT("Timed duration must be positive.")); }
				Duration.Duration.Kind = EDnd5eDurationKind::Timed;
				Duration.Duration.bHasTime = true;
				bool bConcentration = false;
				if (!ReadOptionalBool(*Time, TEXT("upTo"), TimePointer, Duration.Duration.bIsMaximum, State) || !ReadOptionalBool(*Entry, TEXT("concentration"), EntryPointer, bConcentration, State)) { return false; }
				if (bConcentration) { Duration.Requirements.AddTag(Dnd5e::Trait::Concentration); bAnyConcentration = true; }
			}
			else { return Fail(State, Codes::UnsupportedRequiredValue, AppendPointer(EntryPointer, TEXT("type")), TEXT("Unknown duration type.")); }
			Durations.Add(MoveTemp(Duration));
		}
		Candidate.Durations = MoveTemp(Durations);
		if (bAnyConcentration) { Candidate.Traits.AddTag(Dnd5e::Trait::Concentration); }
		return true;
	}

	bool ReadCombatClassifications(const FJsonObject& Object, const FString& RecordPointer, FDnd5eSpell& Candidate, FReadState& State)
	{
		struct FTag { const TCHAR* Name; FGameplayTag Tag; };
		const FTag Damage[] = {
			{TEXT("acid"), Dnd5e::Damage::Type::Acid}, {TEXT("bludgeoning"), Dnd5e::Damage::Type::Bludgeoning}, {TEXT("cold"), Dnd5e::Damage::Type::Cold}, {TEXT("fire"), Dnd5e::Damage::Type::Fire},
			{TEXT("force"), Dnd5e::Damage::Type::Force}, {TEXT("lightning"), Dnd5e::Damage::Type::Lightning}, {TEXT("necrotic"), Dnd5e::Damage::Type::Necrotic}, {TEXT("piercing"), Dnd5e::Damage::Type::Piercing},
			{TEXT("poison"), Dnd5e::Damage::Type::Poison}, {TEXT("psychic"), Dnd5e::Damage::Type::Psychic}, {TEXT("radiant"), Dnd5e::Damage::Type::Radiant}, {TEXT("slashing"), Dnd5e::Damage::Type::Slashing}, {TEXT("thunder"), Dnd5e::Damage::Type::Thunder}
		};
		const FTag Conditions[] = {
			{TEXT("blinded"), Dnd5e::Condition::Type::Blinded}, {TEXT("charmed"), Dnd5e::Condition::Type::Charmed}, {TEXT("deafened"), Dnd5e::Condition::Type::Deafened}, {TEXT("exhaustion"), Dnd5e::Condition::Type::Exhaustion},
			{TEXT("frightened"), Dnd5e::Condition::Type::Frightened}, {TEXT("grappled"), Dnd5e::Condition::Type::Grappled}, {TEXT("incapacitated"), Dnd5e::Condition::Type::Incapacitated}, {TEXT("invisible"), Dnd5e::Condition::Type::Invisible},
			{TEXT("paralyzed"), Dnd5e::Condition::Type::Paralyzed}, {TEXT("petrified"), Dnd5e::Condition::Type::Petrified}, {TEXT("poisoned"), Dnd5e::Condition::Type::Poisoned}, {TEXT("prone"), Dnd5e::Condition::Type::Prone},
			{TEXT("restrained"), Dnd5e::Condition::Type::Restrained}, {TEXT("stunned"), Dnd5e::Condition::Type::Stunned}, {TEXT("unconscious"), Dnd5e::Condition::Type::Unconscious}
		};
		const FTag Abilities[] = {
			{TEXT("strength"), Dnd5e::Ability::Strength}, {TEXT("dexterity"), Dnd5e::Ability::Dexterity}, {TEXT("constitution"), Dnd5e::Ability::Constitution},
			{TEXT("intelligence"), Dnd5e::Ability::Intelligence}, {TEXT("wisdom"), Dnd5e::Ability::Wisdom}, {TEXT("charisma"), Dnd5e::Ability::Charisma}
		};
		const FTag Creatures[] = {
			{TEXT("aberration"), Dnd5e::Creature::Type::Aberration}, {TEXT("beast"), Dnd5e::Creature::Type::Beast}, {TEXT("celestial"), Dnd5e::Creature::Type::Celestial}, {TEXT("construct"), Dnd5e::Creature::Type::Construct},
			{TEXT("dragon"), Dnd5e::Creature::Type::Dragon}, {TEXT("elemental"), Dnd5e::Creature::Type::Elemental}, {TEXT("fey"), Dnd5e::Creature::Type::Fey}, {TEXT("fiend"), Dnd5e::Creature::Type::Fiend},
			{TEXT("giant"), Dnd5e::Creature::Type::Giant}, {TEXT("humanoid"), Dnd5e::Creature::Type::Humanoid}, {TEXT("monstrosity"), Dnd5e::Creature::Type::Monstrosity}, {TEXT("ooze"), Dnd5e::Creature::Type::Ooze},
			{TEXT("plant"), Dnd5e::Creature::Type::Plant}, {TEXT("undead"), Dnd5e::Creature::Type::Undead}
		};
		const FTag Attacks[] = {{TEXT("M"), Dnd5e::Attack::Mode::Melee}, {TEXT("R"), Dnd5e::Attack::Mode::Ranged}};
		struct FField { const TCHAR* Name; FGameplayTagContainer* Output; TConstArrayView<FTag> Tags; };
		const FField Fields[] = {
			{TEXT("damageInflict"), &Candidate.DamageInflicted, Damage}, {TEXT("damageResist"), &Candidate.DamageResisted, Damage}, {TEXT("damageImmune"), &Candidate.DamageImmunities, Damage}, {TEXT("damageVulnerable"), &Candidate.DamageVulnerabilities, Damage},
			{TEXT("conditionInflict"), &Candidate.ConditionInflicted, Conditions}, {TEXT("conditionImmune"), &Candidate.ConditionImmunities, Conditions}, {TEXT("savingThrow"), &Candidate.SavingThrowAbilities, Abilities},
			{TEXT("abilityCheck"), &Candidate.AbilityCheckAbilities, Abilities}, {TEXT("affectsCreatureType"), &Candidate.AffectedCreatureTypes, Creatures}, {TEXT("spellAttack"), &Candidate.AttackTypes, Attacks}
		};
		for (const auto& Field : Fields)
		{
			const auto Value = FindExactField(Object, Field.Name);
			if (!Value.IsValid()) { continue; }
			const FString Pointer = AppendPointer(RecordPointer, Field.Name);
			const TArray<TSharedPtr<FJsonValue>>* Values = nullptr;
			if (!RequireArray(Value, Pointer, Values, State)) { return false; }
			for (int32 Index = 0; Index < Values->Num(); ++Index)
			{
				FString Name;
				const auto& Entry = (*Values)[Index];
				const FString EntryPointer = AppendPointer(Pointer, FString::FromInt(Index));
				if (Entry->Type != EJson::String || !Entry->TryGetString(Name)) { return Fail(State, Codes::InvalidFieldValue, EntryPointer, TEXT("Expected a classification string.")); }
				const FTag* Match = nullptr;
				for (const auto& Tag : Field.Tags) { if (Name.Equals(Tag.Name, ESearchCase::CaseSensitive)) { Match = &Tag; break; } }
				if (!Match) { return Fail(State, Codes::UnsupportedRequiredValue, EntryPointer, TEXT("Unknown classification; no tag was invented.")); }
				Field.Output->AddTag(Match->Tag);
			}
		}
		return true;
	}

	bool ReadLevelScaling(const FJsonObject& Object, const FString& RecordPointer, FDnd5eSpell& Candidate, FReadState& State)
	{
		const auto Value = FindExactField(Object, TEXT("scalingLevelDice"));
		if (!Value.IsValid()) { return true; }
		const FString Pointer = AppendPointer(RecordPointer, TEXT("scalingLevelDice"));
		TArray<TSharedPtr<FJsonValue>> Groups;
		if (Value->Type == EJson::Array)
		{
			const TArray<TSharedPtr<FJsonValue>>* Values = nullptr;
			if (!RequireArray(Value, Pointer, Values, State)) { return false; }
			Groups = *Values;
		}
		else { Groups.Add(Value); }
		for (int32 Index = 0; Index < Groups.Num(); ++Index)
		{
			const FString GroupPointer = Value->Type == EJson::Array ? AppendPointer(Pointer, FString::FromInt(Index)) : Pointer;
			TSharedPtr<FJsonObject> Group;
			TSharedPtr<FJsonObject> Scaling;
			FString Label;
			if (!RequireObject(Groups[Index], GroupPointer, Group, State) || !CheckAllowedKeys(*Group, {TEXT("label"), TEXT("scaling")}, GroupPointer, State)) { return false; }
			if (!ReadRequiredString(*Group, TEXT("label"), GroupPointer, Label, State) || !ReadRequiredObject(*Group, TEXT("scaling"), GroupPointer, Scaling, State)) { return false; }
			FDnd5eSpellLevelScaling Result;
			if (!ReadInlines(Label, AppendPointer(GroupPointer, TEXT("label")), Result.Label, State)) { return false; }
			for (const auto& Pair : Scaling->Values)
			{
				const FString Key(Pair.Key.Len(), *Pair.Key);
				const FString EntryPointer = AppendPointer(AppendPointer(GroupPointer, TEXT("scaling")), Key);
				FDnd5eSpellLevelScalingEntry Entry;
				if (!TryParseBoundedIntegerToken(Key, 1, MAX_int32, Entry.Level) || Pair.Value->Type != EJson::String || !Pair.Value->TryGetString(Entry.Dice.Expression) || Entry.Dice.Expression.IsEmpty())
				{
					return Fail(State, Codes::InvalidFieldValue, EntryPointer, TEXT("Scaling requires positive integer level keys and nonempty dice-expression strings."));
				}
				Entry.Dice.Syntax = TEXT("5etools");
				Result.Entries.Add(MoveTemp(Entry));
			}
			Result.Entries.Sort([](const auto& A, const auto& B) { return A.Level < B.Level; });
			Candidate.LevelScaling.Add(MoveTemp(Result));
		}
		return true;
	}
}
