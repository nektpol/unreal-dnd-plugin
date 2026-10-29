#include "Readers/Dnd5eJsonSpellCatalogBuilder.h"
#include "Readers/Dnd5eJsonReaderInternal.h"

FDnd5eJsonSpellCatalogBuildResult FDnd5eJsonSpellCatalogBuilder::BuildCatalog(const FDnd5eJsonSpellCatalogBuildRequest& Request, FDnd5eSpellCatalog& Catalog)
{
	using namespace Dnd5eJsonReader;
	FDnd5eJsonSpellCatalogBuildResult Result;
	Result.Status = EDnd5eJsonReadStatus::Failure;
	const FDnd5eJsonReadContext EmptyContext;
	const FString EmptyText;
	FReadState InitialState{EmptyContext, EmptyText, Result.Diagnostics, FString{}};
	if (Request.SpellDocuments.IsEmpty()) { Fail(InitialState, TEXT("Dnd5e.Json.MissingSpellDocuments"), TEXT(""), TEXT("Supply at least one spell document; use an explicit empty spell array to build an empty catalog.")); return Result; }
	if (Request.SpellDocuments.Num() > 1024) { Fail(InitialState, Codes::ResourceLimitExceeded, TEXT(""), TEXT("A catalog build accepts at most 1024 spell documents.")); return Result; }
	int64 TotalInputLength = Request.bHasAvailabilityLookup ? Request.AvailabilityLookup.JsonText.Len() : 0;
	for (const auto& Document : Request.SpellDocuments) { TotalInputLength += Document.JsonText.Len(); }
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
	TArray<FDnd5eDiagnostic> CatalogDiagnostics;
	if (!Catalog.ReplaceSpells(Records, CatalogDiagnostics)) { Result.Diagnostics.Append(MoveTemp(CatalogDiagnostics)); return Result; }
	Result.SpellCount = Catalog.GetSpellCount();
	Result.Status = EDnd5eJsonReadStatus::Success;
	return Result;
}
