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
		FDnd5eItem& Item, bool& bRetainRawProperty, FReadState& State)
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
        if (Rarity.Equals(TEXT("none"), ESearchCase::CaseSensitive))
        {
            Item.Rarity = Dnd5e::Item::Rarity::None;
        }
        else if (Rarity.Equals(TEXT("unknown"), ESearchCase::CaseSensitive))
        {
            Item.Rarity = Dnd5e::Item::Rarity::Unknown;
        }
        else
        {
            bRetainRawRarity = true;
            AddDiagnostic(State, EDnd5eDiagnosticSeverity::Warning, Codes::PreservedUnsupportedMetadata,
                AppendPointer(Pointer, TEXT("rarity")), TEXT("Unmapped base item rarity retained."));
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
	bool ReadBaseItemMetadata(const FJsonObject& Object, const FString& Pointer,
        bool bRetainRawProperty, bool bRetainRawRarity, bool bRetainRawPrice,
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

    bool ReadBaseItemRow(const FJsonObject& Object, const FString& Pointer, FDnd5eItem& Item, FReadState& State)
    {
        if (FindExactField(Object, TEXT("_copy")).IsValid())
        {
            return Fail(State, Codes::UnsupportedRequiredValue, AppendPointer(Pointer, TEXT("_copy")),
                TEXT("Base item copy assembly is outside this reader batch."));
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
        return ReadBaseItemSelectors(Object, Pointer, Item, bRetainRawProperty, State) &&
            ReadBaseItemValues(Object, Pointer, Item, bRetainRawRarity, bRetainRawPrice, State) &&
            ReadBaseItemMetadata(Object, Pointer, bRetainRawProperty, bRetainRawRarity,
                bRetainRawPrice, Origin, State);
    }
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
			!ReadBaseItemRow(*Object, Pointer, Item, State))
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