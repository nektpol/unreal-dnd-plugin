// Developed by Neko Creative Technologies

#pragma once

#include "CoreMinimal.h"
#include "Spells/Dnd5eSpellContentLibrary.h"
#include "Records/Dnd5eReferenceResolution.h"
#include "Readers/Dnd5eJsonSpellReferenceNormalizer.h"
#include "Dnd5eJsonSpellReferenceReporter.generated.h"

class FDnd5eSpellCatalog;

USTRUCT(BlueprintType)
struct DND5EJSON_API FDnd5eJsonSpellReferenceReportEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Json|References")
	FDnd5eContentReferenceOccurrence Occurrence;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Json|References")
	EDnd5eReferenceResolveStatus OriginalResolutionStatus = EDnd5eReferenceResolveStatus::NotAttempted;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Json|References")
	TArray<FDnd5eDiagnostic> OriginalResolutionDiagnostics;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Json|References")
	EDnd5eJsonSpellReferenceNormalizeStatus NormalizationStatus = EDnd5eJsonSpellReferenceNormalizeStatus::NotAttempted;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Json|References")
	FDnd5eReferenceContent NormalizedReference;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Json|References")
	TArray<FDnd5eDiagnostic> NormalizationDiagnostics;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Json|References")
	EDnd5eReferenceResolveStatus NormalizedResolutionStatus = EDnd5eReferenceResolveStatus::NotAttempted;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Json|References")
	TArray<FDnd5eDiagnostic> NormalizedResolutionDiagnostics;
};

USTRUCT(BlueprintType)
struct DND5EJSON_API FDnd5eJsonSpellReferenceReport
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Json|References")
	bool bCollected = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Json|References")
	TArray<FDnd5eJsonSpellReferenceReportEntry> Entries;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Json|References")
	TArray<FDnd5eDiagnostic> Diagnostics;
};

class DND5EJSON_API FDnd5eJsonSpellReferenceReporter
{
public:
	static FDnd5eJsonSpellReferenceReport BuildReport(const FDnd5eSpellCatalog& Catalog, const FDnd5eSpell& Spell);
};
