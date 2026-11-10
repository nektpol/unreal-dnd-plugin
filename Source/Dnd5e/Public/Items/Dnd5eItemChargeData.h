// Developed by Neko Creative Technologies

#pragma once
#include "CoreMinimal.h"
#include "Values/Dnd5eDiceExpression.h"
#include "Values/Dnd5eValuePresence.h"
#include "Dnd5eItemChargeData.generated.h"

UENUM(BlueprintType)
enum class EDnd5eItemChargeAmountKind : uint8
{
	Unspecified,
	ExplicitNull,
	Fixed,
	Dice
};

UENUM(BlueprintType)
enum class EDnd5eItemRechargeTrigger : uint8
{
	Unspecified,
	Dawn,
	Dusk,
	Midnight,
	ShortRest,
	LongRest,
	Decade,
	Special
};

USTRUCT(BlueprintType)
struct DND5E_API FDnd5eItemChargeAmount
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|ItemCharges")
	EDnd5eItemChargeAmountKind Kind = EDnd5eItemChargeAmountKind::Unspecified;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|ItemCharges", meta = (EditCondition = "Kind == EDnd5eItemChargeAmountKind::Fixed"))
	int32 Fixed = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|ItemCharges", meta = (EditCondition = "Kind == EDnd5eItemChargeAmountKind::Dice"))
	FDnd5eDiceExpression Dice;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|ItemCharges")
	FString SourceLiteral;
};

USTRUCT(BlueprintType)
struct DND5E_API FDnd5eItemChargeData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|ItemCharges")
	FDnd5eItemChargeAmount Charges;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|ItemCharges")
	EDnd5eValuePresence RechargePresence = EDnd5eValuePresence::Unspecified;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|ItemCharges", meta = (EditCondition = "RechargePresence == EDnd5eValuePresence::Present"))
	EDnd5eItemRechargeTrigger RechargeTrigger = EDnd5eItemRechargeTrigger::Unspecified;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|ItemCharges")
	FDnd5eItemChargeAmount RechargeAmount;
};