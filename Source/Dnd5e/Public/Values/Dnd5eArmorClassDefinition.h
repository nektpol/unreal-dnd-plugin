// Developed by Neko Creative Technologies

#pragma once
#include "CoreMinimal.h"
#include "Values/Dnd5eNumber.h"
#include "Dnd5eArmorClassDefinition.generated.h"

UENUM(BlueprintType)
enum class EDnd5eArmorClassValueKind : uint8
{
	Unspecified,
	Base,
	Bonus
};

UENUM(BlueprintType)
enum class EDnd5eArmorClassDexterityMode : uint8
{
	Unspecified,
	NotAdded,
	Uncapped,
	Capped
};

USTRUCT(BlueprintType)
struct DND5E_API FDnd5eArmorClassDefinition
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|ArmorClass")
	FDnd5eNumber Value;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|ArmorClass")
	EDnd5eArmorClassValueKind Kind = EDnd5eArmorClassValueKind::Unspecified;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|ArmorClass")
	EDnd5eArmorClassDexterityMode DexterityMode = EDnd5eArmorClassDexterityMode::Unspecified;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|ArmorClass", meta = (EditCondition = "DexterityMode == EDnd5eArmorClassDexterityMode::Capped"))
	int32 DexterityModifierMaximum = 0;
};