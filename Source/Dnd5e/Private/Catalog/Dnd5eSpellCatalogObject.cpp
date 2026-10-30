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
