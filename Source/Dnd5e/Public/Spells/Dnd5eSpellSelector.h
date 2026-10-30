// Developed by Neko Creative Technologies

#pragma once

#include "CoreMinimal.h"
#include "Dnd5eSpellSelector.generated.h"

UENUM(BlueprintType)
enum class EDnd5eSpellSelectorMatchStatus : uint8
{
	NotAttempted,
	Matched,
	MissingSource,
	InvalidSelector,
	NoMatch,
	Ambiguous,
};

USTRUCT(BlueprintType)
struct DND5E_API FDnd5eSpellSelector
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|SpellSelector")
	FString DatasetNamespace;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|SpellSelector")
	FString Name;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|SpellSelector")
	FString SourceId;
};