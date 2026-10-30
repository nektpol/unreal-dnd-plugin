// Developed by Neko Creative Technologies

#include "Readers/Dnd5eJsonSpellReferenceNormalizer.h"

#include "Catalog/Dnd5eSpellCatalog.h"

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
	const FString& Token = Original.SourceReference;
	if (!Original.SourceFormat.Equals(TEXT("5etools"), ESearchCase::CaseSensitive) || Token.Len() > 65536 ||
		!Token.StartsWith(TEXT("{@spell "), ESearchCase::CaseSensitive) || !Token.EndsWith(TEXT("}"), ESearchCase::CaseSensitive))
	{
		return UnsupportedReference(Diagnostics);
	}

	// Remove the eight-character opening marker and the final closing brace.
	const FString Body = Token.Mid(8, Token.Len() - 9);
	TArray<FString> Parts;
	Body.ParseIntoArray(Parts, TEXT("|"), false);
	if (Parts.IsEmpty() || Parts.Num() > 3) { return UnsupportedReference(Diagnostics); }

	FDnd5eSpellSelector Selector;
	Selector.DatasetNamespace = DatasetNamespace;
	Selector.Name = Parts[0].TrimStartAndEnd();
	Selector.SourceId = Parts.Num() > 1 ? Parts[1].TrimStartAndEnd() : FString{};
	if (Selector.Name.Contains(TEXT("{")) || Selector.Name.Contains(TEXT("}")) ||
		Selector.SourceId.Contains(TEXT("{")) || Selector.SourceId.Contains(TEXT("}")))
	{
		return UnsupportedReference(Diagnostics);
	}

	TArray<FDnd5eRecordId> CandidateIds;
	const EDnd5eSpellSelectorMatchStatus Status = Catalog.MatchSpellSelector(Selector, CandidateIds, Diagnostics);
	switch (Status)
	{
	case EDnd5eSpellSelectorMatchStatus::Matched:
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