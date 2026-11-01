// Developed by Neko Creative Technologies

#include "Readers/Spells/Dnd5eJsonSpellReferenceReporter.h"

#include "Catalog/Dnd5eSpellCatalog.h"

FDnd5eJsonSpellReferenceReport FDnd5eJsonSpellReferenceReporter::BuildReport(const FDnd5eSpellCatalog& Catalog, const FDnd5eSpell& Spell)
{
	FDnd5eJsonSpellReferenceReport Report;
	TArray<FDnd5eContentReferenceOccurrence> Occurrences;
	Report.bCollected = UDnd5eSpellContentLibrary::CollectSpellReferences(Spell, Occurrences, Report.Diagnostics);
	if (!Report.bCollected) { return Report; }
	Report.Entries.Reserve(Occurrences.Num());
	for (const auto& Occurrence : Occurrences)
	{
		FDnd5eJsonSpellReferenceReportEntry Entry;
		Entry.Occurrence = Occurrence;
		FDnd5eSpell ResolvedSpell;
		Entry.OriginalResolutionStatus = Catalog.ResolveSpellReference(Occurrence.Reference, ResolvedSpell, Entry.OriginalResolutionDiagnostics);
		Entry.NormalizationStatus = FDnd5eJsonSpellReferenceNormalizer::NormalizeReference(Catalog, Spell.Header.RecordId.DatasetNamespace,
			Occurrence.Reference, Entry.NormalizedReference, Entry.NormalizationDiagnostics);
		if (Entry.NormalizationStatus == EDnd5eJsonSpellReferenceNormalizeStatus::Normalized)
		{
			Entry.NormalizedResolutionStatus = Catalog.ResolveSpellReference(Entry.NormalizedReference, ResolvedSpell, Entry.NormalizedResolutionDiagnostics);
		}
		Report.Entries.Add(MoveTemp(Entry));
	}
	return Report;
}