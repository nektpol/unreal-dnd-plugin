// Developed by Neko Creative Technologies

#include "Readers/Spells/Dnd5eJsonSpellReferenceNormalizer.h"

#include "Catalog/Dnd5eSpellCatalog.h"
#include "Readers/Dnd5eJsonReaderInternal.h"
#include "Readers/Dnd5eJsonInlineSyntax.h"


namespace
{
	EDnd5eJsonSpellReferenceNormalizeStatus UnsupportedReference(TArray<FDnd5eDiagnostic>& Diagnostics)
	{
		FDnd5eDiagnostic Diagnostic;
		Diagnostic.Severity = EDnd5eDiagnosticSeverity::Warning;
		Diagnostic.Code = TEXT("Dnd5e.Json.SpellReference.Unsupported");
		Diagnostic.Message = TEXT("Supply a supported retained 5etools spell token with a name, optional source, and optional display.");
		Diagnostics.Add(MoveTemp(Diagnostic));
		return EDnd5eJsonSpellReferenceNormalizeStatus::UnsupportedReference;
	}
}

EDnd5eJsonSpellReferenceNormalizeStatus FDnd5eJsonSpellReferenceNormalizer::NormalizeReference(const FDnd5eSpellCatalog& Catalog,
	const FString& DatasetNamespace, const FDnd5eReferenceContent& Reference, FDnd5eReferenceContent& NormalizedReference,
	TArray<FDnd5eDiagnostic>& Diagnostics)
{
	const FDnd5eReferenceContent Original = Reference;
	NormalizedReference = Original;
	Diagnostics.Reset();

	using namespace Dnd5eJsonReader;
	FString Tag, Display; TArray<FString> Parts; FDnd5eRecordSelector Parsed;
	if (!Original.SourceFormat.Equals(TEXT("5etools"), ESearchCase::CaseSensitive) ||
		!TryParseInlineToken(Original.SourceReference, Tag, Parts) || !Tag.Equals(TEXT("spell"), ESearchCase::CaseSensitive))
	{
		return UnsupportedReference(Diagnostics);
	}
	if (!TryReadRecordSelector(Tag, Parts, Parsed, Display))
	{
		// Preserve the existing InvalidSelector contract for empty/unusable names/sources;
		// source syntax/extra fields remain UnsupportedReference.
		if (Parts.IsEmpty() || Parts.Num() > 3) { return UnsupportedReference(Diagnostics); }
		const FString Name = Parts[0].TrimStartAndEnd();
		const FString Source = Parts.Num() > 1 ? Parts[1].TrimStartAndEnd() : FString{};
		if (Name.Contains(TEXT("{")) || Name.Contains(TEXT("}")) || Source.Contains(TEXT("{")) || Source.Contains(TEXT("}"))) { return UnsupportedReference(Diagnostics); }
		FDnd5eDiagnostic Diagnostic; Diagnostic.Severity = EDnd5eDiagnosticSeverity::Error;
		Diagnostic.Code = TEXT("Dnd5e.Catalog.Selector.Invalid"); Diagnostic.Message = TEXT("Retained spell selector has an unusable name or source.");
		Diagnostics.Add(MoveTemp(Diagnostic));
		return EDnd5eJsonSpellReferenceNormalizeStatus::InvalidSelector;
	}
	FDnd5eSpellSelector Selector;
	Selector.DatasetNamespace = DatasetNamespace;
	Selector.Name = Parsed.Name;
	Selector.SourceId = Parsed.SourceId;

	TArray<FDnd5eRecordId> CandidateIds;
	const EDnd5eSpellSelectorMatchStatus Status = Catalog.MatchSpellSelector(Selector, CandidateIds, Diagnostics);
	switch (Status)
	{
	case EDnd5eSpellSelectorMatchStatus::Matched:
		NormalizedReference.bHasTargetSelector = true;
		NormalizedReference.TargetSelector = Parsed;
		NormalizedReference.bHasTargetRecordId = true;
		NormalizedReference.TargetRecordId = CandidateIds[0];
		return EDnd5eJsonSpellReferenceNormalizeStatus::Normalized;
	case EDnd5eSpellSelectorMatchStatus::MissingSource:
		return EDnd5eJsonSpellReferenceNormalizeStatus::MissingSource;
	case EDnd5eSpellSelectorMatchStatus::InvalidSelector:
		return EDnd5eJsonSpellReferenceNormalizeStatus::InvalidSelector;
	case EDnd5eSpellSelectorMatchStatus::NoMatch:
		return EDnd5eJsonSpellReferenceNormalizeStatus::NoMatch;
	case EDnd5eSpellSelectorMatchStatus::Ambiguous:
		return EDnd5eJsonSpellReferenceNormalizeStatus::Ambiguous;
	default:
		FDnd5eDiagnostic Diagnostic;
		Diagnostic.Severity = EDnd5eDiagnosticSeverity::Error;
		Diagnostic.Code = TEXT("Dnd5e.Json.SpellReference.UnexpectedMatchStatus");
		Diagnostic.Message = TEXT("Spell selector matching returned an unexpected status.");
		Diagnostics.Reset();
		Diagnostics.Add(MoveTemp(Diagnostic));
		return EDnd5eJsonSpellReferenceNormalizeStatus::InvalidSelector;
	}
}
