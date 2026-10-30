// Developed by Neko Creative Technologies

#include "Readers/Dnd5eJsonSpellCatalogLibrary.h"

#include "Catalog/Dnd5eSpellCatalogObject.h"
#include "Readers/Dnd5eJsonSpellCatalogLoader.h"

FDnd5eJsonSpellCatalogBuildResult UDnd5eJsonSpellCatalogLibrary::BuildSpellCatalog(UDnd5eSpellCatalogObject* CatalogObject, const FDnd5eJsonSpellCatalogBuildRequest& Request)
{
	if (!IsValid(CatalogObject))
	{
		FDnd5eJsonSpellCatalogBuildResult Result;
		Result.Status = EDnd5eJsonReadStatus::Failure;
		FDnd5eDiagnostic Diagnostic;
		Diagnostic.Severity = EDnd5eDiagnosticSeverity::Error;
		Diagnostic.Code = TEXT("Dnd5e.Json.InvalidCatalogObject");
		Diagnostic.Message = TEXT("Supply a valid spell catalog object.");
		Result.Diagnostics.Add(MoveTemp(Diagnostic));
		return Result;
	}
	return FDnd5eJsonSpellCatalogBuilder::BuildCatalog(Request, CatalogObject->GetMutableCatalog());
}

FDnd5eJsonSpellCatalogBuildResult UDnd5eJsonSpellCatalogLibrary::LoadSpellCatalog(UDnd5eSpellCatalogObject* CatalogObject, const FDnd5eJsonSpellCatalogLoadRequest& Request)
{
	if (!IsValid(CatalogObject))
	{
		FDnd5eJsonSpellCatalogBuildResult Result;
		Result.Status = EDnd5eJsonReadStatus::Failure;
		FDnd5eDiagnostic Diagnostic;
		Diagnostic.Severity = EDnd5eDiagnosticSeverity::Error;
		Diagnostic.Code = TEXT("Dnd5e.Json.InvalidCatalogObject");
		Diagnostic.Message = TEXT("Supply a valid spell catalog object.");
		Result.Diagnostics.Add(MoveTemp(Diagnostic));
		return Result;
	}
	return FDnd5eJsonSpellCatalogLoader::LoadCatalog(Request, CatalogObject->GetMutableCatalog());
}

EDnd5eJsonSpellReferenceNormalizeStatus UDnd5eJsonSpellCatalogLibrary::NormalizeSpellReference(UDnd5eSpellCatalogObject* CatalogObject,
	const FString& DatasetNamespace, const FDnd5eReferenceContent& Reference, FDnd5eReferenceContent& NormalizedReference,
	TArray<FDnd5eDiagnostic>& Diagnostics)
{
	if (!IsValid(CatalogObject))
	{
		NormalizedReference = Reference;
		Diagnostics.Reset();
		FDnd5eDiagnostic Diagnostic;
		Diagnostic.Severity = EDnd5eDiagnosticSeverity::Error;
		Diagnostic.Code = TEXT("Dnd5e.Json.InvalidCatalogObject");
		Diagnostic.Message = TEXT("Supply a valid spell catalog object.");
		Diagnostics.Add(MoveTemp(Diagnostic));
		return EDnd5eJsonSpellReferenceNormalizeStatus::InvalidCatalog;
	}
	return FDnd5eJsonSpellReferenceNormalizer::NormalizeReference(CatalogObject->GetMutableCatalog(), DatasetNamespace, Reference,
		NormalizedReference, Diagnostics);
}

FDnd5eJsonSpellReferenceReport UDnd5eJsonSpellCatalogLibrary::ReportSpellReferences(UDnd5eSpellCatalogObject* CatalogObject, const FDnd5eSpell& Spell)
{
	if (!IsValid(CatalogObject))
	{
		FDnd5eJsonSpellReferenceReport Report;
		FDnd5eDiagnostic Diagnostic;
		Diagnostic.Severity = EDnd5eDiagnosticSeverity::Error;
		Diagnostic.Code = TEXT("Dnd5e.Json.InvalidCatalogObject");
		Diagnostic.Message = TEXT("Supply a valid spell catalog object.");
		Report.Diagnostics.Add(MoveTemp(Diagnostic));
		return Report;
	}
	return FDnd5eJsonSpellReferenceReporter::BuildReport(CatalogObject->GetMutableCatalog(), Spell);
}
