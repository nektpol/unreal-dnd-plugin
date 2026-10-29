#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Records/Dnd5eRecordHeader.h"
#include "Dnd5eSpellAvailability.generated.h"

// A source selector, not an invented normalized ID for an unread provider record.
USTRUCT(BlueprintType)
struct DND5E_API FDnd5eSpellAvailabilityProvider
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|SpellAvailability")
	FGameplayTag Category;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|SpellAvailability")
	FString Name;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|SpellAvailability")
	FString SourceId;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|SpellAvailability")
	FString ShortName;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|SpellAvailability")
	FString ClassName;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|SpellAvailability")
	FString ClassSourceId;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|SpellAvailability")
	FString BaseName;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|SpellAvailability")
	FString BaseSourceId;
};

USTRUCT(BlueprintType)
struct DND5E_API FDnd5eSpellAvailability
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|SpellAvailability")
	FDnd5eSpellAvailabilityProvider Provider;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|SpellAvailability")
	bool bIsVariant = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|SpellAvailability")
	bool bHasUnqualifiedDefinition = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|SpellAvailability")
	TArray<FString> DefinedInSources;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|SpellAvailability")
	TArray<FString> Subgroups;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|SpellAvailability")
	TArray<FString> FeatureTypes;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|SpellAvailability")
	FDnd5eRecordProvenance Provenance;
};
