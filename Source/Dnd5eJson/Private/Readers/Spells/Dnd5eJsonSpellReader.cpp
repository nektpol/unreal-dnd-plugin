#include "Readers/Spells/Dnd5eJsonSpellReader.h"
#include "Readers/Dnd5eJsonReaderInternal.h"

#include "Dnd5eTags.h"
#include "Containers/StringConv.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonTypes.h"
#include "Serialization/JsonWriter.h"
#include <initializer_list>

namespace Dnd5eJsonReader
{
	constexpr int32 MaxInputLength = 16777216;
	constexpr int32 MaxDepth = 64;
	constexpr int32 MaxValueCount = 1000000;

	namespace Codes
	{
		const FName InvalidFieldValue(TEXT("Dnd5e.Json.InvalidFieldValue"));
		const FName MissingRequiredField(TEXT("Dnd5e.Json.MissingRequiredField"));
		const FName InvalidContext(TEXT("Dnd5e.Json.InvalidContext"));
		const FName NoSpellSelection(TEXT("Dnd5e.Json.NoSpellSelection"));
		const FName EmptyInput(TEXT("Dnd5e.Json.EmptyInput"));
		const FName ResourceLimitExceeded(TEXT("Dnd5e.Json.ResourceLimitExceeded"));
		const FName InvalidSyntax(TEXT("Dnd5e.Json.InvalidSyntax"));
		const FName DuplicateProperty(TEXT("Dnd5e.Json.DuplicateProperty"));
		const FName InvalidRoot(TEXT("Dnd5e.Json.InvalidRoot"));
		const FName MissingSpellArray(TEXT("Dnd5e.Json.MissingSpellArray"));
		const FName InvalidSpellArray(TEXT("Dnd5e.Json.InvalidSpellArray"));
		const FName SpellIndexOutOfBounds(TEXT("Dnd5e.Json.SpellIndexOutOfBounds"));
		const FName InvalidSpellObject(TEXT("Dnd5e.Json.InvalidSpellObject"));
		const FName UnsupportedRequiredValue(TEXT("Dnd5e.Json.UnsupportedRequiredValue"));
		const FName PreservedUnsupportedContent(TEXT("Dnd5e.Json.PreservedUnsupportedContent"));
		const FName UnresolvedReference(TEXT("Dnd5e.Json.UnresolvedReference"));
		const FName SerializationFailure(TEXT("Dnd5e.Json.SerializationFailure"));
		const FName PreservedUnsupportedMetadata(TEXT("Dnd5e.Json.PreservedUnsupportedMetadata"));
	}

	void AddDiagnostic(FReadState& State, EDnd5eDiagnosticSeverity Severity, FName Code, const FString& Pointer, const FString& Message)
	{
		FDnd5eDiagnostic Diagnostic;
		Diagnostic.Severity = Severity;
		Diagnostic.Code = Code;
		Diagnostic.Message = Message;
		Diagnostic.RelativeFilePath = State.Context.RelativeFilePath;
		Diagnostic.JsonPointer = Pointer;
		State.Diagnostics.Add(MoveTemp(Diagnostic));
	}

	bool Fail(FReadState& State, FName Code, const FString& Pointer, const FString& Message)
	{
		AddDiagnostic(State, EDnd5eDiagnosticSeverity::Error, Code, Pointer, Message);
		return false;
	}

	FString AppendPointer(const FString& Parent, const FString& Segment)
	{
		FString Escaped = Segment.Replace(TEXT("~"), TEXT("~0"), ESearchCase::CaseSensitive);
		Escaped.ReplaceInline(TEXT("/"), TEXT("~1"), ESearchCase::CaseSensitive);
		return Parent + TEXT("/") + Escaped;
	}

	TSharedPtr<FJsonValue> FindExactField(const FJsonObject& Object, const TCHAR* FieldName)
	{
		for (const auto& Pair : Object.Values)
		{
			const FString Key(Pair.Key.Len(), *Pair.Key);
			if (Key.Equals(FieldName, ESearchCase::CaseSensitive))
			{
				return Pair.Value;
			}
		}
		return nullptr;
	}

	bool ReadRequiredValue(const FJsonObject& Object, const TCHAR* FieldName, EJson ExpectedType, const FString& ObjectPointer, TSharedPtr<FJsonValue>& OutValue, FReadState& State)
	{
		const FString Pointer = AppendPointer(ObjectPointer, FieldName);
		TSharedPtr<FJsonValue> Value = FindExactField(Object, FieldName);
		if (!Value.IsValid())
		{
			return Fail(State, Codes::MissingRequiredField, Pointer,TEXT("Required property is missing."));
		}
		if (Value->Type != ExpectedType)
		{
			return Fail(State, Codes::InvalidFieldValue, Pointer,TEXT("Property has an unexpected JSON type."));
		}
		OutValue = MoveTemp(Value);
		return true;
	}

	bool ReadRequiredString(const FJsonObject& Object, const TCHAR* FieldName, const FString& ObjectPointer, FString& OutText, FReadState& State)
	{
		TSharedPtr<FJsonValue> Value;
		if (!ReadRequiredValue(Object, FieldName, EJson::String, ObjectPointer, Value, State)) { return false; }
		if (!Value->TryGetString(OutText)) { return Fail(State, Codes::InvalidFieldValue, AppendPointer(ObjectPointer, FieldName), TEXT("Expected a string.")); }
		return true;
	}

	bool RequireObject(const TSharedPtr<FJsonValue>& Value, const FString& Pointer, TSharedPtr<FJsonObject>& OutObject, FReadState& State)
	{
		const TSharedPtr<FJsonObject>* ObjectPointer = nullptr;
		if (!Value.IsValid() || Value->Type != EJson::Object || !Value->TryGetObject(ObjectPointer) || !ObjectPointer || !ObjectPointer->IsValid())
		{
			return Fail(State, Codes::InvalidFieldValue, Pointer, TEXT("Expected an object."));
		}
		OutObject = *ObjectPointer;
		return true;
	}

	bool RequireArray(const TSharedPtr<FJsonValue>& Value, const FString& Pointer, const TArray<TSharedPtr<FJsonValue>>*& OutArray, FReadState& State)
	{
		const TArray<TSharedPtr<FJsonValue>>* Values = nullptr;
		if (!Value.IsValid() || Value->Type != EJson::Array || !Value->TryGetArray(Values) || !Values)
		{
			return Fail(State, Codes::InvalidFieldValue, Pointer, TEXT("Expected an array."));
		}
		OutArray = Values;
		return true;
	}

	bool ReadRequiredObject(const FJsonObject& Object, const TCHAR* FieldName, const FString& ObjectPointer, TSharedPtr<FJsonObject>& OutObject, FReadState& State)
	{
		TSharedPtr<FJsonValue> Value;
		return ReadRequiredValue(Object, FieldName, EJson::Object, ObjectPointer, Value, State) && RequireObject(Value, AppendPointer(ObjectPointer, FieldName), OutObject, State);
	}

	bool ReadRequiredArray(const FJsonObject& Object, const TCHAR* FieldName, const FString& ObjectPointer, const TArray<TSharedPtr<FJsonValue>>*& OutArray, FReadState& State)
	{
		TSharedPtr<FJsonValue> Value;
		return ReadRequiredValue(Object, FieldName, EJson::Array, ObjectPointer, Value, State) && RequireArray(Value, AppendPointer(ObjectPointer, FieldName), OutArray, State);
	}

	bool ReadOptionalBool(const FJsonObject& Object, const TCHAR* FieldName, const FString& ObjectPointer, bool& OutValue, FReadState& State)
	{
		const TSharedPtr<FJsonValue> Value = FindExactField(Object, FieldName);
		if (!Value.IsValid()) { return true; }
		bool Parsed = false;
		if (Value->Type != EJson::Boolean || !Value->TryGetBool(Parsed)) { return Fail(State, Codes::InvalidFieldValue, AppendPointer(ObjectPointer, FieldName), TEXT("Expected a Boolean.")); }
		OutValue = Parsed;
		return true;
	}

	bool TryParseBoundedIntegerToken(const FString& Token, int32 Minimum, int32 Maximum, int32& OutValue)
	{
		if (Minimum < 0 || Maximum < Minimum || Token.IsEmpty() || (Token.Len() > 1 && Token[0] == TEXT('0'))) { return false; }
		int32 Value = 0;
		for (TCHAR Character : Token)
		{
			if (Character < TEXT('0') || Character > TEXT('9')) { return false; }
			const int32 Digit = Character - TEXT('0');
			if (Value > Maximum / 10 || (Value == Maximum / 10 && Digit > Maximum % 10)) { return false; }
			Value = Value * 10 + Digit;
		}
		if (Value < Minimum) { return false; }
		OutValue = Value;
		return true;
	}

	bool ReadIntegerValue(const TSharedPtr<FJsonValue>& Value, const FString& Pointer, int32 Minimum, int32 Maximum, int32& OutInteger, FString& OutLiteral, FReadState& State)
	{
		FString Literal;
		int32 Integer = 0;
		if (!Value.IsValid() || Value->Type != EJson::Number || !Value->TryGetString(Literal) || !TryParseBoundedIntegerToken(Literal, Minimum, Maximum, Integer))
		{
			return Fail(State, Codes::InvalidFieldValue, Pointer, TEXT("Expected a nonnegative integer token within the supported range."));
		}
		OutInteger = Integer;
		OutLiteral = MoveTemp(Literal);
		return true;
	}

	bool ReadRequiredInteger(const FJsonObject& Object, const TCHAR* FieldName, const FString& ObjectPointer, int32 Minimum, int32 Maximum, int32& OutInteger, FString& OutLiteral, FReadState& State)
	{
		TSharedPtr<FJsonValue> Value;
		return ReadRequiredValue(Object, FieldName, EJson::Number, ObjectPointer, Value, State) && ReadIntegerValue(Value, AppendPointer(ObjectPointer, FieldName), Minimum, Maximum, OutInteger, OutLiteral, State);
	}

	bool HasControlCharacters(const FString& Text)
	{
		for (TCHAR Character : Text)
		{
			if (FChar::IsControl(Character)) { return true; }
		}
		return false;
	}

	bool HasValidUtf16(const FString& Text)
	{
		for (int32 Index = 0; Index < Text.Len(); ++Index)
		{
			const uint32 Unit = static_cast<uint32>(Text[Index]);
			if (Unit >= 0xD800 && Unit <= 0xDBFF)
			{
				if (++Index >= Text.Len()) { return false; }
				const uint32 Next = static_cast<uint32>(Text[Index]);
				if (Next < 0xDC00 || Next > 0xDFFF) { return false; }
			}
			else if (Unit >= 0xDC00 && Unit <= 0xDFFF) { return false; }
		}
		return true;
	}

	bool ValidateReadContext(FReadState& State)
	{
		const FString& Namespace = State.Context.DatasetNamespace;
		if (Namespace.IsEmpty() || !Namespace.Equals(Namespace.TrimStartAndEnd(), ESearchCase::CaseSensitive) || HasControlCharacters(Namespace) || !HasValidUtf16(Namespace))
		{
			return Fail(State, Codes::InvalidContext, TEXT(""), TEXT("Supply a nonempty dataset namespace without edge whitespace, control characters, or invalid Unicode."));
		}
		const FString& Path = State.Context.RelativeFilePath;
		if (!Path.IsEmpty())
		{
			if (Path.StartsWith(TEXT("/")) || Path.Contains(TEXT("\\")) || Path.Contains(TEXT(":")) || HasControlCharacters(Path) || !HasValidUtf16(Path))
			{
				return Fail(State, Codes::InvalidContext, TEXT(""), TEXT("The source label must be a portable relative path using forward slashes."));
			}
			TArray<FString> Segments;
			Path.ParseIntoArray(Segments, TEXT("/"), false);
			for (const FString& Segment : Segments)
			{
				if (Segment.IsEmpty() || Segment.Equals(TEXT(".")) || Segment.Equals(TEXT("..")))
				{
					return Fail(State, Codes::InvalidContext, TEXT(""), TEXT("The source label cannot contain empty, current-directory, or parent-directory segments."));
				}
			}
		}
		return true;
	}

	bool ValidateDocumentText(FReadState& State)
	{
		if (State.JsonText.Len() > MaxInputLength) { return Fail(State, Codes::ResourceLimitExceeded, TEXT(""), TEXT("JSON text exceeds the input length limit.")); }
		for (TCHAR Character : State.JsonText)
		{
			if (!FChar::IsWhitespace(Character)) { return true; }
		}
		return Fail(State, Codes::EmptyInput, TEXT(""), TEXT("JSON text is empty or whitespace-only."));
	}

	bool ValidateContextAndSelection(FReadState& State, int32 Index)
	{
		if (!ValidateReadContext(State)) { return false; }
		if (Index < 0) { return Fail(State, Codes::NoSpellSelection, TEXT(""), TEXT("Select a nonnegative spell array index explicitly.")); }
		return ValidateDocumentText(State);
	}

	bool CheckAllowedKeys(const FJsonObject& Object, std::initializer_list<const TCHAR*> AllowedKeys, const FString& ObjectPointer, FReadState& State)
	{
		TArray<FString> Keys;
		for (const auto& Pair : Object.Values) { Keys.Emplace(Pair.Key.Len(), *Pair.Key); }
		Keys.Sort([](const FString& A, const FString& B) { return A.Compare(B, ESearchCase::CaseSensitive) < 0; });
		for (const FString& Key : Keys)
		{
			bool bAllowed = false;
			for (const TCHAR* Allowed : AllowedKeys)
			{
				if (Key.Equals(Allowed, ESearchCase::CaseSensitive)) { bAllowed = true; break; }
			}
			if (!bAllowed) { return Fail(State, Codes::UnsupportedRequiredValue, AppendPointer(ObjectPointer, Key), TEXT("Property is outside this reader's supported subset.")); }
		}
		return true;
	}

	bool PreflightJson(FReadState& State)
	{
		struct FJsonScope
		{
			bool bIsObject = false;
			TSet<FString> SeenNames;
		};

		TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(State.JsonText);
		TArray<FJsonScope> Scopes;
		int32 ValueCount = 0;
		EJsonNotation Notation;
		while (Reader->ReadNext(Notation))
		{
			if (Notation == EJsonNotation::Error) { return Fail(State, Codes::InvalidSyntax, TEXT(""), Reader->GetErrorMessage()); }
			const bool bStart = Notation == EJsonNotation::ObjectStart || Notation == EJsonNotation::ArrayStart;
			const bool bEnd = Notation == EJsonNotation::ObjectEnd || Notation == EJsonNotation::ArrayEnd;
			if (!bEnd)
			{
				if (++ValueCount > MaxValueCount) { return Fail(State, Codes::ResourceLimitExceeded, TEXT(""), TEXT("JSON exceeds the value count limit.")); }
				if (!Scopes.IsEmpty() && Scopes.Last().bIsObject)
				{
					const FString& Identifier = Reader->GetIdentifier();
					// FString sets and FJsonObject maps compare names without case.
					if (Scopes.Last().SeenNames.Contains(Identifier))
					{
						return Fail(State, Codes::DuplicateProperty, TEXT(""), TEXT("An object repeats a property or contains names differing only by case, which Unreal's JSON map would collapse."));
					}
					Scopes.Last().SeenNames.Add(Identifier);
				}
			}
			if (bStart)
			{
				if (Scopes.Num() >= MaxDepth) { return Fail(State, Codes::ResourceLimitExceeded, TEXT(""), TEXT("JSON exceeds the container depth limit.")); }
				FJsonScope Scope;
				Scope.bIsObject = Notation == EJsonNotation::ObjectStart;
				Scopes.Add(MoveTemp(Scope));
			}
			else if (bEnd)
			{
				if (Scopes.IsEmpty() || Scopes.Last().bIsObject != (Notation == EJsonNotation::ObjectEnd))
				{
					return Fail(State, Codes::InvalidSyntax, TEXT(""), TEXT("JSON containers do not match."));
				}
				Scopes.Pop();
			}
		}
		if (!Reader->GetErrorMessage().IsEmpty()) { return Fail(State, Codes::InvalidSyntax, TEXT(""), Reader->GetErrorMessage()); }
		if (!Scopes.IsEmpty()) { return Fail(State, Codes::InvalidSyntax, TEXT(""), TEXT("JSON contains an unclosed container.")); }
		return true;
	}

	bool ParseJsonDocument(TSharedPtr<FJsonValue>& OutRoot, FReadState& State)
	{
		TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(State.JsonText);
		TSharedPtr<FJsonValue> Root;
		if (!FJsonSerializer::Deserialize(Reader, Root, FJsonSerializer::EFlags::StoreNumbersAsStrings) || !Root.IsValid() || !Reader->GetErrorMessage().IsEmpty())
		{
			return Fail(State, Codes::InvalidSyntax, TEXT(""), TEXT("Could not deserialize JSON. ") + Reader->GetErrorMessage());
		}
		OutRoot = MoveTemp(Root);
		return true;
	}

	bool ReadSpellArray(const TSharedPtr<FJsonValue>& Root, const TArray<TSharedPtr<FJsonValue>>*& OutArray, FReadState& State)
	{
		if (!Root.IsValid() || Root->Type != EJson::Object) { return Fail(State, Codes::InvalidRoot, TEXT(""), TEXT("The document root must be an object.")); }
		TSharedPtr<FJsonObject> RootObject;
		if (!RequireObject(Root, TEXT(""), RootObject, State)) { return false; }
		const TSharedPtr<FJsonValue> SpellValue = FindExactField(*RootObject, TEXT("spell"));
		if (!SpellValue.IsValid()) { return Fail(State, Codes::MissingSpellArray, TEXT("/spell"), TEXT("The root has no exact spell property.")); }
		if (SpellValue->Type != EJson::Array) { return Fail(State, Codes::InvalidSpellArray, TEXT("/spell"), TEXT("The spell property must be an array.")); }
		return RequireArray(SpellValue, TEXT("/spell"), OutArray, State);
	}

	bool SelectSpell(const TSharedPtr<FJsonValue>& Root, int32 Index, TSharedPtr<FJsonObject>& OutSpellObject, FString& OutRecordPointer, FReadState& State)
	{
		const TArray<TSharedPtr<FJsonValue>>* Spells = nullptr;
		if (!ReadSpellArray(Root, Spells, State)) { return false; }
		const FString Pointer = AppendPointer(TEXT("/spell"), FString::FromInt(Index));
		if (!Spells->IsValidIndex(Index)) { return Fail(State, Codes::SpellIndexOutOfBounds, Pointer, TEXT("Selected spell index is outside the array.")); }
		const TSharedPtr<FJsonValue>& Selected = (*Spells)[Index];
		if (!Selected.IsValid() || Selected->Type != EJson::Object) { return Fail(State, Codes::InvalidSpellObject, Pointer, TEXT("The selected spell must be an object.")); }
		if (!RequireObject(Selected, Pointer, OutSpellObject, State)) { return false; }
		OutRecordPointer = Pointer;
		return true;
	}

	bool SerializeObject(const TSharedRef<FJsonObject>& Object, FString& OutJson, const FString& Pointer, FReadState& State)
	{
		FString Json;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Json);
		if (!FJsonSerializer::Serialize(Object, Writer)) { return Fail(State, Codes::SerializationFailure, Pointer, TEXT("Could not serialize retained metadata.")); }
		OutJson = MoveTemp(Json);
		return true;
	}

	bool IsValidIdentityText(const FString& Text)
	{
		return !Text.IsEmpty() && Text.Equals(Text.TrimStartAndEnd(), ESearchCase::CaseSensitive) && !HasControlCharacters(Text) && HasValidUtf16(Text);
	}

	bool ValidateIdentityText(const FString& Text, const FString& Pointer, FReadState& State)
	{
		if (!IsValidIdentityText(Text))
		{
			return Fail(State, Codes::InvalidFieldValue, Pointer, TEXT("Identity requires nonempty valid Unicode without edge whitespace or control characters."));
		}
		return true;
	}

	FString EscapeIdentityComponent(const FString& Text)
	{
		FTCHARToUTF8 Utf8(*Text, Text.Len());
		FString Escaped;
		const TCHAR* Hex = TEXT("0123456789ABCDEF");
		for (int32 Index = 0; Index < Utf8.Length(); ++Index)
		{
			const uint8 Byte = static_cast<uint8>(Utf8.Get()[Index]);
			const bool bUnreserved = (Byte >= 'A' && Byte <= 'Z') || (Byte >= 'a' && Byte <= 'z') || (Byte >= '0' && Byte <= '9') || Byte == '-' || Byte == '.' || Byte == '_' || Byte == '~';
			if (bUnreserved) { Escaped.AppendChar(static_cast<TCHAR>(Byte)); }
			else
			{
				Escaped.AppendChar(TEXT('%'));
				Escaped.AppendChar(Hex[Byte >> 4]);
				Escaped.AppendChar(Hex[Byte & 0x0F]);
			}
		}
		return Escaped;
	}

	FDnd5eRecordId MakeSpellRecordId(const FString& Name, const FString& Source, const FString& DatasetNamespace)
	{
		FDnd5eRecordId RecordId;
		RecordId.DatasetNamespace = DatasetNamespace;
		RecordId.Category = Dnd5e::Category::Spell;
		RecordId.SourceKey = TEXT("5etools:spell:v1:") + EscapeIdentityComponent(Name) + TEXT("|") + EscapeIdentityComponent(Source);
		return RecordId;
	}

	bool TryNormalizeSpellUid(const FString& Uid, const FString& DatasetNamespace, FDnd5eRecordId& RecordId)
	{
		RecordId = FDnd5eRecordId{};
		TArray<FString> Parts;
		Uid.ParseIntoArray(Parts, TEXT("|"), false);
		if (Parts.Num() != 2) { return false; }
		const FString Name = Parts[0].TrimStartAndEnd();
		const FString Source = Parts[1].TrimStartAndEnd();
		if (!IsValidIdentityText(Name) || !IsValidIdentityText(Source) || !IsValidIdentityText(DatasetNamespace)) { return false; }
		RecordId = MakeSpellRecordId(Name, Source, DatasetNamespace);
		return true;
	}

	bool ReadPage(const TSharedPtr<FJsonValue>& Value, const FString& Pointer, FDnd5ePageReference& OutPage, FReadState& State)
	{
		FDnd5ePageReference Page;
		if (Value->Type == EJson::Number)
		{
			FString Literal;
			if (!ReadIntegerValue(Value, Pointer, 1, MAX_int32, Page.Number, Literal, State)) { return false; }
			Page.Kind = EDnd5ePageReferenceKind::Number;
		}
		else if (Value->Type == EJson::String && Value->TryGetString(Page.Text) && !Page.Text.IsEmpty()) { Page.Kind = EDnd5ePageReferenceKind::Text; }
		else { return Fail(State, Codes::InvalidFieldValue, Pointer, TEXT("Page must be a positive int32 token or nonempty string.")); }
		OutPage = MoveTemp(Page);
		return true;
	}

	bool ReadHeader(const FJsonObject& Object, const FString& RecordPointer, FDnd5eSpell& Candidate, FReadState& State)
	{
		FString Name;
		FString Source;
		if (!ReadRequiredString(Object, TEXT("name"), RecordPointer, Name, State) || !ValidateIdentityText(Name, AppendPointer(RecordPointer, TEXT("name")), State)) { return false; }
		if (!ReadRequiredString(Object, TEXT("source"), RecordPointer, Source, State) || !ValidateIdentityText(Source, AppendPointer(RecordPointer, TEXT("source")), State)) { return false; }
		FDnd5eRecordHeader Header;
		Header.RecordId = MakeSpellRecordId(Name, Source, State.Context.DatasetNamespace);
		Header.DisplayName = Name;
		FDnd5eRecordProvenance Provenance;
		Provenance.SourceId = Source;
		Provenance.RelativeFilePath = State.Context.RelativeFilePath;
		Provenance.JsonPointer = RecordPointer;
		if (const TSharedPtr<FJsonValue> Page = FindExactField(Object, TEXT("page")))
		{
			if (!ReadPage(Page, AppendPointer(RecordPointer, TEXT("page")), Provenance.Page, State)) { return false; }
		}
		TSharedRef<FJsonObject> Identity = MakeShared<FJsonObject>();
		Identity->SetStringField(TEXT("name"), Name);
		Identity->SetStringField(TEXT("source"), Source);
		if (!SerializeObject(Identity, Provenance.IdentityFieldsJson, RecordPointer, State)) { return false; }
		Header.Provenance.Add(MoveTemp(Provenance));
		Candidate.Header = MoveTemp(Header);
		State.SourceId = MoveTemp(Source);
		return true;
	}

	struct FTagMapping
	{
		const TCHAR* Source;
		FGameplayTag Tag;
	};

	bool ReadMappedTag(const FString& Text, std::initializer_list<FTagMapping> Mappings, const FString& Pointer, FGameplayTag& OutTag, FReadState& State)
	{
		for (const FTagMapping& Mapping : Mappings)
		{
			if (Text.Equals(Mapping.Source, ESearchCase::CaseSensitive)) { OutTag = Mapping.Tag; return true; }
		}
		return Fail(State, Codes::UnsupportedRequiredValue, Pointer, TEXT("The source value is not supported by this reader."));
	}

	bool ReadLevelAndSchool(const FJsonObject& Object, const FString& RecordPointer, FDnd5eSpell& Candidate, FReadState& State)
	{
		FString Literal;
		if (!ReadRequiredInteger(Object, TEXT("level"), RecordPointer, 0, 9, Candidate.Level, Literal, State)) { return false; }
		FString School;
		FGameplayTag Tag;
		if (!ReadRequiredString(Object, TEXT("school"), RecordPointer, School, State)) { return false; }
		if (!ReadMappedTag(School, {
			{TEXT("A"), Dnd5e::Magic::School::Abjuration}, {TEXT("C"), Dnd5e::Magic::School::Conjuration}, {TEXT("D"), Dnd5e::Magic::School::Divination},
			{TEXT("E"), Dnd5e::Magic::School::Enchantment}, {TEXT("V"), Dnd5e::Magic::School::Evocation}, {TEXT("I"), Dnd5e::Magic::School::Illusion},
			{TEXT("N"), Dnd5e::Magic::School::Necromancy}, {TEXT("T"), Dnd5e::Magic::School::Transmutation}, {TEXT("P"), Dnd5e::Magic::School::Psionic}
		}, AppendPointer(RecordPointer, TEXT("school")), Tag, State)) { return false; }
		Candidate.Schools.AddTag(Tag);
		return true;
	}

	FDnd5eContentMetadata MakeContentMetadata(const FString& Pointer, const FReadState& State)
	{
		FDnd5eContentMetadata Metadata;
		Metadata.SourceId = State.SourceId;
		Metadata.RelativeFilePath = State.Context.RelativeFilePath;
		Metadata.JsonPointer = Pointer;
		return Metadata;
	}

	bool ReadTextBlock(const FString& Text, const FString& Pointer, FDnd5eContentBlock& OutBlock, FReadState& State)
	{
		return ReadContent(MakeShared<FJsonValueString>(Text), Pointer, OutBlock, State);
	}
	bool ReadTextArray(const TSharedPtr<FJsonValue>& Value, const FString& Pointer, bool bRequireNonempty, TArray<FDnd5eContentBlock>& OutBlocks, FReadState& State)
	{
		const TArray<TSharedPtr<FJsonValue>>* Values = nullptr;
		if (!RequireArray(Value, Pointer, Values, State)) { return false; }
		if (bRequireNonempty && Values->IsEmpty()) { return Fail(State, Codes::InvalidFieldValue, Pointer, TEXT("The description array must not be empty.")); }
		return ReadBlocks(Value, Pointer, OutBlocks, State);
	}
	bool ReadCastingTimes(const FJsonObject& Object, const FString& RecordPointer, FDnd5eSpell& Candidate, FReadState& State)
	{
		const FString Pointer = AppendPointer(RecordPointer, TEXT("time"));
		const TArray<TSharedPtr<FJsonValue>>* Values = nullptr;
		if (!ReadRequiredArray(Object, TEXT("time"), RecordPointer, Values, State)) { return false; }
		if (Values->IsEmpty()) { return Fail(State, Codes::InvalidFieldValue, Pointer, TEXT("At least one casting time is required.")); }
		TArray<FDnd5eCastingTime> Times;
		for (int32 Index = 0; Index < Values->Num(); ++Index)
		{
			const FString EntryPointer = AppendPointer(Pointer, FString::FromInt(Index));
			TSharedPtr<FJsonObject> Entry;
			if (!RequireObject((*Values)[Index], EntryPointer, Entry, State)) { return false; }
			if (!CheckAllowedKeys(*Entry, {TEXT("number"), TEXT("unit"), TEXT("condition"), TEXT("note")}, EntryPointer, State)) { return false; }
			FDnd5eCastingTime Time;
			FString Literal;
			FString Unit;
			if (!ReadRequiredString(*Entry, TEXT("unit"), EntryPointer, Unit, State)) { return false; }
			if (!Unit.Equals(TEXT("special"), ESearchCase::CaseSensitive) || FindExactField(*Entry, TEXT("number")).IsValid())
			{
				if (!ReadRequiredInteger(*Entry, TEXT("number"), EntryPointer, 1, MAX_int32, Time.Quantity, Literal, State)) { return false; }
			}
			if (!ReadMappedTag(Unit, {
				{TEXT("action"), Dnd5e::Activation::Time::Action}, {TEXT("bonus"), Dnd5e::Activation::Time::BonusAction}, {TEXT("reaction"), Dnd5e::Activation::Time::Reaction},
				{TEXT("round"), Dnd5e::Activation::Time::Round}, {TEXT("minute"), Dnd5e::Activation::Time::Minute}, {TEXT("hour"), Dnd5e::Activation::Time::Hour}, {TEXT("special"), Dnd5e::Activation::Time::Special}
			}, AppendPointer(EntryPointer, TEXT("unit")), Time.Unit, State)) { return false; }
			for (const TCHAR* Field : {TEXT("condition"), TEXT("note")})
			{
				if (const TSharedPtr<FJsonValue> Value = FindExactField(*Entry, Field))
				{
					FString Text;
					if (!ReadRequiredString(*Entry, Field, EntryPointer, Text, State)) { return false; }
					FDnd5eContentBlock Block;
					if (!ReadTextBlock(Text, AppendPointer(EntryPointer, Field), Block, State)) { return false; }
					if (FCString::Strcmp(Field, TEXT("condition")) == 0) { Time.Condition.Add(MoveTemp(Block)); }
					else { Time.Note.Add(MoveTemp(Block)); }
				}
			}
			Times.Add(MoveTemp(Time));
		}
		Candidate.CastingTimes = MoveTemp(Times);
		return true;
	}

	bool ReadTimeUnit(const FString& Type, const FString& Pointer, EDnd5eTimeUnit& OutUnit, FReadState& State)
	{
		struct FUnitMapping { const TCHAR* Source; EDnd5eTimeUnit Unit; };
		const FUnitMapping Units[] = {
			{TEXT("turn"), EDnd5eTimeUnit::Turn}, {TEXT("round"), EDnd5eTimeUnit::Round}, {TEXT("minute"), EDnd5eTimeUnit::Minute}, {TEXT("hour"), EDnd5eTimeUnit::Hour},
			{TEXT("day"), EDnd5eTimeUnit::Day}, {TEXT("week"), EDnd5eTimeUnit::Week}, {TEXT("month"), EDnd5eTimeUnit::Month}, {TEXT("year"), EDnd5eTimeUnit::Year}
		};
		for (const auto& Unit : Units)
		{
			if (Type.Equals(Unit.Source, ESearchCase::CaseSensitive)) { OutUnit = Unit.Unit; return true; }
		}
		return Fail(State, Codes::UnsupportedRequiredValue, Pointer, TEXT("Unsupported duration time unit."));
	}

	bool ReadDescriptions(const FJsonObject& Object, const FString& RecordPointer, FDnd5eSpell& Candidate, FReadState& State)
	{
		TSharedPtr<FJsonValue> Entries;
		if (!ReadRequiredValue(Object, TEXT("entries"), EJson::Array, RecordPointer, Entries, State)) { return false; }
		if (!ReadTextArray(Entries, AppendPointer(RecordPointer, TEXT("entries")), true, Candidate.Description, State)) { return false; }
		if (const TSharedPtr<FJsonValue> Higher = FindExactField(Object, TEXT("entriesHigherLevel")))
		{
			if (!ReadTextArray(Higher, AppendPointer(RecordPointer, TEXT("entriesHigherLevel")), false, Candidate.HigherLevelDescription, State)) { return false; }
		}
		return true;
	}

	bool ReadStringArray(const TSharedPtr<FJsonValue>& Value, const FString& Pointer, bool bNonemptyElements, TArray<FString>& OutStrings, FReadState& State)
	{
		const TArray<TSharedPtr<FJsonValue>>* Values = nullptr;
		if (!RequireArray(Value, Pointer, Values, State)) { return false; }
		TArray<FString> Strings;
		for (int32 Index = 0; Index < Values->Num(); ++Index)
		{
			FString Text;
			const auto& Entry = (*Values)[Index];
			if (Entry->Type != EJson::String || !Entry->TryGetString(Text) || (bNonemptyElements && Text.IsEmpty()))
			{
				return Fail(State, Codes::InvalidFieldValue, AppendPointer(Pointer, FString::FromInt(Index)), TEXT("Expected a string satisfying this field's empty-value policy."));
			}
			Strings.Add(MoveTemp(Text));
		}
		OutStrings = MoveTemp(Strings);
		return true;
	}

	bool ValidateSourceReferences(const TSharedPtr<FJsonValue>& Value, const FString& Pointer, FReadState& State)
	{
		const TArray<TSharedPtr<FJsonValue>>* Values = nullptr;
		if (!RequireArray(Value, Pointer, Values, State)) { return false; }
		for (int32 Index = 0; Index < Values->Num(); ++Index)
		{
			const FString EntryPointer = AppendPointer(Pointer, FString::FromInt(Index));
			TSharedPtr<FJsonObject> Entry;
			if (!RequireObject((*Values)[Index], EntryPointer, Entry, State)) { return false; }
			if (!CheckAllowedKeys(*Entry, {TEXT("source"), TEXT("page")}, EntryPointer, State)) { return false; }
			FString Source;
			if (!ReadRequiredString(*Entry, TEXT("source"), EntryPointer, Source, State)) { return false; }
			if (Source.IsEmpty()) { return Fail(State, Codes::InvalidFieldValue, AppendPointer(EntryPointer, TEXT("source")), TEXT("Source reference must not be empty.")); }
			if (const TSharedPtr<FJsonValue> Page = FindExactField(*Entry, TEXT("page")))
			{
				FDnd5ePageReference ParsedPage;
				if (!ReadPage(Page, AppendPointer(EntryPointer, TEXT("page")), ParsedPage, State)) { return false; }
			}
		}
		return true;
	}

	bool ReadMetadataAndRelationships(const FJsonObject& Object, const FString& RecordPointer, FDnd5eSpell& Candidate, FReadState& State)
	{
		if (const TSharedPtr<FJsonValue> Alias = FindExactField(Object, TEXT("alias")))
		{
			if (!ReadStringArray(Alias, AppendPointer(RecordPointer, TEXT("alias")), false, Candidate.Header.Aliases, State)) { return false; }
		}
		const TSharedPtr<FJsonValue> MetaValue = FindExactField(Object, TEXT("meta"));
		if (MetaValue.IsValid())
		{
			const FString Pointer = AppendPointer(RecordPointer, TEXT("meta"));
			TSharedPtr<FJsonObject> Meta;
			if (!RequireObject(MetaValue, Pointer, Meta, State) || !CheckAllowedKeys(*Meta, {TEXT("ritual")}, Pointer, State)) { return false; }
			bool bRitual = false;
			if (!ReadOptionalBool(*Meta, TEXT("ritual"), Pointer, bRitual, State)) { return false; }
			if (bRitual) { Candidate.Traits.AddTag(Dnd5e::Spell::Trait::Ritual); }
		}
		FDnd5eRecordProvenance& Provenance = Candidate.Header.Provenance[0];
		TSharedRef<FJsonObject> Evidence = MakeShared<FJsonObject>();
		Evidence->SetField(TEXT("school"), FindExactField(Object, TEXT("school")));
		if (MetaValue.IsValid()) { Evidence->SetField(TEXT("meta"), MetaValue); }
		for (const TCHAR* Field : {TEXT("miscTags"), TEXT("areaTags")})
		{
			if (const TSharedPtr<FJsonValue> Value = FindExactField(Object, Field))
			{
				TArray<FString> Checked;
				if (!ReadStringArray(Value, AppendPointer(RecordPointer, Field), false, Checked, State)) { return false; }
				Evidence->SetField(Field, Value);
			}
		}
		if (!SerializeObject(Evidence, Provenance.ClassificationEvidenceJson, RecordPointer, State)) { return false; }
		TSharedRef<FJsonObject> Metadata = MakeShared<FJsonObject>();
		for (const TCHAR* Field : {TEXT("srd"), TEXT("srd52"), TEXT("basicRules"), TEXT("basicRules2024"), TEXT("hasFluff"), TEXT("hasFluffImages")})
		{
			if (const TSharedPtr<FJsonValue> Value = FindExactField(Object, Field))
			{
				const bool bAllowsString = FCString::Strcmp(Field, TEXT("srd")) == 0 || FCString::Strcmp(Field, TEXT("srd52")) == 0;
				if (Value->Type != EJson::Boolean && !(bAllowsString && Value->Type == EJson::String))
				{
					return Fail(State, Codes::InvalidFieldValue, AppendPointer(RecordPointer, Field), TEXT("Metadata has an unsupported JSON type."));
				}
				Metadata->SetField(Field, Value);
			}
		}
		for (const TCHAR* Field : {TEXT("otherSources"), TEXT("additionalSources")})
		{
			if (const TSharedPtr<FJsonValue> Value = FindExactField(Object, Field))
			{
				if (!ValidateSourceReferences(Value, AppendPointer(RecordPointer, Field), State)) { return false; }
				Metadata->SetField(Field, Value);
			}
		}
		if (const TSharedPtr<FJsonValue> References = FindExactField(Object, TEXT("referenceSources")))
		{
			TArray<FString> Checked;
			if (!ReadStringArray(References, AppendPointer(RecordPointer, TEXT("referenceSources")), true, Checked, State)) { return false; }
			Metadata->SetField(TEXT("referenceSources"), References);
		}
		if (!Metadata->Values.IsEmpty() && !SerializeObject(Metadata, Provenance.AdditionalMetadataJson, RecordPointer, State)) { return false; }
		if (!ReadTypedSpellMetadata(Object, RecordPointer, Candidate, State)) { return false; }
		if (const TSharedPtr<FJsonValue> Value = FindExactField(Object, TEXT("reprintedAs")))
		{
			const FString Pointer = AppendPointer(RecordPointer, TEXT("reprintedAs"));
			TArray<FString> References;
			if (!ReadStringArray(Value, Pointer, true, References, State)) { return false; }
			for (int32 Index = 0; Index < References.Num(); ++Index)
			{
				FDnd5eRecordRelationship Relationship;
				Relationship.Kind = Dnd5e::Record::Relationship::ReprintedAs;
				Relationship.SourceFormat = TEXT("5etools");
				Relationship.SourceReference = References[Index];
				Relationship.bHasTargetRecord = TryNormalizeSpellUid(Relationship.SourceReference, State.Context.DatasetNamespace, Relationship.TargetRecordId);
				if (!Relationship.bHasTargetRecord)
				{
					AddDiagnostic(State, EDnd5eDiagnosticSeverity::Warning, Codes::UnresolvedReference, AppendPointer(Pointer, FString::FromInt(Index)), TEXT("Reprint selector retained without a normalized target; this mapping requires an explicit name|source selector."));
				}
				Candidate.Relationships.Add(MoveTemp(Relationship));
			}
		}
		return true;
	}

	bool CheckSpellKeys(const FJsonObject& Object, const FString& RecordPointer, FReadState& State)
	{
		return CheckAllowedKeys(Object, {
			TEXT("name"), TEXT("source"), TEXT("page"), TEXT("alias"), TEXT("level"), TEXT("school"), TEXT("time"), TEXT("range"), TEXT("components"), TEXT("duration"),
			TEXT("entries"), TEXT("entriesHigherLevel"), TEXT("meta"), TEXT("reprintedAs"), TEXT("srd"), TEXT("srd52"), TEXT("basicRules"), TEXT("basicRules2024"),
			TEXT("otherSources"), TEXT("additionalSources"), TEXT("referenceSources"), TEXT("hasFluff"), TEXT("hasFluffImages"), TEXT("miscTags"), TEXT("areaTags"),
			TEXT("damageInflict"), TEXT("damageResist"), TEXT("damageImmune"), TEXT("damageVulnerable"), TEXT("conditionInflict"), TEXT("conditionImmune"),
			TEXT("savingThrow"), TEXT("abilityCheck"), TEXT("affectsCreatureType"), TEXT("spellAttack"), TEXT("scalingLevelDice")
		}, RecordPointer, State);
	}

	bool ReadSpellObject(const FJsonObject& Object, const FString& Pointer, FDnd5eSpell& OutSpell, FReadState& State)
	{
		FDnd5eSpell Candidate;
		if (!CheckSpellKeys(Object, Pointer, State) || !ReadHeader(Object, Pointer, Candidate, State) || !ReadLevelAndSchool(Object, Pointer, Candidate, State)) { return false; }
		if (!ReadCastingTimes(Object, Pointer, Candidate, State) || !ReadRange(Object, Pointer, Candidate, State) || !ReadComponents(Object, Pointer, Candidate, State)) { return false; }
		if (!ReadDurations(Object, Pointer, Candidate, State) || !ReadDescriptions(Object, Pointer, Candidate, State)) { return false; }
		if (!ReadCombatClassifications(Object, Pointer, Candidate, State) || !ReadLevelScaling(Object, Pointer, Candidate, State) || !ReadMetadataAndRelationships(Object, Pointer, Candidate, State)) { return false; }
		OutSpell = MoveTemp(Candidate);
		return true;
	}
}

FDnd5eJsonSpellReadResult FDnd5eJsonSpellReader::ReadSpell(const FDnd5eJsonSpellReadRequest& Request)
{
	using namespace Dnd5eJsonReader;
	FDnd5eJsonSpellReadResult Result;
	Result.Status = EDnd5eJsonReadStatus::Failure;
	FReadState State{Request.Context, Request.JsonText, Result.Diagnostics, FString{}};
	TSharedPtr<FJsonValue> Root;
	TSharedPtr<FJsonObject> SpellObject;
	FString RecordPointer;
	FDnd5eSpell Candidate;

	if (!ValidateContextAndSelection(State, Request.SpellIndex) || !PreflightJson(State) || !ParseJsonDocument(Root, State) || !SelectSpell(Root, Request.SpellIndex, SpellObject, RecordPointer, State)) { return Result; }
	if (!ReadSpellObject(*SpellObject, RecordPointer, Candidate, State)) { return Result; }

	Result.Spell = MoveTemp(Candidate);
	Result.Status = EDnd5eJsonReadStatus::Success;
	return Result;
}
