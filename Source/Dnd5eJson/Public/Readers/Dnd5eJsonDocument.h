#pragma once

#include "CoreMinimal.h"
#include "Readers/Dnd5eJsonReadContext.h"
#include "Dnd5eJsonDocument.generated.h"

USTRUCT(BlueprintType)
struct DND5EJSON_API FDnd5eJsonDocument
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Json")
	FString JsonText;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Json")
	FDnd5eJsonReadContext Context;
};
