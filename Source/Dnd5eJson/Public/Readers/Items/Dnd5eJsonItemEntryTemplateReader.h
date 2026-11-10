// Developed by Neko Creative Technologies

#pragma once
#include "CoreMinimal.h"
#include "Diagnostics/Dnd5eDiagnostic.h"
#include "Items/Dnd5eItemEntryTemplate.h"
#include "Readers/Dnd5eJsonDocument.h"
#include "Readers/Dnd5eJsonReadStatus.h"
#include "Dnd5eJsonItemEntryTemplateReader.generated.h"

USTRUCT(BlueprintType)
struct DND5EJSON_API FDnd5eJsonItemEntryTemplateReadResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Dnd5e|Items")
	EDnd5eJsonReadStatus Status = EDnd5eJsonReadStatus::NotAttempted;
	UPROPERTY(BlueprintReadOnly, Category = "Dnd5e|Items")
	FDnd5eItemEntryTemplate EntryTemplate;
	UPROPERTY(BlueprintReadOnly, Category = "Dnd5e|Items")
	TArray<FDnd5eDiagnostic> Diagnostics;
};

class DND5EJSON_API FDnd5eJsonItemEntryTemplateReader
{
public:
	static FDnd5eJsonItemEntryTemplateReadResult ReadAtIndex(const FDnd5eJsonDocument& Document, int32 Index);
};