// Developed by Neko Creative Technologies


#include "Catalog/Dnd5eSpellCatalogObject.h"

bool UDnd5eSpellCatalogObject::ReplaceSpells(const TArray<FDnd5eSpell>& Records, TArray<FDnd5eDiagnostic>& Diagnostics)
{
	return Catalog.ReplaceSpells(Records, Diagnostics);
}

bool UDnd5eSpellCatalogObject::TryGetSpell(const FDnd5eRecordId& Id, FDnd5eSpell& Spell) const
{
	return Catalog.TryGetSpell(Id, Spell);
}

EDnd5eReferenceResolveStatus UDnd5eSpellCatalogObject::ResolveSpellReference(const FDnd5eReferenceContent& Reference, FDnd5eSpell& Spell,
	TArray<FDnd5eDiagnostic>& Diagnostics) const
{
	return Catalog.ResolveSpellReference(Reference, Spell, Diagnostics);
}

EDnd5eReferenceResolveStatus UDnd5eSpellCatalogObject::ResolveSpellRelationship(const FDnd5eRecordRelationship& Relationship, FDnd5eSpell& Spell, TArray<FDnd5eDiagnostic>& Diagnostics) const
{
	return Catalog.ResolveSpellRelationship(Relationship, Spell, Diagnostics);
}

EDnd5eSpellSelectorMatchStatus UDnd5eSpellCatalogObject::MatchSpellSelector(const FDnd5eSpellSelector& Selector, TArray<FDnd5eRecordId>& CandidateIds, TArray<FDnd5eDiagnostic>& Diagnostics) const
{
	return Catalog.MatchSpellSelector(Selector, CandidateIds, Diagnostics);
}

bool UDnd5eSpellCatalogObject::QuerySpellIds(const FDnd5eSpellQuery& Query, TArray<FDnd5eRecordId>& Ids, TArray<FDnd5eDiagnostic>& Diagnostics) const
{
	return Catalog.QuerySpellIds(Query, Ids, Diagnostics);
}

int32 UDnd5eSpellCatalogObject::GetSpellCount() const
{
	return Catalog.GetSpellCount();
}

TArray<FDnd5eRecordId> UDnd5eSpellCatalogObject::GetSpellIds() const
{
	return Catalog.GetSpellIds();
}
