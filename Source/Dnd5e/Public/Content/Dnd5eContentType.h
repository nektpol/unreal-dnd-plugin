// Developed by Neko Creative Technologies

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "StructUtils/InstancedStruct.h"
#include "Records/Dnd5eRecordId.h"
#include "Records/Dnd5eRecordSelector.h"
#include "Values/Dnd5eDiceExpression.h"
#include "Values/Dnd5eNumber.h"
#include "Values/Dnd5eMediaReference.h"
#include "Dnd5eContentType.generated.h"

UENUM(BlueprintType)
enum class EDnd5eListMarkerStyle : uint8
{
	Unspecified,
	Bullet,
	Decimal,
	LowerRoman,
	UpperRoman,
	None
};

UENUM(BlueprintType)
enum class EDnd5eContentAlignment : uint8
{
	Unspecified,
	Left,
	Center,
	Right,
	Justify
};

UENUM(BlueprintType)
enum class EDnd5ePageReferenceKind : uint8
{
	Unspecified,
	Number,
	Text
};

UENUM(BlueprintType)
enum class EDnd5eSourceFragmentKind : uint8
{
	Unspecified,
	Text,
	Json
};

USTRUCT(BlueprintType)
struct DND5E_API FDnd5ePageReference
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|PageReference")
	EDnd5ePageReferenceKind Kind = EDnd5ePageReferenceKind::Unspecified;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|PageReference")
	int32 Number = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|PageReference")
	FString Text;
};

USTRUCT(BlueprintType)
struct DND5E_API FDnd5eContentMetadata
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|ContentMetadata")
	FString SourceType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|ContentMetadata")
	FString AnchorId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|ContentMetadata")
	FString SourceId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|ContentMetadata")
	FDnd5ePageReference Page;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|ContentMetadata")
	FString RelativeFilePath;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|ContentMetadata")
	FString JsonPointer;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|ContentMetadata")
	TArray<FString> Aliases;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|ContentMetadata")
	FString AdditionalMetadataJson;
};

USTRUCT(BlueprintType)
struct DND5E_API FDnd5eInlineContent
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|InlineContent")
	FGameplayTag Kind;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|InlineContent")
	FInstancedStruct Payload;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|ContentMetadata")
	FDnd5eContentMetadata Metadata;
};

USTRUCT(BlueprintType)
struct DND5E_API FDnd5eUnsupportedContent
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|UnsupportedContent")
	FString SourceFormat;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|UnsupportedContent")
	EDnd5eSourceFragmentKind FragmentKind = EDnd5eSourceFragmentKind::Unspecified;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|UnsupportedContent")
	FString SourceFragment;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|UnsupportedContent")
	FString DisplayText;
};

USTRUCT(BlueprintType)
struct DND5E_API FDnd5eAnnotationContent
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|AnnotationContent")
	FGameplayTag Kind;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|AnnotationContent")
	TArray<FDnd5eInlineContent> Display;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|AnnotationContent")
	FInstancedStruct Value;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|ContentMetadata")
	FString SourceFormat;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|ContentMetadata")
	FString SourceAnnotation;
};

USTRUCT(BlueprintType)
struct DND5E_API FDnd5eNumberContent
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|NumberContent")
	FDnd5eNumber Value;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|NumberContent")
	TArray<FDnd5eInlineContent> Display;
};

USTRUCT(BlueprintType)
struct DND5E_API FDnd5eReferenceContent
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|ReferenceContent")
	TArray<FDnd5eInlineContent> Display;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|ReferenceContent")
	FString SourceFormat;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|ReferenceContent")
	FString SourceReference;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|ReferenceContent")
	bool bHasTargetRecordId = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|ReferenceContent", meta = (EditCondition = "bHasTargetRecordId"))
	FDnd5eRecordId TargetRecordId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dnd5e|ReferenceContent")
	bool bHasTargetSelector = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dnd5e|ReferenceContent", meta=(EditCondition="bHasTargetSelector"))
	FDnd5eRecordSelector TargetSelector;

};

USTRUCT(BlueprintType)
struct DND5E_API FDnd5eExternalLinkContent
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|ExternalLinkContent")
	TArray<FDnd5eInlineContent> Display;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|ExternalLinkContent")
	FString Url;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|ExternalLinkContent")
	FString SourceFormat;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|ExternalLinkContent")
	FString SourceReference;
};

USTRUCT(BlueprintType)
struct DND5E_API FDnd5eAnchorLinkContent
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|AnchorLinkContent")
	TArray<FDnd5eInlineContent> Display;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|AnchorLinkContent")
	FString TargetAnchorId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|AnchorLinkContent")
	FString SourceFormat;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|AnchorLinkContent")
	FString SourceReference;
};

USTRUCT(BlueprintType)
struct DND5E_API FDnd5eFormattedContent
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|FormattedContent")
	FGameplayTagContainer Formatting;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|FormattedContent")
	TArray<FDnd5eInlineContent> Inlines;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dnd5e|FormattedContent")
	FString SourceFormat;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dnd5e|FormattedContent")
	FString SourceReference;
};

USTRUCT(BlueprintType)
struct DND5E_API FDnd5eContentBlock
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|ContentBlock")
	FGameplayTag Kind;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|ContentBlock")
	FInstancedStruct Payload;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|ContentMetadata")
	FDnd5eContentMetadata Metadata;
};

USTRUCT(BlueprintType)
struct DND5E_API FDnd5eListItemContent
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|ListItemContent")
	TArray<FDnd5eInlineContent> Label;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|ListItemContent")
	TArray<FDnd5eContentBlock> Blocks;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|ContentMetadata")
	FDnd5eContentMetadata Metadata;
};

USTRUCT(BlueprintType)
struct DND5E_API FDnd5eListContent
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|ListContent", meta = (ToolTip = "Optional list heading"))
	TArray<FDnd5eInlineContent> Heading;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|ListContent", meta = (ToolTip = "Items in reading order"))
	TArray<FDnd5eListItemContent> Items;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|ListContent")
	EDnd5eListMarkerStyle MarkerStyle = EDnd5eListMarkerStyle::Unspecified;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|ListContent", meta = (ToolTip = "Additional layout styles, such as hanging labels"))
	FGameplayTagContainer Presentation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|ListContent", meta = (ToolTip = "Starting number for numbered lists"))
	int32 StartIndex = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|ListContent", meta = (ToolTip = "Whether to use the preferred column count when supplied"))
	bool bHasColumnCount = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|ListContent", meta = (EditCondition = "bHasColumnCount", ToolTip = "Preferred column count when supplied"))
	int32 ColumnCount = 1;
};

USTRUCT(BlueprintType)
struct DND5E_API FDnd5eTableRollRange
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|TableRollRange")
	int32 Minimum = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|TableRollRange")
	int32 Maximum = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|TableRollRange")
	bool bMinimumUnbounded = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|TableRollRange")
	bool bMaximumUnbounded = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|TableRollRange")
	bool bHasDisplayMinimum = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|TableRollRange", meta = (EditCondition = "bHasDisplayMinimum"))
	int32 DisplayMinimum = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|TableRollRange")
	bool bHasDisplayMaximum = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|TableRollRange", meta = (EditCondition = "bHasDisplayMaximum"))
	int32 DisplayMaximum = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|TableRollRange")
	int32 PadToDigits = 0;
};

USTRUCT(BlueprintType)
struct DND5E_API FDnd5eTableCellContent
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|TableCellContent")
	TArray<FDnd5eContentBlock> Blocks;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|TableCellContent")
	int32 ColumnSpan = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|TableCellContent")
	FGameplayTagContainer Presentation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|TableCellContent")
	EDnd5eContentAlignment Alignment = EDnd5eContentAlignment::Unspecified;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|TableCellContent")
	bool bHasRollRange = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|TableCellContent", meta = (EditCondition = "bHasRollRange"))
	FDnd5eTableRollRange RollRange;
};

USTRUCT(BlueprintType)
struct DND5E_API FDnd5eTableRowContent
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|TableRowContent")
	TArray<FDnd5eTableCellContent> Cells;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|TableRowContent")
	FGameplayTagContainer Presentation;
};

USTRUCT(BlueprintType)
struct DND5E_API FDnd5eTableColumnContent
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|TableColumnContent")
	EDnd5eContentAlignment Alignment = EDnd5eContentAlignment::Unspecified;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|TableColumnContent")
	FGameplayTagContainer Presentation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|TableColumnContent")
	bool bHasWidthWeight = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|TableColumnContent", meta = (EditCondition = "bHasWidthWeight"))
	float WidthWeight = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|TableColumnContent")
	FString RollExpression;
};

USTRUCT(BlueprintType)
struct DND5E_API FDnd5eTableContent
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|TableContent")
	TArray<FDnd5eInlineContent> Caption;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|TableContent")
	TArray<FDnd5eTableColumnContent> Columns;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|TableContent")
	TArray<FDnd5eTableRowContent> HeaderRows;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|TableContent")
	TArray<FDnd5eTableRowContent> Rows;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|TableContent")
	TArray<FDnd5eContentBlock> Intro;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|TableContent")
	TArray<FDnd5eContentBlock> Footnotes;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|TableContent")
	TArray<FDnd5eContentBlock> Outro;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|TableContent")
	FGameplayTagContainer Presentation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|TableContent")
	bool bStripedRows = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|TableContent")
	bool bIsNameGenerator = false;
};

USTRUCT(BlueprintType)
struct DND5E_API FDnd5eDocumentLinkContent
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dnd5e|DocumentLink") TArray<FDnd5eInlineContent> Display;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dnd5e|DocumentLink") FGameplayTag DocumentKind;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dnd5e|DocumentLink") FString DocumentId;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dnd5e|DocumentLink") bool bHasChapter = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dnd5e|DocumentLink") int32 ChapterIndex = -1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dnd5e|DocumentLink") FString SectionName;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dnd5e|DocumentLink") bool bHasNamedSectionIndex = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dnd5e|DocumentLink") int32 NamedSectionIndex = -1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dnd5e|DocumentLink") FString SourceFormat;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dnd5e|DocumentLink") FString SourceReference;
};

USTRUCT(BlueprintType)
struct DND5E_API FDnd5eQuickReferenceContent
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dnd5e|QuickReference")
	TArray<FDnd5eInlineContent> Display;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dnd5e|QuickReference")
	FString Name;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dnd5e|QuickReference")
	bool bHasSource = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dnd5e|QuickReference")
	FString SourceId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dnd5e|QuickReference")
	bool bHasChapter = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dnd5e|QuickReference")
	int32 ChapterIndex = -1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dnd5e|QuickReference")
	FString HeaderLocator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dnd5e|QuickReference")
	FString SourceFormat;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dnd5e|QuickReference")
	FString SourceReference;
};

USTRUCT(BlueprintType)
struct DND5E_API FDnd5eQueryClause
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dnd5e|QueryLink")
	FString Field;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dnd5e|QueryLink")
	FString RawValue;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dnd5e|QueryLink")
	FString SourceClause;
};

USTRUCT(BlueprintType)
struct DND5E_API FDnd5eQueryLinkContent
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dnd5e|QueryLink")
	TArray<FDnd5eInlineContent> Display;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dnd5e|QueryLink")
	FString TargetPage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dnd5e|QueryLink")
	TArray<FDnd5eQueryClause> Clauses;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dnd5e|QueryLink")
	FString SourceFormat;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dnd5e|QueryLink")
	FString SourceReference;
};

USTRUCT(BlueprintType)
struct DND5E_API FDnd5eQuoteContent
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|QuoteContent")
	TArray<FDnd5eContentBlock> Blocks;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|QuoteContent")
	TArray<FDnd5eContentBlock> Attribution;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|QuoteContent")
	TArray<FDnd5eInlineContent> Source;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|QuoteContent")
	FGameplayTagContainer Presentation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|QuoteContent")
	bool bShowQuotationMarks = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|QuoteContent")
	bool bItalicize = true;
};

USTRUCT(BlueprintType)
struct DND5E_API FDnd5eInsetContent
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|InsetContent")
	TArray<FDnd5eInlineContent> Heading;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|InsetContent")
	TArray<FDnd5eContentBlock> Blocks;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|InsetContent")
	FGameplayTagContainer Presentation;
};

USTRUCT(BlueprintType)
struct DND5E_API FDnd5eImageContent
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|ImageContent")
	FDnd5eMediaReference Resource;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|ImageContent")
	bool bHasThumbnail = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|ImageContent", meta = (EditCondition = "bHasThumbnail"))
	FDnd5eMediaReference Thumbnail;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|ImageContent")
	TArray<FDnd5eInlineContent> Title;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|ImageContent")
	TArray<FDnd5eInlineContent> Credit;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|ImageContent")
	FString AltText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|ImageContent")
	bool bHasWidth = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|ImageContent", meta = (EditCondition = "bHasWidth"))
	int32 Width = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|ImageContent")
	bool bHasHeight = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|ImageContent", meta = (EditCondition = "bHasHeight"))
	int32 Height = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|ImageContent")
	FGameplayTagContainer Classification;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|ImageContent")
	FGameplayTagContainer Presentation;
};

USTRUCT(BlueprintType)
struct DND5E_API FDnd5eGalleryContent
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|GalleryContent")
	TArray<FDnd5eInlineContent> Heading;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|GalleryContent")
	TArray<FDnd5eContentBlock> Images;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|GalleryContent")
	FGameplayTagContainer Presentation;
};

USTRUCT(BlueprintType)
struct DND5E_API FDnd5eTextContent
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|TextContent")
	FString Text;
};

USTRUCT(BlueprintType)
struct DND5E_API FDnd5eParagraphContent
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|ParagraphContent")
	TArray<FDnd5eInlineContent> Inlines;
};

USTRUCT(BlueprintType)
struct DND5E_API FDnd5eSectionContent
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|SectionContent")
	TArray<FDnd5eInlineContent> Heading;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|SectionContent")
	TArray<FDnd5eContentBlock> Blocks;
};

USTRUCT(BlueprintType)
struct DND5E_API FDnd5eChanceContentValue
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Chance")
	int32 Percent = -1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Chance")
	FString RollName;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Chance")
	TArray<FDnd5eInlineContent> Success;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Chance")
	TArray<FDnd5eInlineContent> Failure;
};
