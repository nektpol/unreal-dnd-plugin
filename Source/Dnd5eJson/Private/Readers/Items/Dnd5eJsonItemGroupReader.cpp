// Developed by Neko Creative Technologies

#include "Readers/Items/Dnd5eJsonItemGroupReader.h"
#include "Items/Dnd5eItemIdentity.h"
#include "Readers/Dnd5eJsonReaderInternal.h"

namespace
{
	using namespace Dnd5eJsonReader;

	constexpr int32 MaxItemGroupRows = 100000;
	constexpr int32 MaxItemGroupMembers = 4096;

	bool ReadItemGroupMember(const TSharedPtr<FJsonValue>& Value, const FString& Pointer,
		FDnd5eItemGroupMember& Out, FReadState& State)
	{
		FString Uid;
		if (!Value.IsValid() || Value->Type != EJson::String || !Value->TryGetString(Uid))
		{
			return Fail(State, Codes::InvalidFieldValue, Pointer, TEXT("Expected an item group member UID string."));
		}

		FDnd5eItemGroupMember Member;
		Member.SourceUid = Uid;
		Member.bHasSource = Uid.Split(TEXT("|"), &Member.Name, &Member.SourceId, ESearchCase::CaseSensitive);
		if (!Member.bHasSource) { Member.Name = MoveTemp(Uid); }
		if (!IsValidIdentityText(Member.Name) ||
			(Member.bHasSource && (!IsValidIdentityText(Member.SourceId) || Member.SourceId.Contains(TEXT("|")))))
		{
			return Fail(State, Codes::InvalidFieldValue, Pointer, TEXT("Expected an item name or an explicit name|source UID."));
		}
		Out = MoveTemp(Member);
		return true;
	}
}

bool Dnd5eJsonReader::ReadItemGroupRow(const FJsonObject& Object, const FString& Pointer, FDnd5eItemGroup& Group, FReadState& State)
{
	FString Name, Source;
	if (!ReadRequiredString(Object, TEXT("name"), Pointer, Name, State) ||
		!ReadRequiredString(Object, TEXT("source"), Pointer, Source, State) ||
		!ValidateIdentityText(Name, AppendPointer(Pointer, TEXT("name")), State) ||
		!ValidateIdentityText(Source, AppendPointer(Pointer, TEXT("source")), State)) { return false; }
	FDnd5eRecordId GroupId;
	if (!Dnd5e::Items::TryMakeGroupId(State.Context.DatasetNamespace, Name, Source, GroupId))
	{
		return Fail(State, Codes::InvalidFieldValue, Pointer, TEXT("Invalid item group identity."));
	}

	const TArray<TSharedPtr<FJsonValue>>* Members = nullptr;
	const FString MembersPointer = AppendPointer(Pointer, TEXT("items"));
	if (!ReadRequiredArray(Object, TEXT("items"), Pointer, Members, State) ||
		!ReadOptionalBool(Object, TEXT("itemsHidden"), Pointer, Group.bItemsHidden, State)) { return false; }
	if (Members->Num() > MaxItemGroupMembers)
	{
		return Fail(State, Codes::ResourceLimitExceeded, MembersPointer, TEXT("Too many item group members."));
	}
	Group.Members.Reserve(Members->Num());
	for (int32 Index = 0; Index < Members->Num(); ++Index)
	{
		FDnd5eItemGroupMember Member;
		if (!ReadItemGroupMember((*Members)[Index], AppendPointer(MembersPointer, FString::FromInt(Index)), Member, State))
		{
			return false;
		}
		Group.Members.Add(MoveTemp(Member));
	}

	FJsonObject Details;
	Details.Values = Object.Values;
	Details.RemoveField(TEXT("items"));
	if (FindExactField(Object, TEXT("itemsHidden")).IsValid()) { Details.RemoveField(TEXT("itemsHidden")); }
	return ReadStandaloneItemRow(Details, Pointer, Group.Details, State, &GroupId);
}

FDnd5eJsonItemGroupReadResult FDnd5eJsonItemGroupReader::ReadAtIndex(const FDnd5eJsonDocument& Document, int32 Index)
{
	using namespace Dnd5eJsonReader;
	FDnd5eJsonItemGroupReadResult Result;
	Result.Status = EDnd5eJsonReadStatus::Failure;
	FReadState State{Document.Context, Document.JsonText, Result.Diagnostics};
	TSharedPtr<FJsonValue> RootValue;
	TSharedPtr<FJsonObject> Root;
	if (!ValidateReadContext(State) || !ValidateDocumentText(State) || !PreflightJson(State) ||
		!ParseJsonDocument(RootValue, State) || !RequireObject(RootValue, TEXT(""), Root, State)) { return Result; }

	const TArray<TSharedPtr<FJsonValue>>* Rows = nullptr;
	if (!ReadRequiredArray(*Root, TEXT("itemGroup"), TEXT(""), Rows, State)) { return Result; }
	if (Rows->Num() > MaxItemGroupRows)
	{
		Fail(State, Codes::ResourceLimitExceeded, TEXT("/itemGroup"), TEXT("Too many item group rows."));
		return Result;
	}
	if (!Rows->IsValidIndex(Index))
	{
		Fail(State, Codes::InvalidFieldValue, TEXT("/itemGroup"), TEXT("Select an existing item group index."));
		return Result;
	}
	const FString Pointer = AppendPointer(TEXT("/itemGroup"), FString::FromInt(Index));
	TSharedPtr<FJsonObject> Object;
	FDnd5eItemGroup Candidate;
	if (!RequireObject((*Rows)[Index], Pointer, Object, State) ||
		!ReadItemGroupRow(*Object, Pointer, Candidate, State)) { return Result; }

	Result.Group = MoveTemp(Candidate);
	Result.Status = EDnd5eJsonReadStatus::Success;
	return Result;
}
