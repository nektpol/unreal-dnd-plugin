#pragma once
#include "CoreMinimal.h"
#include "Content/Dnd5eContentType.h"
#include "Dnd5eAttunementRequirement.generated.h"

UENUM(BlueprintType)
enum class EDnd5eAttunementMode : uint8
{
	Unspecified,
	NotRequired,
	Required,
	Optional
};

USTRUCT(BlueprintType)
struct DND5E_API FDnd5eAttunementRequirement
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Attunement")
	EDnd5eAttunementMode Mode = EDnd5eAttunementMode::Unspecified;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Attunement")
	TArray<FDnd5eInlineContent> Conditions;
};