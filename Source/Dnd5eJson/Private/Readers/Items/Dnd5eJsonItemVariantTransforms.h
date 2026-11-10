// Developed by Neko Creative Technologies

#pragma once
#include "Readers/Dnd5eJsonReaderInternal.h"

namespace Dnd5eJsonReader
{
	bool ApplyItemVariantTransforms(const FJsonObject& Base, const FJsonObject& Inherits,
		const FString& BasePointer, const FString& InheritsPointer, FJsonObject& Row,
		FReadState& BaseState, FReadState& VariantState);
}