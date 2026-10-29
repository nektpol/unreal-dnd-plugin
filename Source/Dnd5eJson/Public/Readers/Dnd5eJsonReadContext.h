// Developed by Neko Creative Technologies

#pragma once

#include "CoreMinimal.h"
#include "Dnd5eJsonReadContext.generated.h"

USTRUCT(BlueprintType)
struct DND5EJSON_API FDnd5eJsonReadContext
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Json|ReadContext")
	FString DatasetNamespace;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Json|ReadContext")
	FString RelativeFilePath;
};
