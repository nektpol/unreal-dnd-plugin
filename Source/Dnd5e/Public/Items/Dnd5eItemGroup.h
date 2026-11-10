// Developed by Neko Creative Technologies

#pragma once
#include "CoreMinimal.h"
#include "Items/Dnd5eItem.h"
#include "Dnd5eItemGroup.generated.h"

USTRUCT(BlueprintType)
struct DND5E_API FDnd5eItemGroupMember
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|ItemGroup")
	FString SourceUid;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|ItemGroup")
	FString Name;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|ItemGroup")
	bool bHasSource = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|ItemGroup", meta = (EditCondition = "bHasSource"))
	FString SourceId;
};

USTRUCT(BlueprintType)
struct DND5E_API FDnd5eItemGroup
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|ItemGroup")
	FDnd5eItem Details;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|ItemGroup")
	TArray<FDnd5eItemGroupMember> Members;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|ItemGroup")
	bool bItemsHidden = false;
};