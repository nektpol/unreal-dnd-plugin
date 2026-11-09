// Developed by Neko Creative Technologies

#include "Readers/Items/Dnd5eJsonItemReader.h"
#include "Readers/Dnd5eJsonReaderInternal.h"

FDnd5eJsonItemReadResult FDnd5eJsonItemReader::ReadAtIndex(const FDnd5eJsonDocument& Document, int32 Index)
{
	using namespace Dnd5eJsonReader;
	FDnd5eJsonItemReadResult Result;
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

	const TArray<TSharedPtr<FJsonValue>>* Rows = nullptr;
	if (!ReadRequiredArray(*Root, TEXT("item"), TEXT(""), Rows, State)) { return Result; }
	if (!Rows->IsValidIndex(Index))
	{
		Fail(State, Codes::InvalidFieldValue, TEXT("/item"), TEXT("Select an existing item index."));
		return Result;
	}

	const FString Pointer = AppendPointer(TEXT("/item"), FString::FromInt(Index));
	TSharedPtr<FJsonObject> Object;
	FDnd5eItem Candidate;
	if (!RequireObject((*Rows)[Index], Pointer, Object, State) ||
		!ReadStandaloneItemRow(*Object, Pointer, Candidate, State))
	{
		return Result;
	}

	Result.Item = MoveTemp(Candidate);
	Result.Status = EDnd5eJsonReadStatus::Success;
	return Result;
}