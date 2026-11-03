// Developed by Neko Creative Technologies

#include "Catalog/Dnd5eItemDefinitionCatalogObject.h"

bool UDnd5eItemDefinitionCatalogObject::ReplaceDefinitions(const TArray<FDnd5eItemTypeDefinition>& Types, const TArray<FDnd5eItemPropertyDefinition>& Properties, const TArray<FDnd5eItemMasteryDefinition>& Masteries, TArray<FDnd5eDiagnostic>& Diagnostics)
{
	return Catalog.ReplaceDefinitions(Types, Properties, Masteries, Diagnostics);
}

bool UDnd5eItemDefinitionCatalogObject::TryGetType(const FDnd5eRecordId& Id, FDnd5eItemTypeDefinition& Out) const
{
	return Catalog.TryGetType(Id, Out);
}

bool UDnd5eItemDefinitionCatalogObject::TryGetProperty(const FDnd5eRecordId& Id, FDnd5eItemPropertyDefinition& Out) const
{
	return Catalog.TryGetProperty(Id, Out);
}

bool UDnd5eItemDefinitionCatalogObject::TryGetMastery(const FDnd5eRecordId& Id, FDnd5eItemMasteryDefinition& Out) const
{
	return Catalog.TryGetMastery(Id, Out);
}

EDnd5eItemDefinitionMatchStatus UDnd5eItemDefinitionCatalogObject::MatchDefinitionSelector(const FString& DatasetNamespace, const FDnd5eItemDefinitionSelector& Selector, TArray<FDnd5eRecordId>& CandidateIds, TArray<FDnd5eDiagnostic>& Diagnostics) const
{
	return Catalog.MatchDefinitionSelector(DatasetNamespace, Selector, CandidateIds, Diagnostics);
}

EDnd5eItemDefinitionMatchStatus UDnd5eItemDefinitionCatalogObject::MatchMasterySelector(const FString& DatasetNamespace, const FDnd5eRecordSelector& Selector, TArray<FDnd5eRecordId>& CandidateIds, TArray<FDnd5eDiagnostic>& Diagnostics) const
{
	return Catalog.MatchMasterySelector(DatasetNamespace, Selector, CandidateIds, Diagnostics);
}

int32 UDnd5eItemDefinitionCatalogObject::GetDefinitionCount() const
{
	return Catalog.GetDefinitionCount();
}

TArray<FDnd5eRecordId> UDnd5eItemDefinitionCatalogObject::GetDefinitionIds() const
{
	return Catalog.GetDefinitionIds();
}