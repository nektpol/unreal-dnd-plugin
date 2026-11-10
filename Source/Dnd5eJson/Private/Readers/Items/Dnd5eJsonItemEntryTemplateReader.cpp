// Developed by Neko Creative Technologies

#include "Readers/Items/Dnd5eJsonItemEntryTemplateReader.h"
#include "Items/Dnd5eItemIdentity.h"
#include "Readers/Dnd5eJsonReaderInternal.h"

namespace
{
	using namespace Dnd5eJsonReader;

	constexpr int32 MaxItemEntryTemplateRows = 100000;

	bool ReadItemEntryTemplateRow(const FJsonObject& Object, const FString& Pointer,
		FDnd5eItemEntryTemplate& Template, FReadState& State)
	{
		FString Name, Source;
		if (!ReadRequiredString(Object, TEXT("name"), Pointer, Name, State) ||
			!ReadRequiredString(Object, TEXT("source"), Pointer, Source, State) ||
			!ValidateIdentityText(Name, AppendPointer(Pointer, TEXT("name")), State) ||
			!ValidateIdentityText(Source, AppendPointer(Pointer, TEXT("source")), State)) { return false; }
		if (!Dnd5e::Items::TryMakeEntryTemplateId(State.Context.DatasetNamespace, Name, Source, Template.Header.RecordId))
		{
			return Fail(State, Codes::InvalidFieldValue, Pointer, TEXT("Invalid item entry template identity."));
		}

		TSharedPtr<FJsonValue> Entries;
		if (!ReadRequiredValue(Object, TEXT("entriesTemplate"), EJson::Array, Pointer, Entries, State) ||
			!SerializeValue(Entries, Template.EntriesTemplateJson, AppendPointer(Pointer, TEXT("entriesTemplate")), State))
		{
			return false;
		}

		FDnd5eRecordProvenance Origin;
		Origin.SourceId = MoveTemp(Source);
		Origin.RelativeFilePath = State.Context.RelativeFilePath;
		Origin.JsonPointer = Pointer;
		if (const auto Page = FindExactField(Object, TEXT("page")); Page.IsValid())
		{
			if (!ReadPage(Page, AppendPointer(Pointer, TEXT("page")), Origin.Page, State)) { return false; }
		}
		auto Metadata = MakeShared<FJsonObject>();
		Metadata->Values = Object.Values;
		for (const TCHAR* Key : {TEXT("name"), TEXT("source"), TEXT("entriesTemplate"), TEXT("page")})
		{
			if (FindExactField(Object, Key).IsValid()) { Metadata->RemoveField(Key); }
		}
		if (!Metadata->Values.IsEmpty() &&
			!SerializeValue(MakeShared<FJsonValueObject>(Metadata), Origin.AdditionalMetadataJson, Pointer, State)) { return false; }

		Template.Header.DisplayName = MoveTemp(Name);
		Template.Header.Provenance.Add(MoveTemp(Origin));
		return true;
	}
}

FDnd5eJsonItemEntryTemplateReadResult FDnd5eJsonItemEntryTemplateReader::ReadAtIndex(const FDnd5eJsonDocument& Document, int32 Index)
{
	using namespace Dnd5eJsonReader;
	FDnd5eJsonItemEntryTemplateReadResult Result;
	Result.Status = EDnd5eJsonReadStatus::Failure;
	FReadState State{Document.Context, Document.JsonText, Result.Diagnostics};
	TSharedPtr<FJsonValue> RootValue;
	TSharedPtr<FJsonObject> Root;
	if (!ValidateReadContext(State) || !ValidateDocumentText(State) || !PreflightJson(State) ||
		!ParseJsonDocument(RootValue, State) || !RequireObject(RootValue, TEXT(""), Root, State)) { return Result; }

	const TArray<TSharedPtr<FJsonValue>>* Rows = nullptr;
	if (!ReadRequiredArray(*Root, TEXT("itemEntry"), TEXT(""), Rows, State)) { return Result; }
	if (Rows->Num() > MaxItemEntryTemplateRows)
	{
		Fail(State, Codes::ResourceLimitExceeded, TEXT("/itemEntry"), TEXT("Too many item entry templates."));
		return Result;
	}
	if (!Rows->IsValidIndex(Index))
	{
		Fail(State, Codes::InvalidFieldValue, TEXT("/itemEntry"), TEXT("Select an existing item entry template index."));
		return Result;
	}
	const FString Pointer = AppendPointer(TEXT("/itemEntry"), FString::FromInt(Index));
	TSharedPtr<FJsonObject> Object;
	FDnd5eItemEntryTemplate Candidate;
	if (!RequireObject((*Rows)[Index], Pointer, Object, State) ||
		!ReadItemEntryTemplateRow(*Object, Pointer, Candidate, State)) { return Result; }

	Result.EntryTemplate = MoveTemp(Candidate);
	Result.Status = EDnd5eJsonReadStatus::Success;
	return Result;
}