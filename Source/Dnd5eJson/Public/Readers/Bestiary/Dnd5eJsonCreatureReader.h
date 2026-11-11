// Developed by Neko Creative Technologies

#pragma once

#include "CoreMinimal.h"
#include "Bestiary/Dnd5eCreature.h"
#include "Diagnostics/Dnd5eDiagnostic.h"
#include "Readers/Dnd5eJsonDocument.h"
#include "Readers/Dnd5eJsonReadStatus.h"
#include "Dnd5eJsonCreatureReader.generated.h"

struct DND5EJSON_API FDnd5eCreatureFieldShape
{
	FString Path;
	FString JsonType;
	FString JsonPointer;
};

struct DND5EJSON_API FDnd5eCreatureAuditRow
{
	FString Name;
	FString SourceId;
	FString JsonPointer;
	bool bIsCopy = false;
	EDnd5eJsonReadStatus Status = EDnd5eJsonReadStatus::NotAttempted;
	TArray<FDnd5eDiagnostic> Diagnostics;
	TArray<FDnd5eCreatureFieldShape> FieldShapes;
};

struct DND5EJSON_API FDnd5eCreatureDocumentAudit
{
	EDnd5eJsonReadStatus Status = EDnd5eJsonReadStatus::NotAttempted;
	TArray<FDnd5eDiagnostic> Diagnostics;
	TArray<FDnd5eCreatureAuditRow> Rows;
};

USTRUCT(BlueprintType)
struct DND5EJSON_API FDnd5eJsonCreatureReadResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Dnd5e|Bestiary")
	EDnd5eJsonReadStatus Status = EDnd5eJsonReadStatus::NotAttempted;

	UPROPERTY(BlueprintReadOnly, Category = "Dnd5e|Bestiary")
	FDnd5eCreature Creature;

	UPROPERTY(BlueprintReadOnly, Category = "Dnd5e|Bestiary")
	TArray<FDnd5eDiagnostic> Diagnostics;
};

class DND5EJSON_API FDnd5eJsonCreatureReader
{
public:
	static FDnd5eJsonCreatureReadResult ReadCreature(const FDnd5eJsonDocument& Document, const FString& Name, const FString& SourceId);
	static FDnd5eCreatureDocumentAudit AuditDocument(const FDnd5eJsonDocument& Document);
};