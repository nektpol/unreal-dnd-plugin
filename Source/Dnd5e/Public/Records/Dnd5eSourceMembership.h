// Developed by Neko Creative Technologies

#pragma once
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Dnd5eSourceMembership.generated.h"

UENUM(BlueprintType)
enum class EDnd5eSourceInclusion : uint8 {
	Unspecified,
	Excluded,
	Included,
	IncludedWithAlternateName
};
UENUM(BlueprintType)
enum class EDnd5eDataAvailability : uint8 { Unknown, Absent, Present };

USTRUCT(BlueprintType)
struct DND5E_API FDnd5eSourceMembership
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dnd5e|Source") FString MembershipId;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dnd5e|Source") EDnd5eSourceInclusion Inclusion = EDnd5eSourceInclusion::Unspecified;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dnd5e|Source") FString AlternateName;
};

USTRUCT(BlueprintType)
struct DND5E_API FDnd5eSourceEditionAssignment
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dnd5e|Source") FString DatasetNamespace;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dnd5e|Source") FString SourceId;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dnd5e|Source") FGameplayTagContainer EditionTags;
};
