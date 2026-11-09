// Developed by Neko Creative Technologies

#include "Readers/Items/Dnd5eJsonItemDefinitionReader.h"
#include "Readers/Dnd5eJsonReaderInternal.h"
#include "Items/Dnd5eItemDefinitionIdentity.h"
#include "Dnd5eTags.h"

namespace
{
	template<typename TVisit>
	bool VisitDefinitionRows(const TArray<TSharedPtr<FJsonValue>>* Rows, const TCHAR* Field, const TVisit& Visit, Dnd5eJsonReader::FReadState& State)
	{
		using namespace Dnd5eJsonReader;
		if (!Rows) { return true; }
		const FString ArrayPointer = AppendPointer(TEXT(""), Field);
		for (int32 Index = 0; Index < Rows->Num(); ++Index)
		{
			const FString Pointer = AppendPointer(ArrayPointer, FString::FromInt(Index));
			TSharedPtr<FJsonObject> Object;
			if (!RequireObject((*Rows)[Index], Pointer, Object, State) || !Visit(Object, Pointer))
			{
				return false;
			}
		}
		return true;
	}

	struct FTypeDefinitionInput
	{
		TSharedPtr<FJsonObject> Object;
		FString Pointer;
		FDnd5eItemTypeDefinition Value;
	};

	bool ReadDefinitionArray(const FJsonObject& Root, const TCHAR* Field,
		const TArray<TSharedPtr<FJsonValue>>*& Out, Dnd5eJsonReader::FReadState& State)
	{
		using namespace Dnd5eJsonReader;
		Out = nullptr;
		const auto Value = FindExactField(Root, Field);
		return !Value.IsValid() || RequireArray(Value, AppendPointer(TEXT(""), Field), Out, State);
	}

	bool IsDefinitionPageZero(const TSharedPtr<FJsonValue>& Value)
	{
		FString Literal;
		return Value.IsValid() && Value->Type == EJson::Number &&
			Value->TryGetString(Literal) && Literal == TEXT("0");
	}

	bool ReadDefinitionHeader(const FJsonObject& Object, const FString& Pointer,
	FGameplayTag Category, FDnd5eRecordHeader& Header, FString& Abbreviation,
	Dnd5eJsonReader::FReadState& State)
	{
		using namespace Dnd5eJsonReader;
		FDnd5eRecordProvenance Origin;
		if (!ReadRequiredString(Object, TEXT("source"), Pointer, Origin.SourceId, State)) { return false; }

		const bool bMastery = Category == Dnd5e::Category::ItemMastery;
		const bool bNeedsName = Category != Dnd5e::Category::ItemProperty;
		if (bNeedsName || FindExactField(Object, TEXT("name")).IsValid())
		{
			if (!ReadRequiredString(Object, TEXT("name"), Pointer, Header.DisplayName, State) ||
				!ValidateIdentityText(Header.DisplayName, AppendPointer(Pointer, TEXT("name")), State))
			{
				return false;
			}
		}
		if (!bMastery && !ReadRequiredString(Object, TEXT("abbreviation"), Pointer, Abbreviation, State))
		{
			return false;
		}

		const FString& Key = bMastery ? Header.DisplayName : Abbreviation;
		const TCHAR* KeyField = bMastery ? TEXT("name") : TEXT("abbreviation");
		if (!ValidateIdentityText(Key, AppendPointer(Pointer, KeyField), State) ||
			!ValidateIdentityText(Origin.SourceId, AppendPointer(Pointer, TEXT("source")), State))
		{
			return false;
		}
		if (!Dnd5e::ItemDefinitions::TryMakeId(State.Context.DatasetNamespace, Category,
			Key, Origin.SourceId, Header.RecordId))
		{
			return Fail(State, Codes::InvalidFieldValue, Pointer, TEXT("Definition identity is invalid."));
		}

		Origin.RelativeFilePath = State.Context.RelativeFilePath;
		Origin.JsonPointer = Pointer;
		if (const auto Page = FindExactField(Object, TEXT("page")))
		{
			if (!IsDefinitionPageZero(Page) && !ReadPage(Page, AppendPointer(Pointer, TEXT("page")), Origin.Page, State)) { return false; }
		}
		Header.Provenance.Add(MoveTemp(Origin));
		return true;
	}

	bool ReadDefinitionEntries(const FJsonObject& Object, const FString& Pointer,
	TArray<FDnd5eContentBlock>& Out, Dnd5eJsonReader::FReadState& State)
	{
		using namespace Dnd5eJsonReader;
		const auto Entries = FindExactField(Object, TEXT("entries"));
		return !Entries.IsValid() || ReadBlocks(Entries, AppendPointer(Pointer, TEXT("entries")), Out, State);
	}

	bool ReadDefinitionRelationships(const FJsonObject& Object, const FString& Pointer,
	TArray<FDnd5eRecordRelationship>& Out, Dnd5eJsonReader::FReadState& State)
	{
		using namespace Dnd5eJsonReader;
		const auto Reprints = FindExactField(Object, TEXT("reprintedAs"));
		if (!Reprints.IsValid()) { return true; }

		const FString ArrayPointer = AppendPointer(Pointer, TEXT("reprintedAs"));
		const TArray<TSharedPtr<FJsonValue>>* Rows = nullptr;
		if (!RequireArray(Reprints, ArrayPointer, Rows, State)) { return false; }

		for (int32 Index = 0; Index < Rows->Num(); ++Index)
		{
			const FString EntryPointer = AppendPointer(ArrayPointer, FString::FromInt(Index));
			const auto& Value = (*Rows)[Index];
			FDnd5eRecordRelationship Link;
			if (!Value.IsValid() || Value->Type != EJson::String || !Value->TryGetString(Link.SourceReference))
			{
				return Fail(State, Codes::InvalidFieldValue, EntryPointer, TEXT("Expected a reprint reference string."));
			}
			if (!ValidateIdentityText(Link.SourceReference, EntryPointer, State)) { return false; }
			Link.Kind = Dnd5e::Record::Relationship::ReprintedAs;
			Link.SourceFormat = TEXT("5etools");
			Out.Add(MoveTemp(Link));
			AddDiagnostic(State, EDnd5eDiagnosticSeverity::Warning, Codes::UnresolvedReference,
				EntryPointer, TEXT("Definition reprint reference retained for later resolution."));
		}
		return true;
	}

	bool ReadDefinitionMetadata(const FJsonObject& Object, const FString& Pointer,
	    FDnd5eRecordProvenance& Origin, FGameplayTag Category, Dnd5eJsonReader::FReadState& State)
	{
	    using namespace Dnd5eJsonReader;
	    for (const TCHAR* Field : {TEXT("srd52"), TEXT("basicRules2024")})
	    {
	        const auto Value = FindExactField(Object, Field);
	        if (!Value.IsValid()) { continue; }

	        bool bIncluded = false;
	        if (Value->Type != EJson::Boolean || !Value->TryGetBool(bIncluded))
	        {
	            return Fail(State, Codes::InvalidFieldValue, AppendPointer(Pointer, Field),
	                TEXT("Expected a Boolean membership flag."));
	        }
	        FDnd5eSourceMembership Membership;
	        Membership.MembershipId = Field;
	        Membership.Inclusion = bIncluded ? EDnd5eSourceInclusion::Included : EDnd5eSourceInclusion::Excluded;
	        Origin.Memberships.Add(MoveTemp(Membership));
	    }

	    auto Extra = MakeShared<FJsonObject>();
	    TArray<FString> Keys;
	    for (const auto& Pair : Object.Values) { Keys.Emplace(Pair.Key.Len(), *Pair.Key); }
	    Keys.Sort([](const FString& A, const FString& B) { return A.Compare(B, ESearchCase::CaseSensitive) < 0; });
	    for (const FString& Key : Keys)
	    {
	        const auto Is = [&Key](const TCHAR* Expected) { return Key.Equals(Expected, ESearchCase::CaseSensitive); };
	        if (Is(TEXT("name")) || Is(TEXT("source")) || (Is(TEXT("page")) && !IsDefinitionPageZero(FindExactField(Object, TEXT("page")))) || Is(TEXT("entries")) ||
	            Is(TEXT("reprintedAs")) || Is(TEXT("srd52")) || Is(TEXT("basicRules2024")) ||
	            (Category != Dnd5e::Category::ItemMastery && Is(TEXT("abbreviation"))) ||
	            (Category == Dnd5e::Category::ItemProperty && Is(TEXT("template"))))
	        {
	            continue;
	        }

	        Extra->SetField(Key, FindExactField(Object, *Key));
	        if (!Is(TEXT("page")) && !(Category == Dnd5e::Category::ItemType && Is(TEXT("_copy"))))
	        {
	            AddDiagnostic(State, EDnd5eDiagnosticSeverity::Warning, Codes::PreservedUnsupportedMetadata,
	                AppendPointer(Pointer, Key), TEXT("Unmodeled definition field retained."));
	        }
	    }
	    return Extra->Values.IsEmpty() ||
	        SerializeValue(MakeShared<FJsonValueObject>(Extra), Origin.AdditionalMetadataJson, Pointer, State);
	}

	bool ReadPropertyTemplate(const FJsonObject& Object, const FString& Pointer,
	FDnd5eItemPropertyDefinition& Property, Dnd5eJsonReader::FReadState& State)
	{
		using namespace Dnd5eJsonReader;
		if (!FindExactField(Object, TEXT("template")).IsValid()) { return true; }

		if (!ReadRequiredString(Object, TEXT("template"), Pointer, Property.TemplateText, State))
		{
			return false;
		}
		Property.bHasTemplate = true;
		Property.TemplateSyntax = TEXT("5etools-item-property");
		return true;
	}

	bool ReadTypeDescription(FTypeDefinitionInput& Row, const TArray<FTypeDefinitionInput>& Rows,
	    const TMap<FDnd5eRecordId, int32>& Index, Dnd5eJsonReader::FReadState& State)
	{
	    using namespace Dnd5eJsonReader;
	    const FTypeDefinitionInput* Owner = &Row;
	    const auto CopyValue = FindExactField(*Row.Object, TEXT("_copy"));
	    if (CopyValue.IsValid())
	    {
	        const FString CopyPointer = AppendPointer(Row.Pointer, TEXT("_copy"));
	        TSharedPtr<FJsonObject> Copy;
	        FString Key, Publication;
	        if (!RequireObject(CopyValue, CopyPointer, Copy, State) ||
	            !CheckAllowedKeys(*Copy, {TEXT("abbreviation"), TEXT("source")}, CopyPointer, State) ||
	            !ReadRequiredString(*Copy, TEXT("abbreviation"), CopyPointer, Key, State) ||
	            !ReadRequiredString(*Copy, TEXT("source"), CopyPointer, Publication, State))
	        {
	            return false;
	        }

	        FDnd5eRecordId Target;
	        if (!Dnd5e::ItemDefinitions::TryMakeId(State.Context.DatasetNamespace,
	            Dnd5e::Category::ItemType, Key, Publication, Target))
	        {
	            return Fail(State, Codes::InvalidFieldValue, CopyPointer, TEXT("Invalid copy target identity."));
	        }
	        const int32* TargetIndex = Index.Find(Target);
	        if (!TargetIndex)
	        {
	            return Fail(State, Codes::UnresolvedReference, CopyPointer, TEXT("Copy target is absent from the type array."));
	        }
	        const auto& TargetRow = Rows[*TargetIndex];
	        if (FindExactField(*TargetRow.Object, TEXT("_copy")).IsValid())
	        {
	            return Fail(State, Codes::UnsupportedRequiredValue, CopyPointer, TEXT("Chained type copies are unsupported."));
	        }
	        if (!FindExactField(*Row.Object, TEXT("entries")).IsValid())
	        {
	            Owner = &TargetRow;
	        }
	    }

	    const auto Entries = FindExactField(*Owner->Object, TEXT("entries"));
	    if (Entries.IsValid())
	    {
	        const auto& Origin = Owner->Value.Header.Provenance[0];
	        TGuardValue<FString> SourceGuard(State.SourceId, Origin.SourceId);
	        if (!ReadBlocks(Entries, AppendPointer(Owner->Pointer, TEXT("entries")), Row.Value.Description, State))
	        {
	            return false;
	        }
	        if (Owner != &Row)
	        {
	            Row.Value.Header.Provenance.Add(Origin);
	        }
	    }
	    return true;
	}

	template<typename TDefinition>
	bool ReadDirectDefinition(const FJsonObject& Object, const FString& Pointer, FGameplayTag Category,
		FString& Abbreviation, TDefinition& Out, Dnd5eJsonReader::FReadState& State)
	{
		using namespace Dnd5eJsonReader;
		if (FindExactField(Object, TEXT("_copy")).IsValid())
		{
			return Fail(State, Codes::UnsupportedRequiredValue, AppendPointer(Pointer, TEXT("_copy")),
				TEXT("Only type definitions support copies in this reader."));
		}
		if (!ReadDefinitionHeader(Object, Pointer, Category, Out.Header, Abbreviation, State))
		{
			return false;
		}

		auto& Origin = Out.Header.Provenance[0];
		TGuardValue<FString> SourceGuard(State.SourceId, Origin.SourceId);
		return ReadDefinitionEntries(Object, Pointer, Out.Description, State) &&
			ReadDefinitionRelationships(Object, Pointer, Out.Relationships, State) &&
			ReadDefinitionMetadata(Object, Pointer, Origin, Category, State);
	}

	bool AddUniqueDefinitionId(const FDnd5eRecordId& Id, const FString& Pointer,
		TSet<FDnd5eRecordId>& Seen, Dnd5eJsonReader::FReadState& State)
	{
		using namespace Dnd5eJsonReader;
		if (Seen.Contains(Id))
		{
			return Fail(State, Codes::InvalidFieldValue, Pointer, TEXT("Duplicate canonical definition ID."));
		}
		Seen.Add(Id);
		return true;
	}
}

FDnd5eJsonItemDefinitionReadResult FDnd5eJsonItemDefinitionReader::Read(const FDnd5eJsonDocument& Document)
{
	using namespace Dnd5eJsonReader;
	FDnd5eJsonItemDefinitionReadResult Result;
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

	const TArray<TSharedPtr<FJsonValue>>* TypeRows = nullptr;
	const TArray<TSharedPtr<FJsonValue>>* PropertyRows = nullptr;
	const TArray<TSharedPtr<FJsonValue>>* MasteryRows = nullptr;
	if (!ReadDefinitionArray(*Root, TEXT("itemType"), TypeRows, State) ||
	    !ReadDefinitionArray(*Root, TEXT("itemProperty"), PropertyRows, State) ||
	    !ReadDefinitionArray(*Root, TEXT("itemMastery"), MasteryRows, State))
	{
	    return Result;
	}
	if (!TypeRows && !PropertyRows && !MasteryRows)
	{
	    Fail(State, Codes::MissingRequiredField, TEXT(""), TEXT("Supply at least one definition array."));
	    return Result;
	}

	const auto CountRows = [](const auto* Rows) -> int64 { return Rows ? Rows->Num() : 0; };
	if (CountRows(TypeRows) + CountRows(PropertyRows) + CountRows(MasteryRows) > 100000)
	{
	    Fail(State, Codes::ResourceLimitExceeded, TEXT(""), TEXT("Too many definition rows."));
	    return Result;
	}

	TArray<FTypeDefinitionInput> StagedTypes;
	TMap<FDnd5eRecordId, int32> TypeIndex;
	if (!VisitDefinitionRows(TypeRows, TEXT("itemType"), [&](const TSharedPtr<FJsonObject>& Object, const FString& Pointer)
	{
	    FTypeDefinitionInput Row;
	    Row.Object = Object;
	    Row.Pointer = Pointer;
	    auto& Type = Row.Value;
	    if (!ReadDefinitionHeader(*Object, Pointer, Dnd5e::Category::ItemType, Type.Header, Type.Abbreviation, State) ||
	        !ReadDefinitionRelationships(*Object, Pointer, Type.Relationships, State) ||
	        !ReadDefinitionMetadata(*Object, Pointer, Type.Header.Provenance[0], Dnd5e::Category::ItemType, State))
	    {
	        return false;
	    }
	    if (TypeIndex.Contains(Type.Header.RecordId))
	    {
	        return Fail(State, Codes::InvalidFieldValue, Pointer, TEXT("Duplicate canonical type ID."));
	    }
	    TypeIndex.Add(Type.Header.RecordId, StagedTypes.Num());
	    StagedTypes.Add(MoveTemp(Row));
	    return true;
	}, State))
	{
	    return Result;
	}

	for (auto& Row : StagedTypes)
	{
	    if (!ReadTypeDescription(Row, StagedTypes, TypeIndex, State)) { return Result; }
	}

	TArray<FDnd5eItemPropertyDefinition> Properties;
	TArray<FDnd5eItemMasteryDefinition> Masteries;
	TSet<FDnd5eRecordId> Seen;
	if (!VisitDefinitionRows(PropertyRows, TEXT("itemProperty"), [&](const TSharedPtr<FJsonObject>& Object, const FString& Pointer)
	{
	    FDnd5eItemPropertyDefinition Property;
	    if (!ReadDirectDefinition(*Object, Pointer, Dnd5e::Category::ItemProperty, Property.Abbreviation, Property, State) ||
	        !ReadPropertyTemplate(*Object, Pointer, Property, State) ||
	        !AddUniqueDefinitionId(Property.Header.RecordId, Pointer, Seen, State))
	    {
	        return false;
	    }
	    Properties.Add(MoveTemp(Property));
	    return true;
	}, State))
	{
	    return Result;
	}

	if (!VisitDefinitionRows(MasteryRows, TEXT("itemMastery"), [&](const TSharedPtr<FJsonObject>& Object, const FString& Pointer)
	{
	    FDnd5eItemMasteryDefinition Mastery;
	    FString UnusedAbbreviation;
	    if (!ReadDirectDefinition(*Object, Pointer, Dnd5e::Category::ItemMastery, UnusedAbbreviation, Mastery, State) ||
	        !AddUniqueDefinitionId(Mastery.Header.RecordId, Pointer, Seen, State))
	    {
	        return false;
	    }
	    Masteries.Add(MoveTemp(Mastery));
	    return true;
	}, State))
	{
	    return Result;
	}

	Result.Types.Reserve(StagedTypes.Num());
	for (auto& Row : StagedTypes) { Result.Types.Add(MoveTemp(Row.Value)); }
	Result.Properties = MoveTemp(Properties);
	Result.Masteries = MoveTemp(Masteries);
	Result.Status = EDnd5eJsonReadStatus::Success;
	return Result;
}
