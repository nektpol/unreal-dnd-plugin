// Developed by Neko Creative Technologies

#pragma once
#include "CoreMinimal.h"
#include "Content/Dnd5eContentType.h"
#include "Diagnostics/Dnd5eDiagnostic.h"
#include "Items/Dnd5eItemEntryTemplate.h"
#include "Readers/Dnd5eJsonReadStatus.h"
#include "Dnd5eJsonItemEntryExpander.generated.h"

USTRUCT(BlueprintType)
struct DND5EJSON_API FDnd5eJsonItemEntryExpansionRequest
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Items")
	FString DatasetNamespace;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Items")
	FDnd5eInlineContent Reference;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Items")
	TArray<FDnd5eItemEntryTemplate> Templates;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Items")
	FString OmittedSourceId;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Items")
	bool bHasDetail1 = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Items")
	FString Detail1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Items")
	bool bHasDetail2 = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Items")
	FString Detail2;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Items")
	bool bHasResistances = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Items")
	TArray<FString> Resistances;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Items")
	bool bTitleCaseDamageNames = false;
};

USTRUCT(BlueprintType)
struct DND5EJSON_API FDnd5eJsonItemEntryExpansionResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Dnd5e|Items")
	EDnd5eJsonReadStatus Status = EDnd5eJsonReadStatus::NotAttempted;
	UPROPERTY(BlueprintReadOnly, Category = "Dnd5e|Items")
	TArray<FDnd5eContentBlock> Blocks;
	UPROPERTY(BlueprintReadOnly, Category = "Dnd5e|Items")
	FDnd5eRecordId TemplateId;
	UPROPERTY(BlueprintReadOnly, Category = "Dnd5e|Items")
	FDnd5eRecordProvenance TemplateOrigin;
	UPROPERTY(BlueprintReadOnly, Category = "Dnd5e|Items")
	TArray<FDnd5eDiagnostic> Diagnostics;
};

class DND5EJSON_API FDnd5eJsonItemEntryExpander
{
public:
	static FDnd5eJsonItemEntryExpansionResult Expand(const FDnd5eJsonItemEntryExpansionRequest& Request);
};