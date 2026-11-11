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
	return BuildItemDefinitionCatalogFromDocuments(CatalogObject, {Document}, Diagnostics);
}

bool UDnd5eJsonItemDefinitionCatalogLibrary::BuildItemDefinitionCatalogFromDocuments(
	UDnd5eItemDefinitionCatalogObject* CatalogObject, const TArray<FDnd5eJsonDocument>& Documents,
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

	TArray<FDnd5eItemTypeDefinition> Types;
	TArray<FDnd5eItemPropertyDefinition> Properties;
	TArray<FDnd5eItemMasteryDefinition> Masteries;
	for (const auto& Document : Documents)
	{
		auto Read = FDnd5eJsonItemDefinitionReader::Read(Document);
		Diagnostics.Append(MoveTemp(Read.Diagnostics));
		if (Read.Status != EDnd5eJsonReadStatus::Success) { return false; }

		Types.Append(MoveTemp(Read.Types));
		Properties.Append(MoveTemp(Read.Properties));
		Masteries.Append(MoveTemp(Read.Masteries));
	}

	TArray<FDnd5eDiagnostic> BuildDiagnostics;
	const bool bBuilt = CatalogObject->ReplaceDefinitions(Types, Properties, Masteries, BuildDiagnostics);
	Diagnostics.Append(MoveTemp(BuildDiagnostics));
	return bBuilt;
}
