// Developed by Neko Creative Technologies

#pragma once
#include "Readers/Dnd5eJsonReaderInternal.h"

namespace Dnd5eJsonReader
{
	bool ApplyItemCopyModifiers(const FJsonObject& Modifiers, const FString& Pointer, int32 SourceIndex,
		FJsonObject& Merged, TMap<FString, int32>& FieldSources, FReadState& State);
}
