// Developed by Neko Creative Technologies

#pragma once
#include "CoreMinimal.h"
#include "Items/Dnd5eItemDefinitions.h"
#include "Items/Dnd5eItemTypes.h"
#include "Diagnostics/Dnd5eDiagnostic.h"
#include "Dnd5eItemDefinitionCatalog.generated.h"


UENUM(BlueprintType)
enum class EDnd5eItemDefinitionMatchStatus : uint8
{
	NotAttempted,
	Matched,
	MissingSource,
	InvalidSelector,
	NoMatch
};

class DND5E_API FDnd5eItemDefinitionCatalog
{
public:
	bool ReplaceDefinitions(const TArray<FDnd5eItemTypeDefinition>& Types,
		const TArray<FDnd5eItemPropertyDefinition>& Properties,
		const TArray<FDnd5eItemMasteryDefinition>& Masteries, TArray<FDnd5eDiagnostic>& Diagnostics);

	bool TryGetType(const FDnd5eRecordId& Id, FDnd5eItemTypeDefinition& Out) const;
	bool TryGetProperty(const FDnd5eRecordId& Id, FDnd5eItemPropertyDefinition& Out) const;
	bool TryGetMastery(const FDnd5eRecordId& Id, FDnd5eItemMasteryDefinition& Out) const;

	EDnd5eItemDefinitionMatchStatus MatchDefinitionSelector(const FString& DatasetNamespace,
		const FDnd5eItemDefinitionSelector& Selector, TArray<FDnd5eRecordId>& CandidateIds,
		TArray<FDnd5eDiagnostic>& Diagnostics) const;
	EDnd5eItemDefinitionMatchStatus MatchMasterySelector(const FString& DatasetNamespace,
		const FDnd5eRecordSelector& Selector, TArray<FDnd5eRecordId>& CandidateIds,
		TArray<FDnd5eDiagnostic>& Diagnostics) const;

	int32 GetDefinitionCount() const { return OrderedIds.Num(); }
	TArray<FDnd5eRecordId> GetDefinitionIds() const { return OrderedIds; }

private:
	TMap<FDnd5eRecordId, FDnd5eItemTypeDefinition> TypesById;
	TMap<FDnd5eRecordId, FDnd5eItemPropertyDefinition> PropertiesById;
	TMap<FDnd5eRecordId, FDnd5eItemMasteryDefinition> MasteriesById;
	TArray<FDnd5eRecordId> OrderedIds;
};