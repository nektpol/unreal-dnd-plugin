// Developed by Neko Creative Technologies

#pragma once
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "StructUtils/InstancedStruct.h"
#include "Dnd5eRecordSelector.generated.h"

USTRUCT(BlueprintType)
struct DND5E_API FDnd5eClassFeatureSelector
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dnd5e|Selector")
	FString ClassName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dnd5e|Selector")
	bool bHasClassSource = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dnd5e|Selector")
	FString ClassSourceId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dnd5e|Selector")
	int32 Level = -1;
};

USTRUCT(BlueprintType)
struct DND5E_API FDnd5eRecordSelector
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dnd5e|Selector")
	FGameplayTag Category;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dnd5e|Selector")
	FString Name;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dnd5e|Selector")
	bool bHasSource = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dnd5e|Selector")
	FString SourceId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dnd5e|Selector")
	FInstancedStruct Qualifier;
};
