// Developed by Neko Creative Technologies

#pragma once
#include "CoreMinimal.h"
#include "Records/Dnd5eRecordHeader.h"
#include "Records/Dnd5eRecordRelationship.h"
#include "Records/Dnd5eRecordSelector.h"
#include "Content/Dnd5eContentType.h"
#include "Dnd5eItemDefinitions.generated.h"

USTRUCT(BlueprintType)
struct DND5E_API FDnd5eItemTypeDefinition
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|ItemDefinition")
    FDnd5eRecordHeader Header;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|ItemDefinition")
    FString Abbreviation;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|ItemDefinition")
    TArray<FDnd5eContentBlock> Description;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|ItemDefinition")
    TArray<FDnd5eRecordRelationship> Relationships;
};

USTRUCT(BlueprintType)
struct DND5E_API FDnd5eItemPropertyDefinition
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|ItemDefinition")
    FDnd5eRecordHeader Header;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|ItemDefinition")
    FString Abbreviation;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|ItemDefinition")
    TArray<FDnd5eContentBlock> Description;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|ItemDefinition")
    TArray<FDnd5eRecordRelationship> Relationships;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|ItemDefinition")
    bool bHasTemplate = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|ItemDefinition", meta = (EditCondition = "bHasTemplate"))
    FString TemplateSyntax;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|ItemDefinition", meta = (EditCondition = "bHasTemplate"))
    FString TemplateText;
};

USTRUCT(BlueprintType)
struct DND5E_API FDnd5eItemMasteryDefinition
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|ItemDefinition")
    FDnd5eRecordHeader Header;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|ItemDefinition")
    TArray<FDnd5eContentBlock> Description;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|ItemDefinition")
    TArray<FDnd5eRecordRelationship> Relationships;
};

USTRUCT(BlueprintType)
struct DND5E_API FDnd5eItemMasteryReference
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|ItemMastery")
    FDnd5eRecordSelector Selector;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|ItemMastery")
    TArray<FDnd5eInlineContent> Note;
};