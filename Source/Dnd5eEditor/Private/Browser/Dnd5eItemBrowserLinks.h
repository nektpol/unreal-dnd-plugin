// Developed by Neko Creative Technologies

#pragma once

#include "CoreMinimal.h"
#include "Records/Dnd5eRecordId.h"

class UDnd5eItemCatalogObject;
class UDnd5eItemDefinitionCatalogObject;
class UDnd5eSpellCatalogObject;
struct FDnd5eItem;
struct FDnd5eItemGroup;

enum class EDnd5eItemBrowserLinkKind : uint8 { Member, Definition, Spell };

struct FDnd5eItemBrowserLink
{
	EDnd5eItemBrowserLinkKind Kind;
	FText Label;
	FText Status;
	TOptional<FDnd5eRecordId> Target;
};

namespace Dnd5eItemBrowserLinks
{
	TArray<FDnd5eItemBrowserLink> Build(const FDnd5eItem& Item, const FDnd5eItemGroup* Group,
		const UDnd5eItemCatalogObject* Items, const UDnd5eItemDefinitionCatalogObject* Definitions,
		const UDnd5eSpellCatalogObject* Spells);
	bool ReadDefinition(const UDnd5eItemDefinitionCatalogObject& Catalog, const FDnd5eRecordId& Id,
		FText& Title, FText& Description);
}