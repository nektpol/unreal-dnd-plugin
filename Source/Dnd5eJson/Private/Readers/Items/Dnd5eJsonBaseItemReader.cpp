// Developed by Neko Creative Technologies

#include "Readers/Items/Dnd5eJsonBaseItemReader.h"
#include "Readers/Dnd5eJsonReaderInternal.h"
#include "Items/Dnd5eItemIdentity.h"
#include "Dnd5eTags.h"

namespace
{
	using namespace Dnd5eJsonReader;

	bool IsBaseItemPageZero(const TSharedPtr<FJsonValue>& Value)
	{
		FString Literal;
		return Value.IsValid() && Value->Type == EJson::Number &&
			Value->TryGetString(Literal) && Literal == TEXT("0");
	}

	bool ReadBaseItemHeader(const FJsonObject& Object, const FString& Pointer,
		FDnd5eItem& Item, FReadState& State)
	{
		FDnd5eRecordProvenance Origin;
		if (!ReadRequiredString(Object, TEXT("name"), Pointer, Item.Header.DisplayName, State) ||
			!ReadRequiredString(Object, TEXT("source"), Pointer, Origin.SourceId, State) ||
			!ValidateIdentityText(Item.Header.DisplayName, AppendPointer(Pointer, TEXT("name")), State) ||
			!ValidateIdentityText(Origin.SourceId, AppendPointer(Pointer, TEXT("source")), State))
		{
			return false;
		}

		if (!Dnd5e::Items::TryMakeId(State.Context.DatasetNamespace, Item.Header.DisplayName,
			Origin.SourceId, Item.Header.RecordId))
		{
			return Fail(State, Codes::InvalidFieldValue, Pointer, TEXT("Invalid base item identity."));
		}

		Origin.RelativeFilePath = State.Context.RelativeFilePath;
		Origin.JsonPointer = Pointer;
		if (const auto Page = FindExactField(Object, TEXT("page")))
		{
			if (!IsBaseItemPageZero(Page) &&
				!ReadPage(Page, AppendPointer(Pointer, TEXT("page")), Origin.Page, State))
			{
				return false;
			}
		}
		Item.Header.Provenance.Add(MoveTemp(Origin));
		return true;
	}
	bool ReadBaseItemUid(const FString& Text, const FString& Pointer, FString& Key,
		bool& bHasSource, FString& Source, FReadState& State)
	{
		Key.Reset();
		Source.Reset();
		bHasSource = Text.Split(TEXT("|"), &Key, &Source, ESearchCase::CaseSensitive);
		if (!bHasSource)
		{
			Key = Text;
			return ValidateIdentityText(Key, Pointer, State);
		}
		if (Source.Contains(TEXT("|")))
		{
			return Fail(State, Codes::UnsupportedRequiredValue, Pointer, TEXT("Unsupported item UID shape."));
		}
		return ValidateIdentityText(Key, Pointer, State) && ValidateIdentityText(Source, Pointer, State);
	}

	bool ReadReferenceText(const TSharedPtr<FJsonValue>& Value, const FString& Pointer,
		FString& Uid, FString& Note, bool& bHasNote, FReadState& State)
	{
		bHasNote = false;
		if (Value.IsValid() && Value->Type == EJson::String && Value->TryGetString(Uid))
		{
			return true;
		}

		TSharedPtr<FJsonObject> Object;
		if (!RequireObject(Value, Pointer, Object, State) ||
			!CheckAllowedKeys(*Object, {TEXT("uid"), TEXT("note")}, Pointer, State) ||
			!ReadRequiredString(*Object, TEXT("uid"), Pointer, Uid, State) ||
			!ReadRequiredString(*Object, TEXT("note"), Pointer, Note, State))
		{
			return false;
		}
		bHasNote = true;
		return true;
	}

	bool ReadBaseItemSelectors(const FJsonObject& Object, const FString& Pointer,
		FDnd5eItem& Item, bool& bRetainRawProperty, bool bRequireType, FReadState& State)
	{
		if (bRequireType || FindExactField(Object, TEXT("type")).IsValid())
		{
			FString TypeUid;
			Item.Type.Category = Dnd5e::Category::ItemType;
			if (!ReadRequiredString(Object, TEXT("type"), Pointer, TypeUid, State) ||
				!ReadBaseItemUid(TypeUid, AppendPointer(Pointer, TEXT("type")),
					Item.Type.Abbreviation, Item.Type.bHasSource, Item.Type.SourceId, State))
			{
				return false;
			}
			Item.bHasType = true;
		}

		for (const TCHAR* Field : {TEXT("property"), TEXT("mastery")})
		{
			const auto Value = FindExactField(Object, Field);
			if (!Value.IsValid()) { continue; }

			const FString ArrayPointer = AppendPointer(Pointer, Field);
			const TArray<TSharedPtr<FJsonValue>>* Rows = nullptr;
			if (!RequireArray(Value, ArrayPointer, Rows, State)) { return false; }

			const bool bProperty = FCString::Strcmp(Field, TEXT("property")) == 0;
			for (int32 Index = 0; Index < Rows->Num(); ++Index)
			{
				const FString EntryPointer = AppendPointer(ArrayPointer, FString::FromInt(Index));
				FString Uid, Note;
				bool bHasNote = false;
				if (!ReadReferenceText((*Rows)[Index], EntryPointer, Uid, Note, bHasNote, State))
				{
					return false;
				}
				const FString UidPointer = bHasNote ? AppendPointer(EntryPointer, TEXT("uid")) : EntryPointer;
				if (bProperty)
				{
					FDnd5eItemDefinitionSelector Selector;
					Selector.Category = Dnd5e::Category::ItemProperty;
					if (!ReadBaseItemUid(Uid, UidPointer, Selector.Abbreviation,
						Selector.bHasSource, Selector.SourceId, State))
					{
						return false;
					}
					Item.Properties.Add(MoveTemp(Selector));
					if (bHasNote)
					{
						bRetainRawProperty = true;
						AddDiagnostic(State, EDnd5eDiagnosticSeverity::Warning,
							Codes::PreservedUnsupportedMetadata, AppendPointer(EntryPointer, TEXT("note")),
							TEXT("Property note retained in the original property array."));
					}
				}
				else
				{
					FDnd5eItemMasteryReference Reference;
					Reference.Selector.Category = Dnd5e::Category::ItemMastery;
					if (!ReadBaseItemUid(Uid, UidPointer, Reference.Selector.Name,
						Reference.Selector.bHasSource, Reference.Selector.SourceId, State))
					{
						return false;
					}
					if (bHasNote && !ReadInlines(Note, AppendPointer(EntryPointer, TEXT("note")), Reference.Note, State))
					{
						return false;
					}
					Item.Masteries.Add(MoveTemp(Reference));
				}
			}
		}
		return true;
	}
    bool ReadBaseItemValues(const FJsonObject& Object, const FString& Pointer,
        FDnd5eItem& Item, bool& bRetainRawRarity, bool& bRetainRawPrice, FReadState& State)
    {
        FString Rarity;
        if (!ReadRequiredString(Object, TEXT("rarity"), Pointer, Rarity, State)) { return false; }
		struct FRarityEntry
        {
            const TCHAR* Source;
            FGameplayTag Tag;
        };
		using namespace Dnd5e::Item::Rarity;
		const FRarityEntry Known[] = {
			{TEXT("none"), None}, {TEXT("common"), Common}, {TEXT("uncommon"), Uncommon},
			{TEXT("rare"), Rare}, {TEXT("very rare"), VeryRare}, {TEXT("legendary"), Legendary},
			{TEXT("artifact"), Artifact}, {TEXT("unknown"), Unknown},
			{TEXT("unknown (magic)"), UnknownMagic}, {TEXT("varies"), Varies}
		};
		for (const FRarityEntry& Entry : Known)
		{
			if (Rarity.Equals(Entry.Source, ESearchCase::CaseSensitive))
			{
				Item.Rarity = Entry.Tag;
				break;
			}
		}
		if (!Item.Rarity.IsValid())
		{
			bRetainRawRarity = true;
			AddDiagnostic(State, EDnd5eDiagnosticSeverity::Warning, Codes::PreservedUnsupportedMetadata,
				AppendPointer(Pointer, TEXT("rarity")), TEXT("Unmapped item rarity retained."));
		}

        if (FindExactField(Object, TEXT("currencyConversion")).IsValid())
        {
            bRetainRawPrice = true;
            AddDiagnostic(State, EDnd5eDiagnosticSeverity::Warning, Codes::PreservedUnsupportedMetadata,
                AppendPointer(Pointer, TEXT("currencyConversion")), TEXT("Custom price and currency conversion retained."));
        }
        else if (const auto Value = FindExactField(Object, TEXT("value")))
        {
            if (Value->Type == EJson::Null)
            {
                Item.Price.CostPresence = EDnd5eValuePresence::ExplicitNull;
            }
            else
            {
                FDnd5eMoney Cost;
                if (!ReadNonnegativeNumber(Value, AppendPointer(Pointer, TEXT("value")), Cost.Amount, State))
                {
                    return false;
                }
                Cost.Currency = EDnd5eCurrency::Copper;
                Item.Price.Cost.Add(MoveTemp(Cost));
                Item.Price.CostPresence = EDnd5eValuePresence::Present;
            }
        }

        if (const auto Weight = FindExactField(Object, TEXT("weight")))
        {
            if (!ReadNonnegativeNumber(Weight, AppendPointer(Pointer, TEXT("weight")), Item.Weight.Amount, State))
            {
                return false;
            }
            Item.Weight.Unit = EDnd5eMassUnit::Pounds;
            Item.bHasWeight = true;
        }
        return true;
    }

	FGameplayTag MapBaseItemDamageType(const FString& Code)
	{
	    using namespace Dnd5e::Damage::Type;
	    if (Code.Equals(TEXT("B"), ESearchCase::CaseSensitive)) { return Bludgeoning; }
	    if (Code.Equals(TEXT("P"), ESearchCase::CaseSensitive)) { return Piercing; }
	    if (Code.Equals(TEXT("S"), ESearchCase::CaseSensitive)) { return Slashing; }
	    if (Code.Equals(TEXT("R"), ESearchCase::CaseSensitive)) { return Radiant; }
	    if (Code.Equals(TEXT("N"), ESearchCase::CaseSensitive)) { return Necrotic; }
	    if (Code.Equals(TEXT("C"), ESearchCase::CaseSensitive)) { return Cold; }
	    if (Code.Equals(TEXT("O"), ESearchCase::CaseSensitive)) { return Force; }
	    if (Code.Equals(TEXT("Y"), ESearchCase::CaseSensitive)) { return Psychic; }
	    return {};
	}

	void SetBaseItemFeet(FDnd5eDistance& Distance, int32 Value, const FString& Literal)
	{
	    Distance.Unit = EDnd5eDistanceUnit::Feet;
	    Distance.Amount.Kind = EDnd5eNumberKind::Integer;
	    Distance.Amount.Integer = Value;
	    Distance.Amount.SourceLiteral = Literal;
	}


	bool ReadBaseItemArmor(const FJsonObject& Object, const FString& Pointer,
	    FDnd5eItem& Item, TSet<FString>& Consumed, FReadState& State)
	{
	    auto& Armor = Item.ArmorData;
	    if (FindExactField(Object, TEXT("ac")).IsValid())
	    {
	        int32 Value = 0;
	        FString Literal;
	        if (!ReadRequiredInteger(Object, TEXT("ac"), Pointer, 1, MAX_int32, Value, Literal, State))
	        {
	            return false;
	        }

	        const FString& Type = Item.Type.Abbreviation;
	        const bool bLight = Type.Equals(TEXT("LA"), ESearchCase::CaseSensitive);
	        const bool bMedium = Type.Equals(TEXT("MA"), ESearchCase::CaseSensitive);
	        const bool bHeavy = Type.Equals(TEXT("HA"), ESearchCase::CaseSensitive);
	        const bool bShield = Type.Equals(TEXT("S"), ESearchCase::CaseSensitive);
	        if ((bLight || bMedium || bHeavy || bShield) &&
	            !FindExactField(Object, TEXT("dexterityMax")).IsValid())
	        {
	            auto& AC = Armor.ArmorClass;
	            AC.Value.Kind = EDnd5eNumberKind::Integer;
	            AC.Value.Integer = Value;
	            AC.Value.SourceLiteral = MoveTemp(Literal);
	            AC.Kind = bShield ? EDnd5eArmorClassValueKind::Bonus : EDnd5eArmorClassValueKind::Base;
	            if (bLight) { AC.DexterityMode = EDnd5eArmorClassDexterityMode::Uncapped; }
	            else if (bMedium)
	            {
	                AC.DexterityMode = EDnd5eArmorClassDexterityMode::Capped;
	                AC.DexterityModifierMaximum = 2;
	            }
	            else { AC.DexterityMode = EDnd5eArmorClassDexterityMode::NotAdded; }
	            Armor.bHasArmorClass = true;
	            Item.bHasArmorData = true;
	            Consumed.Add(TEXT("ac"));
	        }
	    }

	    if (const auto Strength = FindExactField(Object, TEXT("strength")))
	    {
	        if (Strength->Type == EJson::Null)
	        {
	            Armor.StrengthPresence = EDnd5eValuePresence::ExplicitNull;
	        }
	        else
	        {
	            FString Token;
	            int32 Minimum = 0;
	            if (Strength->Type != EJson::String || !Strength->TryGetString(Token) ||
	                !TryParseBoundedIntegerToken(Token, 1, MAX_int32, Minimum))
	            {
	                return Fail(State, Codes::InvalidFieldValue, AppendPointer(Pointer, TEXT("strength")),
	                    TEXT("Expected a positive Strength threshold string or null."));
	            }
	            Armor.StrengthMinimum.Ability = Dnd5e::Ability::Strength;
	            Armor.StrengthMinimum.Minimum = Minimum;
	            Armor.StrengthPresence = EDnd5eValuePresence::Present;
	        }
	        Item.bHasArmorData = true;
	        Consumed.Add(TEXT("strength"));
	    }

	    if (FindExactField(Object, TEXT("stealth")).IsValid())
	    {
	        if (!ReadOptionalBool(Object, TEXT("stealth"), Pointer, Armor.bStealthDisadvantage, State))
	        {
	            return false;
	        }
	        Armor.bHasStealthDisadvantage = true;
	        Item.bHasArmorData = true;
	        Consumed.Add(TEXT("stealth"));
	    }
	    return true;
	}
	bool ReadBaseItemWeapon(const FJsonObject& Object, const FString& Pointer,
	    FDnd5eItem& Item, TSet<FString>& Consumed, bool bBaseItem, FReadState& State)
	{
	    auto& Weapon = Item.WeaponData;
	    if (FindExactField(Object, TEXT("weaponCategory")).IsValid())
	    {
	        FString Category;
	        if (!ReadRequiredString(Object, TEXT("weaponCategory"), Pointer, Category, State)) { return false; }
	        if (Category.Equals(TEXT("simple"), ESearchCase::CaseSensitive))
	        {
	            Weapon.Category = EDnd5eWeaponCategory::Simple;
	        }
	        else if (Category.Equals(TEXT("martial"), ESearchCase::CaseSensitive))
	        {
	            Weapon.Category = EDnd5eWeaponCategory::Martial;
	        }
	        if (Weapon.Category != EDnd5eWeaponCategory::Unspecified)
	        {
	            Consumed.Add(TEXT("weaponCategory"));
	            Item.bHasWeaponData = true;
	        }
	    }

	    FGameplayTag DamageType;
	    const bool bHasDamageType = FindExactField(Object, TEXT("dmgType")).IsValid();
	    if (bHasDamageType)
	    {
	        FString Code;
	        if (!ReadRequiredString(Object, TEXT("dmgType"), Pointer, Code, State)) { return false; }
	        DamageType = MapBaseItemDamageType(Code);
	    }

	    const auto ReadDamage = [&](const TCHAR* Field, FDnd5eDamageExpression& Out, bool& bPresent)
	    {
	        if (!FindExactField(Object, Field).IsValid()) { return true; }
	        FString Expression;
	        if (!ReadRequiredString(Object, Field, Pointer, Expression, State) ||
	            !ValidateIdentityText(Expression, AppendPointer(Pointer, Field), State))
	        {
	            return false;
	        }
	        if (bHasDamageType && !DamageType.IsValid()) { return true; }

	        Out.Amount.Expression = MoveTemp(Expression);
	        Out.Amount.Syntax = TEXT("5etools");
	        Out.Type = DamageType;
	        bPresent = true;
	        Item.bHasWeaponData = true;
	        Consumed.Add(Field);
	        if (bHasDamageType) { Consumed.Add(TEXT("dmgType")); }
	        return true;
	    };
	    if (!ReadDamage(TEXT("dmg1"), Weapon.PrimaryDamage, Weapon.bHasPrimaryDamage) ||
	        !ReadDamage(TEXT("dmg2"), Weapon.AlternateDamage, Weapon.bHasAlternateDamage))
	    {
	        return false;
	    }

	    if (FindExactField(Object, TEXT("range")).IsValid())
	    {
	        FString Range, Normal, Long;
	        if (!ReadRequiredString(Object, TEXT("range"), Pointer, Range, State)) { return false; }
            const bool bHasLong = Range.Split(TEXT("/"), &Normal, &Long);
            if (!bHasLong) { Normal = Range; }
            int32 NormalFeet = 0, LongFeet = 0;
            if ((!bHasLong && bBaseItem) ||
				!TryParseBoundedIntegerToken(Normal, 1, MAX_int32, NormalFeet) ||
				(bHasLong && !TryParseBoundedIntegerToken(Long, NormalFeet, MAX_int32, LongFeet)))
            {
                return Fail(State, Codes::InvalidFieldValue, AppendPointer(Pointer, TEXT("range")),
					TEXT("Expected a positive range with any long distance at least normal."));
            }
            SetBaseItemFeet(Weapon.Range.Normal, NormalFeet, Normal);
            if (bHasLong) { SetBaseItemFeet(Weapon.Range.Long, LongFeet, Long); }
            Weapon.Range.bHasLong = bHasLong;
	        Weapon.bHasRange = true;
	        Item.bHasWeaponData = true;
	        Consumed.Add(TEXT("range"));
	    }

	    if (FindExactField(Object, TEXT("reload")).IsValid())
	    {
	        FString Literal;
	        if (!ReadRequiredInteger(Object, TEXT("reload"), Pointer, 1, MAX_int32,
	            Weapon.Reload, Literal, State)) { return false; }
	        Weapon.bHasReload = true;
	        Item.bHasWeaponData = true;
	        Consumed.Add(TEXT("reload"));
	    }
	    return true;
	}

	bool ReadBaseItemMetadata(const FJsonObject& Object, const FString& Pointer,
        bool bRetainRawProperty, bool bRetainRawRarity, bool bRetainRawPrice,
        const TSet<FString>& ConsumedEquipment,
        FDnd5eRecordProvenance& Origin, FReadState& State)
    {
        auto Extra = MakeShared<FJsonObject>();
        TArray<FString> Keys;
        for (const auto& Pair : Object.Values) { Keys.Emplace(Pair.Key.Len(), *Pair.Key); }
        Keys.Sort([](const FString& A, const FString& B)
        {
            return A.Compare(B, ESearchCase::CaseSensitive) < 0;
        });

        for (const FString& Key : Keys)
        {
            bool bConsumed = false;
            for (const FString& Field : ConsumedEquipment)
            {
                if (Key.Equals(Field, ESearchCase::CaseSensitive))
                {
                    bConsumed = true;
                    break;
                }
            }
            if (bConsumed) { continue; }
            const auto Value = FindExactField(Object, *Key);
            const auto Is = [&Key](const TCHAR* Expected)
            {
                return Key.Equals(Expected, ESearchCase::CaseSensitive);
            };
            if (Is(TEXT("name")) || Is(TEXT("source")) || Is(TEXT("type")) ||
                Is(TEXT("mastery")) || Is(TEXT("entries")) || Is(TEXT("weight")) ||
                (Is(TEXT("page")) && !IsBaseItemPageZero(Value)) ||
                (Is(TEXT("property")) && !bRetainRawProperty) ||
                (Is(TEXT("rarity")) && !bRetainRawRarity) ||
                (Is(TEXT("value")) && !bRetainRawPrice))
            {
                continue;
            }

            if (Is(TEXT("srd")) || Is(TEXT("srd52")) || Is(TEXT("basicRules")) || Is(TEXT("basicRules2024")))
            {
                bool bIncluded = false;
                if (Value->Type == EJson::Boolean && Value->TryGetBool(bIncluded))
                {
                    FDnd5eSourceMembership Membership;
                    Membership.MembershipId = Key;
                    Membership.Inclusion = bIncluded ? EDnd5eSourceInclusion::Included : EDnd5eSourceInclusion::Excluded;
                    Origin.Memberships.Add(MoveTemp(Membership));
                    continue;
                }
            }

            Extra->SetField(Key, Value);
            if (!Is(TEXT("page")) && !Is(TEXT("property")) && !Is(TEXT("rarity")) &&
                !Is(TEXT("value")) && !Is(TEXT("currencyConversion")))
            {
                AddDiagnostic(State, EDnd5eDiagnosticSeverity::Warning, Codes::PreservedUnsupportedMetadata,
                    AppendPointer(Pointer, Key), TEXT("Unmodeled base item field retained."));
            }
        }

        return Extra->Values.IsEmpty() ||
            SerializeValue(MakeShared<FJsonValueObject>(Extra), Origin.AdditionalMetadataJson, Pointer, State);
    }
	bool ReadItemAttunement(const FJsonObject& Object, const FString& Pointer,
	FDnd5eItem& Item, TSet<FString>& Consumed, FReadState& State)
	{
		const auto ReadOne = [&](const TCHAR* Field, FDnd5eAttunementRequirement& Out)
		{
			const auto Value = FindExactField(Object, Field);
			if (!Value.IsValid()) { return true; }

			const FString FieldPointer = AppendPointer(Pointer, Field);
			bool bRequired = false;
			FString Text;
			if (Value->Type == EJson::Boolean && Value->TryGetBool(bRequired))
			{
				Out.Mode = bRequired ? EDnd5eAttunementMode::Required : EDnd5eAttunementMode::NotRequired;
			}
			else if (Value->Type == EJson::String && Value->TryGetString(Text) && !Text.TrimStartAndEnd().IsEmpty())
			{
				if (Text.Equals(TEXT("optional"), ESearchCase::CaseSensitive))
				{
					Out.Mode = EDnd5eAttunementMode::Optional;
				}
				else
				{
					Out.Mode = EDnd5eAttunementMode::Required;
					if (!ReadInlines(Text, FieldPointer, Out.Conditions, State)) { return false; }
				}
			}
			else
			{
				return Fail(State, Codes::InvalidFieldValue, FieldPointer,
					TEXT("Expected a Boolean or nonempty attunement string."));
			}

			Consumed.Add(Field);
			return true;
		};

		if (!ReadOne(TEXT("reqAttune"), Item.Attunement) ||
			!ReadOne(TEXT("reqAttuneAlt"), Item.AlternativeAttunement))
		{
			return false;
		}
		Item.bHasAlternativeAttunement = Consumed.Contains(TEXT("reqAttuneAlt"));
		return true;
	}

    bool ReadItemRow(const FJsonObject& Object, const FString& Pointer, FDnd5eItem& Item, bool bBaseItem, FReadState& State)
    {
        if (FindExactField(Object, TEXT("_copy")).IsValid())
        {
            return Fail(State, Codes::UnsupportedRequiredValue, AppendPointer(Pointer, TEXT("_copy")),
                TEXT("Base item copy assembly is outside this reader batch."));
        }
		if (!bBaseItem && FindExactField(Object, TEXT("baseItem")).IsValid())
		{
			return Fail(State, Codes::UnsupportedRequiredValue,
				AppendPointer(Pointer, TEXT("baseItem")), TEXT("This item requires base-item assembly."));
		}
        if (!ReadBaseItemHeader(Object, Pointer, Item, State)) { return false; }

        auto& Origin = Item.Header.Provenance[0];
        TGuardValue<FString> SourceGuard(State.SourceId, Origin.SourceId);
        if (const auto Entries = FindExactField(Object, TEXT("entries")))
        {
            if (!ReadBlocks(Entries, AppendPointer(Pointer, TEXT("entries")), Item.Description, State))
            {
                return false;
            }
        }

        bool bRetainRawProperty = false;
        bool bRetainRawRarity = false;
        bool bRetainRawPrice = false;
		TSet<FString> ConsumedEquipment;
		return ReadBaseItemSelectors(Object, Pointer, Item, bRetainRawProperty, bBaseItem, State) &&
			ReadBaseItemValues(Object, Pointer, Item, bRetainRawRarity, bRetainRawPrice, State) &&
			ReadBaseItemWeapon(Object, Pointer, Item, ConsumedEquipment, bBaseItem, State) &&
			ReadBaseItemArmor(Object, Pointer, Item, ConsumedEquipment, State) &&
			(bBaseItem || ReadItemAttunement(Object, Pointer, Item, ConsumedEquipment, State)) &&
			ReadBaseItemMetadata(Object, Pointer, bRetainRawProperty, bRetainRawRarity,
			bRetainRawPrice, ConsumedEquipment, Origin, State);
    }
}
bool Dnd5eJsonReader::ReadStandaloneItemRow(const FJsonObject& Object, const FString& Pointer,
	FDnd5eItem& Item, FReadState& State)
{
	return ReadItemRow(Object, Pointer, Item, false, State);
}

FDnd5eJsonBaseItemReadResult FDnd5eJsonBaseItemReader::Read(const FDnd5eJsonDocument& Document)
{
	using namespace Dnd5eJsonReader;
	FDnd5eJsonBaseItemReadResult Result;
	Result.Status = EDnd5eJsonReadStatus::Failure;
	FReadState State{Document.Context, Document.JsonText, Result.Diagnostics};

	TSharedPtr<FJsonValue> RootValue;
	TSharedPtr<FJsonObject> Root;
	if (!ValidateReadContext(State) || !ValidateDocumentText(State) ||
		!PreflightJson(State) || !ParseJsonDocument(RootValue, State) ||
		!RequireObject(RootValue, TEXT(""), Root, State))
	{
		return Result;
	}

	const TArray<TSharedPtr<FJsonValue>>* Rows = nullptr;
	if (!ReadRequiredArray(*Root, TEXT("baseitem"), TEXT(""), Rows, State)) { return Result; }
	if (Rows->Num() > 100000)
	{
		Fail(State, Codes::ResourceLimitExceeded, TEXT("/baseitem"), TEXT("Too many base item rows."));
		return Result;
	}

	TArray<FDnd5eItem> Staged;
	TSet<FDnd5eRecordId> Seen;
	Staged.Reserve(Rows->Num());
	for (int32 Index = 0; Index < Rows->Num(); ++Index)
	{
		const FString Pointer = AppendPointer(TEXT("/baseitem"), FString::FromInt(Index));
		TSharedPtr<FJsonObject> Object;
		FDnd5eItem Item;
		if (!RequireObject((*Rows)[Index], Pointer, Object, State) ||
			!ReadItemRow(*Object, Pointer, Item, true, State))
		{
			return Result;
		}
		if (Seen.Contains(Item.Header.RecordId))
		{
			Fail(State, Codes::InvalidFieldValue, Pointer, TEXT("Duplicate canonical base item ID."));
			return Result;
		}
		Seen.Add(Item.Header.RecordId);
		Staged.Add(MoveTemp(Item));
	}

	Result.Items = MoveTemp(Staged);
	Result.Status = EDnd5eJsonReadStatus::Success;
	return Result;
}
