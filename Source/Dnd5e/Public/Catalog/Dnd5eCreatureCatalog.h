// Developed by Neko Creative Technologies

#pragma once

#include "CoreMinimal.h"
#include "Bestiary/Dnd5eCreature.h"
#include "Diagnostics/Dnd5eDiagnostic.h"

struct DND5E_API FDnd5eCreatureQuery
{
	FString DatasetNamespace;
	FString SourceId;
	FString Name;
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

private:
	TArray<FDnd5eCreature> Creatures;
	TMap<FDnd5eRecordId, int32> IndicesById;
};