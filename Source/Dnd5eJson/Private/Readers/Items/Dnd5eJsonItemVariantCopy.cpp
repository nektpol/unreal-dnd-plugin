// Developed by Neko Creative Technologies

#include "Readers/Items/Dnd5eJsonItemVariantCopy.h"
#include "Readers/Items/Dnd5eJsonItemCopyPolicy.h"
#include "Items/Dnd5eItemIdentity.h"

namespace
{
	using namespace Dnd5eJsonReader;

	constexpr int32 MaxVariantCopyHops = 32;
	constexpr int32 MaxVariantCopyDirectives = 64;
	constexpr int32 MaxVariantCopyArrayItems = 4096;

	struct FVariantCopyStep
	{
		int32 Index;
		TSharedPtr<FJsonObject> Row;
		TSharedPtr<FJsonObject> Preserve;
		TSharedPtr<FJsonObject> Modifiers;
	};

	FString VariantRowPointer(int32 Index)
	{
		return AppendPointer(TEXT("/magicvariant"), FString::FromInt(Index));
	}

	bool ReadIdentitySource(const FJsonObject& Row, bool bAllowInheritedSource, FString& Source)
	{
		const auto Value = FindExactField(Row, TEXT("source"));
		if (Value.IsValid() && Value->Type != EJson::Null)
		{
			if (Value->Type != EJson::String || !Value->TryGetString(Source)) { return false; }
			if (!Source.IsEmpty()) { return IsValidIdentityText(Source); }
		}
		if (!bAllowInheritedSource) { return false; }

		const auto Inherits = FindExactField(Row, TEXT("inherits"));
		const TSharedPtr<FJsonObject>* Object = nullptr;
		if (!Inherits.IsValid() || Inherits->Type != EJson::Object || !Inherits->TryGetObject(Object) ||
			!Object || !Object->IsValid()) { return false; }
		const auto NestedSource = FindExactField(**Object, TEXT("source"));
		return NestedSource.IsValid() && NestedSource->Type == EJson::String &&
			NestedSource->TryGetString(Source) && IsValidIdentityText(Source);
	}

	bool TryReadTemplateId(const FJsonObject& Row, bool bAllowInheritedSource,
		const FReadState& State, FDnd5eRecordId& Id, FString& Source)
	{
		const auto Value = FindExactField(Row, TEXT("name"));
		FString Name;
		return Value.IsValid() && Value->Type == EJson::String && Value->TryGetString(Name) &&
			IsValidIdentityText(Name) && ReadIdentitySource(Row, bAllowInheritedSource, Source) &&
			Dnd5e::Items::TryMakeId(State.Context.DatasetNamespace, Name, Source, Id);
	}

	TMap<FDnd5eRecordId, int32> IndexTemplates(const TArray<TSharedPtr<FJsonValue>>& Rows, const FReadState& State)
	{
		TMap<FDnd5eRecordId, int32> Indices;
		for (int32 Index = 0; Index < Rows.Num(); ++Index)
		{
			const auto& Value = Rows[Index];
			const TSharedPtr<FJsonObject>* Row = nullptr;
			FDnd5eRecordId Id;
			FString Source;
			if (!Value.IsValid() || Value->Type != EJson::Object || !Value->TryGetObject(Row) ||
				!Row || !Row->IsValid() || !TryReadTemplateId(**Row, true, State, Id, Source)) { continue; }

			if (int32* Existing = Indices.Find(Id)) { *Existing = INDEX_NONE; }
			else { Indices.Add(MoveTemp(Id), Index); }
		}
		return Indices;
	}

	bool CollectVariantChain(const TArray<TSharedPtr<FJsonValue>>& Rows, int32 Index,
		TArray<FVariantCopyStep>& Chain, FString& SelectedSource, FReadState& State)
	{
		const auto Indices = IndexTemplates(Rows, State);
		TSet<int32> Visited;
		for (int32 Current = Index;;)
		{
			const FString RowPointer = VariantRowPointer(Current);
			FVariantCopyStep Step{Current, nullptr, nullptr, nullptr};
			if (!RequireObject(Rows[Current], RowPointer, Step.Row, State)) { return false; }

			FDnd5eRecordId RowId;
			FString RowSource;
			if (!TryReadTemplateId(*Step.Row, true, State, RowId, RowSource) || Indices.FindRef(RowId, INDEX_NONE) != Current)
			{
				return Fail(State, Codes::InvalidFieldValue, RowPointer, TEXT("Expected a unique template name and source."));
			}
			if (Chain.IsEmpty()) { SelectedSource = MoveTemp(RowSource); }
			Visited.Add(Current);

			const auto CopyValue = FindExactField(*Step.Row, TEXT("_copy"));
			if (!CopyValue.IsValid())
			{
				Chain.Add(MoveTemp(Step));
				return true;
			}
			const FString CopyPointer = AppendPointer(RowPointer, TEXT("_copy"));
			if (Chain.Num() >= MaxVariantCopyHops)
			{
				return Fail(State, Codes::ResourceLimitExceeded, CopyPointer, TEXT("Too many variant copy links."));
			}

			TSharedPtr<FJsonObject> Copy;
			if (!RequireObject(CopyValue, CopyPointer, Copy, State) ||
				!CheckAllowedKeys(*Copy, {TEXT("name"), TEXT("source"), TEXT("_preserve"), TEXT("_mod")}, CopyPointer, State) ||
				!ReadPreserveMap(*Copy, CopyPointer, Step.Preserve, State)) { return false; }
			if (const auto Mod = FindExactField(*Copy, TEXT("_mod")))
			{
				if (!RequireObject(Mod, AppendPointer(CopyPointer, TEXT("_mod")), Step.Modifiers, State)) { return false; }
			}

			FDnd5eRecordId TargetId;
			FString TargetSource;
			if (!TryReadTemplateId(*Copy, false, State, TargetId, TargetSource))
			{
				return Fail(State, Codes::InvalidFieldValue, CopyPointer, TEXT("Expected a template target name and explicit source."));
			}
			const int32* Target = Indices.Find(TargetId);
			if (!Target)
			{
				return Fail(State, Codes::UnresolvedReference, CopyPointer, TEXT("Template target is missing from this document."));
			}
			if (*Target == INDEX_NONE || Visited.Contains(*Target))
			{
				return Fail(State, Codes::InvalidFieldValue, CopyPointer, TEXT("Template target is ambiguous or cyclic."));
			}
			Chain.Add(MoveTemp(Step));
			Current = *Target;
		}
	}

	bool IsOneOfCopyPaths(const FString& Path, std::initializer_list<const TCHAR*> Allowed)
	{
		for (const TCHAR* Key : Allowed)
		{
			if (Path.Equals(Key, ESearchCase::CaseSensitive)) { return true; }
		}
		return false;
	}

	bool AppendVariantCopyItems(FJsonObject& Inherits, const FString& Leaf, const TSharedPtr<FJsonValue>& Items,
		bool bUniqueStrings, const FString& Pointer, FReadState& State)
	{
		if (!Items.IsValid())
		{
			return Fail(State, Codes::MissingRequiredField, Pointer, TEXT("A variant copy append needs items."));
		}
		TArray<TSharedPtr<FJsonValue>> Added;
		if (Items->Type == EJson::Array) { Added = Items->AsArray(); }
		else { Added.Add(Items); }

		const TArray<TSharedPtr<FJsonValue>>* Existing = nullptr;
		if (const auto Value = FindExactField(Inherits, *Leaf); Value.IsValid() && Value->Type != EJson::Null)
		{
			if (!RequireArray(Value, Pointer, Existing, State)) { return false; }
		}
		const int32 ExistingCount = Existing ? Existing->Num() : 0;
		if (int64(ExistingCount) + Added.Num() > MaxVariantCopyArrayItems)
		{
			return Fail(State, Codes::ResourceLimitExceeded, Pointer, TEXT("Too many entries in a variant copy array."));
		}
		TArray<TSharedPtr<FJsonValue>> Combined;
		if (Existing) { Combined = *Existing; }
		if (bUniqueStrings && Existing)
		{
			for (const auto& Value : *Existing)
			{
				FString Text;
				if (Value->Type != EJson::String || !Value->TryGetString(Text))
				{
					return Fail(State, Codes::InvalidFieldValue, Pointer, TEXT("Expected an existing flat string array."));
				}
			}
		}
		for (const auto& Value : Added)
		{
			FString AddedText;
			if (bUniqueStrings)
			{
				if (Value->Type != EJson::String || !Value->TryGetString(AddedText))
				{
					return Fail(State, Codes::InvalidFieldValue, Pointer, TEXT("Expected a flat string array in a variant append."));
				}
			}
			else if (Value->Type != EJson::String && Value->Type != EJson::Object)
			{
				return Fail(State, Codes::InvalidFieldValue, Pointer, TEXT("Expected a variant entry string or object."));
			}

			bool bAlreadyPresent = false;
			if (bUniqueStrings && Existing)
			{
				for (const auto& Current : *Existing)
				{
					bAlreadyPresent |= Current->AsString().Equals(AddedText, ESearchCase::CaseSensitive);
					if (bAlreadyPresent) { break; }
				}
			}
			if (!bAlreadyPresent) { Combined.Add(Value); }
		}
		Inherits.SetArrayField(Leaf, Combined);
		return true;
	}

	bool ApplyVariantDirective(FJsonObject& Inherits, const FString& Path, const TSharedPtr<FJsonValue>& Value,
		const FString& Pointer, FReadState& State)
	{
		const bool bRemove = IsOneOfCopyPaths(Path, {TEXT("inherits.basicRules2024"), TEXT("inherits.classFeatures"),
			TEXT("inherits.lootTables"), TEXT("inherits.srd52")});
		const bool bSetString = IsOneOfCopyPaths(Path, {TEXT("inherits.namePrefix"), TEXT("inherits.nameSuffix"),
			TEXT("inherits.rarity"), TEXT("inherits.source")});
		const bool bSetPage = Path.Equals(TEXT("inherits.page"), ESearchCase::CaseSensitive);
		const bool bAppendEntries = Path.Equals(TEXT("inherits.entries"), ESearchCase::CaseSensitive);
		const bool bAppendStrings = IsOneOfCopyPaths(Path, {TEXT("inherits.propertyAdd"), TEXT("inherits.resist")});
		if (!bRemove && !bSetString && !bSetPage && !bAppendEntries && !bAppendStrings)
		{
			return Fail(State, Codes::UnsupportedRequiredValue, Pointer, TEXT("Unsupported variant copy path."));
		}
		const FString Leaf = Path.RightChop(UE_ARRAY_COUNT(TEXT("inherits.")) - 1);
		if (Inherits.HasField(Leaf) && !FindExactField(Inherits, *Leaf).IsValid())
		{
			return Fail(State, Codes::InvalidFieldValue, Pointer, TEXT("Variant modifier conflicts with a differently cased field."));
		}
		if (ContainsCopyVariable(*Value))
		{
			return Fail(State, Codes::UnsupportedRequiredValue, Pointer, TEXT("Variant copy variables require a separate reader."));
		}
		if (bRemove && Value->Type == EJson::String && Value->AsString().Equals(TEXT("remove"), ESearchCase::CaseSensitive))
		{
			Inherits.RemoveField(Leaf);
			return true;
		}
		if (Value->Type != EJson::Object)
		{
			return Fail(State, Codes::UnsupportedRequiredValue, Pointer, TEXT("Unsupported variant copy directive."));
		}
		TSharedPtr<FJsonObject> Directive;
		FString Mode;
		if (!RequireObject(Value, Pointer, Directive, State) ||
			!ReadRequiredString(*Directive, TEXT("mode"), Pointer, Mode, State)) { return false; }

		if ((bSetString || bSetPage) && Mode.Equals(TEXT("setProp"), ESearchCase::CaseSensitive))
		{
			if (!CheckAllowedKeys(*Directive, {TEXT("mode"), TEXT("value")}, Pointer, State)) { return false; }
			const auto Field = FindExactField(*Directive, TEXT("value"));
			double Page = 0;
			if (!Field.IsValid() || (bSetString && Field->Type != EJson::String) ||
				(bSetPage && (Field->Type != EJson::Number || !Field->TryGetNumber(Page) ||
					Page < 0 || Page > MAX_int32 || FMath::FloorToDouble(Page) != Page)))
			{
				return Fail(State, Codes::InvalidFieldValue, AppendPointer(Pointer, TEXT("value")),
					TEXT("Expected a string or an integer page for this variant field."));
			}
			Inherits.SetField(Leaf, Field);
			return true;
		}

		if ((bAppendEntries && Mode.Equals(TEXT("appendArr"), ESearchCase::CaseSensitive)) ||
			(bAppendStrings && Mode.Equals(TEXT("appendIfNotExistsArr"), ESearchCase::CaseSensitive)))
		{
			if (!CheckAllowedKeys(*Directive, {TEXT("mode"), TEXT("items")}, Pointer, State)) { return false; }
			return AppendVariantCopyItems(Inherits, Leaf, FindExactField(*Directive, TEXT("items")), bAppendStrings,
				AppendPointer(Pointer, TEXT("items")), State);
		}
		return Fail(State, Codes::UnsupportedRequiredValue, AppendPointer(Pointer, TEXT("mode")),
			TEXT("Unsupported mode for this variant copy path."));
	}

	bool ApplyVariantCopyModifiers(FJsonObject& Merged, const FVariantCopyStep& Step, FReadState& State)
	{
		if (!Step.Modifiers.IsValid() || Step.Modifiers->Values.IsEmpty()) { return true; }
		const FString ModPointer = AppendPointer(AppendPointer(VariantRowPointer(Step.Index), TEXT("_copy")), TEXT("_mod"));
		TSharedPtr<FJsonObject> Original;
		if (!RequireObject(FindExactField(Merged, TEXT("inherits")), ModPointer, Original, State)) { return false; }
		auto Inherits = MakeShared<FJsonObject>();
		Inherits->Values = Original->Values;

		TArray<FString> Paths;
		for (const auto& Pair : Step.Modifiers->Values) { Paths.Emplace(Pair.Key.Len(), *Pair.Key); }
		Paths.Sort([](const FString& A, const FString& B) { return A.Compare(B, ESearchCase::CaseSensitive) < 0; });
		for (const FString& Path : Paths)
		{
			const FString Pointer = AppendPointer(ModPointer, Path);
			const auto Directive = FindExactField(*Step.Modifiers, *Path);
			if (Directive->Type != EJson::Array)
			{
				if (!ApplyVariantDirective(*Inherits, Path, Directive, Pointer, State)) { return false; }
				continue;
			}
			const auto& Directives = Directive->AsArray();
			if (Directives.IsEmpty() || Directives.Num() > MaxVariantCopyDirectives)
			{
				return Fail(State, Codes::InvalidFieldValue, Pointer, TEXT("Expected one to 64 variant copy directives."));
			}
			for (int32 Index = 0; Index < Directives.Num(); ++Index)
			{
				if (!ApplyVariantDirective(*Inherits, Path, Directives[Index],
					AppendPointer(Pointer, FString::FromInt(Index)), State)) { return false; }
			}
		}
		Merged.SetObjectField(TEXT("inherits"), Inherits);
		return true;
	}

	bool MergeVariantCopyStep(const FVariantCopyStep& Step, TSharedPtr<FJsonObject>& Parent, FReadState& State)
	{
		auto Merged = MakeShared<FJsonObject>();
		Merged->Values = Step.Row->Values;
		Merged->RemoveField(TEXT("_copy"));
		const FString Pointer = VariantRowPointer(Step.Index);
		for (const auto& Pair : Parent->Values)
		{
			const FString Key(Pair.Key.Len(), *Pair.Key);
			if (const auto ChildValue = FindExactField(*Step.Row, *Key))
			{
				if (ChildValue->Type == EJson::Null) { Merged->RemoveField(Key); }
				continue;
			}
			if (IsProtectedCopyField(Key) && !IsPreserved(Step.Preserve, *Key)) { continue; }
			if (Merged->HasField(Key))
			{
				return Fail(State, Codes::InvalidFieldValue, AppendPointer(Pointer, Key),
					TEXT("Template copy fields differ only by case."));
			}
			Merged->SetField(Key, Pair.Value);
		}
		if (!ApplyVariantCopyModifiers(*Merged, Step, State)) { return false; }
		Parent = Merged;
		return true;
	}
}

bool Dnd5eJsonReader::ResolveItemVariantTemplate(const TArray<TSharedPtr<FJsonValue>>& Rows, int32 Index,
	FResolvedItemVariantTemplate& Out, FReadState& State)
{
	Out = {};
	if (!Rows.IsValidIndex(Index))
	{
		return Fail(State, Codes::InvalidFieldValue, TEXT("/magicvariant"), TEXT("Select an existing variant index."));
	}
	TSharedPtr<FJsonObject> Row;
	if (!RequireObject(Rows[Index], VariantRowPointer(Index), Row, State)) { return false; }
	if (!FindExactField(*Row, TEXT("_copy")).IsValid())
	{
		Out = {MoveTemp(Row), {}, {Index}};
		return true;
	}

	TArray<FVariantCopyStep> Chain;
	FString IdentitySource;
	if (!CollectVariantChain(Rows, Index, Chain, IdentitySource, State)) { return false; }
	TSharedPtr<FJsonObject> Resolved = Chain.Last().Row;
	for (int32 Step = Chain.Num() - 2; Step >= 0; --Step)
	{
		if (!MergeVariantCopyStep(Chain[Step], Resolved, State)) { return false; }
	}
	TArray<int32> Indices;
	for (const FVariantCopyStep& Step : Chain) { Indices.Add(Step.Index); }
	Out = {MoveTemp(Resolved), MoveTemp(IdentitySource), MoveTemp(Indices)};
	return true;
}