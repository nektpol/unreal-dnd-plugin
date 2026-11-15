// Developed by Neko Creative Technologies

#include "Catalog/Dnd5eCreatureCatalog.h"
#include "Dnd5eTags.h"

bool FDnd5eCreatureCatalog::ReplaceCreatures(TArray<FDnd5eCreature> Records,
	TArray<FDnd5eDiagnostic>& Diagnostics)
{
	Diagnostics.Reset();
	const auto Fail = [&Diagnostics](const TCHAR* Code, const TCHAR* Message,
		const FDnd5eRecordProvenance* Origin)
	{
		auto& Diagnostic = Diagnostics.AddDefaulted_GetRef();
		Diagnostic.Severity = EDnd5eDiagnosticSeverity::Error;
		Diagnostic.Code = FName(Code);
		Diagnostic.Message = Message;
		if (Origin)
		{
			Diagnostic.RelativeFilePath = Origin->RelativeFilePath;
			Diagnostic.JsonPointer = Origin->JsonPointer;
		}
		return false;
	};

	if (Records.Num() > 100000)
	{
		return Fail(TEXT("Dnd5e.Catalog.ResourceLimitExceeded"),
			TEXT("A creature catalog accepts at most 100000 records."), nullptr);
	}

	TMap<FDnd5eRecordId, int32> CandidateIndices;
	CandidateIndices.Reserve(Records.Num());
	for (int32 Index = 0; Index < Records.Num(); ++Index)
	{
		const auto& Header = Records[Index].Header;
		const auto& Id = Header.RecordId;
		const auto* Origin = Header.Provenance.IsEmpty() ? nullptr : &Header.Provenance[0];
		if (Id.Category != Dnd5e::Category::Creature ||
			Id.DatasetNamespace.TrimStartAndEnd().IsEmpty() ||
			Id.SourceKey.TrimStartAndEnd().IsEmpty() || Header.DisplayName.TrimStartAndEnd().IsEmpty() ||
			!Origin || Origin->SourceId.TrimStartAndEnd().IsEmpty())
		{
			return Fail(TEXT("Dnd5e.Catalog.InvalidCreature"),
				TEXT("A catalog creature needs a complete creature ID, display name, and primary source."), Origin);
		}
		if (CandidateIndices.Contains(Id))
		{
			return Fail(TEXT("Dnd5e.Catalog.DuplicateRecordId"), TEXT("Duplicate creature record ID."), Origin);
		}
		CandidateIndices.Add(Id, Index);
	}

	Creatures = MoveTemp(Records);
	IndicesById = MoveTemp(CandidateIndices);
	return true;
}

bool FDnd5eCreatureCatalog::TryGetCreature(const FDnd5eRecordId& Id, FDnd5eCreature& OutCreature) const
{
	const auto* Index = IndicesById.Find(Id);
	if (!Index)
	{
		OutCreature = FDnd5eCreature{};
		return false;
	}
	OutCreature = Creatures[*Index];
	return true;
}

TArray<FDnd5eRecordId> FDnd5eCreatureCatalog::QueryCreatureIds(const FDnd5eCreatureQuery& Query) const
{
	TArray<FDnd5eRecordId> Ids;
	for (const auto& Creature : Creatures)
	{
		const auto& Header = Creature.Header;
		if ((!Query.DatasetNamespace.IsEmpty() &&
				!Header.RecordId.DatasetNamespace.Equals(Query.DatasetNamespace, ESearchCase::CaseSensitive)) ||
			(!Query.SourceId.IsEmpty() &&
				!Header.Provenance[0].SourceId.Equals(Query.SourceId, ESearchCase::CaseSensitive)) ||
			(!Query.Name.IsEmpty() &&
				!Header.DisplayName.Equals(Query.Name, ESearchCase::IgnoreCase)))
		{
			continue;
		}
		if (Query.CreatureType.IsValid() && Query.CreatureType != Creature.CreatureType &&
			!Creature.CreatureTypeChoices.Contains(Query.CreatureType))
		{
			continue;
		}
		Ids.Add(Header.RecordId);
	}
	return Ids;
}

EDnd5eReferenceResolveStatus FDnd5eCreatureCatalog::ResolveCreatureRelationship(
	const FDnd5eRecordRelationship& Relationship, FDnd5eCreature& OutCreature) const
{
	const bool bHasTargetRecord = Relationship.bHasTargetRecord;
	const FDnd5eRecordId Id = Relationship.TargetRecordId;
	OutCreature = FDnd5eCreature{};
	if (!bHasTargetRecord) { return EDnd5eReferenceResolveStatus::MissingTargetId; }
	if (!Id.Category.IsValid() || Id.DatasetNamespace.TrimStartAndEnd().IsEmpty() ||
		Id.SourceKey.TrimStartAndEnd().IsEmpty())
	{
		return EDnd5eReferenceResolveStatus::InvalidTargetId;
	}
	if (Id.Category != Dnd5e::Category::Creature) { return EDnd5eReferenceResolveStatus::UnsupportedCategory; }

	return TryGetCreature(Id, OutCreature) ? EDnd5eReferenceResolveStatus::Resolved :
		EDnd5eReferenceResolveStatus::TargetNotFound;
}
