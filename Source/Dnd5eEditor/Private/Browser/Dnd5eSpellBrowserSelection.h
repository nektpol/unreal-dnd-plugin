// Developed by Neko Creative Technologies

#pragma once

#include "CoreMinimal.h"
#include "Spells/Dnd5eSpell.h"
#include "UObject/Object.h"
#include "Readers/Dnd5eJsonSpellReferenceReporter.h"
#include "Dnd5eSpellBrowserSelection.generated.h"

UCLASS(Transient)
class UDnd5eSpellBrowserSelection : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(VisibleAnywhere, Transient, Category = "Dnd5e|Spell Browser")
	FDnd5eSpell Spell;

	UPROPERTY(VisibleAnywhere, Transient, Category = "Dnd5e|Reference Report")
	FDnd5eJsonSpellReferenceReport ReferenceReport;
};
