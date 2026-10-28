// Developed by Neko Creative Technologies

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Dnd5eRecordId.generated.h"

USTRUCT(BlueprintType)
struct DND5E_API FDnd5eRecordId
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|RecordId")
	FString DatasetNamespace;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|RecordId")
	FGameplayTag Category;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|RecordId")
	FString SourceKey;
};