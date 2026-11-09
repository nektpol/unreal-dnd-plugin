// Developed by Neko Creative Technologies

#include "Readers/Items/Dnd5eJsonItemDefinitionCatalogLibrary.h"

FDnd5eJsonItemDefinitionReadResult UDnd5eJsonItemDefinitionCatalogLibrary::ReadItemDefinitions(
	const FDnd5eJsonDocument& Document)
{
	return FDnd5eJsonItemDefinitionReader::Read(Document);
}

bool UDnd5eJsonItemDefinitionCatalogLibrary::BuildItemDefinitionCatalog(
	UDnd5eItemDefinitionCatalogObject* CatalogObject, const FDnd5eJsonDocument& Document,
	TArray<FDnd5eDiagnostic>& Diagnostics)
{
	Diagnostics.Reset();
	if (!IsValid(CatalogObject))
	{
		FDnd5eDiagnostic Diagnostic;
		Diagnostic.Severity = EDnd5eDiagnosticSeverity::Error;
		Diagnostic.Code = TEXT("Dnd5e.ItemDefinitions.InvalidCatalog");
		Diagnostic.Message = TEXT("Supply a valid item definition catalog object.");
		Diagnostics.Add(MoveTemp(Diagnostic));
		return false;
	}

	auto Result = FDnd5eJsonItemDefinitionReader::Read(Document);
	Diagnostics = MoveTemp(Result.Diagnostics);
	if (Result.Status != EDnd5eJsonReadStatus::Success) { return false; }

	TArray<FDnd5eDiagnostic> BuildDiagnostics;
	const bool bBuilt = CatalogObject->ReplaceDefinitions(
		Result.Types, Result.Properties, Result.Masteries, BuildDiagnostics);
	Diagnostics.Append(MoveTemp(BuildDiagnostics));
	return bBuilt;
}