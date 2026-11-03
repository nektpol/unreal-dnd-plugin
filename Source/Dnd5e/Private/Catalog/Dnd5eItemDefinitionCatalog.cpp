// Developed by Neko Creative Technologies

#include "Catalog/Dnd5eItemDefinitionCatalog.h"
#include "Items/Dnd5eItemDefinitionIdentity.h"
#include "Dnd5eTags.h"

namespace
{
	void AddDefinitionDiagnostic(TArray<FDnd5eDiagnostic>& Diagnostics, EDnd5eDiagnosticSeverity Severity,
		FName Code, const TCHAR* Message, const FDnd5eRecordHeader* Header = nullptr)
	{
		FDnd5eDiagnostic Diagnostic;
		Diagnostic.Severity = Severity;
		Diagnostic.Code = Code;
		Diagnostic.Message = Message;
		if (Header && !Header->Provenance.IsEmpty())
		{
			Diagnostic.RelativeFilePath = Header->Provenance[0].RelativeFilePath;
			Diagnostic.JsonPointer = Header->Provenance[0].JsonPointer;
		}
		Diagnostics.Add(MoveTemp(Diagnostic));
	}

	template<typename TRecord, typename TGetKey>
	bool StageDefinitionRecords(const TArray<TRecord>& Input, FGameplayTag Category,
		const TGetKey& GetKey, TMap<FDnd5eRecordId, TRecord>& Staged,
		TArray<FDnd5eRecordId>& StagedIds, TArray<FDnd5eDiagnostic>& Diagnostics)
	{
		Staged.Reserve(Input.Num());
		for (const TRecord& Record : Input)
		{
			const auto& Header = Record.Header;
			FDnd5eRecordId Expected;
			if (Header.Provenance.IsEmpty() ||
				!Dnd5e::ItemDefinitions::TryMakeId(Header.RecordId.DatasetNamespace, Category,
					GetKey(Record), Header.Provenance[0].SourceId, Expected) ||
				Header.RecordId != Expected)
			{
				AddDefinitionDiagnostic(Diagnostics, EDnd5eDiagnosticSeverity::Error,
					TEXT("Dnd5e.ItemDefinitions.InvalidRecord"),
					TEXT("The definition needs a canonical ID matching its category, key and primary publication."), &Header);
				return false;
			}
			if (Staged.Contains(Expected))
			{
				AddDefinitionDiagnostic(Diagnostics, EDnd5eDiagnosticSeverity::Error,
					TEXT("Dnd5e.ItemDefinitions.DuplicateRecordId"),
					TEXT("A definition ID occurs more than once."), &Header);
				return false;
			}
			Staged.Add(Expected, Record);
			StagedIds.Add(Expected);
		}
		return true;
	}

	EDnd5eItemDefinitionMatchStatus InvalidDefinitionSelector(TArray<FDnd5eRecordId>& CandidateIds, TArray<FDnd5eDiagnostic>& Diagnostics)
	{
	    CandidateIds.Reset();
	    Diagnostics.Reset();
	    AddDefinitionDiagnostic(Diagnostics, EDnd5eDiagnosticSeverity::Error,
	        TEXT("Dnd5e.ItemDefinitions.InvalidSelector"),
	        TEXT("The selector needs a valid namespace, category, key and any explicitly supplied publication."));
	    return EDnd5eItemDefinitionMatchStatus::InvalidSelector;
	}

	template<typename TRecord, typename TGetKey>
	EDnd5eItemDefinitionMatchStatus MatchDefinitionRecords(const FString& DatasetNamespace,
	    FGameplayTag Category, const FString& Key, bool bHasSource, const FString& SourceId,
	    const TMap<FDnd5eRecordId, TRecord>& Records, const TArray<FDnd5eRecordId>& OrderedIds,
	    const TGetKey& GetKey, TArray<FDnd5eRecordId>& CandidateIds,
	    TArray<FDnd5eDiagnostic>& Diagnostics)
	{
	    CandidateIds.Reset();
	    Diagnostics.Reset();

	    FDnd5eRecordId RequestedId;
	    const FString Publication = bHasSource ? SourceId : FString(TEXT("validation"));
	    if (!Dnd5e::ItemDefinitions::TryMakeId(DatasetNamespace, Category, Key, Publication, RequestedId))
	    {
	        return InvalidDefinitionSelector(CandidateIds, Diagnostics);
	    }

	    if (bHasSource)
	    {
	        if (Records.Contains(RequestedId))
	        {
	            CandidateIds.Add(RequestedId);
	            return EDnd5eItemDefinitionMatchStatus::Matched;
	        }

	        AddDefinitionDiagnostic(Diagnostics, EDnd5eDiagnosticSeverity::Warning,
	            TEXT("Dnd5e.ItemDefinitions.NoMatch"),
	            TEXT("No definition matches the supplied namespace, key and publication."));
	        return EDnd5eItemDefinitionMatchStatus::NoMatch;
	    }

	    const FString NormalizedKey = Key.TrimStartAndEnd().ToLower();
	    for (const FDnd5eRecordId& Id : OrderedIds)
	    {
	        if (Id.Category != Category ||
	            !Id.DatasetNamespace.Equals(RequestedId.DatasetNamespace, ESearchCase::CaseSensitive))
	        {
	            continue;
	        }

	        const TRecord* Record = Records.Find(Id);
	        if (Record && GetKey(*Record).TrimStartAndEnd().ToLower().Equals(NormalizedKey, ESearchCase::CaseSensitive))
	        {
	            CandidateIds.Add(Id);
	        }
	    }

	    AddDefinitionDiagnostic(Diagnostics, EDnd5eDiagnosticSeverity::Warning,
	        TEXT("Dnd5e.ItemDefinitions.MissingSource"),
	        TEXT("Supply an explicit publication to select a definition."));
	    return EDnd5eItemDefinitionMatchStatus::MissingSource;
	}

}

bool FDnd5eItemDefinitionCatalog::ReplaceDefinitions(const TArray<FDnd5eItemTypeDefinition>& Types,
	const TArray<FDnd5eItemPropertyDefinition>& Properties,
	const TArray<FDnd5eItemMasteryDefinition>& Masteries, TArray<FDnd5eDiagnostic>& Diagnostics)
{
	Diagnostics.Reset();
	const int64 Total = int64(Types.Num()) + Properties.Num() + Masteries.Num();
	if (Total > 100000)
	{
		AddDefinitionDiagnostic(Diagnostics, EDnd5eDiagnosticSeverity::Error,
			TEXT("Dnd5e.ItemDefinitions.ResourceLimitExceeded"),
			TEXT("An item definition catalog accepts at most 100000 records."));
		return false;
	}

	// Stage all three domains before replacing the active catalog.
	TMap<FDnd5eRecordId, FDnd5eItemTypeDefinition> StagedTypes;
	TMap<FDnd5eRecordId, FDnd5eItemPropertyDefinition> StagedProperties;
	TMap<FDnd5eRecordId, FDnd5eItemMasteryDefinition> StagedMasteries;
	TArray<FDnd5eRecordId> StagedIds;
	StagedIds.Reserve(static_cast<int32>(Total));

	if (!StageDefinitionRecords(Types, Dnd5e::Category::ItemType,
			[](const FDnd5eItemTypeDefinition& Record) -> const FString& { return Record.Abbreviation; },
			StagedTypes, StagedIds, Diagnostics) ||
		!StageDefinitionRecords(Properties, Dnd5e::Category::ItemProperty,
			[](const FDnd5eItemPropertyDefinition& Record) -> const FString& { return Record.Abbreviation; },
			StagedProperties, StagedIds, Diagnostics) ||
		!StageDefinitionRecords(Masteries, Dnd5e::Category::ItemMastery,
			[](const FDnd5eItemMasteryDefinition& Record) -> const FString& { return Record.Header.DisplayName; },
			StagedMasteries, StagedIds, Diagnostics))
	{
		return false;
	}

	TypesById = MoveTemp(StagedTypes);
	PropertiesById = MoveTemp(StagedProperties);
	MasteriesById = MoveTemp(StagedMasteries);
	OrderedIds = MoveTemp(StagedIds);
	return true;
}

bool FDnd5eItemDefinitionCatalog::TryGetType(const FDnd5eRecordId& Id, FDnd5eItemTypeDefinition& Out) const
{
	const auto* Found = TypesById.Find(Id);
	if (!Found) { Out = {}; return false; }
	Out = *Found;
	return true;
}

bool FDnd5eItemDefinitionCatalog::TryGetProperty(const FDnd5eRecordId& Id, FDnd5eItemPropertyDefinition& Out) const
{
	const auto* Found = PropertiesById.Find(Id);
	if (!Found) { Out = {}; return false; }
	Out = *Found;
	return true;
}

bool FDnd5eItemDefinitionCatalog::TryGetMastery(const FDnd5eRecordId& Id, FDnd5eItemMasteryDefinition& Out) const
{
	const auto* Found = MasteriesById.Find(Id);
	if (!Found) { Out = {}; return false; }
	Out = *Found;
	return true;
}

EDnd5eItemDefinitionMatchStatus FDnd5eItemDefinitionCatalog::MatchDefinitionSelector(
	const FString& DatasetNamespace, const FDnd5eItemDefinitionSelector& Selector,
	TArray<FDnd5eRecordId>& CandidateIds, TArray<FDnd5eDiagnostic>& Diagnostics) const
{
	if (Selector.Category == Dnd5e::Category::ItemType)
	{
		return MatchDefinitionRecords(DatasetNamespace, Selector.Category, Selector.Abbreviation,
			Selector.bHasSource, Selector.SourceId, TypesById, OrderedIds,
			[](const FDnd5eItemTypeDefinition& Record) -> const FString& { return Record.Abbreviation; },
			CandidateIds, Diagnostics);
	}

	if (Selector.Category == Dnd5e::Category::ItemProperty)
	{
		return MatchDefinitionRecords(DatasetNamespace, Selector.Category, Selector.Abbreviation,
			Selector.bHasSource, Selector.SourceId, PropertiesById, OrderedIds,
			[](const FDnd5eItemPropertyDefinition& Record) -> const FString& { return Record.Abbreviation; },
			CandidateIds, Diagnostics);
	}

	return InvalidDefinitionSelector(CandidateIds, Diagnostics);
}

EDnd5eItemDefinitionMatchStatus FDnd5eItemDefinitionCatalog::MatchMasterySelector(
	const FString& DatasetNamespace, const FDnd5eRecordSelector& Selector,
	TArray<FDnd5eRecordId>& CandidateIds, TArray<FDnd5eDiagnostic>& Diagnostics) const
{
	if (Selector.Category != Dnd5e::Category::ItemMastery || Selector.Qualifier.IsValid())
	{
		return InvalidDefinitionSelector(CandidateIds, Diagnostics);
	}

	return MatchDefinitionRecords(DatasetNamespace, Selector.Category, Selector.Name,
		Selector.bHasSource, Selector.SourceId, MasteriesById, OrderedIds,
		[](const FDnd5eItemMasteryDefinition& Record) -> const FString& { return Record.Header.DisplayName; },
		CandidateIds, Diagnostics);
}
