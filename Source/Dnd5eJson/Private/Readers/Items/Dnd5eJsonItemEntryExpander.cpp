// Developed by Neko Creative Technologies

#include "Readers/Items/Dnd5eJsonItemEntryExpander.h"
#include "Readers/Items/Dnd5eJsonItemEntryStrings.h"
#include "Content/Dnd5eContentLibrary.h"
#include "Items/Dnd5eItemIdentity.h"
#include "Dnd5eTags.h"

namespace
{
	using namespace Dnd5eJsonReader;

	constexpr int32 MaxTemplateStringLength = 65536;
	constexpr int32 MaxTemplateTokensPerString = 256;
	constexpr int64 MaxTemplateOutputCharacters = 2 * 1024 * 1024;

	const FDnd5eItemEntryTemplate* FindExpansionTemplate(const FDnd5eJsonItemEntryExpansionRequest& Request,
		const FString& Pointer, FReadState& State)
	{
		FDnd5eReferenceContent Reference;
		if (!UDnd5eContentLibrary::TryGetReference(Request.Reference, Reference) || !Reference.bHasTargetSelector ||
			Reference.TargetSelector.Category != Dnd5e::Category::ItemEntry.GetTag() || Reference.TargetSelector.Qualifier.IsValid())
		{
			Fail(State, Codes::InvalidFieldValue, Pointer, TEXT("Supply an item entry template reference."));
			return nullptr;
		}
		const auto& Selector = Reference.TargetSelector;
		const FString& Source = Selector.bHasSource ? Selector.SourceId : Request.OmittedSourceId;
		if (Source.IsEmpty())
		{
			Fail(State, Codes::UnresolvedReference, Pointer, TEXT("The template selector has no publication and no omitted-source policy."));
			return nullptr;
		}
		FDnd5eRecordId TargetId;
		if (!Dnd5e::Items::TryMakeEntryTemplateId(Request.DatasetNamespace, Selector.Name, Source, TargetId) ||
			(Reference.bHasTargetRecordId && Reference.TargetRecordId != TargetId))
		{
			Fail(State, Codes::InvalidFieldValue, Pointer, TEXT("The template selector and canonical identity disagree."));
			return nullptr;
		}
		if (Request.Templates.Num() > 4096)
		{
			Fail(State, Codes::ResourceLimitExceeded, Pointer, TEXT("Too many supplied templates."));
			return nullptr;
		}
		const FDnd5eItemEntryTemplate* Found = nullptr;
		for (const auto& Template : Request.Templates)
		{
			if (Template.Header.RecordId != TargetId) { continue; }
			if (Found)
			{
				Fail(State, Codes::InvalidFieldValue, Pointer, TEXT("The template identity is ambiguous."));
				return nullptr;
			}
			Found = &Template;
		}
		if (!Found) { Fail(State, Codes::UnresolvedReference, Pointer, TEXT("The selected template was not supplied.")); }
		return Found;
	}

	bool RenderTemplateDamageNames(const FDnd5eJsonItemEntryExpansionRequest& Request, bool bFormatted,
		const FString& Pointer, FString& Out, FReadState& State)
	{
		if (!Request.bHasResistances)
		{
			return Fail(State, Codes::MissingRequiredField, Pointer, TEXT("The template needs the item's resistance values."));
		}
		const TCHAR* const DamageNames[] = {TEXT("acid"), TEXT("bludgeoning"), TEXT("cold"), TEXT("fire"), TEXT("force"),
			TEXT("lightning"), TEXT("necrotic"), TEXT("piercing"), TEXT("poison"), TEXT("psychic"),
			TEXT("radiant"), TEXT("slashing"), TEXT("thunder")};
		if (Request.Resistances.Num() > 128)
		{
			return Fail(State, Codes::ResourceLimitExceeded, Pointer, TEXT("Too many resistance values for a flat template."));
		}
		TArray<FString> Names;
		bool bFullOrderedList = Request.Resistances.Num() == UE_ARRAY_COUNT(DamageNames);
		for (int32 Index = 0; Index < Request.Resistances.Num(); ++Index)
		{
			FString Name = Request.Resistances[Index];
			bool bKnown = false;
			for (const TCHAR* DamageName : DamageNames) { bKnown |= Name.Equals(DamageName, ESearchCase::CaseSensitive); }
			if (!bKnown) { return Fail(State, Codes::UnsupportedRequiredValue, Pointer, TEXT("Expected a canonical flat damage name.")); }
			if (bFullOrderedList) { bFullOrderedList = Name.Equals(DamageNames[Index], ESearchCase::CaseSensitive); }
			if (bFormatted && Request.bTitleCaseDamageNames) { Name[0] = FChar::ToUpper(Name[0]); }
			Names.Add(MoveTemp(Name));
		}
		if (bFormatted && bFullOrderedList)
		{
			Out = Request.bTitleCaseDamageNames ? TEXT("All Damage") : TEXT("all damage");
		}
		else { Out = FString::Join(Names, bFormatted ? TEXT(", ") : TEXT(",")); }
		return true;
	}

	bool ResolveEntryTemplateToken(const FString& Token, const FDnd5eJsonItemEntryExpansionRequest& Request,
		const FString& Pointer, FString& Out, FReadState& State)
	{
		TArray<FString> Parts;
		Token.ParseIntoArrayWS(Parts);
		if (Parts.Num() == 1)
		{
			if (Parts[0].Equals(TEXT("item.detail1"), ESearchCase::CaseSensitive) && Request.bHasDetail1) { Out = Request.Detail1; return true; }
			if (Parts[0].Equals(TEXT("item.detail2"), ESearchCase::CaseSensitive) && Request.bHasDetail2) { Out = Request.Detail2; return true; }
			if (Parts[0].Equals(TEXT("item.resist"), ESearchCase::CaseSensitive))
			{
				return RenderTemplateDamageNames(Request, false, Pointer, Out, State);
			}
			if (Parts[0].Equals(TEXT("item.detail1"), ESearchCase::CaseSensitive) || Parts[0].Equals(TEXT("item.detail2"), ESearchCase::CaseSensitive))
			{
				return Fail(State, Codes::MissingRequiredField, Pointer, TEXT("The template needs an item detail string."));
			}
		}
		if (Parts.Num() == 2 && Parts[0].Equals(TEXT("getFullImmRes"), ESearchCase::CaseSensitive) &&
			Parts[1].Equals(TEXT("item.resist"), ESearchCase::CaseSensitive))
		{
			return RenderTemplateDamageNames(Request, true, Pointer, Out, State);
		}
		return Fail(State, Codes::UnsupportedRequiredValue, Pointer, TEXT("Unsupported reusable entry template token."));
	}

	bool RenderEntryTemplateString(const FString& Text, const FDnd5eJsonItemEntryExpansionRequest& Request,
		const FString& Pointer, FString& Out, int64& TotalCharacters, FReadState& State)
	{
		FString Candidate;
		int32 Cursor = 0, Tokens = 0;
		for (;;)
		{
			const int32 Open = Cursor < Text.Len()
				? Text.Find(TEXT("{{"), ESearchCase::CaseSensitive, ESearchDir::FromStart, Cursor) : INDEX_NONE;
			if (Open == INDEX_NONE)
			{
				if (int64(Candidate.Len()) + Text.Len() - Cursor > MaxTemplateStringLength)
				{
					return Fail(State, Codes::ResourceLimitExceeded, Pointer, TEXT("Expanded template string is too long."));
				}
				Candidate += Text.Mid(Cursor);
				break;
			}
			const int32 Close = Text.Find(TEXT("}}"), ESearchCase::CaseSensitive, ESearchDir::FromStart, Open + 2);
			if (Close == INDEX_NONE) { return Fail(State, Codes::InvalidFieldValue, Pointer, TEXT("Unclosed reusable entry token.")); }
			if (++Tokens > MaxTemplateTokensPerString || Close - Open > 256)
			{
				return Fail(State, Codes::ResourceLimitExceeded, Pointer, TEXT("Reusable entry token limit exceeded."));
			}
			FString Replacement;
			if (!ResolveEntryTemplateToken(Text.Mid(Open + 2, Close - Open - 2), Request, Pointer, Replacement, State)) { return false; }
			if (int64(Candidate.Len()) + Open - Cursor + Replacement.Len() > MaxTemplateStringLength)
			{
				return Fail(State, Codes::ResourceLimitExceeded, Pointer, TEXT("Expanded template string is too long."));
			}
			Candidate += Text.Mid(Cursor, Open - Cursor);
			Candidate += Replacement;
			Cursor = Close + 2;
		}
		TotalCharacters += Candidate.Len();
		if (TotalCharacters > MaxTemplateOutputCharacters)
		{
			return Fail(State, Codes::ResourceLimitExceeded, Pointer, TEXT("Expanded template output exceeds its total budget."));
		}
		Out = MoveTemp(Candidate);
		return true;
	}
}

FDnd5eJsonItemEntryExpansionResult FDnd5eJsonItemEntryExpander::Expand(const FDnd5eJsonItemEntryExpansionRequest& Request)
{
	using namespace Dnd5eJsonReader;
	FDnd5eJsonItemEntryExpansionResult Result;
	Result.Status = EDnd5eJsonReadStatus::Failure;
	FDnd5eJsonReadContext ReferenceContext;
	ReferenceContext.DatasetNamespace = Request.DatasetNamespace;
	ReferenceContext.RelativeFilePath = Request.Reference.Metadata.RelativeFilePath;
	const FString EmptyJson;
	FReadState LookupState{ReferenceContext, EmptyJson, Result.Diagnostics};
	if (!ValidateReadContext(LookupState)) { return Result; }
	const FString& ReferencePointer = Request.Reference.Metadata.JsonPointer;
	const auto* Template = FindExpansionTemplate(Request, ReferencePointer, LookupState);
	if (!Template) { return Result; }
	if (Template->Header.Provenance.IsEmpty())
	{
		Fail(LookupState, Codes::InvalidFieldValue, ReferencePointer, TEXT("The supplied template needs its original provenance."));
		return Result;
	}
	const auto& Origin = Template->Header.Provenance[0];
	FDnd5eRecordId ExpectedId;
	if (!Dnd5e::Items::TryMakeEntryTemplateId(Request.DatasetNamespace, Template->Header.DisplayName, Origin.SourceId, ExpectedId) ||
		ExpectedId != Template->Header.RecordId)
	{
		Fail(LookupState, Codes::InvalidFieldValue, ReferencePointer, TEXT("The supplied template header and identity disagree."));
		return Result;
	}

	FDnd5eJsonReadContext TemplateContext;
	TemplateContext.DatasetNamespace = Request.DatasetNamespace;
	TemplateContext.RelativeFilePath = Origin.RelativeFilePath;
	FReadState State{TemplateContext, Template->EntriesTemplateJson, Result.Diagnostics};
	State.SourceId = Origin.SourceId;
	TSharedPtr<FJsonValue> Root;
	const TArray<TSharedPtr<FJsonValue>>* Entries = nullptr;
	const FString Pointer = AppendPointer(Origin.JsonPointer, TEXT("entriesTemplate"));
	if (!ValidateReadContext(State)) { return Result; }
	const int32 FirstBodyDiagnostic = Result.Diagnostics.Num();
	if (!ValidateDocumentText(State) || !PreflightJson(State) || !ParseJsonDocument(Root, State))
	{
		for (int32 Index = FirstBodyDiagnostic; Index < Result.Diagnostics.Num(); ++Index)
		{
			auto& Diagnostic = Result.Diagnostics[Index];
			Diagnostic.JsonPointer = Pointer + Diagnostic.JsonPointer;
		}
		return Result;
	}
	if (!RequireArray(Root, Pointer, Entries, State)) { return Result; }

	int64 TotalCharacters = 0;
	TSharedPtr<FJsonValue> Rendered;
	if (!TransformItemEntryStrings(Root, Pointer, Rendered, State,
		[&Request, &TotalCharacters](const FString& Text, const FString& Location, FString& Out, FReadState& ReadState)
		{
			return RenderEntryTemplateString(Text, Request, Location, Out, TotalCharacters, ReadState);
		})) { return Result; }
	TArray<FDnd5eContentBlock> Blocks;
	if (!ReadBlocks(Rendered, Pointer, Blocks, State)) { return Result; }
	Result.Blocks = MoveTemp(Blocks);
	Result.TemplateId = Template->Header.RecordId;
	Result.TemplateOrigin = Origin;
	Result.Status = EDnd5eJsonReadStatus::Success;
	return Result;
}
