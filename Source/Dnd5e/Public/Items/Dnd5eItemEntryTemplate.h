// Developed by Neko Creative Technologies

#pragma once
#include "CoreMinimal.h"
#include "Records/Dnd5eRecordHeader.h"
#include "Dnd5eItemEntryTemplate.generated.h"

USTRUCT(BlueprintType)
struct DND5E_API FDnd5eItemEntryTemplate
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|ItemEntry")
	FDnd5eRecordHeader Header;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|ItemEntry")
	FString EntriesTemplateJson;
};