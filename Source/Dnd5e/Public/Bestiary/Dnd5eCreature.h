// Developed by Neko Creative Technologies

#pragma once
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Records/Dnd5eRecordHeader.h"
#include "Values/Dnd5eDistance.h"
#include "Values/Dnd5eSize.h"
#include "Bestiary/Dnd5eCreatureTypes.h"
#include "Values/Dnd5eValuePresence.h"
#include "Content/Dnd5eSupplementaryContent.h"
#include "Values/Dnd5eMediaReference.h"
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
	TArray<FGameplayTag> CreatureTypeChoices;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Creature")
	TArray<FDnd5eCreatureSubtype> CreatureSubtypes;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Creature")
	FGameplayTagContainer Alignment;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Creature")
	bool bHasAlignment = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Creature")
	TArray<FDnd5eCreatureAlignmentOption> AlignmentOptions;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Creature")
	TArray<FDnd5eCreatureArmorClass> ArmorClasses;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Creature")
	EDnd5eValuePresence ArmorClassPresence = EDnd5eValuePresence::Unspecified;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Creature")
	FDnd5eCreatureHitPoints HitPoints;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Creature")
	FDnd5eDistance WalkSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Creature")
	FDnd5eCreatureMovement Movement;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Creature")
	FDnd5eCreatureAbilityScores AbilityScores;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Creature")
	EDnd5eValuePresence SavingThrowsPresence = EDnd5eValuePresence::Unspecified;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Creature")
	TArray<FDnd5eCreatureSavingThrowBonus> SavingThrows;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Creature")
	FDnd5eCreatureDefenseList DamageVulnerabilities;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Creature")
	FDnd5eCreatureDefenseList DamageResistances;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Creature")
	FDnd5eCreatureDefenseList DamageImmunities;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Creature")
	FDnd5eCreatureDefenseList ConditionImmunities;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Creature")
	TArray<FDnd5eCreatureSkillBonus> Skills;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Creature")
	TArray<FDnd5eCreatureSkillChoice> SkillChoices;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Creature")
	TArray<FString> Senses;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Creature")
	int32 PassivePerception = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Creature")
	EDnd5eValuePresence PassivePerceptionPresence = EDnd5eValuePresence::Unspecified;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Creature")
	FString PassivePerceptionFormula;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Creature")
	TArray<FString> Languages;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Creature")
	FDnd5eCreatureChallengeRating ChallengeRating;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Creature")
	TArray<FDnd5eCreatureEntry> Traits;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Creature")
	TArray<FDnd5eCreatureEntry> Actions;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Creature")
	TArray<FDnd5eCreatureEntry> BonusActions;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Creature")
	FDnd5eCreatureActionGroup Reactions;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Creature")
	FDnd5eCreatureActionGroup LegendaryActions;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Creature")
	FDnd5eCreatureActionGroup MythicActions;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Creature")
	bool bHasLegendaryActionCount = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Creature")
	int32 LegendaryActionCount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Creature")
	bool bHasLegendaryActionCountInLair = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Creature")
	int32 LegendaryActionCountInLair = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Creature")
	EDnd5eValuePresence SpellcastingPresence = EDnd5eValuePresence::Unspecified;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Creature")
	TArray<FDnd5eCreatureSpellcasting> Spellcasting;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Creature")
	FDnd5eSupplementaryContent SupplementaryContent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Creature")
	FDnd5eMediaReference Token;
};
