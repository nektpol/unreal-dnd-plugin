// Developed by Neko Creative Technologies

#pragma once

#include "CoreMinimal.h"
#include "Dnd5eDiagnostic.generated.h"

UENUM(BlueprintType)
enum class EDnd5eDiagnosticSeverity : uint8 
{
	Unspecified,
	Info,
	Warning,
	Error
};

USTRUCT(BlueprintType)
struct DND5E_API FDnd5eDiagnostic
{
	GENERATED_BODY()
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Dnd5eDiagnostic")
	EDnd5eDiagnosticSeverity Severity = EDnd5eDiagnosticSeverity::Unspecified;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Dnd5eDiagnostic")
	FName Code;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Dnd5eDiagnostic")
	FString Message;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Dnd5eDiagnostic")
	FString RelativeFilePath;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Dnd5eDiagnostic")
	FString JsonPointer;
};
