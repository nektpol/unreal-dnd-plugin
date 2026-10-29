#pragma once

#include "CoreMinimal.h"
#include "Readers/Dnd5eJsonSpellReadRequest.h"
#include "Readers/Dnd5eJsonSpellReadResult.h"

class DND5EJSON_API FDnd5eJsonSpellReader
{
public:
	// Reads one selected spell from supplied text; failures return diagnostics and a default spell.
	static FDnd5eJsonSpellReadResult ReadSpell(const FDnd5eJsonSpellReadRequest& Request);
};
