// Developed by Neko Creative Technologies

#include "Readers/Items/Dnd5eJsonItemVariantGenerator.h"
#include "Readers/Items/Dnd5eJsonItemVariantMatcherInternal.h"
#include "Readers/Dnd5eJsonReaderInternal.h"
#include "Items/Dnd5eItemIdentity.h"

namespace
{
	using namespace Dnd5eJsonReader;

	bool IsOneOf(const FString& Key, std::initializer_list<const TCHAR*> Keys)
	{
		for (const TCHAR* Candidate : Keys)
		{
			if (Key.Equals(Candidate, ESearchCase::CaseSensitive)) { return true; }
		}
		return false;
	}

	bool ReadNamePart(const FJsonObject& Inherits, const TCHAR* Key, const FString& Pointer,
		FString& Out, FReadState& State)
	{
		const auto Value = FindExactField(Inherits, Key);
		if (!Value.IsValid()) { return true; }
		if (Value->Type == EJson::String && Value->TryGetString(Out)) { return true; }
		return Fail(State, Codes::InvalidFieldValue, AppendPointer(Pointer, Key), TEXT("Expected a name operation string."));
	}

	bool ReadVariantArrayValues(const FJsonObject& Object, const TCHAR* Key, const FString& Pointer,
		TArray<TSharedPtr<FJsonValue>>& Out, FReadState& State)
	{
		const auto Value = FindExactField(Object, Key);
		if (!Value.IsValid() || Value->Type == EJson::Null) { return true; }
		const TArray<TSharedPtr<FJsonValue>>* Values = nullptr;
		if (!RequireArray(Value, AppendPointer(Pointer, Key), Values, State)) { return false; }
		Out = *Values;
		return true;
	}

	bool CheckLiteralEntries(const FJsonValue& Value, const FString& Pointer, FReadState& State)
	{
		if (Value.Type == EJson::String && Value.AsString().Contains(TEXT("{="), ESearchCase::CaseSensitive))
		{
			return Fail(State, Codes::UnsupportedRequiredValue, Pointer, TEXT("Variant entry injection needs a later generation pass."));
		}
		if (Value.Type == EJson::Array)
		{
			const auto& Entries = Value.AsArray();
			for (int32 Index = 0; Index < Entries.Num(); ++Index)
			{
				if (!CheckLiteralEntries(*Entries[Index], AppendPointer(Pointer, FString::FromInt(Index)), State)) { return false; }
			}
		}
		if (Value.Type == EJson::Object)
		{
			for (const auto& Pair : Value.AsObject()->Values)
			{
				const FString Key(Pair.Key.Len(), *Pair.Key);
				if (!CheckLiteralEntries(*Pair.Value, AppendPointer(Pointer, Key), State)) { return false; }
			}
		}
		return true;
	}

	bool TryReadPropertyUid(const FJsonValue& Value, FString& Out)
	{
		if (Value.Type == EJson::String) { return Value.TryGetString(Out) && !Out.IsEmpty(); }
		if (Value.Type != EJson::Object) { return false; }
		const auto Uid = FindExactField(*Value.AsObject(), TEXT("uid"));
		return Uid.IsValid() && Uid->Type == EJson::String && Uid->TryGetString(Out) && !Out.IsEmpty();
	}

	bool AddVariantProperties(const FJsonObject& Base, const FJsonObject& Inherits, const FString& BasePointer,
		const FString& InheritsPointer, FJsonObject& Row, FReadState& BaseState, FReadState& VariantState)
	{
		if (!FindExactField(Inherits, TEXT("propertyAdd")).IsValid()) { return true; }
		const FString AddPointer = AppendPointer(InheritsPointer, TEXT("propertyAdd"));
		if (FindExactField(Inherits, TEXT("property")).IsValid())
		{
			return Fail(VariantState, Codes::UnsupportedRequiredValue, AddPointer,
				TEXT("Combining a property override with propertyAdd is outside this preview."));
		}
		TArray<TSharedPtr<FJsonValue>> Original;
		const TArray<TSharedPtr<FJsonValue>>* Additions = nullptr;
		if (!ReadVariantArrayValues(Base, TEXT("property"), BasePointer, Original, BaseState) ||
			!ReadRequiredArray(Inherits, TEXT("propertyAdd"), InheritsPointer, Additions, VariantState)) { return false; }
		if (Original.Num() > 4096 || Additions->Num() > 4096)
		{
			return Fail(VariantState, Codes::ResourceLimitExceeded, AddPointer, TEXT("Too many variant property values."));
		}

		TArray<FString> OriginalUids;
		const FString PropertyPointer = AppendPointer(BasePointer, TEXT("property"));
		for (int32 Index = 0; Index < Original.Num(); ++Index)
		{
			FString Uid;
			if (!TryReadPropertyUid(*Original[Index], Uid))
			{
				return Fail(BaseState, Codes::InvalidFieldValue, AppendPointer(PropertyPointer, FString::FromInt(Index)),
					TEXT("Expected a property UID."));
			}
			OriginalUids.Add(MoveTemp(Uid));
		}
		for (int32 Index = 0; Index < Additions->Num(); ++Index)
		{
			FString Uid;
			if (!TryReadPropertyUid(*(*Additions)[Index], Uid))
			{
				return Fail(VariantState, Codes::InvalidFieldValue, AppendPointer(AddPointer, FString::FromInt(Index)),
					TEXT("Expected an added property UID."));
			}
			if (!OriginalUids.ContainsByPredicate([&Uid](const FString& Existing)
			{
				return Existing.Equals(Uid, ESearchCase::CaseSensitive);
			}))
			{
				Original.Add((*Additions)[Index]);
			}
		}
		Row.SetArrayField(TEXT("property"), Original);
		return true;
	}

	bool AppendLinkedLoot(const FJsonObject& VariantRoot, const FString& Source, const FString& Name,
		const FString& LootPointer, FJsonObject& Row, FReadState& LootState, FReadState& State)
	{
		const auto Lookup = FindExactField(VariantRoot, TEXT("linkedLootTables"));
		if (!Lookup.IsValid() || Lookup->Type == EJson::Null) { return true; }
		TSharedPtr<FJsonObject> Sources;
		const FString LookupPointer = TEXT("/linkedLootTables");
		if (!RequireObject(Lookup, LookupPointer, Sources, State)) { return false; }
		const auto Bucket = FindExactField(*Sources, *Source);
		if (!Bucket.IsValid()) { return true; }

		const FString SourcePointer = AppendPointer(LookupPointer, Source);
		TSharedPtr<FJsonObject> Names;
		if (!RequireObject(Bucket, SourcePointer, Names, State)) { return false; }
		const auto Entry = FindExactField(*Names, *Name);
		if (!Entry.IsValid()) { return true; }
		const TArray<TSharedPtr<FJsonValue>>* Linked = nullptr;
		if (!RequireArray(Entry, AppendPointer(SourcePointer, Name), Linked, State)) { return false; }
		TArray<TSharedPtr<FJsonValue>> Loot;
		if (!ReadVariantArrayValues(Row, TEXT("lootTables"), LootPointer, Loot, LootState)) { return false; }
		Loot.Append(*Linked);
		Row.SetArrayField(TEXT("lootTables"), Loot);
		return true;
	}

	bool BuildSpecificVariantRow(const FJsonObject& Base, const FJsonObject& Variant, const FJsonObject& Inherits,
		const FJsonObject& VariantRoot, const FString& BasePointer, const FString& VariantPointer,
		const FString& BaseName, const FString& BaseSource, const FString& TemplateName, const FString& TemplateSource,
		FJsonObject& Row, FReadState& BaseState, FReadState& VariantState)
	{
		const FString InheritsPointer = AppendPointer(VariantPointer, TEXT("inherits"));
		FString Remove, Prefix, Suffix;
		if (!ReadNamePart(Inherits, TEXT("nameRemove"), InheritsPointer, Remove, VariantState) ||
			!ReadNamePart(Inherits, TEXT("namePrefix"), InheritsPointer, Prefix, VariantState) ||
			!ReadNamePart(Inherits, TEXT("nameSuffix"), InheritsPointer, Suffix, VariantState)) { return false; }

		FString Name = BaseName;
		if (!Remove.IsEmpty()) { Name.ReplaceInline(*Remove, TEXT(""), ESearchCase::CaseSensitive); }
		Name = Prefix + Name + Suffix;
		if (!ValidateIdentityText(Name, InheritsPointer, VariantState)) { return false; }

		Row.Values = Base.Values;
		for (const TCHAR* Key : {TEXT("value"), TEXT("srd"), TEXT("srd52"), TEXT("basicRules"), TEXT("basicRules2024"),
			TEXT("page"), TEXT("reprintedAs"), TEXT("referenceSources"), TEXT("hasFluff"), TEXT("hasFluffImages"),
			TEXT("_isBaseItem"), TEXT("_isEnhanced"), TEXT("_fullEntries"), TEXT("__prop"), TEXT("_category")})
		{
			if (FindExactField(Row, Key).IsValid()) { Row.RemoveField(Key); }
		}

		TArray<FString> Keys;
		for (const auto& Pair : Inherits.Values) { Keys.Emplace(Pair.Key.Len(), *Pair.Key); }
		Keys.Sort([](const FString& A, const FString& B) { return A.Compare(B, ESearchCase::CaseSensitive) < 0; });
		for (const FString& Key : Keys)
		{
			if (IsOneOf(Key, {TEXT("nameRemove"), TEXT("namePrefix"), TEXT("nameSuffix"), TEXT("entries"), TEXT("propertyAdd")})) { continue; }
			const FString Pointer = AppendPointer(InheritsPointer, Key);
			if (IsOneOf(Key, {TEXT("name"), TEXT("baseItem"), TEXT("genericVariant"), TEXT("_copy"), TEXT("requires"),
				TEXT("excludes"), TEXT("inherits"), TEXT("weightExpression"), TEXT("valueExpression"), TEXT("barding"),
				TEXT("propertyRemove"), TEXT("conditionImmune"), TEXT("vulnerable"), TEXT("resist"), TEXT("immune")}))
			{
				return Fail(VariantState, Codes::UnsupportedRequiredValue, Pointer, TEXT("Unsupported item variant inheritance directive."));
			}
			if (Row.HasField(Key) && !FindExactField(Row, *Key).IsValid())
			{
				return Fail(VariantState, Codes::InvalidFieldValue, Pointer, TEXT("Variant overlay has a differently cased field."));
			}
			Row.SetField(Key, FindExactField(Inherits, *Key));
		}

		if (const auto Entries = FindExactField(Inherits, TEXT("entries")); Entries.IsValid() && Entries->Type != EJson::Null)
		{
			const FString EntriesPointer = AppendPointer(InheritsPointer, TEXT("entries"));
			const TArray<TSharedPtr<FJsonValue>>* Inserted = nullptr;
			TArray<TSharedPtr<FJsonValue>> BaseEntries;
			if (!RequireArray(Entries, EntriesPointer, Inserted, VariantState) ||
				!CheckLiteralEntries(*Entries, EntriesPointer, VariantState) ||
				!ReadVariantArrayValues(Base, TEXT("entries"), BasePointer, BaseEntries, BaseState)) { return false; }
			TArray<TSharedPtr<FJsonValue>> Combined = *Inserted;
			Combined.Append(BaseEntries);
			Row.SetArrayField(TEXT("entries"), Combined);
		}
		if (!AddVariantProperties(Base, Inherits, BasePointer, InheritsPointer, Row, BaseState, VariantState)) { return false; }

		for (const TCHAR* Key : {TEXT("hasFluff"), TEXT("hasFluffImages")})
		{
			const auto Value = FindExactField(Variant, Key);
			if (!Value.IsValid()) { continue; }
			bool bEnabled = false;
			if (Value->Type != EJson::Boolean || !Value->TryGetBool(bEnabled))
			{
				return Fail(VariantState, Codes::InvalidFieldValue, AppendPointer(VariantPointer, Key), TEXT("Expected a fluff flag."));
			}
			if (bEnabled) { Row.SetBoolField(Key, true); }
		}

		for (const TCHAR* Key : {TEXT("baseItem"), TEXT("genericVariant")})
		{
			if (Row.HasField(Key) && !FindExactField(Row, Key).IsValid())
			{
				return Fail(VariantState, Codes::InvalidFieldValue, InheritsPointer,
					TEXT("A generated link conflicts with a differently cased input key."));
			}
		}
		Row.SetStringField(TEXT("name"), Name);
		Row.SetStringField(TEXT("source"), TemplateSource);
		Row.SetStringField(TEXT("baseItem"), BaseName + TEXT("|") + BaseSource);
		auto TemplateLink = MakeShared<FJsonObject>();
		TemplateLink->SetStringField(TEXT("name"), TemplateName);
		TemplateLink->SetStringField(TEXT("source"), TemplateSource);
		Row.SetObjectField(TEXT("genericVariant"), TemplateLink);
		const bool bTemplateLoot = FindExactField(Inherits, TEXT("lootTables")).IsValid();
		return AppendLinkedLoot(VariantRoot, TemplateSource, Name, bTemplateLoot ? InheritsPointer : BasePointer, Row,
			bTemplateLoot ? VariantState : BaseState, VariantState);
	}
}

FDnd5eJsonItemVariantGenerationResult FDnd5eJsonItemVariantGenerator::GenerateAtIndex(const FDnd5eJsonDocument& VariantDocument,
	int32 VariantIndex, const FDnd5eJsonDocument& BaseItemDocument, int32 BaseIndex,
	EDnd5eJsonItemVariantEditionPolicy Policy)
{
	using namespace Dnd5eJsonReader;
	FDnd5eJsonItemVariantGenerationResult Result;
	Result.Status = EDnd5eJsonReadStatus::Failure;
	FItemVariantMatchInputs Inputs;
	auto Match = MatchItemVariant(VariantDocument, VariantIndex, BaseItemDocument, Policy, &Inputs);
	Result.Diagnostics = MoveTemp(Match.Diagnostics);
	if (Match.Status != EDnd5eJsonReadStatus::Success) { return Result; }

	FReadState BaseState{BaseItemDocument.Context, BaseItemDocument.JsonText, Result.Diagnostics};
	FReadState VariantState{VariantDocument.Context, VariantDocument.JsonText, Result.Diagnostics};
	if (!Inputs.Bases->IsValidIndex(BaseIndex))
	{
		Fail(BaseState, Codes::InvalidFieldValue, TEXT("/baseitem"), TEXT("Select an existing base item index."));
		return Result;
	}
	const FString BasePointer = AppendPointer(TEXT("/baseitem"), FString::FromInt(BaseIndex));
	if (!Match.MatchingBaseIndices.Contains(BaseIndex))
	{
		Fail(BaseState, Codes::InvalidFieldValue, BasePointer, TEXT("The selected base does not match this template and edition policy."));
		return Result;
	}

	const auto Base = (*Inputs.Bases)[BaseIndex]->AsObject();
	const FString VariantPointer = AppendPointer(TEXT("/magicvariant"), FString::FromInt(VariantIndex));
	const auto Inherits = FindExactField(*Inputs.Variant, TEXT("inherits"))->AsObject();
	FString BaseName, BaseSource;
	if (!ReadRequiredString(*Base, TEXT("name"), BasePointer, BaseName, BaseState) ||
		!ReadRequiredString(*Base, TEXT("source"), BasePointer, BaseSource, BaseState) ||
		!ValidateIdentityText(BaseName, AppendPointer(BasePointer, TEXT("name")), BaseState) ||
		!ValidateIdentityText(BaseSource, AppendPointer(BasePointer, TEXT("source")), BaseState)) { return Result; }
	if (BaseName.Contains(TEXT("|")) || BaseSource.Contains(TEXT("|")))
	{
		Fail(BaseState, Codes::InvalidFieldValue, BasePointer, TEXT("The base name and source must form an explicit name|source link."));
		return Result;
	}

	FDnd5eRecordId BaseId, TemplateKey;
	const FString& Dataset = BaseItemDocument.Context.DatasetNamespace;
	if (!Dnd5e::Items::TryMakeId(Dataset, BaseName, BaseSource, BaseId) ||
		!Dnd5e::Items::TryMakeId(Dataset, Match.VariantName, Match.VariantSourceId, TemplateKey))
	{
		Fail(VariantState, Codes::InvalidFieldValue, VariantPointer, TEXT("Could not build a canonical variant pair identity."));
		return Result;
	}
	FDnd5eRecordId GeneratedId = BaseId;
	GeneratedId.SourceKey = FString::Printf(TEXT("dnd5e:item-variant:v1:%s|%s"),
		*EscapeIdentityComponent(BaseId.SourceKey), *EscapeIdentityComponent(TemplateKey.SourceKey));

	auto Row = MakeShared<FJsonObject>();
	if (!BuildSpecificVariantRow(*Base, *Inputs.Variant, *Inherits, *Inputs.VariantRoot, BasePointer, VariantPointer,
		BaseName, BaseSource, Match.VariantName, Match.VariantSourceId, *Row, BaseState, VariantState)) { return Result; }

	auto Preview = MakeShared<FJsonObject>();
	const TArray<TSharedPtr<FJsonValue>> Rows{MakeShared<FJsonValueObject>(Row)};
	Preview->SetArrayField(TEXT("item"), Rows);
	FString Json;
	if (!SerializeValue(MakeShared<FJsonValueObject>(Preview), Json, VariantPointer, VariantState)) { return Result; }
	FReadState PreviewState{VariantDocument.Context, Json, Result.Diagnostics};
	if (!ValidateDocumentText(PreviewState) || !PreflightJson(PreviewState)) { return Result; }

	Result.GeneratedJson = MoveTemp(Json);
	Result.GeneratedId = MoveTemp(GeneratedId);
	Result.BaseOrigin.SourceId = MoveTemp(BaseSource);
	Result.BaseOrigin.RelativeFilePath = BaseItemDocument.Context.RelativeFilePath;
	Result.BaseOrigin.JsonPointer = BasePointer;
	Result.TemplateOrigin.SourceId = MoveTemp(Match.VariantSourceId);
	Result.TemplateOrigin.RelativeFilePath = VariantDocument.Context.RelativeFilePath;
	Result.TemplateOrigin.JsonPointer = VariantPointer;
	Result.Status = EDnd5eJsonReadStatus::Success;
	return Result;
}