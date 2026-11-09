// Developed by Neko Creative Technologies

#include "Readers/Items/Dnd5eJsonItemLibrary.h"

FDnd5eJsonItemReadResult UDnd5eJsonItemLibrary::ReadItemAtIndex(const FDnd5eJsonDocument& Document, int32 Index)
{
	return FDnd5eJsonItemReader::ReadAtIndex(Document, Index);
}