// Developed by Neko Creative Technologies

#pragma once
#include "Readers/Items/Dnd5eJsonItemVariantMatcher.h"
#include "Dom/JsonObject.h"

namespace Dnd5eJsonReader
{
	struct FItemVariantMatchInputs
	{
		TSharedPtr<FJsonObject> VariantRoot;
		TSharedPtr<FJsonObject> Variant;
		TSharedPtr<FJsonObject> BaseRoot;
		const TArray<TSharedPtr<FJsonValue>>* Bases = nullptr;
	};

	FDnd5eJsonItemVariantMatchResult MatchItemVariant(const FDnd5eJsonDocument& VariantDocument,
		int32 VariantIndex, const FDnd5eJsonDocument& BaseItemDocument, EDnd5eJsonItemVariantEditionPolicy Policy,
		FItemVariantMatchInputs* OutInputs);
}