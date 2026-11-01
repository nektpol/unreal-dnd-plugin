// Developed by Neko Creative Technologies

#pragma once

#include "CoreMinimal.h"
#include "Dnd5eContentType.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Values/Dnd5eD20Roll.h"
#include "Values/Dnd5eScaledRoll.h"
#include "Dnd5eContentLibrary.generated.h"


/**
 *
 */
UCLASS()
class DND5E_API UDnd5eContentLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Dnd5e|ContentLibrary")
	static bool TryGetSection(const FDnd5eContentBlock& ContentBlock, FDnd5eSectionContent& Section);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Dnd5e|ContentLibrary")
	static bool TryGetParagraph(const FDnd5eContentBlock& ContentBlock, FDnd5eParagraphContent& Paragraph);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Dnd5e|ContentLibrary")
	static bool TryGetInlineText(const FDnd5eInlineContent& InlineContent, FDnd5eTextContent& Text);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Dnd5e|ContentLibrary")
	static bool TryGetFormatted(const FDnd5eInlineContent& InlineContent, FDnd5eFormattedContent& FormattedInline);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Dnd5e|ContentLibrary")
	static bool TryGetList( const FDnd5eContentBlock& ContentBlock, FDnd5eListContent& List);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Dnd5e|ContentLibrary")
	static bool TryGetTable(const FDnd5eContentBlock& ContentBlock, FDnd5eTableContent& Table);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Dnd5e|ContentLibrary")
	static bool TryGetQuote(const FDnd5eContentBlock& ContentBlock, FDnd5eQuoteContent& Quote);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Dnd5e|ContentLibrary")
	static bool TryGetInset(const FDnd5eContentBlock& ContentBlock, FDnd5eInsetContent& Inset);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Dnd5e|ContentLibrary")
	static bool TryGetReference(const FDnd5eInlineContent& InlineContent, FDnd5eReferenceContent& Reference);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Dnd5e|ContentLibrary")
	static bool TryGetAnnotation(const FDnd5eInlineContent& InlineContent, FDnd5eAnnotationContent& Annotation);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Dnd5e|ContentLibrary")
	static bool TryGetNumber(const FDnd5eInlineContent& InlineContent, FDnd5eNumberContent& Number);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Dnd5e|ContentLibrary")
	static bool TryGetAnnotationDiceExpression(const FDnd5eAnnotationContent& Annotation, FDnd5eDiceExpression& Expression);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Dnd5e|ContentLibrary")
	static bool TryGetExternalLink(const FDnd5eInlineContent& InlineContent, FDnd5eExternalLinkContent& Link);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Dnd5e|ContentLibrary")
	static bool TryGetAnchorLink(const FDnd5eInlineContent& InlineContent, FDnd5eAnchorLinkContent& Link);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Dnd5e|ContentLibrary")
	static bool TryGetImage(const FDnd5eContentBlock& ContentBlock, FDnd5eImageContent& Image);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Dnd5e|ContentLibrary")
	static bool TryGetGallery(const FDnd5eContentBlock& ContentBlock, FDnd5eGalleryContent& Gallery);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Dnd5e|ContentLibrary")
	static bool TryGetUnsupportedBlock(const FDnd5eContentBlock& ContentBlock, FDnd5eUnsupportedContent& Unsupported);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Dnd5e|ContentLibrary")
	static bool TryGetUnsupportedInline(const FDnd5eInlineContent& InlineContent, FDnd5eUnsupportedContent& Unsupported);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Dnd5e|ContentLibrary")
	static bool TryGetAnnotationDifficultyClass(const FDnd5eAnnotationContent& Annotation, FDnd5eNumber& Value);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Dnd5e|ContentLibrary")
	static bool TryGetAnnotationD20Roll(const FDnd5eAnnotationContent& Annotation, FDnd5eD20Roll& Value);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Dnd5e|ContentLibrary")
	static bool TryGetAnnotationPercentChance(const FDnd5eAnnotationContent& Annotation, FDnd5eChanceContentValue& Value);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Dnd5e|ContentLibrary")
	static bool TryGetAnnotationScaledRoll(const FDnd5eAnnotationContent& Annotation, FDnd5eScaledRoll& Value);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Dnd5e|ContentLibrary")
	static bool TryGetDocumentLink(const FDnd5eInlineContent& Inline, FDnd5eDocumentLinkContent& Value);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Dnd5e|ContentLibrary")
	static bool TryGetQuickReference(const FDnd5eInlineContent& Inline, FDnd5eQuickReferenceContent& Value);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Dnd5e|ContentLibrary")
	static bool TryGetQueryLink(const FDnd5eInlineContent& Inline, FDnd5eQueryLinkContent& Value);
};
