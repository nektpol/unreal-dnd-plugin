// Developed by Neko Creative Technologies

#pragma once

#include "CoreMinimal.h"
#include "Dnd5eNumber.generated.h"

UENUM(BlueprintType)
enum class EDnd5eNumberKind : uint8 
{
	Unspecified,
	Integer,
	Decimal,
	Literal
};

USTRUCT(BlueprintType)
struct DND5E_API FDnd5eNumber
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|NumberContent")
	EDnd5eNumberKind Kind = EDnd5eNumberKind::Unspecified;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|NumberContent", meta = (EditCondition = "Kind == EDnd5eNumberKind::Integer"))
	int32 Integer = 0;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|NumberContent", meta = (EditCondition = "Kind == EDnd5eNumberKind::Decimal"))
	double Decimal = 0.0;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|NumberContent")
	FString SourceLiteral;
};
