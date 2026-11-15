// Developed by Neko Creative Technologies

#pragma once

#include "CoreMinimal.h"
#include "Bestiary/Dnd5eCreature.h"
#include "Diagnostics/Dnd5eDiagnostic.h"
#include "Records/Dnd5eReferenceResolution.h"
#include "Dnd5eCreatureCatalog.generated.h"

USTRUCT(BlueprintType)
struct DND5E_API FDnd5eCreatureQuery
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|CreatureCatalog")
	FString DatasetNamespace;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|CreatureCatalog")
	FString SourceId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|CreatureCatalog")
	FString Name;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|CreatureCatalog")
	FGameplayTag CreatureType;
};

// The caller owns synchronization when sharing a catalog across threads.
class DND5E_API FDnd5eCreatureCatalog
{
public:
	bool ReplaceCreatures(TArray<FDnd5eCreature> Records, TArray<FDnd5eDiagnostic>& Diagnostics);
	bool TryGetCreature(const FDnd5eRecordId& Id, FDnd5eCreature& OutCreature) const;
	TArray<FDnd5eRecordId> QueryCreatureIds(const FDnd5eCreatureQuery& Query) const;
	int32 GetCreatureCount() const { return Creatures.Num(); }

	EDnd5eReferenceResolveStatus ResolveCreatureRelationship(const FDnd5eRecordRelationship& Relationship, FDnd5eCreature& OutCreature) const;

private:
	TArray<FDnd5eCreature> Creatures;
	TMap<FDnd5eRecordId, int32> IndicesById;
};