// Developed by Neko Creative Technologies

#pragma once

#include "Readers/Dnd5eJsonReadContext.h"
#include "Diagnostics/Dnd5eDiagnostic.h"
#include "Spells/Dnd5eSpell.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include <initializer_list>

struct FDnd5eItem;

namespace Dnd5eJsonReader
{
	struct FReadBudget
	{
		int32 ContentValues = 0;
	};

	struct FReadFieldOrigin
	{
		FString LogicalPointer;
		FString InputPointer;
		FString SourceId;
		bool bExactSubtree = false;
	};

	struct FReadFieldOrigins
	{
		FString RowPrefix;
		TMap<FString, FReadFieldOrigin> Fields;
	};

	struct FReadState
	{
		const FDnd5eJsonReadContext& Context;
		const FString& JsonText;
		TArray<FDnd5eDiagnostic>& Diagnostics;
		FString SourceId;
		TSharedRef<FReadBudget> Budget = MakeShared<FReadBudget>();
		const FReadFieldOrigins* FieldOrigins = nullptr;
		bool bUseFieldSourceId = true;
	};

	namespace Codes
	{
		extern const FName InvalidFieldValue;
		extern const FName MissingRequiredField;
		extern const FName UnsupportedRequiredValue;
		extern const FName PreservedUnsupportedContent;
		extern const FName PreservedUnsupportedMetadata;
		extern const FName UnresolvedReference;
		extern const FName SerializationFailure;
		extern const FName ResourceLimitExceeded;
	}

	void AddDiagnostic(FReadState& State, EDnd5eDiagnosticSeverity Severity, FName Code, const FString& Pointer, const FString& Message);
	bool Fail(FReadState& State, FName Code, const FString& Pointer, const FString& Message);
	FString AppendPointer(const FString& Parent, const FString& Segment);
	TSharedPtr<FJsonValue> FindExactField(const FJsonObject& Object, const TCHAR* FieldName);
	bool ReadRequiredValue(const FJsonObject& Object, const TCHAR* FieldName, EJson ExpectedType, const FString& ObjectPointer, TSharedPtr<FJsonValue>& OutValue, FReadState& State);
	bool ReadRequiredString(const FJsonObject& Object, const TCHAR* FieldName, const FString& ObjectPointer, FString& OutText, FReadState& State);
	bool RequireObject(const TSharedPtr<FJsonValue>& Value, const FString& Pointer, TSharedPtr<FJsonObject>& OutObject, FReadState& State);
	bool RequireArray(const TSharedPtr<FJsonValue>& Value, const FString& Pointer, const TArray<TSharedPtr<FJsonValue>>*& OutArray, FReadState& State);
	bool ReadRequiredObject(const FJsonObject& Object, const TCHAR* FieldName, const FString& ObjectPointer, TSharedPtr<FJsonObject>& OutObject, FReadState& State);
	bool ReadRequiredArray(const FJsonObject& Object, const TCHAR* FieldName, const FString& ObjectPointer, const TArray<TSharedPtr<FJsonValue>>*& OutArray, FReadState& State);
	bool ReadOptionalBool(const FJsonObject& Object, const TCHAR* FieldName, const FString& ObjectPointer, bool& OutValue, FReadState& State);
	bool ReadRequiredInteger(const FJsonObject& Object, const TCHAR* FieldName, const FString& ObjectPointer, int32 Minimum, int32 Maximum, int32& OutInteger, FString& OutLiteral, FReadState& State);
	bool CheckAllowedKeys(const FJsonObject& Object, std::initializer_list<const TCHAR*> AllowedKeys, const FString& ObjectPointer, FReadState& State);
	bool IsValidIdentityText(const FString& Text);
	bool ValidateIdentityText(const FString& Text, const FString& Pointer, FReadState& State);
	FString EscapeIdentityComponent(const FString& Text);
	FDnd5eRecordId MakeSpellRecordId(const FString& Name, const FString& Source, const FString& DatasetNamespace);
	bool TryNormalizeSpellUid(const FString& Uid, const FString& DatasetNamespace, FDnd5eRecordId& RecordId);
	bool ReadPage(const TSharedPtr<FJsonValue>& Value, const FString& Pointer, FDnd5ePageReference& OutPage, FReadState& State);
	FDnd5eContentMetadata MakeContentMetadata(const FString& Pointer, const FReadState& State);
	bool ReadNumberValue(const TSharedPtr<FJsonValue>& Value, const FString& Pointer, FDnd5eNumber& OutNumber, FReadState& State);
	bool ReadNonnegativeNumber(const TSharedPtr<FJsonValue>& Value, const FString& Pointer, FDnd5eNumber& OutNumber, FReadState& State);
	bool TryParseBoundedIntegerToken(const FString& Token, int32 Minimum, int32 Maximum, int32& OutValue);
	bool ReadRange(const FJsonObject& Object, const FString& RecordPointer, FDnd5eSpell& Candidate, FReadState& State);
	bool ReadComponents(const FJsonObject& Object, const FString& RecordPointer, FDnd5eSpell& Candidate, FReadState& State);
	bool ReadDurations(const FJsonObject& Object, const FString& RecordPointer, FDnd5eSpell& Candidate, FReadState& State);
	bool ReadTimeUnit(const FString& Type, const FString& Pointer, EDnd5eTimeUnit& OutUnit, FReadState& State);
	bool ReadCombatClassifications(const FJsonObject& Object, const FString& RecordPointer, FDnd5eSpell& Candidate, FReadState& State);
	bool ReadLevelScaling(const FJsonObject& Object, const FString& RecordPointer, FDnd5eSpell& Candidate, FReadState& State);
	bool ReadInlines(const FString& Text, const FString& Pointer, TArray<FDnd5eInlineContent>& OutInlines, FReadState& State, int32 Depth = 0);
	bool ReadContent(const TSharedPtr<FJsonValue>& Value, const FString& Pointer, FDnd5eContentBlock& OutBlock, FReadState& State, int32 Depth = 0);
	bool ReadBlocks(const TSharedPtr<FJsonValue>& Value, const FString& Pointer, TArray<FDnd5eContentBlock>& OutBlocks, FReadState& State, int32 Depth = 0);
	bool ValidateReadContext(FReadState& State);
	bool ValidateDocumentText(FReadState& State);
	bool PreflightJson(FReadState& State);
	bool ParseJsonDocument(TSharedPtr<FJsonValue>& OutRoot, FReadState& State);
	bool ReadSpellArray(const TSharedPtr<FJsonValue>& Root, const TArray<TSharedPtr<FJsonValue>>*& OutArray, FReadState& State);
	bool ReadSpellObject(const FJsonObject& Object, const FString& Pointer, FDnd5eSpell& OutSpell, FReadState& State);
	bool ReadAvailabilityEntry(const FJsonObject& Root, const FString& SpellName, const FString& SpellSourceId, bool& bHasEntry, TArray<FDnd5eSpellAvailability>& Availability, FReadState& State);
	void UnsupportedInline(const FString& Token, const FString& Pointer, TArray<FDnd5eInlineContent>& Inlines, FReadState& State);
	bool ReadAdditionalAnnotation(const FString& Tag, const TArray<FString>& Parts, const FString& Token, const FString& Pointer, TArray<FDnd5eInlineContent>& Inlines, FReadState& State, int32 Depth, bool& bHandled);
	bool IsRecordReferenceTag(const FString& Tag);
	bool TryReadRecordSelector(const FString& Tag, const TArray<FString>& Parts, FDnd5eRecordSelector& Selector, FString& Display);
	bool ReadRecordReference(const FString& Tag, const TArray<FString>& Parts, const FString& Token, const FString& Pointer, TArray<FDnd5eInlineContent>& Inlines, FReadState& State, int32 Depth);
	bool ConsumeContentBudget(const FString& Pointer, FReadState& State);
	bool ReadDestination(const FString& Tag, const TArray<FString>& Parts, const FString& Token, const FString& Pointer, TArray<FDnd5eInlineContent>& Inlines, FReadState& State, int32 Depth, bool& Handled);
	bool ReadTypedSpellMetadata(const FJsonObject& Object, const FString& Pointer, FDnd5eSpell& Spell, FReadState& State);
	bool ValidateEditionAssignments(const TArray<FDnd5eSourceEditionAssignment>& Assignments, const FString& PolicyId, FReadState& State);
	bool ApplyEditionAssignment(const TArray<FDnd5eSourceEditionAssignment>& Assignments, const FString& PolicyId, FDnd5eSpell& Spell, FReadState& State);
	bool SerializeValue(const TSharedPtr<FJsonValue>& Value, FString& OutJson, const FString& Pointer, FReadState& State);
	bool ReadStandaloneItemRow(const FJsonObject& Object, const FString& Pointer, FDnd5eItem& Item, FReadState& State);
}
