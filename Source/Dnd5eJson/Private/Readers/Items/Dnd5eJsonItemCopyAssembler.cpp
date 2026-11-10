// Developed by Neko Creative Technologies

#include "Readers/Items/Dnd5eJsonItemCopyAssembler.h"
#include "Items/Dnd5eItemIdentity.h"
#include "Readers/Items/Dnd5eJsonItemCopyModifiers.h"
#include "Readers/Items/Dnd5eJsonItemCopyPolicy.h"
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
		TSharedPtr<FJsonObject> Modifiers;
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
			FItemCopyStep Step{Current, Row, nullptr, nullptr};
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
				!CheckAllowedKeys(*Copy, {TEXT("name"), TEXT("source"), TEXT("_preserve"), TEXT("_mod")}, CopyPointer, State) ||
				!ReadPreserveMap(*Copy, CopyPointer, Step.Preserve, State)) { return false; }

			if (const auto Modifiers = FindExactField(*Copy, TEXT("_mod")))
			{
				if (!RequireObject(Modifiers, AppendPointer(CopyPointer, TEXT("_mod")), Step.Modifiers, State)) { return false; }
			}

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
		FItemCopyFieldOrigins& Origins, FReadState& State)
	{
		auto Merged = MakeShared<FJsonObject>();
		FItemCopyFieldOrigins NextOrigins;
		const FString RowPointer = ItemRowPointer(Step.Index);
		for (const auto& Pair : Step.Row->Values)
		{
			const FString Key(Pair.Key.Len(), *Pair.Key);
			if (Key.Equals(TEXT("_copy"), ESearchCase::CaseSensitive)) { continue; }
			Merged->SetField(Key, Pair.Value);
			NextOrigins.Rows.Add(Key, Step.Index);
			NextOrigins.Pointers.Add(Key, AppendPointer(RowPointer, Key));
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
						NextOrigins.Rows.Remove(Key);
						NextOrigins.Pointers.Remove(Key);
					}
					continue;
				}
				if (IsProtectedCopyField(Key) && !IsPreserved(Step.Preserve, *Key)) { continue; }

				if (NextOrigins.Rows.Contains(Key))
				{
					return Fail(State, Codes::InvalidFieldValue, ItemRowPointer(Step.Index),
						TEXT("Copy overlay has conflicting property names that differ only by case."));
				}
				Merged->SetField(Key, Pair.Value);
				NextOrigins.Rows.Add(Key, Origins.Rows.FindChecked(Key));
				NextOrigins.Pointers.Add(Key, Origins.Pointers.FindChecked(Key));
			}
		}
		if (Step.Modifiers.IsValid())
		{
			const FString ModPointer = AppendPointer(AppendPointer(ItemRowPointer(Step.Index), TEXT("_copy")), TEXT("_mod"));
			if (!ApplyItemCopyModifiers(*Step.Modifiers, ModPointer, Step.Index, *Merged, NextOrigins, State)) { return false; }
		}
		if (NextOrigins.Rows.Num() > MaxCopyFields)
		{
			return Fail(State, Codes::ResourceLimitExceeded, ItemRowPointer(Step.Index),
				TEXT("Too many fields in an assembled item copy."));
		}

		Parent = MoveTemp(Merged);
		Origins = MoveTemp(NextOrigins);
		return true;
	}

	struct FAssembledItemCopy
	{
		TSharedPtr<FJsonObject> Row;
		TArray<FItemCopyStep> Chain;
		FItemCopyFieldOrigins Origins;
	};

	bool BuildItemCopy(int32 Index, FAssembledItemCopy& Out, FReadState& State)
	{
		TSharedPtr<FJsonValue> RootValue;
		TSharedPtr<FJsonObject> Root;
		const TArray<TSharedPtr<FJsonValue>>* Rows = nullptr;
		if (!ValidateReadContext(State) || !ValidateDocumentText(State) || !PreflightJson(State) ||
			!ParseJsonDocument(RootValue, State) || !RequireObject(RootValue, TEXT(""), Root, State) ||
			!ReadRequiredArray(*Root, TEXT("item"), TEXT(""), Rows, State)) { return false; }

		if (!Rows->IsValidIndex(Index))
		{
			return Fail(State, Codes::InvalidFieldValue, TEXT("/item"), TEXT("Select an existing item index."));
		}
		if (Rows->Num() > MaxItemRows)
		{
			return Fail(State, Codes::ResourceLimitExceeded, TEXT("/item"), TEXT("Too many item rows to index."));
		}

		FAssembledItemCopy Candidate;
		const auto Indices = IndexItemRows(*Rows, State);
		if (!CollectCopyChain(*Rows, Index, Indices, Candidate.Chain, State)) { return false; }
		for (int32 Step = Candidate.Chain.Num() - 1; Step >= 0; --Step)
		{
			if (!MergeCopyStep(Candidate.Chain[Step], Candidate.Row, Candidate.Origins, State)) { return false; }
		}
		Out = MoveTemp(Candidate);
		return true;
	}

	FReadFieldOrigins BuildReadFieldOrigins(const FAssembledItemCopy& Assembly, const FString& SelectedPointer)
	{
		TMap<int32, FString> SourceIds;
		for (const auto& Step : Assembly.Chain)
		{
			SourceIds.Add(Step.Index, FindExactField(*Step.Row, TEXT("source"))->AsString());
		}

		FReadFieldOrigins Result;
		Result.RowPrefix = SelectedPointer + TEXT("/");
		for (const auto& Field : Assembly.Origins.Rows)
		{
			const FString LogicalPointer = AppendPointer(SelectedPointer, Field.Key);
			const FString& InputPointer = Assembly.Origins.Pointers.FindChecked(Field.Key);
			const bool bExact = InputPointer.Equals(AppendPointer(ItemRowPointer(Field.Value), Field.Key),
				ESearchCase::CaseSensitive);
			Result.Fields.Add(LogicalPointer, FReadFieldOrigin{
				LogicalPointer, InputPointer, SourceIds.FindChecked(Field.Value), bExact});
		}
		return Result;
	}

	void AssignCopyProvenance(const FAssembledItemCopy& Assembly, const FString& RelativePath, FDnd5eItem& Item)
	{
		auto& Provenance = Item.Header.Provenance;
		TMap<int32, int32> EntryBySourceRow;
		EntryBySourceRow.Add(Assembly.Chain[0].Index, 0);
		for (int32 Index = 1; Index < Assembly.Chain.Num(); ++Index)
		{
			const auto& Step = Assembly.Chain[Index];
			FDnd5eRecordProvenance Origin;
			Origin.SourceId = FindExactField(*Step.Row, TEXT("source"))->AsString();
			Origin.RelativeFilePath = RelativePath;
			Origin.JsonPointer = ItemRowPointer(Step.Index);
			EntryBySourceRow.Add(Step.Index, Provenance.Add(MoveTemp(Origin)));
		}

		if (const int32* PageRow = Assembly.Origins.Rows.Find(TEXT("page")))
		{
			const int32 Destination = EntryBySourceRow.FindChecked(*PageRow);
			if (Destination != 0)
			{
				Provenance[Destination].Page = MoveTemp(Provenance[0].Page);
				Provenance[0].Page = {};
			}
		}

		auto Memberships = MoveTemp(Provenance[0].Memberships);
		Provenance[0].Memberships.Reset();
		for (auto& Membership : Memberships)
		{
			const int32 Row = Assembly.Origins.Rows.FindChecked(Membership.MembershipId);
			Provenance[EntryBySourceRow.FindChecked(Row)].Memberships.Add(MoveTemp(Membership));
		}
	}
}

FDnd5eJsonItemCopyReadResult FDnd5eJsonItemCopyAssembler::ReadCopyAtIndex(const FDnd5eJsonDocument& Document, int32 Index)
{
	using namespace Dnd5eJsonReader;
	FDnd5eJsonItemCopyReadResult Result;
	Result.Status = EDnd5eJsonReadStatus::Failure;
	FReadState State{Document.Context, Document.JsonText, Result.Diagnostics};
	FAssembledItemCopy Assembly;
	if (!BuildItemCopy(Index, Assembly, State)) { return Result; }

	const FString Pointer = ItemRowPointer(Index);
	const FReadFieldOrigins FieldOrigins = BuildReadFieldOrigins(Assembly, Pointer);
	State.FieldOrigins = &FieldOrigins;
	FDnd5eItem Candidate;
	if (!ReadStandaloneItemRow(*Assembly.Row, Pointer, Candidate, State)) { return Result; }

	AssignCopyProvenance(Assembly, Document.Context.RelativeFilePath, Candidate);
	Result.Item = MoveTemp(Candidate);
	Result.FieldSourcePointers = MoveTemp(Assembly.Origins.Pointers);
	for (const auto& Step : Assembly.Chain) { Result.SourceRowIndices.Add(Step.Index); }
	Result.Status = EDnd5eJsonReadStatus::Success;
	return Result;
}

FDnd5eJsonItemCopyAssemblyResult FDnd5eJsonItemCopyAssembler::AssembleAtIndex(const FDnd5eJsonDocument& Document, int32 Index)
{
	using namespace Dnd5eJsonReader;
	FDnd5eJsonItemCopyAssemblyResult Result;
	Result.Status = EDnd5eJsonReadStatus::Failure;
	FReadState State{Document.Context, Document.JsonText, Result.Diagnostics};
	FAssembledItemCopy Assembly;
	if (!BuildItemCopy(Index, Assembly, State)) { return Result; }

	auto Preview = MakeShared<FJsonObject>();
	const TArray<TSharedPtr<FJsonValue>> PreviewRows{MakeShared<FJsonValueObject>(Assembly.Row)};
	Preview->SetArrayField(TEXT("item"), PreviewRows);
	FString Json;
	if (!SerializeValue(MakeShared<FJsonValueObject>(Preview), Json, ItemRowPointer(Index), State)) { return Result; }
	FReadState PreviewState{Document.Context, Json, Result.Diagnostics};
	if (!ValidateDocumentText(PreviewState) || !PreflightJson(PreviewState)) { return Result; }

	Result.AssembledJson = MoveTemp(Json);
	Result.FieldSourceRows = MoveTemp(Assembly.Origins.Rows);
	for (const auto& Step : Assembly.Chain) { Result.SourceRowIndices.Add(Step.Index); }
	Result.Status = EDnd5eJsonReadStatus::Success;
	return Result;
}
