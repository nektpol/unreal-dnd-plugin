// Developed by Neko Creative Technologies

#pragma once
#include "CoreMinimal.h"
#include "Values/Dnd5eDamageExpression.h"
#include "Values/Dnd5eAttackRange.h"
#include "Values/Dnd5eArmorClassDefinition.h"
#include "Values/Dnd5eAbilityScoreMinimum.h"
#include "Values/Dnd5eValuePresence.h"
#include "Dnd5eItemEquipment.generated.h"

UENUM(BlueprintType)
enum class EDnd5eWeaponCategory : uint8
{
    Unspecified,
    Simple,
    Martial
};

USTRUCT(BlueprintType)
struct DND5E_API FDnd5eItemWeaponData
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|ItemWeapon")
    EDnd5eWeaponCategory Category = EDnd5eWeaponCategory::Unspecified;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|ItemWeapon")
    bool bHasPrimaryDamage = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|ItemWeapon", meta = (EditCondition = "bHasPrimaryDamage"))
    FDnd5eDamageExpression PrimaryDamage;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|ItemWeapon")
    bool bHasAlternateDamage = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|ItemWeapon", meta = (EditCondition = "bHasAlternateDamage"))
    FDnd5eDamageExpression AlternateDamage;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|ItemWeapon")
    bool bHasRange = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|ItemWeapon", meta = (EditCondition = "bHasRange"))
    FDnd5eAttackRange Range;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|ItemWeapon")
    bool bHasReload = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|ItemWeapon", meta = (EditCondition = "bHasReload"))
    int32 Reload = 0;
};

USTRUCT(BlueprintType)
struct DND5E_API FDnd5eItemArmorData
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|ItemArmor")
    bool bHasArmorClass = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|ItemArmor", meta = (EditCondition = "bHasArmorClass"))
    FDnd5eArmorClassDefinition ArmorClass;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|ItemArmor")
    EDnd5eValuePresence StrengthPresence = EDnd5eValuePresence::Unspecified;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|ItemArmor", meta = (EditCondition = "StrengthPresence == EDnd5eValuePresence::Present"))
    FDnd5eAbilityScoreMinimum StrengthMinimum;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|ItemArmor")
    bool bHasStealthDisadvantage = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|ItemArmor", meta = (EditCondition = "bHasStealthDisadvantage"))
    bool bStealthDisadvantage = false;
};