#pragma once

#include "CoreMinimal.h"
#include "Spells/Dnd5eSpell.h"
#include "UObject/Object.h"
#include "Dnd5eSpellBrowserSelection.generated.h"

UCLASS(Transient)
class UDnd5eSpellBrowserSelection : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(VisibleAnywhere, Transient, Category = "Dnd5e|Spell Browser")
	FDnd5eSpell Spell;
};