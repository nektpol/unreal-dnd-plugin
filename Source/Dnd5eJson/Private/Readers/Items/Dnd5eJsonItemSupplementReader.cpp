// Developed by Neko Creative Technologies

#include "Readers/Items/Dnd5eJsonItemSupplementReader.h"
#include "Readers/Dnd5eJsonReaderInternal.h"
#include "Items/Dnd5eItemIdentity.h"
#include "Dnd5eTags.h"

namespace
{
	using namespace Dnd5eJsonReader;

	constexpr int32 MaxItemSupplementRows = 100000;
	constexpr int32 MaxItemSupplementCopyHops = 32;

	struct FItemSupplementSourceRow
	{
		TSharedPtr<FJsonObject> Object;
		FDnd5eRecordId TargetId;
		FDnd5eRecordProvenance Origin;
	};

	struct FItemSupplementField
	{
		TSharedPtr<FJsonValue> Value;
		int32 Row = INDEX_NONE;
	};

	bool ReadItemSupplementIdentity(const FJsonObject& Object, const FString& Pointer, FString& Source,
		FDnd5eRecordId& OutId, FReadState& State)
	{
		FString Name;
		if (!ReadRequiredString(Object, TEXT("name"), Pointer, Name, State) ||
			!ReadRequiredString(Object, TEXT("source"), Pointer, Source, State) ||
			!ValidateIdentityText(Name, AppendPointer(Pointer, TEXT("name")), State) ||
			!ValidateIdentityText(Source, AppendPointer(Pointer, TEXT("source")), State)) { return false; }
		if (!Dnd5e::Items::TryMakeId(State.Context.DatasetNamespace, Name, Source, OutId))
		{
			return Fail(State, Codes::InvalidFieldValue, Pointer, TEXT("Invalid supplementary item identity."));
		}
		return true;
	}

	bool IndexItemSupplementRows(const TArray<TSharedPtr<FJsonValue>>& Values, TArray<FItemSupplementSourceRow>& OutRows,
		TMap<FDnd5eRecordId, int32>& OutIndices, FReadState& State)
	{
		OutRows.Reserve(Values.Num());
		for (int32 Index = 0; Index < Values.Num(); ++Index)
		{
			FItemSupplementSourceRow Row;
			Row.Origin.RelativeFilePath = State.Context.RelativeFilePath;
			Row.Origin.JsonPointer = AppendPointer(TEXT("/itemFluff"), FString::FromInt(Index));
			const FString& Pointer = Row.Origin.JsonPointer;
			if (!RequireObject(Values[Index], Pointer, Row.Object, State) ||
				!CheckAllowedKeys(*Row.Object, {TEXT("name"), TEXT("source"), TEXT("entries"), TEXT("images"), TEXT("_copy")}, Pointer, State) ||
				!ReadItemSupplementIdentity(*Row.Object, Pointer, Row.Origin.SourceId, Row.TargetId, State)) { return false; }
			if (OutIndices.Contains(Row.TargetId))
			{
				return Fail(State, Codes::InvalidFieldValue, Pointer, TEXT("Duplicate supplementary item identity."));
			}
			OutIndices.Add(Row.TargetId, Index);
			OutRows.Add(MoveTemp(Row));
		}
		return true;
	}

	bool ResolveItemSupplementFields(int32 Index, const TArray<FItemSupplementSourceRow>& Rows,
		const TMap<FDnd5eRecordId, int32>& Indices, FItemSupplementField& Entries, FItemSupplementField& Images,
		TArray<FDnd5eRecordProvenance>& Provenance, FReadState& State)
	{
		TSet<int32> Seen;
		int32 Current = Index;
		for (;;)
		{
			Seen.Add(Current);
			const auto& Row = Rows[Current];
			Provenance.Add(Row.Origin);
			if (Entries.Row == INDEX_NONE)
			{
				if (const auto Value = FindExactField(*Row.Object, TEXT("entries")); Value.IsValid()) { Entries = {Value, Current}; }
			}
			if (Images.Row == INDEX_NONE)
			{
				if (const auto Value = FindExactField(*Row.Object, TEXT("images")); Value.IsValid()) { Images = {Value, Current}; }
			}
			const auto CopyValue = FindExactField(*Row.Object, TEXT("_copy"));
			if (!CopyValue.IsValid()) { return true; }
			const FString CopyPointer = AppendPointer(Row.Origin.JsonPointer, TEXT("_copy"));
			if (Seen.Num() > MaxItemSupplementCopyHops)
			{
				return Fail(State, Codes::ResourceLimitExceeded, CopyPointer, TEXT("Supplement copy chain exceeds its hop limit."));
			}
			TSharedPtr<FJsonObject> Copy;
			FDnd5eRecordId TargetId;
			FString TargetSource;
			if (!RequireObject(CopyValue, CopyPointer, Copy, State) ||
				!CheckAllowedKeys(*Copy, {TEXT("name"), TEXT("source")}, CopyPointer, State) ||
				!ReadItemSupplementIdentity(*Copy, CopyPointer, TargetSource, TargetId, State)) { return false; }
			const int32* Target = Indices.Find(TargetId);
			if (!Target) { return Fail(State, Codes::UnresolvedReference, CopyPointer, TEXT("Supplement copy target was not supplied.")); }
			if (Seen.Contains(*Target)) { return Fail(State, Codes::InvalidFieldValue, CopyPointer, TEXT("Supplement copy chain contains a cycle.")); }
			Current = *Target;
		}
	}

	bool ConvertItemSupplementRow(int32 Index, const TArray<FItemSupplementSourceRow>& Rows,
		const TMap<FDnd5eRecordId, int32>& Indices, FDnd5eJsonItemSupplement& Out, FReadState& State)
	{
		FItemSupplementField Entries, Images;
		if (!ResolveItemSupplementFields(Index, Rows, Indices, Entries, Images, Out.Provenance, State)) { return false; }
		FReadState Child = State;
		if (Entries.Value.IsValid() && Entries.Value->Type != EJson::Null)
		{
			const auto& Origin = Rows[Entries.Row].Origin;
			Child.SourceId = Origin.SourceId;
			if (!ReadBlocks(Entries.Value, AppendPointer(Origin.JsonPointer, TEXT("entries")), Out.Content.Entries, Child)) { return false; }
		}
		if (Images.Value.IsValid() && Images.Value->Type != EJson::Null)
		{
			const auto& Origin = Rows[Images.Row].Origin;
			Child.SourceId = Origin.SourceId;
			if (!ReadSupplementaryImages(Images.Value, AppendPointer(Origin.JsonPointer, TEXT("images")), Out.Content.Images, Child))
			{
				return false;
			}
		}
		Out.TargetItemId = Rows[Index].TargetId;
		return true;
	}
}

FDnd5eJsonItemSupplementReadResult FDnd5eJsonItemSupplementReader::ReadDocument(const FDnd5eJsonDocument& Document)
{
	using namespace Dnd5eJsonReader;
	FDnd5eJsonItemSupplementReadResult Result;
	Result.Status = EDnd5eJsonReadStatus::Failure;
	FReadState State{Document.Context, Document.JsonText, Result.Diagnostics};
	TSharedPtr<FJsonValue> RootValue;
	TSharedPtr<FJsonObject> Root;
	const TArray<TSharedPtr<FJsonValue>>* Values = nullptr;
	if (!ValidateReadContext(State) || !ValidateDocumentText(State) || !PreflightJson(State) || !ParseJsonDocument(RootValue, State) ||
		!RequireObject(RootValue, TEXT(""), Root, State) || !ReadRequiredArray(*Root, TEXT("itemFluff"), TEXT(""), Values, State))
	{
		return Result;
	}
	if (Values->Num() > MaxItemSupplementRows)
	{
		Fail(State, Codes::ResourceLimitExceeded, TEXT("/itemFluff"), TEXT("Too many supplementary item rows."));
		return Result;
	}

	TArray<FItemSupplementSourceRow> Rows;
	TMap<FDnd5eRecordId, int32> Indices;
	if (!IndexItemSupplementRows(*Values, Rows, Indices, State)) { return Result; }
	TArray<FDnd5eJsonItemSupplement> Candidate;
	Candidate.Reserve(Rows.Num());
	for (int32 Index = 0; Index < Rows.Num(); ++Index)
	{
		FDnd5eJsonItemSupplement Supplement;
		if (!ConvertItemSupplementRow(Index, Rows, Indices, Supplement, State)) { return Result; }
		Candidate.Add(MoveTemp(Supplement));
	}
	Result.Rows = MoveTemp(Candidate);
	Result.Status = EDnd5eJsonReadStatus::Success;
	return Result;
}

bool FDnd5eJsonItemSupplementReader::TryApply(const FDnd5eItem& Item, const FDnd5eJsonItemSupplement& Supplement, FDnd5eItem& OutItem)
{
	const auto& Header = Item.Header;
	const auto& Category = Header.RecordId.Category;
	FDnd5eRecordId JoinId;
	if ((Category != Dnd5e::Category::Item.GetTag() && Category != Dnd5e::Category::ItemGroup.GetTag()) ||
		Header.Provenance.IsEmpty() ||
		!Dnd5e::Items::TryMakeId(Header.RecordId.DatasetNamespace, Header.DisplayName, Header.Provenance[0].SourceId, JoinId) ||
		JoinId != Supplement.TargetItemId)
	{
		OutItem = {};
		return false;
	}
	FDnd5eItem Candidate = Item;
	Candidate.SupplementaryContent = Supplement.Content;
	Candidate.bHasSupplementaryContent = true;
	OutItem = MoveTemp(Candidate);
	return true;
}
