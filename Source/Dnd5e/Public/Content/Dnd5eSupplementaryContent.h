// Developed by Neko Creative Technologies

#pragma once

#include "CoreMinimal.h"
#include "Content/Dnd5eContentType.h"
#include "Dnd5eSupplementaryContent.generated.h"

USTRUCT(BlueprintType)
struct DND5E_API FDnd5eSupplementaryContent
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dnd5e|Supplement") TArray<FDnd5eContentBlock> Entries;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dnd5e|Supplement") TArray<FDnd5eContentBlock> Images;
};
