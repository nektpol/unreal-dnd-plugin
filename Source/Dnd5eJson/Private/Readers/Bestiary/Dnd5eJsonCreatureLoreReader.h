// Developed by Neko Creative Technologies

#pragma once

#include "Bestiary/Dnd5eCreature.h"
#include "Diagnostics/Dnd5eDiagnostic.h"
#include "Readers/Dnd5eJsonDocument.h"

namespace Dnd5eJsonCreatureLoreReader
{
	bool ReadAndAttach(const TArray<FDnd5eJsonDocument>& Documents, TArray<FDnd5eCreature>& Creatures,
		TArray<FDnd5eDiagnostic>& Diagnostics);
}
