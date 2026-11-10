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
