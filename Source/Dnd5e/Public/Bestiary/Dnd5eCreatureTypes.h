// Developed by Neko Creative Technologies

#pragma once
#include "CoreMinimal.h"
#include "Content/Dnd5eContentType.h"
#include "Values/Dnd5eDiceExpression.h"
#include "Values/Dnd5eValuePresence.h"
#include "Dnd5eCreatureTypes.generated.h"

USTRUCT(BlueprintType)
struct DND5E_API FDnd5eCreatureArmorClass
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Creature")
	int32 Value = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Creature")
	TArray<FDnd5eContentBlock> Sources;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Creature")
	bool bHasValue = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Creature")
	TArray<FDnd5eInlineContent> Condition;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Creature")
	TArray<FDnd5eInlineContent> Special;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Creature")
	bool bWrapInParentheses = false;
};

USTRUCT(BlueprintType)
struct DND5E_API FDnd5eCreatureHitPoints
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Creature")
	int32 Average = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Creature")
	FDnd5eDiceExpression Formula;
};

USTRUCT(BlueprintType)
struct DND5E_API FDnd5eCreatureAbilityScores
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Creature")
	int32 Strength = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Creature")
	int32 Dexterity = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Creature")
	int32 Constitution = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Creature")
	int32 Intelligence = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Creature")
	int32 Wisdom = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Creature")
	int32 Charisma = 0;
};

USTRUCT(BlueprintType)
struct DND5E_API FDnd5eCreatureSkillBonus
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Creature")
	FString Skill;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Creature")
	int32 Bonus = 0;
};

USTRUCT(BlueprintType)
struct DND5E_API FDnd5eCreatureEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Creature")
	FString Name;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Creature")
	TArray<FDnd5eContentBlock> Entries;
};

USTRUCT(BlueprintType)
struct DND5E_API FDnd5eCreatureAlignmentOption
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Creature")
	TArray<FString> Codes;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Creature")
	bool bHasChance = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Creature")
	int32 ChancePercent = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Creature")
	FString Note;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Creature")
	FString Special;
};

USTRUCT(BlueprintType)
struct DND5E_API FDnd5eCreatureChallengeRating
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Creature")
	EDnd5eValuePresence Presence = EDnd5eValuePresence::Unspecified;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Creature")
	FString Base;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Creature")
	FString Lair;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Creature")
	FString Coven;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Creature")
	bool bHasXp = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Creature")
	int32 Xp = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Creature")
	bool bHasLairXp = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Creature")
	int32 LairXp = 0;
};
