#include "Readers/Spells/Dnd5eJsonSpellCatalogBuilder.h"
#include "Readers/Dnd5eJsonReaderInternal.h"

FDnd5eJsonSpellCatalogBuildResult FDnd5eJsonSpellCatalogBuilder::BuildCatalog(const FDnd5eJsonSpellCatalogBuildRequest& Request, FDnd5eSpellCatalog& Catalog)
{
	using namespace Dnd5eJsonReader;
	FDnd5eJsonSpellCatalogBuildResult Result;
	Result.Status = EDnd5eJsonReadStatus::Failure;
	const FDnd5eJsonReadContext EmptyContext;
	const FString EmptyText;
	FReadState InitialState{EmptyContext, EmptyText, Result.Diagnostics, FString{}};
	if (!ValidateEditionAssignments(Request.EditionAssignments, Request.EditionPolicyId, InitialState)) { return Result; }
	if (Request.SpellDocuments.IsEmpty()) { Fail(InitialState, TEXT("Dnd5e.Json.MissingSpellDocuments"), TEXT(""), TEXT("Supply at least one spell document; use an explicit empty spell array to build an empty catalog.")); return Result; }
	const int64 DocumentCount = static_cast<int64>(Request.SpellDocuments.Num()) + Request.SpellFluffDocuments.Num() + (Request.bHasAvailabilityLookup ? 1 : 0);
	if (DocumentCount > 1024) { Fail(InitialState, Codes::ResourceLimitExceeded, TEXT(""), TEXT("Catalog selection exceeds 1024 total documents.")); return Result; }
	int64 TotalInputLength = Request.bHasAvailabilityLookup ? Request.AvailabilityLookup.JsonText.Len() : 0;
	for (const auto& Document : Request.SpellDocuments) { TotalInputLength += Document.JsonText.Len(); }
	for (const auto& Document : Request.SpellFluffDocuments) { TotalInputLength += Document.JsonText.Len(); }
	if (TotalInputLength > 67108864) { Fail(InitialState, Codes::ResourceLimitExceeded, TEXT(""), TEXT("Catalog input exceeds 67108864 TCHAR code units.")); return Result; }
	TSharedPtr<FJsonObject> Lookup;
	if (Request.bHasAvailabilityLookup)
	{
		const auto& Document = Request.AvailabilityLookup;
		FReadState State{Document.Context, Document.JsonText, Result.Diagnostics, FString{}};
		TSharedPtr<FJsonValue> Root;
		if (!ValidateReadContext(State) || !ValidateDocumentText(State) || !PreflightJson(State) || !ParseJsonDocument(Root, State) || !RequireObject(Root, TEXT(""), Lookup, State)) { return Result; }
	}
	TArray<FDnd5eSpell> Records;
	int32 ConvertedValues = 0;
	for (const auto& Document : Request.SpellDocuments)
	{
		FReadState DocumentState{Document.Context, Document.JsonText, Result.Diagnostics, FString{}};
		if (Request.bHasAvailabilityLookup && !Document.Context.DatasetNamespace.Equals(Request.AvailabilityLookup.Context.DatasetNamespace, ESearchCase::CaseSensitive))
		{
			Fail(DocumentState, TEXT("Dnd5e.Json.AvailabilityNamespaceMismatch"), TEXT(""), TEXT("A supplied lookup applies only to spell documents in the same exact dataset namespace."));
			return Result;
		}
		TSharedPtr<FJsonValue> Root;
		const TArray<TSharedPtr<FJsonValue>>* Spells = nullptr;
		if (!ValidateReadContext(DocumentState) || !ValidateDocumentText(DocumentState) || !PreflightJson(DocumentState) || !ParseJsonDocument(Root, DocumentState) || !ReadSpellArray(Root, Spells, DocumentState)) { return Result; }
		if (Spells->Num() > 100000 - Records.Num()) { Fail(DocumentState, Codes::ResourceLimitExceeded, TEXT("/spell"), TEXT("A catalog accepts at most 100000 spells.")); return Result; }
		for (int32 Index = 0; Index < Spells->Num(); ++Index)
		{
			const FString Pointer = AppendPointer(TEXT("/spell"), FString::FromInt(Index));
			FReadState State{Document.Context, Document.JsonText, Result.Diagnostics, FString{}};
			TSharedPtr<FJsonObject> Object;
			if ((*Spells)[Index]->Type != EJson::Object) { Fail(State, TEXT("Dnd5e.Json.InvalidSpellObject"), Pointer, TEXT("Each selected catalog record must be a spell object.")); return Result; }
			if (!RequireObject((*Spells)[Index], Pointer, Object, State)) { return Result; }
			FDnd5eSpell Spell;
			if (!ReadSpellObject(*Object, Pointer, Spell, State)) { return Result; }
			if (!ApplyEditionAssignment(Request.EditionAssignments, Request.EditionPolicyId, Spell, State)) { return Result; }
			ConvertedValues += State.Budget->ContentValues;
			if (Lookup.IsValid())
			{
				const auto& LookupDocument = Request.AvailabilityLookup;
				FReadState AvailabilityState{LookupDocument.Context, LookupDocument.JsonText, Result.Diagnostics, FString{}};
				if (!ReadAvailabilityEntry(*Lookup, Spell.Header.DisplayName, Spell.Header.Provenance[0].SourceId, Spell.bHasAvailabilityData, Spell.Availability, AvailabilityState)) { return Result; }
				ConvertedValues += AvailabilityState.Budget->ContentValues;
			}
			if (ConvertedValues > 1000000) { Fail(State, Codes::ResourceLimitExceeded, Pointer, TEXT("Catalog conversion exceeds 1000000 Content/availability values.")); return Result; }
			Records.Add(MoveTemp(Spell));
		}
	}
	TMap<FDnd5eRecordId, int32> Primary;
	for (int32 Index = 0; Index < Records.Num(); ++Index)
	{
		const auto& Spell = Records[Index];
		const auto Key = MakeSpellRecordId(Spell.Header.DisplayName, Spell.Header.Provenance[0].SourceId, Spell.Header.RecordId.DatasetNamespace);
		if (Primary.Contains(Key)) { Fail(InitialState, TEXT("Dnd5e.Catalog.DuplicateRecordId"), TEXT(""), TEXT("Duplicate primary identity before supplementary joining.")); return Result; }
		Primary.Add(Key, Index);
	}
	TSet<FDnd5eRecordId> Seen; TArray<FDnd5eJsonSpellSupplement> Unmatched;
	for (const auto& Document : Request.SpellFluffDocuments)
	{
		TArray<FDnd5eJsonSpellSupplement> Rows; TArray<FDnd5eDiagnostic> Diagnostics; int32 Values = 0;
		if (!FDnd5eJsonSpellSupplementReader::ReadDocument(Document, Rows, Values, Diagnostics)) { Result.Diagnostics.Append(MoveTemp(Diagnostics)); return Result; }
		Result.Diagnostics.Append(MoveTemp(Diagnostics));
		FReadState State{Document.Context, Document.JsonText, Result.Diagnostics, FString{}};
		if (Values > 1000000 - ConvertedValues) { Fail(State, Codes::ResourceLimitExceeded, TEXT("/spellFluff"), TEXT("Catalog conversion budget exceeded.")); return Result; }
		ConvertedValues += Values;
		for (auto& Row : Rows)
		{
			const auto Key = MakeSpellRecordId(Row.Name, Row.SourceId, Row.DatasetNamespace);
			if (Seen.Contains(Key)) { Fail(State, Codes::InvalidFieldValue, Row.Provenance.JsonPointer, TEXT("Duplicate supplementary identity.")); return Result; }
			if (Seen.Num() >= 100000) { Fail(State, Codes::ResourceLimitExceeded, TEXT("/spellFluff"), TEXT("Too many supplementary identities.")); return Result; }
			Seen.Add(Key);
			if (const int32* Index = Primary.Find(Key))
			{
				auto& Spell = Records[*Index]; Spell.bHasSupplementaryContent = true;
				Spell.SupplementaryContent = MoveTemp(Row.Content); Spell.Header.Provenance.Add(MoveTemp(Row.Provenance));
			}
			else
			{
				AddDiagnostic(State, EDnd5eDiagnosticSeverity::Info, TEXT("Dnd5e.Json.UnmatchedSupplement"), Row.Provenance.JsonPointer, TEXT("Supplement retained without a selected primary spell."));
				Unmatched.Add(MoveTemp(Row));
			}
		}
	}
	if (Request.bValidateContent)
	{
		for (const auto& Spell : Records)
		{
			auto Validation = UDnd5eContentValidationLibrary::ValidateSpellContent(Spell);
			Result.ContentValidationIssues.Append(MoveTemp(Validation.Issues));
			if (!Validation.bValid)
			{
				Fail(InitialState, Codes::InvalidFieldValue, TEXT(""), TEXT("Candidate Content validation failed; inspect ContentValidationIssues for model locations."));
				return Result;
			}
		}
	}
	TArray<FDnd5eDiagnostic> CatalogDiagnostics;
	if (!Catalog.ReplaceSpells(Records, CatalogDiagnostics)) { Result.Diagnostics.Append(MoveTemp(CatalogDiagnostics)); return Result; }
	Result.SpellCount = Catalog.GetSpellCount();
	Result.Status = EDnd5eJsonReadStatus::Success;
	Result.UnmatchedSupplements = MoveTemp(Unmatched);
	return Result;
}
