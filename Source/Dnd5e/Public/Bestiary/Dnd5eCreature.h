// Developed by Neko Creative Technologies

#pragma once
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Records/Dnd5eRecordHeader.h"
#include "Values/Dnd5eDistance.h"
#include "Values/Dnd5eSize.h"
#include "Bestiary/Dnd5eCreatureTypes.h"
#include "Dnd5eCreature.generated.h"

USTRUCT(BlueprintType)
struct DND5E_API FDnd5eCreature
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Creature")
	FDnd5eRecordHeader Header;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Creature")
	TArray<EDnd5eSize> Sizes;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Creature")
	FGameplayTag CreatureType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Creature")
	TArray<FString> CreatureSubtypes;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Creature")
	FGameplayTagContainer Alignment;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Creature")
	TArray<FDnd5eCreatureArmorClass> ArmorClasses;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Creature")
	FDnd5eCreatureHitPoints HitPoints;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Creature")
	FDnd5eDistance WalkSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Creature")
	FDnd5eCreatureAbilityScores AbilityScores;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Creature")
	TArray<FDnd5eCreatureSkillBonus> Skills;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Creature")
	TArray<FString> Senses;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Creature")
	int32 PassivePerception = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Creature")
	TArray<FString> Languages;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Creature")
	FString ChallengeRating;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Creature")
	TArray<FDnd5eCreatureEntry> Traits;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Creature")
	TArray<FDnd5eCreatureEntry> Actions;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Creature")
	TArray<FDnd5eCreatureEntry> BonusActions;
};
