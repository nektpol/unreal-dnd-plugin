// Developed by Neko Creative Technologies

#pragma once

#include "CoreMinimal.h"
#include "Dnd5eReferenceResolution.generated.h"

UENUM(BlueprintType)
enum class EDnd5eReferenceResolveStatus : uint8
{
	NotAttempted,
	Resolved,
	MissingTargetId,
	InvalidTargetId,
	UnsupportedCategory,
	TargetNotFound,
};