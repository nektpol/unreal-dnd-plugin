// Developed by Neko Creative Technologies

#include "Content/Dnd5eContentValidationLibrary.h"
#include "Dnd5eTags.h"
#include "Values/Dnd5eD20Roll.h"
#include "Values/Dnd5eScaledRoll.h"
#include "UObject/UnrealType.h"

namespace
{
	struct FValidationState
	{
		FDnd5eContentValidationResult& Result;
		const FDnd5eRecordId& Owner;
		int32 Visited = 0;
		int32 Opaque = 0;
	};
	bool Fail(FValidationState& State, const FString& Path, const TCHAR* Message)
	{
		FDnd5eContentValidationIssue Issue; Issue.OwnerRecordId = State.Owner; Issue.ContentPath = Path;
		Issue.Diagnostic.Severity = EDnd5eDiagnosticSeverity::Error;
		Issue.Diagnostic.Code = TEXT("Dnd5e.Content.Validation.Invalid"); Issue.Diagnostic.Message = Message;
		State.Result.Issues.Add(MoveTemp(Issue)); return false;
	}
	bool Identity(const FString& Value)
	{
		if (Value.TrimStartAndEnd().IsEmpty()) { return false; }
		for (int32 Index = 0; Index < Value.Len(); ++Index)
		{
			const uint32 Unit = static_cast<uint32>(Value[Index]);
			if (FChar::IsControl(Value[Index])) { return false; }
			if (Unit >= 0xD800 && Unit <= 0xDBFF)
			{
				if (++Index >= Value.Len()) { return false; }
				const uint32 Low = static_cast<uint32>(Value[Index]);
				if (Low < 0xDC00 || Low > 0xDFFF) { return false; }
			}
			else if (Unit >= 0xDC00 && Unit <= 0xDFFF) { return false; }
		}
		return true;
	}
	bool Number(const FDnd5eNumber& Value)
	{
		return Value.Kind == EDnd5eNumberKind::Integer ||
			(Value.Kind == EDnd5eNumberKind::Decimal && FMath::IsFinite(Value.Decimal)) ||
			(Value.Kind == EDnd5eNumberKind::Literal && Identity(Value.SourceLiteral));
	}
	bool Expression(const FDnd5eDiceExpression& Value) { return Identity(Value.Expression) && Identity(Value.Syntax); }
	bool SourcePair(const FString& Format, const FString& Fragment)
	{
		return (Format.IsEmpty() && Fragment.IsEmpty()) || (Identity(Format) && !Fragment.IsEmpty());
	}
	bool HttpUrl(const FString& Url)
	{
		const int32 Prefix = Url.StartsWith(TEXT("https://"), ESearchCase::IgnoreCase) ? 8 :
			(Url.StartsWith(TEXT("http://"), ESearchCase::IgnoreCase) ? 7 : 0);
		if (!Prefix || Url.Len() <= Prefix || Url[Prefix] == '/' || Url[Prefix] == '?' || Url[Prefix] == '#') { return false; }
		for (TCHAR Character : Url) { if (FChar::IsWhitespace(Character) || FChar::IsControl(Character)) { return false; } }
		return Identity(Url);
	}
	bool Media(const FDnd5eMediaReference& Value)
	{
		if (Value.Kind == EDnd5eMediaReferenceKind::DatasetRelative) { return Dnd5eMedia::IsDatasetRelativePath(Value.RelativePath); }
		return Value.Kind == EDnd5eMediaReferenceKind::ExternalUrl && HttpUrl(Value.Url);
	}
	bool Selector(const FDnd5eRecordSelector& Value)
	{
		if (!Value.Category.IsValid() || !Identity(Value.Name) || (Value.bHasSource && !Identity(Value.SourceId))) { return false; }
		if (Value.Category != Dnd5e::Category::ClassFeature) { return !Value.Qualifier.IsValid(); }
		const auto* Feature = Value.Qualifier.GetPtr<FDnd5eClassFeatureSelector>();
		return Feature && Identity(Feature->ClassName) && (!Feature->bHasClassSource || Identity(Feature->ClassSourceId)) && Feature->Level >= 1 && Feature->Level <= 20;
	}
	template<typename T> const T* As(const UScriptStruct* Type, const void* Data)
	{
		return Type == T::StaticStruct() ? static_cast<const T*>(Data) : nullptr;
	}
	bool IsContentStruct(const UScriptStruct* Type)
	{
		static const TSet<const UScriptStruct*> Types = {
			FDnd5eContentBlock::StaticStruct(), FDnd5eInlineContent::StaticStruct(),
			FDnd5eSectionContent::StaticStruct(), FDnd5eParagraphContent::StaticStruct(),
			FDnd5eListContent::StaticStruct(), FDnd5eListItemContent::StaticStruct(),
			FDnd5eTableContent::StaticStruct(), FDnd5eTableColumnContent::StaticStruct(),
			FDnd5eTableRowContent::StaticStruct(), FDnd5eTableCellContent::StaticStruct(),
			FDnd5eQuoteContent::StaticStruct(), FDnd5eInsetContent::StaticStruct(),
			FDnd5eImageContent::StaticStruct(), FDnd5eGalleryContent::StaticStruct(),
			FDnd5eTextContent::StaticStruct(), FDnd5eFormattedContent::StaticStruct(),
			FDnd5eReferenceContent::StaticStruct(), FDnd5eAnnotationContent::StaticStruct(),
			FDnd5eChanceContentValue::StaticStruct(),
			FDnd5eDocumentLinkContent::StaticStruct(), FDnd5eQuickReferenceContent::StaticStruct(), FDnd5eQueryLinkContent::StaticStruct(),
			FDnd5eNumberContent::StaticStruct(), FDnd5eExternalLinkContent::StaticStruct(),
			FDnd5eAnchorLinkContent::StaticStruct(), FDnd5eUnsupportedContent::StaticStruct()
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
	bool CheckLocal(const UScriptStruct* Type, const void* Data, const FString& Path, FValidationState& State)
	{
		auto Reject = [&](const TCHAR* Message) { return Fail(State, Path, Message); };
		if (const auto* X = As<FDnd5eUnsupportedContent>(Type, Data))
		{
			if (!Identity(X->SourceFormat) || X->SourceFragment.IsEmpty() ||
				(X->FragmentKind != EDnd5eSourceFragmentKind::Text && X->FragmentKind != EDnd5eSourceFragmentKind::Json)) { return Reject(TEXT("Unsupported content lacks its source evidence.")); }
			++State.Opaque;
		}
		if (const auto* X = As<FDnd5eReferenceContent>(Type, Data))
		{
			if (!SourcePair(X->SourceFormat, X->SourceReference) || (X->bHasTargetSelector && !Selector(X->TargetSelector))) { return Reject(TEXT("Invalid reference evidence or selector.")); }
			if (X->bHasTargetRecordId && (!Identity(X->TargetRecordId.DatasetNamespace) || !X->TargetRecordId.Category.IsValid() || !Identity(X->TargetRecordId.SourceKey))) { return Reject(TEXT("Active reference target ID is incomplete.")); }
		}
		if (const auto* X = As<FDnd5eFormattedContent>(Type, Data))
		{
			if (!SourcePair(X->SourceFormat, X->SourceReference)) { return Reject(TEXT("Formatting evidence is incomplete.")); }
		}
		if (const auto* X = As<FDnd5eExternalLinkContent>(Type, Data))
		{
			if (!HttpUrl(X->Url) || !SourcePair(X->SourceFormat, X->SourceReference)) { return Reject(TEXT("Invalid external link or evidence.")); }
		}
		if (const auto* X = As<FDnd5eAnchorLinkContent>(Type, Data))
		{
			if (!Identity(X->TargetAnchorId) || !SourcePair(X->SourceFormat, X->SourceReference)) { return Reject(TEXT("Invalid anchor link or evidence.")); }
		}
		if (const auto* X = As<FDnd5eNumberContent>(Type, Data)) { if (!Number(X->Value)) { return Reject(TEXT("Invalid number kind or active value.")); } }
		if (const auto* X = As<FDnd5eChanceContentValue>(Type, Data)) { if (X->Percent < 0 || X->Percent > 100) { return Reject(TEXT("Percent must be between zero and one hundred.")); } }
		if (const auto* X = As<FDnd5eListContent>(Type, Data))
		{
			if (X->StartIndex < 0 || (X->bHasColumnCount && X->ColumnCount <= 0)) { return Reject(TEXT("Invalid list start or supplied column count.")); }
		}
		if (const auto* X = As<FDnd5eTableColumnContent>(Type, Data))
		{
			if (X->bHasWidthWeight && (!FMath::IsFinite(X->WidthWeight) || X->WidthWeight <= 0)) { return Reject(TEXT("Supplied column weight must be finite and positive.")); }
		}
		if (const auto* X = As<FDnd5eTableCellContent>(Type, Data))
		{
			if (X->ColumnSpan < 1 || X->ColumnSpan > 4096) { return Reject(TEXT("Invalid table column span.")); }
			const auto& R = X->RollRange;
			if (X->bHasRollRange && ((!R.bMinimumUnbounded && !R.bMaximumUnbounded && R.Minimum > R.Maximum) || R.PadToDigits < 0)) { return Reject(TEXT("Invalid finite roll range or display padding.")); }
		}
		if (const auto* X = As<FDnd5eTableContent>(Type, Data))
		{
			if (X->Columns.Num() > 4096) { return Reject(TEXT("Too many table columns.")); }
			if (static_cast<int64>(X->HeaderRows.Num()) + X->Rows.Num() > 10000) { return Reject(TEXT("Too many table rows.")); }
			int32 Cells = 0;
			for (const auto* Rows : { &X->HeaderRows, &X->Rows })
			{
				for (const auto& Row : *Rows)
				{
					if (Row.Cells.Num() > 10000 - Cells) { return Reject(TEXT("Too many table cells.")); } Cells += Row.Cells.Num();
					int64 Width = 0;
					for (const auto& Cell : Row.Cells) { if (Cell.ColumnSpan < 1 || Cell.ColumnSpan > 4096) { return Reject(TEXT("Invalid table column span.")); } Width += Cell.ColumnSpan; }
					if (Width > X->Columns.Num()) { return Reject(TEXT("Table row exceeds declared columns.")); }
				}
			}
		}
		if (const auto* X = As<FDnd5eImageContent>(Type, Data))
		{
			if (!Media(X->Resource) || (X->bHasThumbnail && !Media(X->Thumbnail)) || (X->bHasWidth && X->Width <= 0) || (X->bHasHeight && X->Height <= 0)) { return Reject(TEXT("Invalid image locator or supplied dimension.")); }
		}
		if (const auto* X = As<FDnd5eGalleryContent>(Type, Data))
		{
			for (const auto& Image : X->Images) { if (Image.Kind != Dnd5e::Content::Block::Image && Image.Kind != Dnd5e::Content::Block::Unsupported) { return Reject(TEXT("Gallery contains a non-image block.")); } }
		}
		if (const auto* X = As<FDnd5eDocumentLinkContent>(Type, Data))
		{
			if ((X->DocumentKind != Dnd5e::Document::Kind::Book && X->DocumentKind != Dnd5e::Document::Kind::Adventure) || !Identity(X->DocumentId) ||
				(X->bHasChapter && X->ChapterIndex < 0) || (X->bHasNamedSectionIndex && X->NamedSectionIndex < 0) || !SourcePair(X->SourceFormat, X->SourceReference)) { return Reject(TEXT("Invalid document destination.")); }
		}
		if (const auto* X = As<FDnd5eQuickReferenceContent>(Type, Data))
		{
			if (!Identity(X->Name) || (X->bHasSource && !Identity(X->SourceId)) || (X->bHasChapter && X->ChapterIndex < 0) || !SourcePair(X->SourceFormat, X->SourceReference)) { return Reject(TEXT("Invalid quick-reference destination.")); }
		}
		if (const auto* X = As<FDnd5eQueryLinkContent>(Type, Data))
		{
			if (!Identity(X->TargetPage) || X->Clauses.IsEmpty() || X->Clauses.Num() > 10000 || !SourcePair(X->SourceFormat, X->SourceReference)) { return Reject(TEXT("Invalid query destination.")); }
			for (const auto& Clause : X->Clauses) { if (!Identity(Clause.Field) || !Identity(Clause.SourceClause)) { return Reject(TEXT("Query clause lacks its field or source evidence.")); } }
		}
		return true;
	}
	const UScriptStruct* AnnotationType(const FDnd5eAnnotationContent& X)
	{
		using namespace Dnd5e::Content::Annotation;
		if (!SourcePair(X.SourceFormat, X.SourceAnnotation)) { return nullptr; }
		if (X.Kind == DiceRoll || X.Kind == DamageRoll)
		{
			const auto* V = X.Value.GetPtr<FDnd5eDiceExpression>();
			return V && Expression(*V) ? FDnd5eDiceExpression::StaticStruct() : nullptr;
		}
		if (X.Kind == DifficultyClass)
		{
			const auto* V = X.Value.GetPtr<FDnd5eNumber>();
			return V && V->Kind == EDnd5eNumberKind::Integer && V->Integer >= 0 ? FDnd5eNumber::StaticStruct() : nullptr;
		}
		if (X.Kind == AttackRoll || X.Kind == D20Roll)
		{
			const auto* V = X.Value.GetPtr<FDnd5eD20Roll>();
			return V && V->DieSides == 20 && Number(V->Modifier) ? FDnd5eD20Roll::StaticStruct() : nullptr;
		}
		if (X.Kind == PercentChance) { return X.Value.GetScriptStruct() == FDnd5eChanceContentValue::StaticStruct() ? FDnd5eChanceContentValue::StaticStruct() : nullptr; }
		if (X.Kind == ScaledDiceRoll || X.Kind == ScaledDamageRoll)
		{
			const auto* V = X.Value.GetPtr<FDnd5eScaledRoll>();
			if (!V || V->BaseExpressions.IsEmpty() || V->BaseExpressions.Num() > 10000 || V->InputValues.IsEmpty() || V->InputValues.Num() > 10000 || !Expression(V->Increment)) { return nullptr; }
			const bool Slot = V->InputKind == Dnd5e::Scaling::Input::SpellSlotLevel;
			if (!Slot && V->InputKind != Dnd5e::Scaling::Input::PsiPoints) { return nullptr; }
			int32 Previous = 0;
			for (int32 Input : V->InputValues) { if (Input <= Previous || (Slot && Input > 9)) { return nullptr; } Previous = Input; }
			for (const auto& Base : V->BaseExpressions) { if (!Expression(Base)) { return nullptr; } }
			return FDnd5eScaledRoll::StaticStruct();
		}
		return nullptr;
	}
	bool Visit(const UScriptStruct* Type, const void* Data, const FString& Path, int32 Depth, FValidationState& State)
	{
		if (Depth > 64 || ++State.Visited > 10000) { return Fail(State, Path, TEXT("Content exceeds depth or visited-struct limit.")); }
		const FInstancedStruct* Payload = nullptr; UScriptStruct* Expected = nullptr;
		if (const auto* X = As<FDnd5eContentBlock>(Type, Data)) { Payload = &X->Payload; Expected = ExpectedPayload(X->Kind, true); }
		else if (const auto* Inline = As<FDnd5eInlineContent>(Type, Data)) { Payload = &Inline->Payload; Expected = ExpectedPayload(Inline->Kind, false); }
		if (Payload)
		{
			if (!Expected || Payload->GetScriptStruct() != Expected) { return Fail(State, Path, TEXT("Unknown Content kind or missing/mismatched payload.")); }
			return Visit(Expected, Payload->GetMemory(), Path + TEXT("/Payload"), Depth + 1, State);
		}
		if (!CheckLocal(Type, Data, Path, State)) { return false; }
		if (const auto* X = As<FDnd5eAnnotationContent>(Type, Data))
		{
			const UScriptStruct* ValueType = AnnotationType(*X);
			if (!ValueType) { return Fail(State, Path + TEXT("/Value"), TEXT("Annotation role/value is invalid.")); }
			if (ValueType == FDnd5eChanceContentValue::StaticStruct() && !Visit(ValueType, X->Value.GetMemory(), Path + TEXT("/Value"), Depth + 1, State)) { return false; }
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
	bool VisitBlocks(const TArray<FDnd5eContentBlock>& Blocks, const TCHAR* Root, FValidationState& State, bool ImagesOnly = false)
	{
		for (int32 Index = 0; Index < Blocks.Num(); ++Index)
		{
			const FString Path = FString(Root) + TEXT("/") + FString::FromInt(Index);
			if (ImagesOnly && Blocks[Index].Kind != Dnd5e::Content::Block::Image && Blocks[Index].Kind != Dnd5e::Content::Block::Unsupported) { return Fail(State, Path, TEXT("Supplementary image array contains another block kind.")); }
			if (!Visit(FDnd5eContentBlock::StaticStruct(), &Blocks[Index], Path, 0, State)) { return false; }
		}
		return true;
	}
}

FDnd5eContentValidationResult UDnd5eContentValidationLibrary::ValidateSpellContent(const FDnd5eSpell& Spell)
{
	FDnd5eContentValidationResult Result; FValidationState State{Result, Spell.Header.RecordId};
	if (!VisitBlocks(Spell.Description, TEXT("/Description"), State) || !VisitBlocks(Spell.HigherLevelDescription, TEXT("/HigherLevelDescription"), State)) { return Result; }
	if (Spell.bHasSupplementaryContent && (!VisitBlocks(Spell.SupplementaryContent.Entries, TEXT("/SupplementaryContent/Entries"), State) ||
		!VisitBlocks(Spell.SupplementaryContent.Images, TEXT("/SupplementaryContent/Images"), State, true))) { return Result; }
	Result.bValid = true;
	if (State.Opaque > 0)
	{
		FDnd5eContentValidationIssue Issue; Issue.OwnerRecordId = Spell.Header.RecordId;
		Issue.Diagnostic.Severity = EDnd5eDiagnosticSeverity::Warning; Issue.Diagnostic.Code = TEXT("Dnd5e.Content.Validation.OpaqueFallback");
		Issue.Diagnostic.Message = FString::Printf(TEXT("Validated structure includes %d preserved opaque Content leaves."), State.Opaque);
		Result.Issues.Add(MoveTemp(Issue));
	}
	return Result;
}
