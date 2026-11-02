// Developed by Neko Creative Technologies

#pragma once
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Records/Dnd5eRecordSelector.h"
#include "Content/Dnd5eContentType.h"
#include "Values/Dnd5eAbilityScoreMinimum.h"
#include "Values/Dnd5eSize.h"
#include "Values/Dnd5eAlignmentRequirement.h"
#include "Dnd5eAttunementIndex.generated.h"

USTRUCT(BlueprintType)
struct DND5E_API FDnd5eAttunementIndexGroup
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Attunement")
    TArray<FDnd5eRecordSelector> RecordRequirements;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Attunement")
    TArray<FString> LanguageProficiencies;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Attunement")
    FGameplayTag CreatureType;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Attunement")
    EDnd5eSize Size = EDnd5eSize::Unspecified;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Attunement")
    bool bHasAlignmentFilter = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Attunement", meta = (EditCondition = "bHasAlignmentFilter"))
    TArray<FDnd5eAlignmentRequirement> AlignmentOptions;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Attunement")
    TArray<FDnd5eAbilityScoreMinimum> MinimumAbilityScores;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Attunement")
    bool bHasSpellcasting = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Attunement", meta = (EditCondition = "bHasSpellcasting"))
    bool bSpellcasting = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Attunement")
    bool bHasPsionics = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Attunement", meta = (EditCondition = "bHasPsionics"))
    bool bPsionics = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Attunement")
    TArray<FDnd5eUnsupportedContent> UnsupportedContent;
};

USTRUCT(BlueprintType)
struct DND5E_API FDnd5eAttunementIndex
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Attunement")
    TArray<FDnd5eAttunementIndexGroup> Groups;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Attunement")
    bool bIsFullyRepresented = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Attunement")
    TArray<FDnd5eUnsupportedContent> UnsupportedContent;
};