// Developed by Neko Creative Technologies

#include "Readers/Dnd5eJsonReaderInternal.h"
#include "Dnd5eTags.h"

namespace Dnd5eJsonReader
{
	namespace
	{
		template<typename T> void AppendDestination(FGameplayTag Kind, T&& Value, const FString& Pointer,
			TArray<FDnd5eInlineContent>& Inlines, FReadState& State)
		{
			FDnd5eInlineContent Inline; Inline.Kind = Kind; Inline.Payload = FInstancedStruct::Make<T>(MoveTemp(Value));
			Inline.Metadata = MakeContentMetadata(Pointer, State); Inlines.Add(MoveTemp(Inline));
		}
		bool OptionalIndex(const TArray<FString>& Parts, int32 Index, bool& Has, int32& Value)
		{
			Has = Index < Parts.Num() && !Parts[Index].TrimStartAndEnd().IsEmpty(); Value = -1;
			return !Has || TryParseBoundedIntegerToken(Parts[Index].TrimStartAndEnd(), 0, MAX_int32, Value);
		}
	}

	bool ReadDestination(const FString& Tag, const TArray<FString>& Parts, const FString& Token, const FString& Pointer,
		TArray<FDnd5eInlineContent>& Inlines, FReadState& State, int32 Depth, bool& Handled)
	{
		const bool Book = Tag.Equals(TEXT("book"), ESearchCase::CaseSensitive);
		const bool Adventure = Tag.Equals(TEXT("adventure"), ESearchCase::CaseSensitive);
		const bool Quick = Tag.Equals(TEXT("quickref"), ESearchCase::CaseSensitive);
		const bool Filter = Tag.Equals(TEXT("filter"), ESearchCase::CaseSensitive);
		Handled = Book || Adventure || Quick || Filter; if (!Handled) { return true; }
		auto Unsupported = [&]() { UnsupportedInline(Token, Pointer, Inlines, State); return true; };
		if (Book || Adventure)
		{
			if (Parts.Num() < 2 || Parts.Num() > 5 || !IsValidIdentityText(Parts[1].TrimStartAndEnd())) { return Unsupported(); }
			FDnd5eDocumentLinkContent Value; Value.DocumentId = Parts[1].TrimStartAndEnd();
			Value.DocumentKind = Book ? Dnd5e::Document::Kind::Book : Dnd5e::Document::Kind::Adventure;
			if (!OptionalIndex(Parts, 2, Value.bHasChapter, Value.ChapterIndex) || !OptionalIndex(Parts, 4, Value.bHasNamedSectionIndex, Value.NamedSectionIndex)) { return Unsupported(); }
			Value.SectionName = Parts.Num() > 3 ? Parts[3].TrimStartAndEnd() : FString{};
			Value.SourceFormat = TEXT("5etools"); Value.SourceReference = Token;
			if (!ReadInlines(Parts[0], Pointer, Value.Display, State, Depth + 1)) { return false; }
			AppendDestination(Dnd5e::Content::Inline::DocumentLink, MoveTemp(Value), Pointer, Inlines, State); return true;
		}
		if (Quick)
		{
			if (Parts.IsEmpty() || Parts.Num() > 5 || !IsValidIdentityText(Parts[0].TrimStartAndEnd())) { return Unsupported(); }
			FDnd5eQuickReferenceContent Value; Value.Name = Parts[0].TrimStartAndEnd();
			Value.SourceId = Parts.Num() > 1 ? Parts[1].TrimStartAndEnd() : FString{}; Value.bHasSource = !Value.SourceId.IsEmpty();
			if ((Value.bHasSource && !IsValidIdentityText(Value.SourceId)) || !OptionalIndex(Parts, 2, Value.bHasChapter, Value.ChapterIndex)) { return Unsupported(); }
			Value.HeaderLocator = Parts.Num() > 3 ? Parts[3].TrimStartAndEnd() : FString{};
			Value.SourceFormat = TEXT("5etools"); Value.SourceReference = Token;
			if (!ReadInlines(Parts.Num() > 4 && !Parts[4].IsEmpty() ? Parts[4] : Value.Name, Pointer, Value.Display, State, Depth + 1)) { return false; }
			AppendDestination(Dnd5e::Content::Inline::QuickReference, MoveTemp(Value), Pointer, Inlines, State); return true;
		}
		if (Parts.Num() < 3 || !IsValidIdentityText(Parts[1].TrimStartAndEnd())) { return Unsupported(); }
		FDnd5eQueryLinkContent Value; Value.TargetPage = Parts[1].TrimStartAndEnd();
		for (int32 Index = 2; Index < Parts.Num(); ++Index)
		{
			const int32 Equals = Parts[Index].Find(TEXT("="), ESearchCase::CaseSensitive);
			if (Equals <= 0 || !IsValidIdentityText(Parts[Index].Left(Equals).TrimStartAndEnd())) { return Unsupported(); }
			FDnd5eQueryClause Clause; Clause.Field = Parts[Index].Left(Equals).TrimStartAndEnd();
			Clause.RawValue = Parts[Index].Mid(Equals + 1); Clause.SourceClause = Parts[Index]; Value.Clauses.Add(MoveTemp(Clause));
		}
		for (int32 Index = 0; Index < Value.Clauses.Num(); ++Index) { if (!ConsumeContentBudget(Pointer, State)) { return false; } }
		Value.SourceFormat = TEXT("5etools"); Value.SourceReference = Token;
		if (!ReadInlines(Parts[0], Pointer, Value.Display, State, Depth + 1)) { return false; }
		AppendDestination(Dnd5e::Content::Inline::QueryLink, MoveTemp(Value), Pointer, Inlines, State); return true;
	}
}
