// Developed by Neko Creative Technologies


#include "Spells/Dnd5eSpellContentLibrary.h"

#include "Dnd5eTags.h"
#include "UObject/UnrealType.h"

namespace
{
	struct FCollectionState
	{
		int32 Visited = 0;
		int32 Unsupported = 0;
		TArray<TPair<FString, const FDnd5eReferenceContent*>> Pending;
		TArray<FDnd5eDiagnostic>& Diagnostics;
	};

	bool Fail(FCollectionState& State, const TCHAR* Code, const TCHAR* Message)
	{
		FDnd5eDiagnostic Diagnostic;
		Diagnostic.Severity = EDnd5eDiagnosticSeverity::Error;
		Diagnostic.Code = Code; Diagnostic.Message = Message;
		State.Diagnostics.Add(MoveTemp(Diagnostic));
		return false;
	}

	bool IsContentStruct(const UScriptStruct* Type)
	{
		static const TSet<const UScriptStruct*> Types = {
			FDnd5eContentBlock::StaticStruct(),				FDnd5eInlineContent::StaticStruct(),		FDnd5eSectionContent::StaticStruct(),		FDnd5eParagraphContent::StaticStruct(),
			FDnd5eListContent::StaticStruct(),				FDnd5eListItemContent::StaticStruct(),		FDnd5eTableContent::StaticStruct(),			FDnd5eTableColumnContent::StaticStruct(),
			FDnd5eTableRowContent::StaticStruct(),			FDnd5eTableCellContent::StaticStruct(),		FDnd5eQuoteContent::StaticStruct(),			FDnd5eInsetContent::StaticStruct(),
			FDnd5eImageContent::StaticStruct(),				FDnd5eGalleryContent::StaticStruct(),		FDnd5eTextContent::StaticStruct(),			FDnd5eFormattedContent::StaticStruct(),
			FDnd5eReferenceContent::StaticStruct(),			FDnd5eAnnotationContent::StaticStruct(),	FDnd5eChanceContentValue::StaticStruct(),	FDnd5eNumberContent::StaticStruct(),
			FDnd5eExternalLinkContent::StaticStruct(),		FDnd5eAnchorLinkContent::StaticStruct(),	FDnd5eUnsupportedContent::StaticStruct(),	FDnd5eDocumentLinkContent::StaticStruct(),
			FDnd5eQuickReferenceContent::StaticStruct(),	FDnd5eQueryLinkContent::StaticStruct(),
		};
		return Types.Contains(Type);
	}

	UScriptStruct* ExpectedPayload(FGameplayTag Kind, bool bBlock)
	{
		if (bBlock)
		{
			if (Kind == Dnd5e::Content::Block::Section) { return FDnd5eSectionContent::StaticStruct(); }
			if (Kind == Dnd5e::Content::Block::Paragraph) { return FDnd5eParagraphContent::StaticStruct(); }
			if (Kind == Dnd5e::Content::Block::List) { return FDnd5eListContent::StaticStruct(); }
			if (Kind == Dnd5e::Content::Block::Table) { return FDnd5eTableContent::StaticStruct(); }
			if (Kind == Dnd5e::Content::Block::Quote) { return FDnd5eQuoteContent::StaticStruct(); }
			if (Kind == Dnd5e::Content::Block::Inset) { return FDnd5eInsetContent::StaticStruct(); }
			if (Kind == Dnd5e::Content::Block::Image) { return FDnd5eImageContent::StaticStruct(); }
			if (Kind == Dnd5e::Content::Block::Gallery) { return FDnd5eGalleryContent::StaticStruct(); }
			if (Kind == Dnd5e::Content::Block::Unsupported) { return FDnd5eUnsupportedContent::StaticStruct(); }
		}
		else
		{
			if (Kind == Dnd5e::Content::Inline::Text) { return FDnd5eTextContent::StaticStruct(); }
			if (Kind == Dnd5e::Content::Inline::Formatted) { return FDnd5eFormattedContent::StaticStruct(); }
			if (Kind == Dnd5e::Content::Inline::Reference) { return FDnd5eReferenceContent::StaticStruct(); }
			if (Kind == Dnd5e::Content::Inline::Annotation) { return FDnd5eAnnotationContent::StaticStruct(); }
			if (Kind == Dnd5e::Content::Inline::Number) { return FDnd5eNumberContent::StaticStruct(); }
			if (Kind == Dnd5e::Content::Inline::ExternalLink) { return FDnd5eExternalLinkContent::StaticStruct(); }
			if (Kind == Dnd5e::Content::Inline::AnchorLink) { return FDnd5eAnchorLinkContent::StaticStruct(); }
			if (Kind == Dnd5e::Content::Inline::Unsupported) { return FDnd5eUnsupportedContent::StaticStruct(); }
			if (Kind == Dnd5e::Content::Inline::DocumentLink) { return FDnd5eDocumentLinkContent::StaticStruct(); }
			if (Kind == Dnd5e::Content::Inline::QuickReference) { return FDnd5eQuickReferenceContent::StaticStruct(); }
			if (Kind == Dnd5e::Content::Inline::QueryLink) { return FDnd5eQueryLinkContent::StaticStruct(); }
		}
		return nullptr;
	}

	bool Visit(const UScriptStruct* Type, const void* Data, const FString& Path, int32 Depth, FCollectionState& State)
	{
		if (Depth > 64 || ++State.Visited > 10000)
		{
			return Fail(State, TEXT("Dnd5e.Content.References.Limit"), TEXT("Reference collection exceeded its depth or visited-struct limit."));
		}
		const FInstancedStruct* Payload = nullptr;
		UScriptStruct* Expected = nullptr;
		if (Type == FDnd5eContentBlock::StaticStruct())
		{
			const auto& Block = *static_cast<const FDnd5eContentBlock*>(Data);
			Payload = &Block.Payload; Expected = ExpectedPayload(Block.Kind, true);
		}
		else if (Type == FDnd5eInlineContent::StaticStruct())
		{
			const auto& Inline = *static_cast<const FDnd5eInlineContent*>(Data);
			Payload = &Inline.Payload; Expected = ExpectedPayload(Inline.Kind, false);
		}
		if (Payload)
		{
			if (!Expected || !Payload->IsValid() || Payload->GetScriptStruct() != Expected)
			{
				return Fail(State, TEXT("Dnd5e.Content.References.InvalidContent"), TEXT("A Content wrapper has an unknown kind, empty payload, or mismatched payload type."));
			}
			return Visit(Expected, Payload->GetMemory(), Path + TEXT("/Payload"), Depth + 1, State);
		}
		if (Type == FDnd5eUnsupportedContent::StaticStruct()) { ++State.Unsupported; return true; }
		if (Type == FDnd5eReferenceContent::StaticStruct())
		{
			if (State.Pending.Num() >= 1024)
			{
				return Fail(State, TEXT("Dnd5e.Content.References.Limit"), TEXT("Reference collection exceeded its occurrence limit."));
			}
			State.Pending.Emplace(Path, static_cast<const FDnd5eReferenceContent*>(Data));
		}
		if (Type == FDnd5eAnnotationContent::StaticStruct())
		{
			const auto& Annotation = *static_cast<const FDnd5eAnnotationContent*>(Data);
			if (Annotation.Kind == Dnd5e::Content::Annotation::PercentChance)
			{
				if (Annotation.Value.GetScriptStruct() != FDnd5eChanceContentValue::StaticStruct())
				{
					return Fail(State, TEXT("Dnd5e.Content.References.InvalidContent"), TEXT("A percent-chance annotation has a mismatched value payload."));
				}
				if (!Visit(FDnd5eChanceContentValue::StaticStruct(), Annotation.Value.GetMemory(), Path + TEXT("/Value"), Depth + 1, State)) { return false; }
			}
		}
		for (TFieldIterator<FProperty> It(Type); It; ++It)
		{
			const auto* Array = CastField<FArrayProperty>(*It);
			const auto* Element = Array ? CastField<FStructProperty>(Array->Inner) : nullptr;
			if (!Element || !IsContentStruct(Element->Struct)) { continue; }
			FScriptArrayHelper Values(Array, Array->ContainerPtrToValuePtr<void>(Data));
			const FString ArrayPath = Path + TEXT("/") + Array->GetName();
			for (int32 Index = 0; Index < Values.Num(); ++Index)
			{
				if (!Visit(Element->Struct, Values.GetRawPtr(Index), ArrayPath + TEXT("/") + FString::FromInt(Index), Depth + 1, State)) { return false; }
			}
		}
		return true;
	}

	bool VisitBlocks(const TArray<FDnd5eContentBlock>& Blocks, const TCHAR* Root, FCollectionState& State)
	{
		for (int32 Index = 0; Index < Blocks.Num(); ++Index)
		{
			if (!Visit(FDnd5eContentBlock::StaticStruct(), &Blocks[Index], FString(Root) + TEXT("/") + FString::FromInt(Index), 0, State)) { return false; }
		}
		return true;
	}
}

bool UDnd5eSpellContentLibrary::CollectSpellReferences(const FDnd5eSpell& Spell, TArray<FDnd5eContentReferenceOccurrence>& Occurrences,
	TArray<FDnd5eDiagnostic>& Diagnostics)
{
	Occurrences.Reset(); Diagnostics.Reset();
	FCollectionState State{0, 0, {}, Diagnostics};
	if (!VisitBlocks(Spell.Description, TEXT("/Description"), State) ||
		!VisitBlocks(Spell.HigherLevelDescription, TEXT("/HigherLevelDescription"), State)) { return false; }
	Occurrences.Reserve(State.Pending.Num());

	for (const auto& Item : State.Pending)
	{
		FDnd5eContentReferenceOccurrence Occurrence;
		Occurrence.ContentPath = Item.Key; Occurrence.Reference = *Item.Value;
		Occurrences.Add(MoveTemp(Occurrence));
	}
	if (State.Unsupported > 0)
	{
		FDnd5eDiagnostic Diagnostic;
		Diagnostic.Severity = EDnd5eDiagnosticSeverity::Warning;
		Diagnostic.Code = TEXT("Dnd5e.Content.References.OpaqueFallback");
		Diagnostic.Message = FString::Printf(TEXT("Reference collection left %d preserved unsupported Content payloads opaque."), State.Unsupported);
		Diagnostics.Add(MoveTemp(Diagnostic));
	}
	return true;
}
