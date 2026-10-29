#include "Catalog/Dnd5eSpellCatalog.h"
#include "Dnd5eTags.h"

namespace
{
	void CatalogDiagnostic(TArray<FDnd5eDiagnostic>& Diagnostics, EDnd5eDiagnosticSeverity Severity, FName Code, const TCHAR* Message, const FDnd5eSpell* Spell = nullptr)
	{
		FDnd5eDiagnostic Diagnostic;
		Diagnostic.Severity = Severity;
		Diagnostic.Code = Code;
		Diagnostic.Message = Message;
		if (Spell && !Spell->Header.Provenance.IsEmpty())
		{
			Diagnostic.RelativeFilePath = Spell->Header.Provenance[0].RelativeFilePath;
			Diagnostic.JsonPointer = Spell->Header.Provenance[0].JsonPointer;
		}
		Diagnostics.Add(MoveTemp(Diagnostic));
	}

	bool NonemptyText(const FString& Text)
	{
		if (Text.TrimStartAndEnd().IsEmpty()) { return false; }
		for (TCHAR Character : Text) { if (FChar::IsControl(Character)) { return false; } }
		return true;
	}

	bool MatchText(const FString& Filter, const FString& Value, ESearchCase::Type Case)
	{
		return Filter.IsEmpty() || Value.Equals(Filter, Case);
	}

	bool ProviderFilterPresent(const FDnd5eSpellAvailabilityProvider& Provider)
	{
		return Provider.Category.IsValid() || !Provider.Name.IsEmpty() || !Provider.SourceId.IsEmpty() || !Provider.ShortName.IsEmpty() ||
			!Provider.ClassName.IsEmpty() || !Provider.ClassSourceId.IsEmpty() || !Provider.BaseName.IsEmpty() || !Provider.BaseSourceId.IsEmpty();
	}

	bool MatchesAvailability(const FDnd5eSpellAvailability& Entry, const FDnd5eSpellQuery& Query)
	{
		const auto& Filter = Query.AvailabilityProvider;
		const auto& Provider = Entry.Provider;
		if (Entry.bIsVariant && !Query.bIncludeVariantAvailability) { return false; }
		if (Filter.Category.IsValid() && !Provider.Category.MatchesTag(Filter.Category)) { return false; }
		if (!MatchText(Filter.Name, Provider.Name, ESearchCase::IgnoreCase) || !MatchText(Filter.ShortName, Provider.ShortName, ESearchCase::IgnoreCase)) { return false; }
		if (!MatchText(Filter.ClassName, Provider.ClassName, ESearchCase::IgnoreCase) || !MatchText(Filter.BaseName, Provider.BaseName, ESearchCase::IgnoreCase)) { return false; }
		if (!MatchText(Filter.SourceId, Provider.SourceId, ESearchCase::CaseSensitive) || !MatchText(Filter.ClassSourceId, Provider.ClassSourceId, ESearchCase::CaseSensitive) || !MatchText(Filter.BaseSourceId, Provider.BaseSourceId, ESearchCase::CaseSensitive)) { return false; }
		if (!Query.AvailabilitySubgroup.IsEmpty() && !Entry.Subgroups.ContainsByPredicate([&](const FString& Value) { return Value.Equals(Query.AvailabilitySubgroup, ESearchCase::IgnoreCase); })) { return false; }
		if (!Query.AvailabilityDefinedInSource.IsEmpty() && !Entry.DefinedInSources.ContainsByPredicate([&](const FString& Value) { return Value.Equals(Query.AvailabilityDefinedInSource, ESearchCase::CaseSensitive); })) { return false; }
		return true;
	}
}

bool FDnd5eSpellCatalog::ReplaceSpells(const TArray<FDnd5eSpell>& Records, TArray<FDnd5eDiagnostic>& Diagnostics)
{
	Diagnostics.Reset();
	if (Records.Num() > 100000)
	{
		CatalogDiagnostic(Diagnostics, EDnd5eDiagnosticSeverity::Error, TEXT("Dnd5e.Catalog.ResourceLimitExceeded"), TEXT("A spell catalog accepts at most 100000 records."));
		return false;
	}
	TMap<FDnd5eRecordId, FDnd5eSpell> Candidate;
	TArray<FDnd5eRecordId> CandidateIds;
	Candidate.Reserve(Records.Num());
	CandidateIds.Reserve(Records.Num());
	for (const auto& Spell : Records)
	{
		const auto& Id = Spell.Header.RecordId;
		if (Id.Category != Dnd5e::Category::Spell || !NonemptyText(Id.DatasetNamespace) || !NonemptyText(Id.SourceKey) || !NonemptyText(Spell.Header.DisplayName) || Spell.Level < 0 || Spell.Level > 9)
		{
			CatalogDiagnostic(Diagnostics, EDnd5eDiagnosticSeverity::Error, TEXT("Dnd5e.Catalog.InvalidSpell"), TEXT("A catalog spell needs a complete spell ID, display name, and level from 0 to 9."), &Spell);
			return false;
		}
		if (Candidate.Contains(Id))
		{
			CatalogDiagnostic(Diagnostics, EDnd5eDiagnosticSeverity::Error, TEXT("Dnd5e.Catalog.DuplicateRecordId"), TEXT("Duplicate exact record identity; the active catalog was preserved."), &Spell);
			return false;
		}
		if (!Spell.bHasAvailabilityData && !Spell.Availability.IsEmpty())
		{
			CatalogDiagnostic(Diagnostics, EDnd5eDiagnosticSeverity::Error, TEXT("Dnd5e.Catalog.InvalidAvailability"), TEXT("Availability links require their explicit data-presence flag."), &Spell);
			return false;
		}
		for (const auto& Entry : Spell.Availability)
		{
			const auto& Provider = Entry.Provider;
			const bool bInvalidSubclass = Provider.Category == Dnd5e::Category::Subclass && (!NonemptyText(Provider.ClassName) || !NonemptyText(Provider.ClassSourceId) || !NonemptyText(Provider.ShortName));
			if (!Provider.Category.IsValid() || !NonemptyText(Provider.Name) || !NonemptyText(Provider.SourceId) || bInvalidSubclass || (!Entry.bHasUnqualifiedDefinition && Entry.DefinedInSources.IsEmpty()))
			{
				CatalogDiagnostic(Diagnostics, EDnd5eDiagnosticSeverity::Error, TEXT("Dnd5e.Catalog.InvalidAvailability"), TEXT("An availability entry lacks its provider identity or definition qualification."), &Spell);
				return false;
			}
		}
		Candidate.Add(Id, Spell);
		CandidateIds.Add(Id);
	}
	Spells = MoveTemp(Candidate);
	OrderedIds = MoveTemp(CandidateIds);
	return true;
}

bool FDnd5eSpellCatalog::TryGetSpell(const FDnd5eRecordId& Id, FDnd5eSpell& Spell) const
{
	const auto* Found = Spells.Find(Id);
	if (!Found) { Spell = FDnd5eSpell{}; return false; }
	Spell = *Found;
	return true;
}

bool FDnd5eSpellCatalog::QuerySpellIds(const FDnd5eSpellQuery& Query, TArray<FDnd5eRecordId>& Ids, TArray<FDnd5eDiagnostic>& Diagnostics) const
{
	Ids.Reset();
	Diagnostics.Reset();
	if (Query.Level < INDEX_NONE || Query.Level > 9)
	{
		CatalogDiagnostic(Diagnostics, EDnd5eDiagnosticSeverity::Error, TEXT("Dnd5e.Catalog.InvalidQuery"), TEXT("Query level must be unspecified (-1) or from 0 to 9."));
		return false;
	}
	const bool bFilterAvailability = Query.bRequireAvailability || ProviderFilterPresent(Query.AvailabilityProvider) || !Query.AvailabilitySubgroup.IsEmpty() || !Query.AvailabilityDefinedInSource.IsEmpty();
	bool bMissingAvailability = false;
	for (const auto& Id : OrderedIds)
	{
		const auto& Spell = Spells.FindChecked(Id);
		if (!MatchText(Query.DatasetNamespace, Id.DatasetNamespace, ESearchCase::CaseSensitive)) { continue; }
		if (Query.Level != INDEX_NONE && Query.Level != Spell.Level) { continue; }
		if (!Spell.Schools.HasAll(Query.Schools) || !Spell.Traits.HasAll(Query.Traits)) { continue; }
		if (!Query.SourceId.IsEmpty() && (Spell.Header.Provenance.IsEmpty() || !Spell.Header.Provenance[0].SourceId.Equals(Query.SourceId, ESearchCase::CaseSensitive))) { continue; }
		if (!Query.Name.IsEmpty() && !Spell.Header.DisplayName.Equals(Query.Name, ESearchCase::IgnoreCase))
		{
			if (!Query.bIncludeAliases || !Spell.Header.Aliases.ContainsByPredicate([&](const FString& Alias) { return Alias.Equals(Query.Name, ESearchCase::IgnoreCase); })) { continue; }
		}
		if (bFilterAvailability)
		{
			if (!Spell.bHasAvailabilityData) { bMissingAvailability = true; continue; }
			if (!Spell.Availability.ContainsByPredicate([&](const auto& Entry) { return MatchesAvailability(Entry, Query); })) { continue; }
		}
		Ids.Add(Id);
	}
	if (bMissingAvailability)
	{
		CatalogDiagnostic(Diagnostics, EDnd5eDiagnosticSeverity::Warning, TEXT("Dnd5e.Catalog.IncompleteAvailability"), TEXT("Some otherwise matching spells lack supplied availability data; results contain known matches only."));
	}
	return true;
}
