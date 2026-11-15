// Developed by Neko Creative Technologies

#include "Catalog/Dnd5eCreatureCatalogObject.h"

bool UDnd5eCreatureCatalogObject::TryGetCreature(const FDnd5eRecordId& Id, FDnd5eCreature& OutCreature) const
{
	return Catalog.TryGetCreature(Id, OutCreature);
}

EDnd5eReferenceResolveStatus UDnd5eCreatureCatalogObject::ResolveCreatureRelationship(
	const FDnd5eRecordRelationship& Relationship, FDnd5eCreature& OutCreature) const
{
	return Catalog.ResolveCreatureRelationship(Relationship, OutCreature);
}

TArray<FDnd5eRecordId> UDnd5eCreatureCatalogObject::QueryCreatureIds(const FDnd5eCreatureQuery& Query) const
{
	return Catalog.QueryCreatureIds(Query);
}

int32 UDnd5eCreatureCatalogObject::GetCreatureCount() const
{
	return Catalog.GetCreatureCount();
}