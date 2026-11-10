// Developed by Neko Creative Technologies

#include "Readers/Items/Dnd5eJsonItemCatalogBuilder.h"
#include "Readers/Items/Dnd5eJsonBaseItemReader.h"
#include "Readers/Items/Dnd5eJsonItemCopyAssembler.h"
#include "Readers/Dnd5eJsonReaderInternal.h"
#include "Items/Dnd5eItemIdentity.h"
#include "Readers/Items/Dnd5eJsonItemSupplementReader.h"
#include "Readers/Items/Dnd5eJsonItemVariantGenerator.h"

namespace
{
	using namespace Dnd5eJsonReader;

	bool ReadPrimaryItemDocument(const FDnd5eJsonDocument& Document, TArray<FDnd5eItem>& Items,
		TArray<FDnd5eItemGroup>& Groups, TArray<FDnd5eDiagnostic>& Diagnostics)
	{
		FReadState State{Document.Context, Document.JsonText, Diagnostics};
		TSharedPtr<FJsonValue> RootValue;
		TSharedPtr<FJsonObject> Root;
		if (!ValidateReadContext(State) || !ValidateDocumentText(State) || !PreflightJson(State) ||
			!ParseJsonDocument(RootValue, State) || !RequireObject(RootValue, TEXT(""), Root, State)) { return false; }

		const auto ItemValue = FindExactField(*Root, TEXT("item"));
		const auto GroupValue = FindExactField(*Root, TEXT("itemGroup"));
		if (!ItemValue.IsValid() && !GroupValue.IsValid())
		{
			return Fail(State, Codes::MissingRequiredField, TEXT(""), TEXT("Supply an item or itemGroup array."));
		}
		const TArray<TSharedPtr<FJsonValue>>* ItemRows = nullptr;
		const TArray<TSharedPtr<FJsonValue>>* GroupRows = nullptr;
		if (ItemValue.IsValid() && !RequireArray(ItemValue, TEXT("/item"), ItemRows, State)) { return false; }
		if (GroupValue.IsValid() && !RequireArray(GroupValue, TEXT("/itemGroup"), GroupRows, State)) { return false; }
		if (int64(Items.Num()) + Groups.Num() + (ItemRows ? ItemRows->Num() : 0) + (GroupRows ? GroupRows->Num() : 0) > 100000)
		{
			return Fail(State, Codes::ResourceLimitExceeded, TEXT(""), TEXT("Item catalog build exceeds 100000 records."));
		}

		if (ItemRows)
		{
			for (int32 Index = 0; Index < ItemRows->Num(); ++Index)
			{
				const FString Pointer = AppendPointer(TEXT("/item"), FString::FromInt(Index));
				TSharedPtr<FJsonObject> Row;
				if (!RequireObject((*ItemRows)[Index], Pointer, Row, State)) { return false; }
				if (FindExactField(*Row, TEXT("_copy")).IsValid())
				{
					auto Copy = FDnd5eJsonItemCopyAssembler::ReadCopyAtIndex(Document, Index);
					Diagnostics.Append(MoveTemp(Copy.Diagnostics));
					if (Copy.Status != EDnd5eJsonReadStatus::Success) { return false; }
					Items.Add(MoveTemp(Copy.Item));
				}
				else
				{
					FDnd5eItem Item;
					if (!ReadStandaloneItemRow(*Row, Pointer, Item, State)) { return false; }
					Items.Add(MoveTemp(Item));
				}
			}
		}
		if (GroupRows)
		{
			for (int32 Index = 0; Index < GroupRows->Num(); ++Index)
			{
				const FString Pointer = AppendPointer(TEXT("/itemGroup"), FString::FromInt(Index));
				TSharedPtr<FJsonObject> Row;
				FDnd5eItemGroup Group;
				if (!RequireObject((*GroupRows)[Index], Pointer, Row, State) ||
					!ReadItemGroupRow(*Row, Pointer, Group, State)) { return false; }
				Groups.Add(MoveTemp(Group));
			}
		}
		return true;
	}

	bool FailItemBuild(TArray<FDnd5eDiagnostic>& Diagnostics, FName Code, const TCHAR* Message)
	{
		FDnd5eDiagnostic Diagnostic;
		Diagnostic.Severity = EDnd5eDiagnosticSeverity::Error;
		Diagnostic.Code = Code;
		Diagnostic.Message = Message;
		Diagnostics.Add(MoveTemp(Diagnostic));
		return false;
	}

	bool AppendSelectedVariants(const FDnd5eJsonItemCatalogBuildRequest& Request, TArray<FDnd5eItem>& Items,
		int32 GroupCount, TArray<FDnd5eDiagnostic>& Diagnostics)
	{
		if (int64(Items.Num()) + GroupCount + Request.VariantSelections.Num() > 100000)
		{
			return FailItemBuild(Diagnostics, Codes::ResourceLimitExceeded, TEXT("Item catalog build exceeds 100000 records."));
		}
		for (int32 Index = 0; Index < Request.VariantSelections.Num(); ++Index)
		{
			const auto& Selection = Request.VariantSelections[Index];
			if (!Request.VariantDocuments.IsValidIndex(Selection.VariantDocumentIndex) ||
				!Request.BaseDocuments.IsValidIndex(Selection.BaseDocumentIndex))
			{
				const FString Message = FString::Printf(TEXT("Variant selection %d refers to a missing request document."), Index);
				return FailItemBuild(Diagnostics, Codes::InvalidFieldValue, *Message);
			}
			auto Read = FDnd5eJsonItemVariantGenerator::ReadAtIndex(
				Request.VariantDocuments[Selection.VariantDocumentIndex], Selection.VariantIndex,
				Request.BaseDocuments[Selection.BaseDocumentIndex], Selection.BaseIndex, Selection.Policy);
			Diagnostics.Append(MoveTemp(Read.Diagnostics));
			if (Read.Status != EDnd5eJsonReadStatus::Success) { return false; }
			Items.Add(MoveTemp(Read.Item));
		}
		return true;
	}

	bool ReadItemBuildSupplements(const TArray<FDnd5eJsonDocument>& Documents,
		TMap<FDnd5eRecordId, FDnd5eJsonItemSupplement>& Out, TArray<FDnd5eDiagnostic>& Diagnostics)
	{
		for (const auto& Document : Documents)
		{
			auto Read = FDnd5eJsonItemSupplementReader::ReadDocument(Document);
			Diagnostics.Append(MoveTemp(Read.Diagnostics));
			if (Read.Status != EDnd5eJsonReadStatus::Success) { return false; }
			if (int64(Out.Num()) + Read.Rows.Num() > 100000)
			{
				FReadState State{Document.Context, Document.JsonText, Diagnostics};
				return Fail(State, Codes::ResourceLimitExceeded, TEXT("/itemFluff"), TEXT("Too many supplied supplement rows."));
			}
			for (auto& Row : Read.Rows)
			{
				const FDnd5eRecordId Key = Row.TargetItemId;
				if (Out.Contains(Key))
				{
					FReadState State{Document.Context, Document.JsonText, Diagnostics};
					return Fail(State, Codes::InvalidFieldValue, Row.Provenance[0].JsonPointer,
						TEXT("Duplicate supplement identity across supplied documents."));
				}
				Out.Add(Key, MoveTemp(Row));
			}
		}
		return true;
	}

	void AttachItemBuildSupplement(FDnd5eItem& Item, const TMap<FDnd5eRecordId, FDnd5eJsonItemSupplement>& Supplements)
	{
		const auto& Header = Item.Header;
		if (Header.Provenance.IsEmpty()) { return; }
		FDnd5eRecordId Key;
		if (!Dnd5e::Items::TryMakeId(Header.RecordId.DatasetNamespace, Header.DisplayName, Header.Provenance[0].SourceId, Key))
		{
			return;
		}
		const auto* Supplement = Supplements.Find(Key);
		if (!Supplement) { return; }
		FDnd5eItem Attached;
		if (FDnd5eJsonItemSupplementReader::TryApply(Item, *Supplement, Attached))
		{
			Item = MoveTemp(Attached);
		}
	}
}

FDnd5eJsonItemCatalogBuildResult UDnd5eJsonItemCatalogBuilder::BuildItemCatalog(
	const FDnd5eJsonItemCatalogBuildRequest& Request, UDnd5eItemCatalogObject* Catalog)
{
	using namespace Dnd5eJsonReader;
	FDnd5eJsonItemCatalogBuildResult Result;
	Result.Status = EDnd5eJsonReadStatus::Failure;
	if (!IsValid(Catalog))
	{
		FDnd5eDiagnostic Diagnostic;
		Diagnostic.Severity = EDnd5eDiagnosticSeverity::Error;
		Diagnostic.Code = TEXT("Dnd5e.Catalog.InvalidTarget");
		Diagnostic.Message = TEXT("Supply a valid item catalog object.");
		Result.Diagnostics.Add(MoveTemp(Diagnostic));
		return Result;
	}

	TArray<FDnd5eItem> Items;
	TArray<FDnd5eItemGroup> Groups;
	for (const auto& Document : Request.BaseDocuments)
	{
		auto Read = FDnd5eJsonBaseItemReader::Read(Document);
		Result.Diagnostics.Append(MoveTemp(Read.Diagnostics));
		if (Read.Status != EDnd5eJsonReadStatus::Success) { return Result; }
		if (int64(Items.Num()) + Read.Items.Num() > 100000)
		{
			FReadState State{Document.Context, Document.JsonText, Result.Diagnostics};
			Fail(State, Codes::ResourceLimitExceeded, TEXT("/baseitem"), TEXT("Item catalog build exceeds 100000 records."));
			return Result;
		}
		Items.Append(MoveTemp(Read.Items));
	}
	for (const auto& Document : Request.ItemDocuments)
	{
		if (!ReadPrimaryItemDocument(Document, Items, Groups, Result.Diagnostics)) { return Result; }
	}

	if (!AppendSelectedVariants(Request, Items, Groups.Num(), Result.Diagnostics)) { return Result; }
	if (!Request.SupplementDocuments.IsEmpty())
	{
		TMap<FDnd5eRecordId, FDnd5eJsonItemSupplement> Supplements;
		if (!ReadItemBuildSupplements(Request.SupplementDocuments, Supplements, Result.Diagnostics)) { return Result; }
		for (auto& Item : Items) { AttachItemBuildSupplement(Item, Supplements); }
		for (auto& Group : Groups) { AttachItemBuildSupplement(Group.Details, Supplements); }
	}

	TArray<FDnd5eDiagnostic> CatalogDiagnostics;
	const bool bReplaced = Catalog->ReplaceRecords(Items, Groups, CatalogDiagnostics);
	Result.Diagnostics.Append(MoveTemp(CatalogDiagnostics));
	if (!bReplaced) { return Result; }
	Result.RecordCount = Catalog->GetRecordCount();
	Result.Status = EDnd5eJsonReadStatus::Success;
	return Result;
}
