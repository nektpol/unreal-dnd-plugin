// Developed by Neko Creative Technologies

#include "Readers/Items/Dnd5eJsonItemCatalogBuilder.h"
#include "Readers/Items/Dnd5eJsonBaseItemReader.h"
#include "Readers/Items/Dnd5eJsonItemCopyAssembler.h"
#include "Readers/Dnd5eJsonReaderInternal.h"

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

	TArray<FDnd5eDiagnostic> CatalogDiagnostics;
	const bool bReplaced = Catalog->ReplaceRecords(Items, Groups, CatalogDiagnostics);
	Result.Diagnostics.Append(MoveTemp(CatalogDiagnostics));
	if (!bReplaced) { return Result; }
	Result.RecordCount = Catalog->GetRecordCount();
	Result.Status = EDnd5eJsonReadStatus::Success;
	return Result;
}