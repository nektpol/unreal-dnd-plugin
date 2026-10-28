// Developed by Neko Creative Technologies

#pragma once

#include "CoreMinimal.h"
#include "Dnd5eMediaReference.generated.h"

UENUM(BlueprintType)
enum class EDnd5eMediaReferenceKind : uint8

{
	Unspecified,
	DatasetRelative,
	ExternalUrl
};

USTRUCT(BlueprintType)
struct DND5E_API FDnd5eMediaReference
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|MediaReference")
	EDnd5eMediaReferenceKind Kind = EDnd5eMediaReferenceKind::Unspecified;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|MediaReference", meta = (EditCondition = "Kind == EDnd5eMediaReferenceKind::DatasetRelative"))
	FString RelativePath;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|MediaReference", meta = (EditCondition = "Kind == EDnd5eMediaReferenceKind::ExternalUrl"))
	FString Url;
};