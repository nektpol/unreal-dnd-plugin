// Developed by Neko Creative Technologies

#pragma once
#include "CoreMinimal.h"

struct FDnd5eSpell;

struct FDnd5eSpellBrowserDetails
{
	FText Title;
	FText Source;
	FText Classification;
	FText Mechanics;
	FText Description;
};

namespace Dnd5eSpellBrowserDetails
{
	FDnd5eSpellBrowserDetails Build(const FDnd5eSpell& Spell);
}