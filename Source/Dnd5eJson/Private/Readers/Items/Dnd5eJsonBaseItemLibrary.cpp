// Developed by Neko Creative Technologies

#include "Readers/Items/Dnd5eJsonBaseItemLibrary.h"

FDnd5eJsonBaseItemReadResult UDnd5eJsonBaseItemLibrary::ReadBaseItems(const FDnd5eJsonDocument& Document)
{
	return FDnd5eJsonBaseItemReader::Read(Document);
}