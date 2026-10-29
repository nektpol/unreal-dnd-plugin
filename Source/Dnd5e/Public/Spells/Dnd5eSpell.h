// Developed by Neko Creative Technologies

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Records/Dnd5eRecordHeader.h"
#include "Records/Dnd5eRecordRelationship.h"
#include "Values/Dnd5eCastingTime.h"
#include "Values/Dnd5eRange.h"
#include "Values/Dnd5eComponents.h"
#include "Spells/Dnd5eSpellTypes.h"
#include "Content/Dnd5eContentType.h"
#include "Dnd5eSpell.generated.h"

USTRUCT(BlueprintType)
struct DND5E_API FDnd5eSpell 
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Spell")
	FDnd5eRecordHeader Header;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Spell")
	int32 Level = -1;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Spell")
	FGameplayTagContainer Schools;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Spell")
	TArray<FDnd5eCastingTime> CastingTimes;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Spell")
	FDnd5eRange Range;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Spell")
	TArray<FDnd5eSpellArea> Areas;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Spell")
	FDnd5eComponents Components;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Spell")
	TArray<FDnd5eSpellDuration> Durations;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Spell")
	TArray<FDnd5eContentBlock> Description;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Spell")
	TArray<FDnd5eContentBlock> HigherLevelDescription;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Spell")
	FGameplayTagContainer Traits;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Spell")
	FGameplayTagContainer DamageInflicted;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Spell")
	FGameplayTagContainer DamageResisted;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Spell")
	FGameplayTagContainer DamageImmunities;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Spell")
	FGameplayTagContainer DamageVulnerabilities;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Spell")
	FGameplayTagContainer ConditionInflicted;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Spell")
	FGameplayTagContainer ConditionImmunities;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Spell")
	FGameplayTagContainer SavingThrowAbilities;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Spell")
	FGameplayTagContainer AbilityCheckAbilities;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Spell")
	FGameplayTagContainer AttackTypes;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Spell")
	FGameplayTagContainer AffectedCreatureTypes;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Spell")
	TArray<FDnd5eRecordRelationship> Relationships; 
};
