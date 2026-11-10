// Developed by Neko Creative Technologies

#pragma once
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Readers/Items/Dnd5eJsonItemReader.h"
#include "Readers/Items/Dnd5eJsonItemCopyAssembler.h"
#include "Readers/Items/Dnd5eJsonItemVariantMatcher.h"
#include "Readers/Items/Dnd5eJsonItemVariantGenerator.h"
#include "Readers/Items/Dnd5eJsonItemGroupReader.h"
#include "Readers/Items/Dnd5eJsonItemEntryTemplateReader.h"
#include "Readers/Items/Dnd5eJsonItemEntryExpander.h"
#include "Readers/Items/Dnd5eJsonItemSupplementReader.h"
#include "Dnd5eJsonItemLibrary.generated.h"

UCLASS()
class DND5EJSON_API UDnd5eJsonItemLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Dnd5e|Items")
	static FDnd5eJsonItemReadResult ReadItemAtIndex(const FDnd5eJsonDocument& Document, int32 Index);

	UFUNCTION(BlueprintCallable, Category = "Dnd5e|Items")
	static FDnd5eJsonItemCopyAssemblyResult AssembleItemCopyAtIndex(const FDnd5eJsonDocument& Document, int32 Index);

	UFUNCTION(BlueprintCallable, Category = "Dnd5e|Items")
	static FDnd5eJsonItemCopyReadResult ReadItemCopyAtIndex(const FDnd5eJsonDocument& Document, int32 Index);

	UFUNCTION(BlueprintCallable, Category = "Dnd5e|Items")
	static FDnd5eJsonItemVariantMatchResult MatchMagicItemVariantAtIndex(const FDnd5eJsonDocument& VariantDocument,
		int32 VariantIndex, const FDnd5eJsonDocument& BaseItemDocument, EDnd5eJsonItemVariantEditionPolicy Policy);

	UFUNCTION(BlueprintCallable, Category = "Dnd5e|Items")
	static FDnd5eJsonItemVariantGenerationResult GenerateMagicItemVariantAtIndex(const FDnd5eJsonDocument& VariantDocument,
		int32 VariantIndex, const FDnd5eJsonDocument& BaseItemDocument, int32 BaseIndex,
		EDnd5eJsonItemVariantEditionPolicy Policy);

	UFUNCTION(BlueprintCallable, Category = "Dnd5e|Items")
	static FDnd5eJsonItemVariantReadResult ReadMagicItemVariantAtIndex(const FDnd5eJsonDocument& VariantDocument,
		int32 VariantIndex, const FDnd5eJsonDocument& BaseItemDocument, int32 BaseIndex,
		EDnd5eJsonItemVariantEditionPolicy Policy);

	UFUNCTION(BlueprintCallable, Category = "Dnd5e|Items")
	static FDnd5eJsonItemGroupReadResult ReadItemGroupAtIndex(const FDnd5eJsonDocument& Document, int32 Index);

	UFUNCTION(BlueprintCallable, Category = "Dnd5e|Items")
	static FDnd5eJsonItemEntryTemplateReadResult ReadItemEntryTemplateAtIndex(const FDnd5eJsonDocument& Document, int32 Index);

	UFUNCTION(BlueprintCallable, Category = "Dnd5e|Items")
	static FDnd5eJsonItemEntryExpansionResult ExpandItemEntryReference(const FDnd5eJsonItemEntryExpansionRequest& Request);

	UFUNCTION(BlueprintCallable, Category = "Dnd5e|Items")
	static FDnd5eJsonItemSupplementReadResult ReadItemSupplements(const FDnd5eJsonDocument& Document);

	UFUNCTION(BlueprintCallable, Category = "Dnd5e|Items")
	static bool TryApplyItemSupplement(const FDnd5eItem& Item, const FDnd5eJsonItemSupplement& Supplement, FDnd5eItem& OutItem);
};
