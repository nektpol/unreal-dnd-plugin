// Developed by Neko Creative Technologies

#include "Items/Dnd5eItemLibrary.h"
#include "Dnd5eTags.h"

bool UDnd5eItemLibrary::TryGetReferencedBaseItem(const FDnd5eItem& Item, const TArray<FDnd5eItem>& BaseItems,
	FDnd5eItem& OutBaseItem)
{
	const FDnd5eRecordId& Target = Item.BaseItemId;
	if (!Item.bHasBaseItemId || Target.Category != Dnd5e::Category::Item ||
		Target.DatasetNamespace.IsEmpty() || Target.SourceKey.IsEmpty() || Target == Item.Header.RecordId)
	{
		OutBaseItem = {};
		return false;
	}

	const FDnd5eItem* Match = nullptr;
	for (const FDnd5eItem& Candidate : BaseItems)
	{
		if (Candidate.Header.RecordId != Target) { continue; }
		if (Match)
		{
			OutBaseItem = {};
			return false;
		}
		Match = &Candidate;
	}

	if (!Match)
	{
		OutBaseItem = {};
		return false;
	}

	OutBaseItem = *Match;
	return true;
}
