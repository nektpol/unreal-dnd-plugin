// Developed by Neko Creative Technologies

#include "Readers/Bestiary/Dnd5eJsonCreatureCatalogLibrary.h"
#include "Catalog/Dnd5eCreatureCatalogObject.h"

FDnd5eJsonCreatureCatalogBlueprintLoadResult UDnd5eJsonCreatureCatalogLibrary::LoadCreatureCatalog(
	UDnd5eCreatureCatalogObject* CatalogObject, const FDnd5eJsonCreatureCatalogLoadRequest& Request)
{
	FDnd5eJsonCreatureCatalogBlueprintLoadResult Result;
	if (!IsValid(CatalogObject))
	{
		Result.Status = EDnd5eJsonReadStatus::Failure;
		auto& Diagnostic = Result.Diagnostics.AddDefaulted_GetRef();
		Diagnostic.Severity = EDnd5eDiagnosticSeverity::Error;
		Diagnostic.Code = TEXT("Dnd5e.Json.InvalidCatalogObject");
		Diagnostic.Message = TEXT("Supply a valid creature catalog object.");
		return Result;
	}

	auto Loaded = FDnd5eJsonCreatureCatalogLoader::LoadCatalog(Request, CatalogObject->GetMutableCatalog());
	Result.Status = Loaded.Status;
	Result.CreatureCount = Loaded.CreatureCount;
	Result.Diagnostics = MoveTemp(Loaded.Diagnostics);
	for (auto& Document : Loaded.Documents)
	{
		Result.Diagnostics.Append(MoveTemp(Document.Diagnostics));
		for (auto& Row : Document.Rows)
		{
			Result.Diagnostics.Append(MoveTemp(Row.Diagnostics));
		}
	}
	return Result;
}