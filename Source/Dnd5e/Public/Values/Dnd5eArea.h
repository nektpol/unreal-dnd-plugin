// Developed by Neko Creative Technologies

#pragma once

#include "CoreMinimal.h"
#include "Dnd5eDistance.h"
#include "GameplayTagContainer.h"
#include "Content/Dnd5eContentType.h"
#include "Dnd5eArea.generated.h"

UENUM(BlueprintType)
enum class EDnd5eAreaDimensionKind : uint8
{
	Unspecified,
	Radius,
	Diameter,
	Length,
	Width,
	Height,
	EdgeLength,
	Extent,
};

USTRUCT(BlueprintType)
struct DND5E_API FDnd5eAreaDimension
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|AreaDimension")
	EDnd5eAreaDimensionKind Kind = EDnd5eAreaDimensionKind::Unspecified;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|AreaDimension")
	FDnd5eDistance Measurement;
};

USTRUCT(BlueprintType)
struct DND5E_API FDnd5eArea
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Area")
	FGameplayTag Shape;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Area")
	TArray<FDnd5eAreaDimension> Dimensions;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Area")
	TArray<FDnd5eContentBlock> Description;
};