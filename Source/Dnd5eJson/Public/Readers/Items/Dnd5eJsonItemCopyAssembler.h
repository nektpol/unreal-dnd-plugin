// Developed by Neko Creative Technologies

#pragma once
#include "CoreMinimal.h"
#include "Diagnostics/Dnd5eDiagnostic.h"
#include "Items/Dnd5eItem.h"
#include "Readers/Dnd5eJsonDocument.h"
#include "Readers/Dnd5eJsonReadStatus.h"
#include "Dnd5eJsonItemCopyAssembler.generated.h"

USTRUCT(BlueprintType)
struct DND5EJSON_API FDnd5eJsonItemCopyAssemblyResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Dnd5e|Items")
	EDnd5eJsonReadStatus Status = EDnd5eJsonReadStatus::NotAttempted;
	UPROPERTY(BlueprintReadOnly, Category = "Dnd5e|Items")
	FString AssembledJson;
	UPROPERTY(BlueprintReadOnly, Category = "Dnd5e|Items")
	TArray<int32> SourceRowIndices;
	UPROPERTY(BlueprintReadOnly, Category = "Dnd5e|Items")
	TMap<FString, int32> FieldSourceRows;
	UPROPERTY(BlueprintReadOnly, Category = "Dnd5e|Items")
	TArray<FDnd5eDiagnostic> Diagnostics;
};

USTRUCT(BlueprintType)
struct DND5EJSON_API FDnd5eJsonItemCopyReadResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Dnd5e|Items")
	EDnd5eJsonReadStatus Status = EDnd5eJsonReadStatus::NotAttempted;
	UPROPERTY(BlueprintReadOnly, Category = "Dnd5e|Items")
	FDnd5eItem Item;
	UPROPERTY(BlueprintReadOnly, Category = "Dnd5e|Items")
	TArray<int32> SourceRowIndices;
	UPROPERTY(BlueprintReadOnly, Category = "Dnd5e|Items")
	TMap<FString, FString> FieldSourcePointers;
	UPROPERTY(BlueprintReadOnly, Category = "Dnd5e|Items")
	TArray<FDnd5eDiagnostic> Diagnostics;
};

class DND5EJSON_API FDnd5eJsonItemCopyAssembler
{
public:
	static FDnd5eJsonItemCopyAssemblyResult AssembleAtIndex(const FDnd5eJsonDocument& Document, int32 Index);
	static FDnd5eJsonItemCopyReadResult ReadCopyAtIndex(const FDnd5eJsonDocument& Document, int32 Index);
};
