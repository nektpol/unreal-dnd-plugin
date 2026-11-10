// Developed by Neko Creative Technologies

#include "Readers/Items/Dnd5eJsonItemVariantGenerator.h"
#include "Readers/Items/Dnd5eJsonItemVariantMatcherInternal.h"
#include "Readers/Dnd5eJsonReaderInternal.h"
#include "Readers/Items/Dnd5eJsonItemVariantEntries.h"
#include "Readers/Items/Dnd5eJsonItemVariantTransforms.h"
#include "Items/Dnd5eItemIdentity.h"

namespace
{
	using namespace Dnd5eJsonReader;

	struct FStagedItemVariant
	{
		TSharedPtr<FJsonObject> Row;
		TSharedPtr<FJsonObject> Base;
		TSharedPtr<FJsonObject> VariantRoot;
		FDnd5eRecordId GeneratedId;
		FDnd5eRecordProvenance BaseOrigin;
		FDnd5eRecordProvenance TemplateOrigin;
		TArray<int32> TemplateSourceRowIndices;
	};

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

	bool TryReadPropertyUid(const FJsonValue& Value, FString& Out)
	{
		if (Value.Type == EJson::String) { return Value.TryGetString(Out) && !Out.IsEmpty(); }
		if (Value.Type != EJson::Object) { return false; }
		const auto Uid = FindExactField(*Value.AsObject(), TEXT("uid"));
		return Uid.IsValid() && Uid->Type == EJson::String && Uid->TryGetString(Out) && !Out.IsEmpty();
	}

	bool ApplyVariantPropertyOperations(const FJsonObject& Base, const FJsonObject& Inherits, const FString& BasePointer,
		const FString& InheritsPointer, FJsonObject& Row, FReadState& BaseState, FReadState& VariantState)
	{
		const auto Add = FindExactField(Inherits, TEXT("propertyAdd"));
		const auto Remove = FindExactField(Inherits, TEXT("propertyRemove"));
		if (!Add.IsValid() && !Remove.IsValid()) { return true; }

		const FString AddPointer = AppendPointer(InheritsPointer, TEXT("propertyAdd"));
		const FString RemovePointer = AppendPointer(InheritsPointer, TEXT("propertyRemove"));
		const FString& OperationPointer = Remove.IsValid() ? RemovePointer : AddPointer;
		if (FindExactField(Inherits, TEXT("property")).IsValid() ||
			(Row.HasField(TEXT("property")) && !FindExactField(Row, TEXT("property")).IsValid()))
		{
			return Fail(VariantState, Codes::UnsupportedRequiredValue, OperationPointer,
				TEXT("Property operations cannot be combined with a conflicting property override."));
		}

		const TArray<TSharedPtr<FJsonValue>>* Additions = nullptr;
		const TArray<TSharedPtr<FJsonValue>>* Removals = nullptr;
		TArray<TSharedPtr<FJsonValue>> Original;
		if (!ReadVariantArrayValues(Base, TEXT("property"), BasePointer, Original, BaseState) ||
			(Add.IsValid() && !RequireArray(Add, AddPointer, Additions, VariantState)) ||
			(Remove.IsValid() && !RequireArray(Remove, RemovePointer, Removals, VariantState))) { return false; }
		if (Original.Num() > 4096 || (Additions && Additions->Num() > 4096) || (Removals && Removals->Num() > 4096))
		{
			return Fail(VariantState, Codes::ResourceLimitExceeded, OperationPointer, TEXT("Too many variant property values."));
		}

		const auto ContainsUid = [](const TArray<FString>& Values, const FString& Uid)
		{
			return Values.ContainsByPredicate([&Uid](const FString& Existing)
			{
				return Existing.Equals(Uid, ESearchCase::CaseSensitive);
			});
		};
		TArray<FString> RemoveUids;
		if (Removals)
		{
			for (int32 Index = 0; Index < Removals->Num(); ++Index)
			{
				const auto& Value = (*Removals)[Index];
				FString Uid;
				if (Value->Type != EJson::String || !Value->TryGetString(Uid) || Uid.IsEmpty())
				{
					return Fail(VariantState, Codes::InvalidFieldValue, AppendPointer(RemovePointer, FString::FromInt(Index)),
						TEXT("Expected a property UID string to remove."));
				}
				RemoveUids.Add(MoveTemp(Uid));
			}
		}

		TArray<TSharedPtr<FJsonValue>> Properties;
		TArray<FString> KeptUids;
		const FString PropertyPointer = AppendPointer(BasePointer, TEXT("property"));
		for (int32 Index = 0; Index < Original.Num(); ++Index)
		{
			FString Uid;
			if (!TryReadPropertyUid(*Original[Index], Uid))
			{
				return Fail(BaseState, Codes::InvalidFieldValue, AppendPointer(PropertyPointer, FString::FromInt(Index)),
					TEXT("Expected a property UID."));
			}
			if (ContainsUid(RemoveUids, Uid)) { continue; }
			Properties.Add(Original[Index]);
			KeptUids.Add(MoveTemp(Uid));
		}

		if (Additions)
		{
			for (int32 Index = 0; Index < Additions->Num(); ++Index)
			{
				FString Uid;
				if (!TryReadPropertyUid(*(*Additions)[Index], Uid))
				{
					return Fail(VariantState, Codes::InvalidFieldValue, AppendPointer(AddPointer, FString::FromInt(Index)),
						TEXT("Expected an added property UID."));
				}
				if (!ContainsUid(KeptUids, Uid)) { Properties.Add((*Additions)[Index]); }
			}
			Row.SetArrayField(TEXT("property"), Properties);
		}
		else if (const auto Existing = FindExactField(Base, TEXT("property")); Existing.IsValid() && Existing->Type == EJson::Array)
		{
			if (Properties.IsEmpty()) { Row.RemoveField(TEXT("property")); }
			else { Row.SetArrayField(TEXT("property"), Properties); }
		}
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
		const FString& TemplateIdentitySource, FJsonObject& Row, FReadState& BaseState, FReadState& VariantState)
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
			if (IsOneOf(Key, {TEXT("nameRemove"), TEXT("namePrefix"), TEXT("nameSuffix"), TEXT("entries"),
				TEXT("propertyAdd"), TEXT("propertyRemove"), TEXT("valueExpression"), TEXT("weightExpression"),
				TEXT("barding"), TEXT("conditionImmune"), TEXT("vulnerable"), TEXT("resist"), TEXT("immune")})) { continue; }
			const FString Pointer = AppendPointer(InheritsPointer, Key);
			if (IsOneOf(Key, {TEXT("name"), TEXT("baseItem"), TEXT("genericVariant"), TEXT("_copy"),
				TEXT("requires"), TEXT("excludes"), TEXT("inherits")}))
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
			TArray<TSharedPtr<FJsonValue>> Combined, BaseEntries;
			if (!RequireArray(Entries, EntriesPointer, Inserted, VariantState) ||
				!RenderItemVariantEntries(*Inserted, Base, Inherits, BaseName, EntriesPointer, Combined, VariantState) ||
				!ReadVariantArrayValues(Base, TEXT("entries"), BasePointer, BaseEntries, BaseState)) { return false; }
			Combined.Append(BaseEntries);
			Row.SetArrayField(TEXT("entries"), Combined);
		}
		if (!ApplyVariantPropertyOperations(Base, Inherits, BasePointer, InheritsPointer, Row, BaseState, VariantState) ||
			!ApplyItemVariantTransforms(Base, Inherits, BasePointer, InheritsPointer, Row, BaseState, VariantState)) { return false; }

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
		TemplateLink->SetStringField(TEXT("source"), TemplateIdentitySource);
		Row.SetObjectField(TEXT("genericVariant"), TemplateLink);
		const bool bTemplateLoot = FindExactField(Inherits, TEXT("lootTables")).IsValid();
		return AppendLinkedLoot(VariantRoot, TemplateSource, Name, bTemplateLoot ? InheritsPointer : BasePointer, Row,
			bTemplateLoot ? VariantState : BaseState, VariantState);
	}

	bool StageSpecificVariant(const FDnd5eJsonDocument& VariantDocument, int32 VariantIndex,
		const FDnd5eJsonDocument& BaseItemDocument, int32 BaseIndex, EDnd5eJsonItemVariantEditionPolicy Policy,
		FStagedItemVariant& Out, TArray<FDnd5eDiagnostic>& Diagnostics)
	{
		FItemVariantMatchInputs Inputs;
		auto Match = MatchItemVariant(VariantDocument, VariantIndex, BaseItemDocument, Policy, &Inputs);
		Diagnostics.Append(MoveTemp(Match.Diagnostics));
		if (Match.Status != EDnd5eJsonReadStatus::Success) { return false; }

		FReadState BaseState{BaseItemDocument.Context, BaseItemDocument.JsonText, Diagnostics};
		FReadState VariantState{VariantDocument.Context, VariantDocument.JsonText, Diagnostics};
		if (!Inputs.Bases->IsValidIndex(BaseIndex))
		{
			Fail(BaseState, Codes::InvalidFieldValue, TEXT("/baseitem"), TEXT("Select an existing base item index."));
			return false;
		}
		const FString BasePointer = AppendPointer(TEXT("/baseitem"), FString::FromInt(BaseIndex));
		if (!Match.MatchingBaseIndices.Contains(BaseIndex))
		{
			Fail(BaseState, Codes::InvalidFieldValue, BasePointer, TEXT("The selected base does not match this template and edition policy."));
			return false;
		}

		const auto Base = (*Inputs.Bases)[BaseIndex]->AsObject();
		const FString VariantPointer = AppendPointer(TEXT("/magicvariant"), FString::FromInt(VariantIndex));
		const auto Inherits = FindExactField(*Inputs.Variant, TEXT("inherits"))->AsObject();
		FString BaseName, BaseSource;
		if (!ReadRequiredString(*Base, TEXT("name"), BasePointer, BaseName, BaseState) ||
			!ReadRequiredString(*Base, TEXT("source"), BasePointer, BaseSource, BaseState) ||
			!ValidateIdentityText(BaseName, AppendPointer(BasePointer, TEXT("name")), BaseState) ||
			!ValidateIdentityText(BaseSource, AppendPointer(BasePointer, TEXT("source")), BaseState)) { return false; }
		if (BaseName.Contains(TEXT("|")) || BaseSource.Contains(TEXT("|")))
		{
			Fail(BaseState, Codes::InvalidFieldValue, BasePointer, TEXT("The base name and source must form an explicit name|source link."));
			return false;
		}

		const FString& TemplateIdentitySource = Inputs.TemplateIdentitySource.IsEmpty()
			? Match.VariantSourceId : Inputs.TemplateIdentitySource;
		FDnd5eRecordId BaseId, TemplateKey;
		const FString& Dataset = BaseItemDocument.Context.DatasetNamespace;
		if (!Dnd5e::Items::TryMakeId(Dataset, BaseName, BaseSource, BaseId) ||
			!Dnd5e::Items::TryMakeId(Dataset, Match.VariantName, TemplateIdentitySource, TemplateKey))
		{
			Fail(VariantState, Codes::InvalidFieldValue, VariantPointer, TEXT("Could not build a canonical variant pair identity."));
			return false;
		}
		FDnd5eRecordId GeneratedId = BaseId;
		GeneratedId.SourceKey = FString::Printf(TEXT("dnd5e:item-variant:v1:%s|%s"),
			*EscapeIdentityComponent(BaseId.SourceKey), *EscapeIdentityComponent(TemplateKey.SourceKey));

		auto Row = MakeShared<FJsonObject>();
		if (!BuildSpecificVariantRow(*Base, *Inputs.Variant, *Inherits, *Inputs.VariantRoot, BasePointer, VariantPointer,
			BaseName, BaseSource, Match.VariantName, Match.VariantSourceId, TemplateIdentitySource,
			*Row, BaseState, VariantState)) { return false; }

		Out.Row = MoveTemp(Row);
		Out.Base = Base;
		Out.VariantRoot = MoveTemp(Inputs.VariantRoot);
		Out.GeneratedId = MoveTemp(GeneratedId);
		Out.BaseOrigin.SourceId = MoveTemp(BaseSource);
		Out.BaseOrigin.RelativeFilePath = BaseItemDocument.Context.RelativeFilePath;
		Out.BaseOrigin.JsonPointer = BasePointer;
		Out.TemplateOrigin.SourceId = TemplateIdentitySource;
		Out.TemplateOrigin.RelativeFilePath = VariantDocument.Context.RelativeFilePath;
		Out.TemplateOrigin.JsonPointer = VariantPointer;
		Out.TemplateSourceRowIndices = MoveTemp(Inputs.TemplateSourceRowIndices);
		return true;
	}

	void AddSharedVariantFields(const FJsonObject& Input, const FJsonObject& Row, const FString& RowPointer,
		const FDnd5eRecordProvenance& InputOrigin, FReadFieldOrigins& Out)
	{
		for (const auto& Field : Input.Values)
		{
			const auto* Value = Row.Values.Find(Field.Key);
			if (!Value || *Value != Field.Value) { continue; }
			const FString Key(Field.Key.Len(), *Field.Key);
			const FString Logical = AppendPointer(RowPointer, Key);
			Out.Fields.Add(Logical, FReadFieldOrigin{Logical, AppendPointer(InputOrigin.JsonPointer, Key),
				InputOrigin.SourceId, true, InputOrigin.RelativeFilePath});
		}
	}

	FReadFieldOrigins BuildVariantReadOrigins(const FStagedItemVariant& Staged, const FString& RowPointer)
	{
		FReadFieldOrigins Origins;
		Origins.RowPrefix = RowPointer + TEXT("/");
		Origins.Fields.Add(RowPointer, FReadFieldOrigin{RowPointer, Staged.TemplateOrigin.JsonPointer,
			Staged.TemplateOrigin.SourceId, false, Staged.TemplateOrigin.RelativeFilePath});
		AddSharedVariantFields(*Staged.Base, *Staged.Row, RowPointer, Staged.BaseOrigin, Origins);

		const auto& Templates = FindExactField(*Staged.VariantRoot, TEXT("magicvariant"))->AsArray();
		for (const int32 Index : Staged.TemplateSourceRowIndices)
		{
			const auto Template = Templates[Index]->AsObject();
			const auto Value = FindExactField(*Template, TEXT("inherits"));
			if (!Value.IsValid() || Value->Type != EJson::Object) { continue; }
			const auto Inherits = Value->AsObject();
			FString Source;
			if (const auto RootSource = FindExactField(*Template, TEXT("source")); RootSource.IsValid())
			{
				RootSource->TryGetString(Source);
			}
			if (Source.IsEmpty())
			{
				if (const auto NestedSource = FindExactField(*Inherits, TEXT("source")); NestedSource.IsValid())
				{
					NestedSource->TryGetString(Source);
				}
			}
			auto Origin = Staged.TemplateOrigin;
			if (!Source.IsEmpty()) { Origin.SourceId = MoveTemp(Source); }
			Origin.JsonPointer = AppendPointer(AppendPointer(TEXT("/magicvariant"), FString::FromInt(Index)), TEXT("inherits"));
			AddSharedVariantFields(*Inherits, *Staged.Row, RowPointer, Origin, Origins);
		}

		const auto BaseEntries = FindExactField(*Staged.Base, TEXT("entries"));
		const auto BuiltEntries = FindExactField(*Staged.Row, TEXT("entries"));
		if (!BaseEntries.IsValid() || !BuiltEntries.IsValid() || BaseEntries == BuiltEntries ||
			BaseEntries->Type != EJson::Array || BuiltEntries->Type != EJson::Array) { return Origins; }

		const auto& Original = BaseEntries->AsArray();
		const auto& Built = BuiltEntries->AsArray();
		const int32 Offset = Built.Num() - Original.Num();
		if (Offset < 0) { return Origins; }
		const FString LogicalEntries = AppendPointer(RowPointer, TEXT("entries"));
		const FString InputEntries = AppendPointer(Staged.BaseOrigin.JsonPointer, TEXT("entries"));
		for (int32 Index = 0; Index < Original.Num(); ++Index)
		{
			if (Built[Index + Offset] != Original[Index]) { continue; }
			const FString Logical = AppendPointer(LogicalEntries, FString::FromInt(Index + Offset));
			const FString Input = AppendPointer(InputEntries, FString::FromInt(Index));
			Origins.Fields.Add(Logical, FReadFieldOrigin{Logical, Input, Staged.BaseOrigin.SourceId,
				true, Staged.BaseOrigin.RelativeFilePath});
		}
		return Origins;
	}
}

FDnd5eJsonItemVariantGenerationResult FDnd5eJsonItemVariantGenerator::GenerateAtIndex(const FDnd5eJsonDocument& VariantDocument,
	int32 VariantIndex, const FDnd5eJsonDocument& BaseItemDocument, int32 BaseIndex, EDnd5eJsonItemVariantEditionPolicy Policy)
{
	using namespace Dnd5eJsonReader;
	FDnd5eJsonItemVariantGenerationResult Result;
	Result.Status = EDnd5eJsonReadStatus::Failure;
	FStagedItemVariant Staged;
	if (!StageSpecificVariant(VariantDocument, VariantIndex, BaseItemDocument, BaseIndex, Policy, Staged, Result.Diagnostics)) { return Result; }

	auto Preview = MakeShared<FJsonObject>();
	const TArray<TSharedPtr<FJsonValue>> Rows{MakeShared<FJsonValueObject>(Staged.Row)};
	Preview->SetArrayField(TEXT("item"), Rows);
	FString Json;
	FReadState VariantState{VariantDocument.Context, VariantDocument.JsonText, Result.Diagnostics};
	if (!SerializeValue(MakeShared<FJsonValueObject>(Preview), Json, Staged.TemplateOrigin.JsonPointer, VariantState)) { return Result; }
	FReadState PreviewState{VariantDocument.Context, Json, Result.Diagnostics};
	if (!ValidateDocumentText(PreviewState) || !PreflightJson(PreviewState)) { return Result; }

	Result.GeneratedJson = MoveTemp(Json);
	Result.GeneratedId = MoveTemp(Staged.GeneratedId);
	Result.BaseOrigin = MoveTemp(Staged.BaseOrigin);
	Result.TemplateOrigin = MoveTemp(Staged.TemplateOrigin);
	Result.TemplateSourceRowIndices = MoveTemp(Staged.TemplateSourceRowIndices);
	Result.Status = EDnd5eJsonReadStatus::Success;
	return Result;
}

FDnd5eJsonItemVariantReadResult FDnd5eJsonItemVariantGenerator::ReadAtIndex(const FDnd5eJsonDocument& VariantDocument,
	int32 VariantIndex, const FDnd5eJsonDocument& BaseItemDocument, int32 BaseIndex, EDnd5eJsonItemVariantEditionPolicy Policy)
{
	using namespace Dnd5eJsonReader;
	FDnd5eJsonItemVariantReadResult Result;
	Result.Status = EDnd5eJsonReadStatus::Failure;
	FStagedItemVariant Staged;
	if (!StageSpecificVariant(VariantDocument, VariantIndex, BaseItemDocument, BaseIndex, Policy, Staged, Result.Diagnostics)) { return Result; }

	const FString RowPointer = TEXT("/item/0");
	const FReadFieldOrigins FieldOrigins = BuildVariantReadOrigins(Staged, RowPointer);
	FReadState State{VariantDocument.Context, VariantDocument.JsonText, Result.Diagnostics};
	State.FieldOrigins = &FieldOrigins;
	FDnd5eItem Candidate;
	if (!ReadStandaloneItemRow(*Staged.Row, RowPointer, Candidate, State, &Staged.GeneratedId)) { return Result; }

	auto& Provenance = Candidate.Header.Provenance;
	Provenance[0].RelativeFilePath.Reset();
	Provenance[0].JsonPointer.Reset();
	Provenance.Add(Staged.TemplateOrigin);
	Provenance.Add(Staged.BaseOrigin);

	Result.Item = MoveTemp(Candidate);
	Result.BaseOrigin = MoveTemp(Staged.BaseOrigin);
	Result.TemplateOrigin = MoveTemp(Staged.TemplateOrigin);
	Result.TemplateSourceRowIndices = MoveTemp(Staged.TemplateSourceRowIndices);
	Result.Status = EDnd5eJsonReadStatus::Success;
	return Result;
}