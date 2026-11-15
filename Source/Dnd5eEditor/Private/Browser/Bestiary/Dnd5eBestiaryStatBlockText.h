// Developed by Neko Creative Technologies

#pragma once

#include "CoreMinimal.h"

struct FDnd5eCreature;

namespace Dnd5eBestiaryStatBlockText
{
	struct FAbility
	{
		FText Label;
		FText Value;
	};

	struct FSection
	{
		FText Heading;
		FText Body;
	};

	struct FStatBlock
	{
		FText Classification;
		FText Vitals;
		TArray<FAbility> Abilities;
		FText Facts;
		TArray<FSection> Sections;
		FText Environment;
		FText References;
	};

	FStatBlock Build(const FDnd5eCreature& Creature);
	FText Format(const FDnd5eCreature& Creature);
}