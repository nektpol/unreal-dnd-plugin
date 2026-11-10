// Developed by Neko Creative Technologies

#pragma once
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Records/Dnd5eRecordId.h"
#include "Records/Dnd5eRecordSelector.h"
#include "Values/Dnd5eValuePresence.h"
#include "Dnd5eItemMagicData.generated.h"

UENUM(BlueprintType)
enum class EDnd5eItemBonusKind : uint8
{
	Unspecified,
	ArmorClass,
	WeaponAttackAndDamage,
	WeaponAttack,
	WeaponDamage,
	SpellAttack,
	SpellDamage,
	SpellSaveDc,
	SavingThrow,
	ConcentrationSavingThrow,
	AbilityCheck,
	ProficiencyBonus
};

USTRUCT(BlueprintType)
struct DND5E_API FDnd5eItemBonus
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|ItemBonus")
	EDnd5eItemBonusKind Kind = EDnd5eItemBonusKind::Unspecified;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|ItemBonus")
	int32 Amount = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|ItemBonus")
	FString SourceLiteral;
};

UENUM(BlueprintType)
enum class EDnd5eItemSpellUse : uint8
{
	Unspecified,
	AtWill,
	Ritual,
	Other,
	Charges,
	Daily,
	Rest,
	Limited
};

USTRUCT(BlueprintType)
struct DND5E_API FDnd5eItemAttachedSpell
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|ItemSpell")
	FDnd5eRecordSelector Selector;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|ItemSpell")
	bool bHasTargetId = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|ItemSpell")
	FDnd5eRecordId TargetId;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|ItemSpell")
	FString SourceUid;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|ItemSpell")
	EDnd5eItemSpellUse Use = EDnd5eItemSpellUse::Unspecified;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|ItemSpell")
	bool bHasCount = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|ItemSpell")
	int32 Count = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|ItemSpell")
	bool bEach = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|ItemSpell")
	int32 CastLevel = INDEX_NONE;
};

USTRUCT(BlueprintType)
struct DND5E_API FDnd5eItemAttachedSpellData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|ItemSpell")
	EDnd5eValuePresence Presence = EDnd5eValuePresence::Unspecified;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|ItemSpell")
	FGameplayTag Ability;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|ItemSpell")
	TArray<FDnd5eItemAttachedSpell> Spells;
};