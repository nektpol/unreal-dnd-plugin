// Developed by Neko Creative Technologies

#include "Readers/Items/Dnd5eJsonItemLibrary.h"

FDnd5eJsonItemReadResult UDnd5eJsonItemLibrary::ReadItemAtIndex(const FDnd5eJsonDocument& Document, int32 Index)
{
	return FDnd5eJsonItemReader::ReadAtIndex(Document, Index);
}

FDnd5eJsonItemCopyAssemblyResult UDnd5eJsonItemLibrary::AssembleItemCopyAtIndex(const FDnd5eJsonDocument& Document, int32 Index)
{
	return FDnd5eJsonItemCopyAssembler::AssembleAtIndex(Document, Index);
}

FDnd5eJsonItemCopyReadResult UDnd5eJsonItemLibrary::ReadItemCopyAtIndex(const FDnd5eJsonDocument& Document, int32 Index)
{
	return FDnd5eJsonItemCopyAssembler::ReadCopyAtIndex(Document, Index);
}

FDnd5eJsonItemVariantMatchResult UDnd5eJsonItemLibrary::MatchMagicItemVariantAtIndex(const FDnd5eJsonDocument& VariantDocument,
	int32 VariantIndex, const FDnd5eJsonDocument& BaseItemDocument, EDnd5eJsonItemVariantEditionPolicy Policy)
{
	return FDnd5eJsonItemVariantMatcher::MatchAtIndex(VariantDocument, VariantIndex, BaseItemDocument, Policy);
}

FDnd5eJsonItemVariantGenerationResult UDnd5eJsonItemLibrary::GenerateMagicItemVariantAtIndex(const FDnd5eJsonDocument& VariantDocument,
	int32 VariantIndex, const FDnd5eJsonDocument& BaseItemDocument, int32 BaseIndex, EDnd5eJsonItemVariantEditionPolicy Policy)
{
	return FDnd5eJsonItemVariantGenerator::GenerateAtIndex(VariantDocument, VariantIndex, BaseItemDocument, BaseIndex, Policy);
}

FDnd5eJsonItemVariantReadResult UDnd5eJsonItemLibrary::ReadMagicItemVariantAtIndex(const FDnd5eJsonDocument& VariantDocument,
	int32 VariantIndex, const FDnd5eJsonDocument& BaseItemDocument, int32 BaseIndex, EDnd5eJsonItemVariantEditionPolicy Policy)
{
	return FDnd5eJsonItemVariantGenerator::ReadAtIndex(VariantDocument, VariantIndex, BaseItemDocument, BaseIndex, Policy);
}

FDnd5eJsonItemGroupReadResult UDnd5eJsonItemLibrary::ReadItemGroupAtIndex(const FDnd5eJsonDocument& Document, int32 Index)
{
	return FDnd5eJsonItemGroupReader::ReadAtIndex(Document, Index);
}

FDnd5eJsonItemEntryTemplateReadResult UDnd5eJsonItemLibrary::ReadItemEntryTemplateAtIndex(const FDnd5eJsonDocument& Document, int32 Index)
{
	return FDnd5eJsonItemEntryTemplateReader::ReadAtIndex(Document, Index);
}

FDnd5eJsonItemEntryExpansionResult UDnd5eJsonItemLibrary::ExpandItemEntryReference(const FDnd5eJsonItemEntryExpansionRequest& Request)
{
	return FDnd5eJsonItemEntryExpander::Expand(Request);
}

FDnd5eJsonItemSupplementReadResult UDnd5eJsonItemLibrary::ReadItemSupplements(const FDnd5eJsonDocument& Document)
{
	return FDnd5eJsonItemSupplementReader::ReadDocument(Document);
}

bool UDnd5eJsonItemLibrary::TryApplyItemSupplement(const FDnd5eItem& Item, const FDnd5eJsonItemSupplement& Supplement, FDnd5eItem& OutItem)
{
	return FDnd5eJsonItemSupplementReader::TryApply(Item, Supplement, OutItem);
}