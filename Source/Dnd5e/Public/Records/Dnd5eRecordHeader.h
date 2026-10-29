// Developed by Neko Creative Technologies

#pragma once

#include "CoreMinimal.h"
#include "Content/Dnd5eContentType.h"
#include "Records/Dnd5eRecordId.h"
#include "GameplayTagContainer.h"
#include "Dnd5eRecordHeader.generated.h"

USTRUCT(BlueprintType)
struct DND5E_API FDnd5eRecordProvenance
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|RecordProvenance", meta = (ToolTip = "Original publication/source identifier"))
	FString SourceId;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|RecordProvenance", meta = (ToolTip = "Numeric/textual page reference when supplied"))
	FDnd5ePageReference Page;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|RecordProvenance", meta = (ToolTip = "File relative to the selected external dataset root"))
	FString RelativeFilePath;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|RecordProvenance", meta = (ToolTip = "Source record locator in that file"))
	FString JsonPointer;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|RecordProvenance", meta = (ToolTip = "Optional JSON object retaining original category-specific identity fields"))
	FString IdentityFieldsJson;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|RecordProvenance", meta = (ToolTip = "Optional JSON object retaining original classification values and the basis of any derived tags"))
	FString ClassificationEvidenceJson;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|RecordProvenance", meta = (ToolTip = "Optional JSON object retaining other record metadata not otherwise represented"))
	FString AdditionalMetadataJson;
};

USTRUCT(BlueprintType)
struct DND5E_API FDnd5eRecordHeader
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|RecordHeader", meta = (ToolTip = "Shared namespace/category/source-key identity"))
	FDnd5eRecordId RecordId;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|RecordHeader", meta = (ToolTip = "Human-readable record name"))
	FString DisplayName;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|RecordHeader", meta = (ToolTip = "Additional source-provided names"))
	TArray<FString> Aliases;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|RecordHeader", meta = (ToolTip = "Supported record classifications, keeping distinct roles in their own tag branches"))
	FGameplayTagContainer Classification;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|RecordHeader", meta = (ToolTip = "Primary origin followed by any contributing origins"))
	TArray<FDnd5eRecordProvenance> Provenance;
};