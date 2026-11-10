// Developed by Neko Creative Technologies

#include "Readers/Items/Dnd5eJsonItemCopyAssembler.h"
#include "Items/Dnd5eItemIdentity.h"
#include "Readers/Dnd5eJsonReaderInternal.h"

namespace
{
	using namespace Dnd5eJsonReader;

	constexpr int32 MaxCopyHops = 32;
	constexpr int32 MaxItemRows = 100000;
	constexpr int32 MaxCopyFields = 512;

	struct FItemCopyStep
	{
		int32 Index;
		TSharedPtr<FJsonObject> Row;
		TSharedPtr<FJsonObject> Preserve;
	};

	FString ItemRowPointer(int32 Index)
	{
		return AppendPointer(TEXT("/item"), FString::FromInt(Index));
	}

	bool TryReadItemIdentity(const FJsonObject& Object, const FReadState& State, FDnd5eRecordId& Out)
	{
		const auto NameValue = FindExactField(Object, TEXT("name"));
		const auto SourceValue = FindExactField(Object, TEXT("source"));
		FString Name, Source;
		return NameValue.IsValid() && NameValue->Type == EJson::String && NameValue->TryGetString(Name) &&
			SourceValue.IsValid() && SourceValue->Type == EJson::String && SourceValue->TryGetString(Source) &&
			IsValidIdentityText(Name) && IsValidIdentityText(Source) &&
			Dnd5e::Items::TryMakeId(State.Context.DatasetNamespace, Name, Source, Out);
	}

	TMap<FDnd5eRecordId, int32> IndexItemRows(const TArray<TSharedPtr<FJsonValue>>& Rows, const FReadState& State)
	{
		TMap<FDnd5eRecordId, int32> Indices;
		for (int32 Index = 0; Index < Rows.Num(); ++Index)
		{
			const auto& Value = Rows[Index];
			const TSharedPtr<FJsonObject>* Row = nullptr;
			FDnd5eRecordId Id;
			if (!Value.IsValid() || Value->Type != EJson::Object || !Value->TryGetObject(Row) ||
				!Row || !Row->IsValid() || !TryReadItemIdentity(**Row, State, Id)) { continue; }

			if (int32* Existing = Indices.Find(Id)) { *Existing = INDEX_NONE; }
			else { Indices.Add(MoveTemp(Id), Index); }
		}
		return Indices;
	}

	bool ReadPreserveMap(const FJsonObject& Copy, const FString& Pointer, TSharedPtr<FJsonObject>& Out, FReadState& State)
	{
		const auto Value = FindExactField(Copy, TEXT("_preserve"));
		if (!Value.IsValid()) { return true; }

		const FString PreservePointer = AppendPointer(Pointer, TEXT("_preserve"));
		if (!RequireObject(Value, PreservePointer, Out, State)) { return false; }
		for (const auto& Pair : Out->Values)
		{
			if (Pair.Value->Type != EJson::Boolean)
			{
				const FString Key(Pair.Key.Len(), *Pair.Key);
				return Fail(State, Codes::InvalidFieldValue, AppendPointer(PreservePointer, Key),
					TEXT("Expected a Boolean copy-preserve value."));
			}
		}
		return true;
	}

	bool CollectCopyChain(const TArray<TSharedPtr<FJsonValue>>& Rows, int32 Index,
		const TMap<FDnd5eRecordId, int32>& Indices, TArray<FItemCopyStep>& Chain, FReadState& State)
	{
		TSet<int32> Visited;
		for (int32 Current = Index;;)
		{
			const FString Pointer = ItemRowPointer(Current);
			TSharedPtr<FJsonObject> Row;
			if (!RequireObject(Rows[Current], Pointer, Row, State)) { return false; }
			FDnd5eRecordId RowId;
			if (!TryReadItemIdentity(*Row, State, RowId))
			{
				return Fail(State, Codes::InvalidFieldValue, Pointer, TEXT("Expected an item name and explicit source."));
			}
			const int32* UniqueIndex = Indices.Find(RowId);
			if (!UniqueIndex || *UniqueIndex != Current)
			{
				return Fail(State, Codes::InvalidFieldValue, Pointer, TEXT("Item identity is ambiguous."));
			}
			if (Row->Values.Num() > MaxCopyFields)
			{
				return Fail(State, Codes::ResourceLimitExceeded, Pointer, TEXT("Too many fields in an item copy row."));
			}
			Visited.Add(Current);
			FItemCopyStep Step{Current, Row, nullptr};
			const FString CopyPointer = AppendPointer(Pointer, TEXT("_copy"));
			const auto CopyValue = FindExactField(*Row, TEXT("_copy"));
			if (!CopyValue.IsValid())
			{
				if (Chain.IsEmpty())
				{
					return Fail(State, Codes::MissingRequiredField, CopyPointer, TEXT("Select an item with a copy declaration."));
				}
				Chain.Add(MoveTemp(Step));
				return true;
			}
			if (Chain.Num() >= MaxCopyHops)
			{
				return Fail(State, Codes::ResourceLimitExceeded, CopyPointer, TEXT("Too many links in the item copy chain."));
			}

			TSharedPtr<FJsonObject> Copy;
			if (!RequireObject(CopyValue, CopyPointer, Copy, State) ||
				!CheckAllowedKeys(*Copy, {TEXT("name"), TEXT("source"), TEXT("_preserve")}, CopyPointer, State) ||
				!ReadPreserveMap(*Copy, CopyPointer, Step.Preserve, State)) { return false; }

			FDnd5eRecordId TargetId;
			if (!TryReadItemIdentity(*Copy, State, TargetId))
			{
				return Fail(State, Codes::InvalidFieldValue, CopyPointer, TEXT("A copy target requires a name and explicit source."));
			}
			const int32* Target = Indices.Find(TargetId);
			if (!Target)
			{
				return Fail(State, Codes::UnresolvedReference, CopyPointer, TEXT("Copy target is missing from the item array."));
			}
			if (*Target == INDEX_NONE || Visited.Contains(*Target))
			{
				return Fail(State, Codes::InvalidFieldValue, CopyPointer, TEXT("Copy target is ambiguous or cyclic."));
			}

			Chain.Add(MoveTemp(Step));
			Current = *Target;
		}
	}

	bool IsProtectedCopyField(const FString& Key)
	{
		const TCHAR* const Protected[] = {
			TEXT("page"), TEXT("otherSources"), TEXT("referenceSources"),
			TEXT("srd"), TEXT("srd52"), TEXT("basicRules"), TEXT("basicRules2024"),
			TEXT("reprintedAs"), TEXT("hasFluff"), TEXT("hasFluffImages"),
			TEXT("hasToken"), TEXT("tokenCredit"), TEXT("tokenCustom"), TEXT("foundryTokenScale"),
			TEXT("altArt"), TEXT("_versions"), TEXT("lootTables"), TEXT("tier")
		};
		for (const TCHAR* Field : Protected)
		{
			if (Key.Equals(Field, ESearchCase::CaseSensitive)) { return true; }
		}
		return false;
	}

	bool IsPreserved(const TSharedPtr<FJsonObject>& Preserve, const TCHAR* Key)
	{
		if (!Preserve.IsValid()) { return false; }
		const auto IsTrue = [&Preserve](const TCHAR* Field)
		{
			const auto Value = FindExactField(*Preserve, Field);
			bool bValue = false;
			return Value.IsValid() && Value->TryGetBool(bValue) && bValue;
		};
		return IsTrue(TEXT("*")) || IsTrue(Key);
	}

	bool MergeCopyStep(const FItemCopyStep& Step, TSharedPtr<FJsonObject>& Parent,
		TMap<FString, int32>& FieldSources, FReadState& State)
	{
		auto Merged = MakeShared<FJsonObject>();
		TMap<FString, int32> NextSources;
		for (const auto& Pair : Step.Row->Values)
		{
			const FString Key(Pair.Key.Len(), *Pair.Key);
			if (Key.Equals(TEXT("_copy"), ESearchCase::CaseSensitive)) { continue; }
			Merged->SetField(Key, Pair.Value);
			NextSources.Add(Key, Step.Index);
		}

		if (Parent.IsValid())
		{
			for (const auto& Pair : Parent->Values)
			{
				const FString Key(Pair.Key.Len(), *Pair.Key);
				if (const auto Own = FindExactField(*Step.Row, *Key))
				{
					if (Own->Type == EJson::Null)
					{
						Merged->RemoveField(Key);
						NextSources.Remove(Key);
					}
					continue;
				}
				if (IsProtectedCopyField(Key) && !IsPreserved(Step.Preserve, *Key)) { continue; }

				if (NextSources.Contains(Key))
				{
					return Fail(State, Codes::InvalidFieldValue, ItemRowPointer(Step.Index),
						TEXT("Copy overlay has conflicting property names that differ only by case."));
				}
				Merged->SetField(Key, Pair.Value);
				NextSources.Add(Key, FieldSources.FindChecked(Key));
			}
		}
		if (NextSources.Num() > MaxCopyFields)
		{
			return Fail(State, Codes::ResourceLimitExceeded, ItemRowPointer(Step.Index),
				TEXT("Too many fields in an assembled item copy."));
		}

		Parent = MoveTemp(Merged);
		FieldSources = MoveTemp(NextSources);
		return true;
	}
}

FDnd5eJsonItemCopyAssemblyResult FDnd5eJsonItemCopyAssembler::AssembleAtIndex(const FDnd5eJsonDocument& Document, int32 Index)
{
	using namespace Dnd5eJsonReader;
	FDnd5eJsonItemCopyAssemblyResult Result;
	Result.Status = EDnd5eJsonReadStatus::Failure;
	FReadState State{Document.Context, Document.JsonText, Result.Diagnostics};

	TSharedPtr<FJsonValue> RootValue;
	TSharedPtr<FJsonObject> Root;
	const TArray<TSharedPtr<FJsonValue>>* Rows = nullptr;
	if (!ValidateReadContext(State) || !ValidateDocumentText(State) || !PreflightJson(State) ||
		!ParseJsonDocument(RootValue, State) || !RequireObject(RootValue, TEXT(""), Root, State) ||
		!ReadRequiredArray(*Root, TEXT("item"), TEXT(""), Rows, State)) { return Result; }

	if (!Rows->IsValidIndex(Index))
	{
		Fail(State, Codes::InvalidFieldValue, TEXT("/item"), TEXT("Select an existing item index."));
		return Result;
	}
	if (Rows->Num() > MaxItemRows)
	{
		Fail(State, Codes::ResourceLimitExceeded, TEXT("/item"), TEXT("Too many item rows to index."));
		return Result;
	}

	const auto Indices = IndexItemRows(*Rows, State);
	TArray<FItemCopyStep> Chain;
	if (!CollectCopyChain(*Rows, Index, Indices, Chain, State)) { return Result; }

	TSharedPtr<FJsonObject> Assembled;
	TMap<FString, int32> FieldSources;
	for (int32 Step = Chain.Num() - 1; Step >= 0; --Step)
	{
		if (!MergeCopyStep(Chain[Step], Assembled, FieldSources, State)) { return Result; }
	}

	auto Preview = MakeShared<FJsonObject>();
	const TArray<TSharedPtr<FJsonValue>> PreviewRows{MakeShared<FJsonValueObject>(Assembled)};
	Preview->SetArrayField(TEXT("item"), PreviewRows);
	FString Json;
	if (!SerializeValue(MakeShared<FJsonValueObject>(Preview), Json, ItemRowPointer(Index), State)) { return Result; }
	FReadState PreviewState{Document.Context, Json, Result.Diagnostics};
	if (!ValidateDocumentText(PreviewState)) { return Result; }

	Result.AssembledJson = MoveTemp(Json);
	Result.FieldSourceRows = MoveTemp(FieldSources);
	for (const auto& Step : Chain) { Result.SourceRowIndices.Add(Step.Index); }
	Result.Status = EDnd5eJsonReadStatus::Success;
	return Result;
}
