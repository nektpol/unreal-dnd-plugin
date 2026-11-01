// Developed by Neko Creative Technologies

#pragma once

#include "CoreMinimal.h"
#include "Content/Dnd5eContentType.h"
#include "Diagnostics/Dnd5eDiagnostic.h"
#include "Dnd5eJsonSpellReferenceNormalizer.generated.h"

class FDnd5eSpellCatalog;

UENUM(BlueprintType)
enum class EDnd5eJsonSpellReferenceNormalizeStatus : uint8
{
	NotAttempted,
	Normalized,
	UnsupportedReference,
	MissingSource,
	InvalidSelector,
	NoMatch,
	Ambiguous,
	InvalidCatalog,
};

class DND5EJSON_API FDnd5eJsonSpellReferenceNormalizer
{
public:
	static EDnd5eJsonSpellReferenceNormalizeStatus NormalizeReference(
		const FDnd5eSpellCatalog& Catalog,
		const FString& DatasetNamespace,
		const FDnd5eReferenceContent& Reference,
		FDnd5eReferenceContent& NormalizedReference,
		TArray<FDnd5eDiagnostic>& Diagnostics);
};