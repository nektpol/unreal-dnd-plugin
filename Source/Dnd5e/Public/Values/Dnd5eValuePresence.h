// Developed by Neko Creative Technologies

#pragma once
#include "CoreMinimal.h"
#include "Dnd5eValuePresence.generated.h"

UENUM(BlueprintType)
enum class EDnd5eValuePresence : uint8
{
	Unspecified,
	ExplicitNull,
	Present
};