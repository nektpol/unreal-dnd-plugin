// Developed by Neko Creative Technologies

#pragma once

#include "CoreMinimal.h"
#include "Diagnostics/Dnd5eDiagnostic.h"
#include "Spells/Dnd5eSpell.h"
#include "Readers/Dnd5eJsonReadStatus.h"
#include "Dnd5eJsonSpellReadResult.generated.h"

USTRUCT(BlueprintType)
struct DND5EJSON_API FDnd5eJsonSpellReadResult
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Json|SpellReadResult")
	EDnd5eJsonReadStatus Status = EDnd5eJsonReadStatus::NotAttempted;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Json|SpellReadResult")
	FDnd5eSpell Spell;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|Json|SpellReadResult")
	TArray<FDnd5eDiagnostic> Diagnostics;
};
