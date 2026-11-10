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