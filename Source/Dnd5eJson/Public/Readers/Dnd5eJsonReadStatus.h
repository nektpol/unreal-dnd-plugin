#pragma once

#include "CoreMinimal.h"
#include "Dnd5eJsonReadStatus.generated.h"

UENUM(BlueprintType)
enum class EDnd5eJsonReadStatus : uint8
{
	NotAttempted,
	Success,
	Failure
};
