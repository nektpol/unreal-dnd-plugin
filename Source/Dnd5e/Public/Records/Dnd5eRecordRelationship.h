// Developed by Neko Creative Technologies

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Records/Dnd5eRecordId.h"
#include "Dnd5eRecordRelationship.generated.h"

USTRUCT(BlueprintType)
struct DND5E_API FDnd5eRecordRelationship
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|RecordRelationship")
	FGameplayTag Kind;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|RecordRelationship")
	bool bHasTargetRecord = false;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|RecordRelationship", meta = (EditCondition = "bHasTargetRecord"))
	FDnd5eRecordId TargetRecordId;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|RecordRelationship")
	FString SourceFormat;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|RecordRelationship")
	FString SourceReference;
};

