// Developed by Neko Creative Technologies

#pragma once

#include "CoreMinimal.h"
#include "Dnd5eJsonReadContext.h"
#include "Dnd5eJsonSpellReadRequest.generated.h"

USTRUCT(BlueprintType)
struct DND5EJSON_API FDnd5eJsonSpellReadRequest
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Json|SpellReadRequest")
	FString JsonText;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Json|SpellReadRequest")
	FDnd5eJsonReadContext Context;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Json|SpellReadRequest")
	int32 SpellIndex = INDEX_NONE;
};
