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

EDnd5eSpellSelectorMatchStatus UDnd5eItemLibrary::MatchItemAttachedSpell(const FString& DatasetNamespace,
	const FDnd5eItemAttachedSpell& AttachedSpell, const UDnd5eSpellCatalogObject* SpellCatalog,
	TArray<FDnd5eRecordId>& CandidateIds, TArray<FDnd5eDiagnostic>& Diagnostics)
{
	CandidateIds.Reset();
	Diagnostics.Reset();
	const auto& Selector = AttachedSpell.Selector;
	if (!IsValid(SpellCatalog) || Selector.Category != Dnd5e::Category::Spell.GetTag() || Selector.Qualifier.IsValid() ||
		(Selector.bHasSource && Selector.SourceId.TrimStartAndEnd().IsEmpty()))
	{
		FDnd5eDiagnostic Diagnostic;
		Diagnostic.Severity = EDnd5eDiagnosticSeverity::Error;
		Diagnostic.Code = TEXT("Dnd5e.Items.InvalidAttachedSpellLookup");
		Diagnostic.Message = TEXT("Supply a spell catalog and a plain spell selector with a value for any explicit publication.");
		Diagnostics.Add(MoveTemp(Diagnostic));
		return EDnd5eSpellSelectorMatchStatus::InvalidSelector;
	}

	FDnd5eSpellSelector SpellSelector;
	SpellSelector.DatasetNamespace = DatasetNamespace;
	SpellSelector.Name = Selector.Name;
	SpellSelector.SourceId = Selector.bHasSource ? Selector.SourceId : FString();
	return SpellCatalog->MatchSpellSelector(SpellSelector, CandidateIds, Diagnostics);
}
