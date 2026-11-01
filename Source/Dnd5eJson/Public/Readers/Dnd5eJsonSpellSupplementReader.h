// Developed by Neko Creative Technologies

#pragma once

#include "CoreMinimal.h"
#include "Spells/Dnd5eSpell.h"
#include "Readers/Dnd5eJsonDocument.h"
#include "Diagnostics/Dnd5eDiagnostic.h"
#include "Dnd5eJsonSpellSupplementReader.generated.h"
USTRUCT(BlueprintType)
struct DND5EJSON_API FDnd5eJsonSpellSupplement
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dnd5e|Json|Supplement")
	FString DatasetNamespace;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dnd5e|Json|Supplement")
	FString Name;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dnd5e|Json|Supplement")
	FString SourceId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dnd5e|Json|Supplement")
	FDnd5eSupplementaryContent Content;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dnd5e|Json|Supplement")
	FDnd5eRecordProvenance Provenance;
};

class DND5EJSON_API FDnd5eJsonSpellSupplementReader
{

public:

	static bool ReadDocument(const FDnd5eJsonDocument& Document, TArray<FDnd5eJsonSpellSupplement>& Rows,int32& ConvertedValues, TArray<FDnd5eDiagnostic>& Diagnostics);
};
