// Developed by Neko Creative Technologies

#pragma once
#include "CoreMinimal.h"
#include "Dnd5eSize.generated.h"

UENUM(BlueprintType)
enum class EDnd5eSize : uint8
{
	Unspecified,
	Tiny,
	Small,
	Medium,
	Large,
	Huge,
	Gargantuan
};