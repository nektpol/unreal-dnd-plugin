#include "Readers/Dnd5eJsonReaderInternal.h"

#include "Dnd5eTags.h"
#include "String/LexFromString.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

namespace Dnd5eJsonReader
{
	bool ConsumeContentBudget(const FString& Pointer, FReadState& State)
	{
		return ++State.Budget->ContentValues <= 100000 || Fail(State, Codes::ResourceLimitExceeded, Pointer, TEXT("Converted Content exceeds the 100000 entry/inline fragment budget."));
	}

	bool CheckContentDepth(int32 Depth, const FString& Pointer, FReadState& State)
	{
		return Depth <= 64 || Fail(State, Codes::ResourceLimitExceeded, Pointer, TEXT("Content/inline nesting exceeds the conversion depth limit."));
	}

	bool SerializeValue(const TSharedPtr<FJsonValue>& Value, FString& OutJson, const FString& Pointer, FReadState& State)
	{
		FString Json;
		if (!FJsonSerializer::Serialize(Value, TEXT(""), TJsonWriterFactory<>::Create(&Json)))
		{
			return Fail(State, Codes::SerializationFailure, Pointer, TEXT("Could not retain the complete unsupported JSON value."));
		}
		OutJson = MoveTemp(Json);
		return true;
	}

	template<typename T>
	FDnd5eContentBlock MakeBlock(FGameplayTag Kind, const T& Payload, const FDnd5eContentMetadata& Metadata)
	{
		FDnd5eContentBlock Block;
		Block.Kind = Kind;
		Block.Payload = FInstancedStruct::Make<T>(Payload);
		Block.Metadata = Metadata;
		return Block;
	}

	template<typename T>
	FDnd5eInlineContent MakeInline(FGameplayTag Kind, const T& Payload, const FDnd5eContentMetadata& Metadata)
	{
		FDnd5eInlineContent Inline;
		Inline.Kind = Kind;
		Inline.Payload = FInstancedStruct::Make<T>(Payload);
		Inline.Metadata = Metadata;
		return Inline;
	}

	bool HasOnlyKeys(const FJsonObject& Object, std::initializer_list<const TCHAR*> Keys)
	{
		for (const auto& Pair : Object.Values)
		{
			const FString Key(Pair.Key.Len(), *Pair.Key);
			bool bKnown = false;
			for (const TCHAR* Known : Keys) { if (Key.Equals(Known, ESearchCase::CaseSensitive)) { bKnown = true; break; } }
			if (!bKnown) { return false; }
		}
		return true;
	}

	bool ReadNodeMetadata(const FJsonObject& Object, const FString& Pointer, FDnd5eContentMetadata& Metadata, FReadState& State)
	{
		if (const auto Source = FindExactField(Object, TEXT("source")))
		{
			if (Source->Type != EJson::String || !Source->TryGetString(Metadata.SourceId)) { return Fail(State, Codes::InvalidFieldValue, AppendPointer(Pointer, TEXT("source")), TEXT("Expected source text.")); }
		}
		if (const auto Page = FindExactField(Object, TEXT("page")))
		{
			if (!ReadPage(Page, AppendPointer(Pointer, TEXT("page")), Metadata.Page, State)) { return false; }
		}
		if (const auto Id = FindExactField(Object, TEXT("id")))
		{
			if (Id->Type != EJson::String || !Id->TryGetString(Metadata.AnchorId)) { return Fail(State, Codes::InvalidFieldValue, AppendPointer(Pointer, TEXT("id")), TEXT("Expected an anchor identifier string.")); }
		}
		return true;
	}

	bool PreservePresentation(const FJsonObject& Object, const TCHAR* Key, FDnd5eContentMetadata& Metadata, const FString& Pointer, FReadState& State)
	{
		const auto Value = FindExactField(Object, Key);
		if (!Value.IsValid()) { return true; }
		TSharedRef<FJsonObject> Evidence = MakeShared<FJsonObject>();
		Evidence->SetField(Key, Value);
		if (!SerializeValue(MakeShared<FJsonValueObject>(Evidence), Metadata.AdditionalMetadataJson, Pointer, State)) { return false; }
		AddDiagnostic(State, EDnd5eDiagnosticSeverity::Warning, Codes::PreservedUnsupportedMetadata, AppendPointer(Pointer, Key), TEXT("Presentation metadata is retained without a complete typed layout mapping."));
		return true;
	}

	bool UnsupportedBlock(const TSharedPtr<FJsonValue>& Value, const FString& Pointer, FDnd5eContentBlock& OutBlock, FReadState& State)
	{
		FDnd5eUnsupportedContent Unsupported;
		Unsupported.SourceFormat = TEXT("5etools");
		Unsupported.FragmentKind = EDnd5eSourceFragmentKind::Json;
		if (!SerializeValue(Value, Unsupported.SourceFragment, Pointer, State)) { return false; }
		auto Metadata = MakeContentMetadata(Pointer, State);
		if (Value->Type == EJson::Object)
		{
			const auto Object = Value->AsObject();
			if (const auto Type = FindExactField(*Object, TEXT("type")))
			{
				if (Type->Type == EJson::String) { Type->TryGetString(Metadata.SourceType); }
			}
			if (!ReadNodeMetadata(*Object, Pointer, Metadata, State)) { return false; }
		}
		OutBlock = MakeBlock(Dnd5e::Content::Block::Unsupported, Unsupported, Metadata);
		AddDiagnostic(State, EDnd5eDiagnosticSeverity::Warning, Codes::PreservedUnsupportedContent, Pointer, TEXT("The complete unmodeled Content object is retained as JSON."));
		return true;
	}

	void UnsupportedInline(const FString& Token, const FString& Pointer, TArray<FDnd5eInlineContent>& Inlines, FReadState& State)
	{
		FDnd5eUnsupportedContent Unsupported;
		Unsupported.SourceFormat = TEXT("5etools");
		Unsupported.FragmentKind = EDnd5eSourceFragmentKind::Text;
		Unsupported.SourceFragment = Token;
		Inlines.Add(MakeInline(Dnd5e::Content::Inline::Unsupported, Unsupported, MakeContentMetadata(Pointer, State)));
		AddDiagnostic(State, EDnd5eDiagnosticSeverity::Warning, Codes::PreservedUnsupportedContent, Pointer, TEXT("The complete unmodeled inline token is retained as text."));
	}

	TArray<FString> SplitInlineArguments(const FString& Body)
	{
		TArray<FString> Parts;
		int32 Depth = 0, Start = 0;
		for (int32 Index = 0; Index < Body.Len(); ++Index)
		{
			if (Body[Index] == TEXT('{')) { ++Depth; }
			else if (Body[Index] == TEXT('}')) { --Depth; }
			else if (Body[Index] == TEXT('|') && Depth == 0) { Parts.Add(Body.Mid(Start, Index - Start)); Start = Index + 1; }
		}
		Parts.Add(Body.Mid(Start));
		return Parts;
	}

	bool ReadInlineToken(const FString& Token, const FString& Pointer, TArray<FDnd5eInlineContent>& Inlines, FReadState& State, int32 Depth)
	{
		int32 BodyStart = 2;
		while (BodyStart < Token.Len() - 1 && !FChar::IsWhitespace(Token[BodyStart])) { ++BodyStart; }
		const FString Tag = Token.Mid(2, BodyStart - 2);
		if (BodyStart < Token.Len() - 1 && FChar::IsWhitespace(Token[BodyStart])) { ++BodyStart; }
		const FString Body = Token.Mid(BodyStart, Token.Len() - BodyStart - 1);
		const auto Parts = SplitInlineArguments(Body);
		const auto Metadata = MakeContentMetadata(Pointer, State);
		if (Tag.Equals(TEXT("b"), ESearchCase::CaseSensitive) || Tag.Equals(TEXT("bold"), ESearchCase::CaseSensitive) || Tag.Equals(TEXT("i"), ESearchCase::CaseSensitive) || Tag.Equals(TEXT("italic"), ESearchCase::CaseSensitive))
		{
			FDnd5eFormattedContent Formatted;
			Formatted.Formatting.AddTag(Tag.Equals(TEXT("b"), ESearchCase::CaseSensitive) || Tag.Equals(TEXT("bold"), ESearchCase::CaseSensitive) ? Dnd5e::Content::Formatting::Bold : Dnd5e::Content::Formatting::Italic);
			if (!ReadInlines(Body, Pointer, Formatted.Inlines, State, Depth + 1)) { return false; }
			Inlines.Add(MakeInline(Dnd5e::Content::Inline::Formatted, Formatted, Metadata));
		}
		else if ((Tag.Equals(TEXT("dice"), ESearchCase::CaseSensitive) || Tag.Equals(TEXT("damage"), ESearchCase::CaseSensitive)) && !Parts[0].IsEmpty())
		{
			FDnd5eAnnotationContent Annotation;
			Annotation.Kind = Tag.Equals(TEXT("dice"), ESearchCase::CaseSensitive) ? Dnd5e::Content::Annotation::DiceRoll : Dnd5e::Content::Annotation::DamageRoll;
			FDnd5eDiceExpression Dice;
			Dice.Expression = Parts[0];
			Dice.Syntax = TEXT("5etools");
			Annotation.Value = FInstancedStruct::Make<FDnd5eDiceExpression>(Dice);
			Annotation.SourceFormat = TEXT("5etools");
			Annotation.SourceAnnotation = Token;
			if (!ReadInlines(Parts.Num() > 1 && !Parts[1].IsEmpty() ? Parts[1] : Parts[0], Pointer, Annotation.Display, State, Depth + 1)) { return false; }
			Inlines.Add(MakeInline(Dnd5e::Content::Inline::Annotation, Annotation, Metadata));
		}
		else if (Tag.Equals(TEXT("spell"), ESearchCase::CaseSensitive) || Tag.Equals(TEXT("item"), ESearchCase::CaseSensitive) || Tag.Equals(TEXT("creature"), ESearchCase::CaseSensitive) || Tag.Equals(TEXT("condition"), ESearchCase::CaseSensitive) || Tag.Equals(TEXT("feat"), ESearchCase::CaseSensitive) || Tag.Equals(TEXT("race"), ESearchCase::CaseSensitive) || Tag.Equals(TEXT("background"), ESearchCase::CaseSensitive))
		{
			if (Parts.Num() > 3 || Parts[0].TrimStartAndEnd().IsEmpty()) { UnsupportedInline(Token, Pointer, Inlines, State); return true; }
			FDnd5eReferenceContent Reference;
			Reference.SourceFormat = TEXT("5etools");
			Reference.SourceReference = Token;
			const FString Name = Parts[0].TrimStartAndEnd();
			if (!ReadInlines(Parts.Num() > 2 && !Parts[2].IsEmpty() ? Parts[2] : Name, Pointer, Reference.Display, State, Depth + 1)) { return false; }
			if (Tag.Equals(TEXT("spell"), ESearchCase::CaseSensitive) && Parts.Num() > 1 && !Parts[1].TrimStartAndEnd().IsEmpty())
			{
				const FString Source = Parts[1].TrimStartAndEnd();
				if (!ValidateIdentityText(Name, Pointer, State) || !ValidateIdentityText(Source, Pointer, State)) { return false; }
				Reference.bHasTargetRecordId = true;
				Reference.TargetRecordId.DatasetNamespace = State.Context.DatasetNamespace;
				Reference.TargetRecordId.Category = Dnd5e::Category::Spell;
				Reference.TargetRecordId.SourceKey = TEXT("5etools:spell:v1:") + EscapeIdentityComponent(Name) + TEXT("|") + EscapeIdentityComponent(Source);
			}
			else { AddDiagnostic(State, EDnd5eDiagnosticSeverity::Warning, Codes::UnresolvedReference, Pointer, TEXT("Reference retained without inventing an omitted source or unsupported category identity.")); }
			Inlines.Add(MakeInline(Dnd5e::Content::Inline::Reference, Reference, Metadata));
		}
		else if (Tag.Equals(TEXT("link"), ESearchCase::CaseSensitive) && Parts.Num() == 2 && (Parts[1].StartsWith(TEXT("https://"), ESearchCase::IgnoreCase) || Parts[1].StartsWith(TEXT("http://"), ESearchCase::IgnoreCase)))
		{
			FDnd5eExternalLinkContent Link;
			Link.Url = Parts[1];
			Link.SourceFormat = TEXT("5etools");
			Link.SourceReference = Token;
			if (!ReadInlines(Parts[0], Pointer, Link.Display, State, Depth + 1)) { return false; }
			Inlines.Add(MakeInline(Dnd5e::Content::Inline::ExternalLink, Link, Metadata));
		}
		else { UnsupportedInline(Token, Pointer, Inlines, State); }
		return true;
	}

	bool ReadInlines(const FString& Text, const FString& Pointer, TArray<FDnd5eInlineContent>& OutInlines, FReadState& State, int32 Depth)
	{
		if (!CheckContentDepth(Depth, Pointer, State)) { return false; }
		TArray<FDnd5eInlineContent> Inlines;
		int32 Position = 0;
		while (Position < Text.Len())
		{
			const int32 Start = Text.Find(TEXT("{@"), ESearchCase::CaseSensitive, ESearchDir::FromStart, Position);
			const int32 EndText = Start == INDEX_NONE ? Text.Len() : Start;
			if (EndText > Position)
			{
				if (!ConsumeContentBudget(Pointer, State)) { return false; }
				FDnd5eTextContent Plain;
				Plain.Text = Text.Mid(Position, EndText - Position);
				Inlines.Add(MakeInline(Dnd5e::Content::Inline::Text, Plain, MakeContentMetadata(Pointer, State)));
			}
			if (Start == INDEX_NONE) { break; }
			if (!ConsumeContentBudget(Pointer, State)) { return false; }
			int32 Nesting = 1, End = Start + 2;
			for (; End < Text.Len(); ++End)
			{
				if (Text[End] == TEXT('{')) { if (++Nesting + Depth > 64) { return Fail(State, Codes::ResourceLimitExceeded, Pointer, TEXT("Inline token depth exceeded.")); } }
				else if (Text[End] == TEXT('}') && --Nesting == 0) { break; }
			}
			if (End >= Text.Len()) { UnsupportedInline(Text.Mid(Start), Pointer, Inlines, State); break; }
			if (!ReadInlineToken(Text.Mid(Start, End - Start + 1), Pointer, Inlines, State, Depth)) { return false; }
			Position = End + 1;
		}
		if (Text.IsEmpty())
		{
			if (!ConsumeContentBudget(Pointer, State)) { return false; }
			FDnd5eTextContent Empty;
			Inlines.Add(MakeInline(Dnd5e::Content::Inline::Text, Empty, MakeContentMetadata(Pointer, State)));
		}
		OutInlines = MoveTemp(Inlines);
		return true;
	}

	bool ReadBlocks(const TSharedPtr<FJsonValue>& Value, const FString& Pointer, TArray<FDnd5eContentBlock>& OutBlocks, FReadState& State, int32 Depth)
	{
		if (!CheckContentDepth(Depth, Pointer, State)) { return false; }
		const TArray<TSharedPtr<FJsonValue>>* Values = nullptr;
		if (!RequireArray(Value, Pointer, Values, State)) { return false; }
		TArray<FDnd5eContentBlock> Blocks;
		for (int32 Index = 0; Index < Values->Num(); ++Index)
		{
			FDnd5eContentBlock Block;
			if (!ReadContent((*Values)[Index], AppendPointer(Pointer, FString::FromInt(Index)), Block, State, Depth + 1)) { return false; }
			Blocks.Add(MoveTemp(Block));
		}
		OutBlocks = MoveTemp(Blocks);
		return true;
	}

	bool ReadInlineField(const FJsonObject& Object, const TCHAR* Key, const FString& Pointer, TArray<FDnd5eInlineContent>& OutInlines, FReadState& State, int32 Depth)
	{
		if (!FindExactField(Object, Key).IsValid()) { return true; }
		FString Text;
		return ReadRequiredString(Object, Key, Pointer, Text, State) && ReadInlines(Text, AppendPointer(Pointer, Key), OutInlines, State, Depth + 1);
	}

	bool ReadBlockField(const FJsonObject& Object, const TCHAR* Key, const FString& Pointer, TArray<FDnd5eContentBlock>& OutBlocks, FReadState& State, int32 Depth)
	{
		const auto Value = FindExactField(Object, Key);
		return !Value.IsValid() || ReadBlocks(Value, AppendPointer(Pointer, Key), OutBlocks, State, Depth + 1);
	}

	bool ReadRollInteger(const FJsonObject& Object, const TCHAR* Key, const FString& Pointer, int32& OutInteger, FReadState& State)
	{
		TSharedPtr<FJsonValue> Value;
		FDnd5eNumber Number;
		if (!ReadRequiredValue(Object, Key, EJson::Number, Pointer, Value, State) || !ReadNumberValue(Value, AppendPointer(Pointer, Key), Number, State)) { return false; }
		if (Number.Kind != EDnd5eNumberKind::Integer) { return Fail(State, Codes::InvalidFieldValue, AppendPointer(Pointer, Key), TEXT("Expected a finite int32 roll/display integer.")); }
		OutInteger = Number.Integer;
		return true;
	}

	bool ReadRollBound(const FJsonObject& Object, const TCHAR* Key, const FString& Pointer, bool bMinimum, FDnd5eTableRollRange& Range, FReadState& State)
	{
		TSharedPtr<FJsonValue> Value;
		FDnd5eNumber Number;
		if (!ReadRequiredValue(Object, Key, EJson::Number, Pointer, Value, State) || !ReadNumberValue(Value, AppendPointer(Pointer, Key), Number, State)) { return false; }
		const FString Expected = bMinimum ? TEXT("-100000000000000000000") : TEXT("100000000000000000000");
		const FString Exponent = bMinimum ? TEXT("-1e20") : TEXT("1e20");
		const FString SignedExponent = bMinimum ? TEXT("-1e+20") : TEXT("1e+20");
		if (Number.SourceLiteral.Equals(Expected, ESearchCase::CaseSensitive) || Number.SourceLiteral.Equals(Exponent, ESearchCase::IgnoreCase) || Number.SourceLiteral.Equals(SignedExponent, ESearchCase::IgnoreCase))
		{
			if (bMinimum) { Range.bMinimumUnbounded = true; }
			else { Range.bMaximumUnbounded = true; }
			return true;
		}
		if (Number.Kind != EDnd5eNumberKind::Integer) { return Fail(State, Codes::InvalidFieldValue, AppendPointer(Pointer, Key), TEXT("Roll bound is neither int32 nor a recognized source infinity sentinel.")); }
		const int32 Bound = Number.Integer == 0 ? 100 : Number.Integer;
		if (bMinimum)
		{
			Range.Minimum = Bound;
			if (Number.Integer == 0) { Range.bHasDisplayMinimum = true; Range.DisplayMinimum = 0; }
		}
		else
		{
			Range.Maximum = Bound;
			if (Number.Integer == 0) { Range.bHasDisplayMaximum = true; Range.DisplayMaximum = 0; }
		}
		return true;
	}

	bool ReadTableCell(const TSharedPtr<FJsonValue>& Value, const FString& Pointer, FDnd5eTableCellContent& OutCell, FReadState& State, int32 Depth)
	{
		FDnd5eTableCellContent Cell;
		if (Value->Type != EJson::Object)
		{
			FDnd5eContentBlock Block;
			if (!ReadContent(Value, Pointer, Block, State, Depth + 1)) { return false; }
			Cell.Blocks.Add(MoveTemp(Block));
		}
		else
		{
			const auto Object = Value->AsObject();
			if (!CheckAllowedKeys(*Object, {TEXT("type"), TEXT("roll"), TEXT("entry"), TEXT("width")}, Pointer, State)) { return false; }
			FString Type;
			if (!ReadRequiredString(*Object, TEXT("type"), Pointer, Type, State)) { return false; }
			if (!Type.Equals(TEXT("cell"), ESearchCase::CaseSensitive) && !Type.Equals(TEXT("cellHeader"), ESearchCase::CaseSensitive)) { return Fail(State, Codes::UnsupportedRequiredValue, Pointer, TEXT("Unknown table-cell form.")); }
			if (FindExactField(*Object, TEXT("width")).IsValid())
			{
				FString Literal;
				if (!ReadRequiredInteger(*Object, TEXT("width"), Pointer, 1, MAX_int32, Cell.ColumnSpan, Literal, State)) { return false; }
			}
			if (const auto Entry = FindExactField(*Object, TEXT("entry")))
			{
				FDnd5eContentBlock Block;
				if (!ReadContent(Entry, AppendPointer(Pointer, TEXT("entry")), Block, State, Depth + 1)) { return false; }
				Cell.Blocks.Add(MoveTemp(Block));
			}
			if (const auto RollValue = FindExactField(*Object, TEXT("roll")))
			{
				TSharedPtr<FJsonObject> Roll;
				const FString RollPointer = AppendPointer(Pointer, TEXT("roll"));
				if (!RequireObject(RollValue, RollPointer, Roll, State) || !CheckAllowedKeys(*Roll, {TEXT("exact"), TEXT("min"), TEXT("max"), TEXT("pad"), TEXT("displayMin"), TEXT("displayMax")}, RollPointer, State)) { return false; }
				Cell.bHasRollRange = true;
				FString Literal;
				if (FindExactField(*Roll, TEXT("exact")).IsValid())
				{
					if (FindExactField(*Roll, TEXT("min")).IsValid() || FindExactField(*Roll, TEXT("max")).IsValid()) { return Fail(State, Codes::InvalidFieldValue, RollPointer, TEXT("Use exact or min/max, not both.")); }
					if (!ReadRollBound(*Roll, TEXT("exact"), RollPointer, true, Cell.RollRange, State)) { return false; }
					if (Cell.RollRange.bMinimumUnbounded) { return Fail(State, Codes::InvalidFieldValue, RollPointer, TEXT("An exact roll needs a finite value.")); }
					Cell.RollRange.Maximum = Cell.RollRange.Minimum;
					Cell.RollRange.bHasDisplayMaximum = Cell.RollRange.bHasDisplayMinimum;
					Cell.RollRange.DisplayMaximum = Cell.RollRange.DisplayMinimum;
				}
				else
				{
					if (!ReadRollBound(*Roll, TEXT("min"), RollPointer, true, Cell.RollRange, State) || !ReadRollBound(*Roll, TEXT("max"), RollPointer, false, Cell.RollRange, State)) { return false; }
					if (!Cell.RollRange.bMinimumUnbounded && !Cell.RollRange.bMaximumUnbounded && Cell.RollRange.Maximum < Cell.RollRange.Minimum) { return Fail(State, Codes::InvalidFieldValue, RollPointer, TEXT("Roll range is reversed.")); }
				}
				bool bPad = false;
				if (!ReadOptionalBool(*Roll, TEXT("pad"), RollPointer, bPad, State)) { return false; }
				Cell.RollRange.PadToDigits = bPad ? 2 : 0;
				if (FindExactField(*Roll, TEXT("displayMin")).IsValid())
				{
					if (!ReadRollInteger(*Roll, TEXT("displayMin"), RollPointer, Cell.RollRange.DisplayMinimum, State)) { return false; }
					Cell.RollRange.bHasDisplayMinimum = true;
				}
				if (FindExactField(*Roll, TEXT("displayMax")).IsValid())
				{
					if (!ReadRollInteger(*Roll, TEXT("displayMax"), RollPointer, Cell.RollRange.DisplayMaximum, State)) { return false; }
					Cell.RollRange.bHasDisplayMaximum = true;
				}
				if (Cell.Blocks.IsEmpty())
				{
					const bool bExact = FindExactField(*Roll, TEXT("exact")).IsValid();
					auto Display = [&](const TCHAR* BoundKey, const TCHAR* DisplayKey)
					{
						FString Text;
						const auto Override = FindExactField(*Roll, DisplayKey);
						(Override.IsValid() ? Override : FindExactField(*Roll, BoundKey))->TryGetString(Text);
						if (bPad && Text.Len() < 2) { Text = TEXT("0") + Text; }
						return Text;
					};
					FString Label = bExact ? Display(TEXT("exact"), TEXT("displayMin")) : Display(TEXT("min"), TEXT("displayMin")) + TEXT("-") + Display(TEXT("max"), TEXT("displayMax"));
					FDnd5eContentBlock Block;
					if (!ReadContent(MakeShared<FJsonValueString>(Label), Pointer, Block, State, Depth + 1)) { return false; }
					Cell.Blocks.Add(MoveTemp(Block));
				}
			}
			if (!Cell.bHasRollRange && Cell.Blocks.IsEmpty()) { return Fail(State, Codes::MissingRequiredField, Pointer, TEXT("A table cell needs entry or roll.")); }
		}
		OutCell = MoveTemp(Cell);
		return true;
	}

	bool ReadTableRow(const TSharedPtr<FJsonValue>& Value, const FString& Pointer, FDnd5eTableRowContent& OutRow, FReadState& State, int32 Depth)
	{
		const TArray<TSharedPtr<FJsonValue>>* Cells = nullptr;
		if (!RequireArray(Value, Pointer, Cells, State)) { return false; }
		FDnd5eTableRowContent Row;
		for (int32 Index = 0; Index < Cells->Num(); ++Index)
		{
			FDnd5eTableCellContent Cell;
			if (!ReadTableCell((*Cells)[Index], AppendPointer(Pointer, FString::FromInt(Index)), Cell, State, Depth + 1)) { return false; }
			Row.Cells.Add(MoveTemp(Cell));
		}
		OutRow = MoveTemp(Row);
		return true;
	}

	bool ReadTable(const FJsonObject& Object, const FString& Pointer, FDnd5eContentBlock& OutBlock, FDnd5eContentMetadata Metadata, FReadState& State, int32 Depth)
	{
		FDnd5eTableContent Table;
		if (!ReadInlineField(Object, TEXT("caption"), Pointer, Table.Caption, State, Depth)) { return false; }
		if (!ReadBlockField(Object, TEXT("intro"), Pointer, Table.Intro, State, Depth) || !ReadBlockField(Object, TEXT("footnotes"), Pointer, Table.Footnotes, State, Depth) || !ReadBlockField(Object, TEXT("outro"), Pointer, Table.Outro, State, Depth)) { return false; }
		if (!ReadOptionalBool(Object, TEXT("isStriped"), Pointer, Table.bStripedRows, State) || !ReadOptionalBool(Object, TEXT("isNameGenerator"), Pointer, Table.bIsNameGenerator, State)) { return false; }
		if (const auto Labels = FindExactField(Object, TEXT("colLabels")))
		{
			if (FindExactField(Object, TEXT("colLabelRows")).IsValid()) { return Fail(State, Codes::InvalidFieldValue, Pointer, TEXT("Simultaneous colLabels/colLabelRows needs a separate grouping policy.")); }
			FDnd5eTableRowContent Row;
			if (!ReadTableRow(Labels, AppendPointer(Pointer, TEXT("colLabels")), Row, State, Depth)) { return false; }
			Table.HeaderRows.Add(MoveTemp(Row));
		}
		if (const auto LabelRows = FindExactField(Object, TEXT("colLabelRows")))
		{
			const TArray<TSharedPtr<FJsonValue>>* Values = nullptr;
			if (!RequireArray(LabelRows, AppendPointer(Pointer, TEXT("colLabelRows")), Values, State)) { return false; }
			for (int32 Index = 0; Index < Values->Num(); ++Index)
			{
				FDnd5eTableRowContent Row;
				if (!ReadTableRow((*Values)[Index], AppendPointer(AppendPointer(Pointer, TEXT("colLabelRows")), FString::FromInt(Index)), Row, State, Depth)) { return false; }
				Table.HeaderRows.Add(MoveTemp(Row));
			}
		}
		const TArray<TSharedPtr<FJsonValue>>* Rows = nullptr;
		if (!ReadRequiredArray(Object, TEXT("rows"), Pointer, Rows, State)) { return false; }
		int32 ColumnCount = 0;
		for (const auto& Row : Table.HeaderRows) { int32 Width = 0; for (const auto& Cell : Row.Cells) { if (Cell.ColumnSpan > MAX_int32 - Width) { return Fail(State, Codes::ResourceLimitExceeded, Pointer, TEXT("Table width overflow.")); } Width += Cell.ColumnSpan; } ColumnCount = FMath::Max(ColumnCount, Width); }
		for (int32 Index = 0; Index < Rows->Num(); ++Index)
		{
			FDnd5eTableRowContent Row;
			if (!ReadTableRow((*Rows)[Index], AppendPointer(AppendPointer(Pointer, TEXT("rows")), FString::FromInt(Index)), Row, State, Depth)) { return false; }
			int32 Width = 0;
			for (const auto& Cell : Row.Cells) { if (Cell.ColumnSpan > MAX_int32 - Width) { return Fail(State, Codes::ResourceLimitExceeded, Pointer, TEXT("Table width overflow.")); } Width += Cell.ColumnSpan; }
			ColumnCount = FMath::Max(ColumnCount, Width);
			Table.Rows.Add(MoveTemp(Row));
		}
		if (ColumnCount > 4096) { return Fail(State, Codes::ResourceLimitExceeded, Pointer, TEXT("Table expansion exceeds 4096 columns.")); }
		Table.Columns.SetNum(ColumnCount);
		if (const auto Styles = FindExactField(Object, TEXT("colStyles")))
		{
			const TArray<TSharedPtr<FJsonValue>>* Values = nullptr;
			if (!RequireArray(Styles, AppendPointer(Pointer, TEXT("colStyles")), Values, State)) { return false; }
			if (Values->Num() > 4096) { return Fail(State, Codes::ResourceLimitExceeded, Pointer, TEXT("Table styles exceed 4096 columns.")); }
			if (Values->Num() > Table.Columns.Num()) { Table.Columns.SetNum(Values->Num()); }
			bool bUnmodeled = false;
			for (int32 Index = 0; Index < Values->Num(); ++Index)
			{
				FString Style;
				if ((*Values)[Index]->Type != EJson::String || !(*Values)[Index]->TryGetString(Style)) { return Fail(State, Codes::InvalidFieldValue, AppendPointer(Pointer, TEXT("colStyles")), TEXT("Expected column-style strings.")); }
				TArray<FString> Classes;
				Style.ParseIntoArrayWS(Classes);
				for (const FString& Class : Classes)
				{
					if (Class.Equals(TEXT("text-center"), ESearchCase::CaseSensitive)) { Table.Columns[Index].Alignment = EDnd5eContentAlignment::Center; }
					else if (Class.Equals(TEXT("text-right"), ESearchCase::CaseSensitive)) { Table.Columns[Index].Alignment = EDnd5eContentAlignment::Right; }
					else if (Class.Equals(TEXT("text-left"), ESearchCase::CaseSensitive)) { Table.Columns[Index].Alignment = EDnd5eContentAlignment::Left; }
					else if (Class.StartsWith(TEXT("col-"), ESearchCase::CaseSensitive))
					{
						FString Digits = Class.Mid(4);
						bool bValid = !Digits.IsEmpty();
						int32 DashCount = 0;
						for (TCHAR Character : Digits) { if (Character == TEXT('-')) { ++DashCount; } else if (Character < TEXT('0') || Character > TEXT('9')) { bValid = false; } }
						bValid &= DashCount <= 1 && !Digits.StartsWith(TEXT("-")) && !Digits.EndsWith(TEXT("-"));
						Digits.ReplaceInline(TEXT("-"), TEXT("."));
						float Weight = 0.0f;
						if (bValid && LexTryParseString(Weight, *Digits) && FMath::IsFinite(Weight) && Weight > 0.0f) { Table.Columns[Index].bHasWidthWeight = true; Table.Columns[Index].WidthWeight = Weight; }
						else { bUnmodeled = true; }
					}
					else { bUnmodeled = true; }
				}
			}
			if (bUnmodeled && !PreservePresentation(Object, TEXT("colStyles"), Metadata, Pointer, State)) { return false; }
		}
		OutBlock = MakeBlock(Dnd5e::Content::Block::Table, Table, Metadata);
		return true;
	}

	bool ReadContent(const TSharedPtr<FJsonValue>& Value, const FString& Pointer, FDnd5eContentBlock& OutBlock, FReadState& State, int32 Depth)
	{
		if (!CheckContentDepth(Depth, Pointer, State)) { return false; }
		if (!Value.IsValid()) { return Fail(State, Codes::InvalidFieldValue, Pointer, TEXT("Missing Content value.")); }
		if (!ConsumeContentBudget(Pointer, State)) { return false; }
		auto Metadata = MakeContentMetadata(Pointer, State);
		if (Value->Type == EJson::String)
		{
			FString Text;
			Value->TryGetString(Text);
			FDnd5eParagraphContent Paragraph;
			if (!ReadInlines(Text, Pointer, Paragraph.Inlines, State, Depth + 1)) { return false; }
			OutBlock = MakeBlock(Dnd5e::Content::Block::Paragraph, Paragraph, Metadata);
			return true;
		}
		if (Value->Type == EJson::Number)
		{
			FDnd5eNumberContent Number;
			if (!ReadNumberValue(Value, Pointer, Number.Value, State)) { return false; }
			FDnd5eParagraphContent Paragraph;
			Paragraph.Inlines.Add(MakeInline(Dnd5e::Content::Inline::Number, Number, Metadata));
			OutBlock = MakeBlock(Dnd5e::Content::Block::Paragraph, Paragraph, Metadata);
			return true;
		}
		if (Value->Type == EJson::Array) { return UnsupportedBlock(Value, Pointer, OutBlock, State); }
		if (Value->Type != EJson::Object) { return Fail(State, Codes::InvalidFieldValue, Pointer, TEXT("Content must be text, number, or an entry object.")); }
		const auto Object = Value->AsObject();
		FString Type;
		if (const auto TypeValue = FindExactField(*Object, TEXT("type")))
		{
			if (TypeValue->Type != EJson::String || !TypeValue->TryGetString(Type)) { return Fail(State, Codes::InvalidFieldValue, AppendPointer(Pointer, TEXT("type")), TEXT("Expected an entry-type string.")); }
		}
		else if (FindExactField(*Object, TEXT("entries")).IsValid()) { Type = TEXT("entries"); }
		else { return UnsupportedBlock(Value, Pointer, OutBlock, State); }
		Metadata.SourceType = FindExactField(*Object, TEXT("type")).IsValid() ? Type : FString{};
		if (!ReadNodeMetadata(*Object, Pointer, Metadata, State)) { return false; }
		FReadState ChildState{State.Context, State.JsonText, State.Diagnostics, Metadata.SourceId, State.Budget};
		if (Type.Equals(TEXT("entries"), ESearchCase::CaseSensitive) || Type.Equals(TEXT("section"), ESearchCase::CaseSensitive))
		{
			if (!HasOnlyKeys(*Object, {TEXT("type"), TEXT("entries"), TEXT("name"), TEXT("source"), TEXT("page"), TEXT("id")})) { return UnsupportedBlock(Value, Pointer, OutBlock, State); }
			FDnd5eSectionContent Section;
			TSharedPtr<FJsonValue> Entries;
			if (!ReadRequiredValue(*Object, TEXT("entries"), EJson::Array, Pointer, Entries, State)) { return false; }
			if (!ReadInlineField(*Object, TEXT("name"), Pointer, Section.Heading, ChildState, Depth) || !ReadBlocks(Entries, AppendPointer(Pointer, TEXT("entries")), Section.Blocks, ChildState, Depth + 1)) { return false; }
			OutBlock = MakeBlock(Dnd5e::Content::Block::Section, Section, Metadata);
		}
		else if (Type.Equals(TEXT("list"), ESearchCase::CaseSensitive))
		{
			if (!HasOnlyKeys(*Object, {TEXT("type"), TEXT("items"), TEXT("name"), TEXT("style"), TEXT("start"), TEXT("columns"), TEXT("source"), TEXT("page"), TEXT("id")})) { return UnsupportedBlock(Value, Pointer, OutBlock, State); }
			FDnd5eListContent List;
			List.MarkerStyle = EDnd5eListMarkerStyle::Bullet;
			if (!ReadInlineField(*Object, TEXT("name"), Pointer, List.Heading, ChildState, Depth)) { return false; }
			FString Literal;
			if (FindExactField(*Object, TEXT("start")).IsValid() && !ReadRequiredInteger(*Object, TEXT("start"), Pointer, 0, MAX_int32, List.StartIndex, Literal, State)) { return false; }
			if (FindExactField(*Object, TEXT("columns")).IsValid())
			{
				if (!ReadRequiredInteger(*Object, TEXT("columns"), Pointer, 1, MAX_int32, List.ColumnCount, Literal, State)) { return false; }
				List.bHasColumnCount = true;
			}
			if (FindExactField(*Object, TEXT("style")).IsValid())
			{
				FString Style;
				if (!ReadRequiredString(*Object, TEXT("style"), Pointer, Style, State)) { return false; }
				if (Style.Equals(TEXT("list-decimal"), ESearchCase::CaseSensitive)) { List.MarkerStyle = EDnd5eListMarkerStyle::Decimal; }
				else if (Style.Equals(TEXT("list-lower-roman"), ESearchCase::CaseSensitive)) { List.MarkerStyle = EDnd5eListMarkerStyle::LowerRoman; }
				else if (Style.Equals(TEXT("list-upper-roman"), ESearchCase::CaseSensitive)) { List.MarkerStyle = EDnd5eListMarkerStyle::UpperRoman; }
				else if (Style.Equals(TEXT("list-no-bullets"), ESearchCase::CaseSensitive)) { List.MarkerStyle = EDnd5eListMarkerStyle::None; }
				else if (!PreservePresentation(*Object, TEXT("style"), Metadata, Pointer, State)) { return false; }
			}
			const TArray<TSharedPtr<FJsonValue>>* Items = nullptr;
			if (!ReadRequiredArray(*Object, TEXT("items"), Pointer, Items, State)) { return false; }
			for (int32 Index = 0; Index < Items->Num(); ++Index)
			{
				const auto& ItemValue = (*Items)[Index];
				const FString ItemPointer = AppendPointer(AppendPointer(Pointer, TEXT("items")), FString::FromInt(Index));
				FDnd5eListItemContent Item;
				Item.Metadata = MakeContentMetadata(ItemPointer, ChildState);
				if (ItemValue->Type == EJson::Object)
				{
					const auto ItemObject = ItemValue->AsObject();
					FString ItemType;
					if (!ReadRequiredString(*ItemObject, TEXT("type"), ItemPointer, ItemType, State)) { return false; }
					if (!ItemType.Equals(TEXT("item"), ESearchCase::CaseSensitive) || !HasOnlyKeys(*ItemObject, {TEXT("type"), TEXT("name"), TEXT("entries"), TEXT("entry"), TEXT("source"), TEXT("page"), TEXT("id")}))
					{
						return UnsupportedBlock(Value, Pointer, OutBlock, State);
					}
					Item.Metadata.SourceType = ItemType;
					if (!ReadNodeMetadata(*ItemObject, ItemPointer, Item.Metadata, ChildState)) { return false; }
					FReadState ItemState{State.Context, State.JsonText, State.Diagnostics, Item.Metadata.SourceId, State.Budget};
					if (!ReadInlineField(*ItemObject, TEXT("name"), ItemPointer, Item.Label, ItemState, Depth + 1)) { return false; }
					const auto Entries = FindExactField(*ItemObject, TEXT("entries"));
					const auto Entry = FindExactField(*ItemObject, TEXT("entry"));
					if (Entries.IsValid() && Entry.IsValid()) { return Fail(State, Codes::InvalidFieldValue, ItemPointer, TEXT("An item cannot supply both entry and entries.")); }
					if (Entries.IsValid()) { if (!ReadBlocks(Entries, AppendPointer(ItemPointer, TEXT("entries")), Item.Blocks, ItemState, Depth + 1)) { return false; } }
					else if (Entry.IsValid())
					{
						FDnd5eContentBlock Block;
						if (!ReadContent(Entry, AppendPointer(ItemPointer, TEXT("entry")), Block, ItemState, Depth + 1)) { return false; }
						Item.Blocks.Add(MoveTemp(Block));
					}
					else { return Fail(State, Codes::MissingRequiredField, AppendPointer(ItemPointer, TEXT("entries")), TEXT("List item needs entry or entries.")); }
				}
				else
				{
					FDnd5eContentBlock Block;
					if (!ReadContent(ItemValue, ItemPointer, Block, ChildState, Depth + 1)) { return false; }
					Item.Blocks.Add(MoveTemp(Block));
				}
				List.Items.Add(MoveTemp(Item));
			}
			OutBlock = MakeBlock(Dnd5e::Content::Block::List, List, Metadata);
		}
		else if (Type.Equals(TEXT("table"), ESearchCase::CaseSensitive))
		{
			if (!HasOnlyKeys(*Object, {TEXT("type"), TEXT("caption"), TEXT("colLabels"), TEXT("colLabelRows"), TEXT("colStyles"), TEXT("rows"), TEXT("intro"), TEXT("footnotes"), TEXT("outro"), TEXT("isStriped"), TEXT("isNameGenerator"), TEXT("source"), TEXT("page"), TEXT("id")})) { return UnsupportedBlock(Value, Pointer, OutBlock, State); }
			return ReadTable(*Object, Pointer, OutBlock, Metadata, ChildState, Depth);
		}
		else if (Type.Equals(TEXT("inset"), ESearchCase::CaseSensitive) || Type.Equals(TEXT("insetReadaloud"), ESearchCase::CaseSensitive))
		{
			if (!HasOnlyKeys(*Object, {TEXT("type"), TEXT("name"), TEXT("entries"), TEXT("source"), TEXT("page"), TEXT("id")})) { return UnsupportedBlock(Value, Pointer, OutBlock, State); }
			FDnd5eInsetContent Inset;
			TSharedPtr<FJsonValue> Entries;
			if (!ReadRequiredValue(*Object, TEXT("entries"), EJson::Array, Pointer, Entries, State)) { return false; }
			if (!ReadInlineField(*Object, TEXT("name"), Pointer, Inset.Heading, ChildState, Depth) || !ReadBlocks(Entries, AppendPointer(Pointer, TEXT("entries")), Inset.Blocks, ChildState, Depth + 1)) { return false; }
			if (Type.Equals(TEXT("insetReadaloud"), ESearchCase::CaseSensitive)) { Inset.Presentation.AddTag(Dnd5e::Content::Presentation::ReadAloud); }
			OutBlock = MakeBlock(Dnd5e::Content::Block::Inset, Inset, Metadata);
		}
		else if (Type.Equals(TEXT("quote"), ESearchCase::CaseSensitive))
		{
			if (!HasOnlyKeys(*Object, {TEXT("type"), TEXT("entries"), TEXT("by"), TEXT("from"), TEXT("skipMarks"), TEXT("skipItalics"), TEXT("source"), TEXT("page"), TEXT("id")})) { return UnsupportedBlock(Value, Pointer, OutBlock, State); }
			FDnd5eQuoteContent Quote;
			TSharedPtr<FJsonValue> Entries;
			if (!ReadRequiredValue(*Object, TEXT("entries"), EJson::Array, Pointer, Entries, State) || !ReadBlocks(Entries, AppendPointer(Pointer, TEXT("entries")), Quote.Blocks, ChildState, Depth + 1)) { return false; }
			if (const auto By = FindExactField(*Object, TEXT("by")))
			{
				if (By->Type == EJson::Array) { if (!ReadBlocks(By, AppendPointer(Pointer, TEXT("by")), Quote.Attribution, ChildState, Depth + 1)) { return false; } }
				else
				{
					FDnd5eContentBlock Block;
					if (!ReadContent(By, AppendPointer(Pointer, TEXT("by")), Block, ChildState, Depth + 1)) { return false; }
					Quote.Attribution.Add(MoveTemp(Block));
				}
			}
			if (!ReadInlineField(*Object, TEXT("from"), Pointer, Quote.Source, ChildState, Depth)) { return false; }
			bool bSkipMarks = false, bSkipItalics = false;
			if (!ReadOptionalBool(*Object, TEXT("skipMarks"), Pointer, bSkipMarks, State) || !ReadOptionalBool(*Object, TEXT("skipItalics"), Pointer, bSkipItalics, State)) { return false; }
			Quote.bShowQuotationMarks = !bSkipMarks;
			Quote.bItalicize = !bSkipItalics;
			OutBlock = MakeBlock(Dnd5e::Content::Block::Quote, Quote, Metadata);
		}
		else { return UnsupportedBlock(Value, Pointer, OutBlock, State); }
		return true;
	}
}
