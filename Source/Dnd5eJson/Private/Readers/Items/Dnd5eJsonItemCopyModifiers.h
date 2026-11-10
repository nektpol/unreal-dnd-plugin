// Developed by Neko Creative Technologies

#pragma once
#include "Readers/Dnd5eJsonReaderInternal.h"

namespace Dnd5eJsonReader
{
	struct FItemCopyFieldOrigins
	{
		TMap<FString, int32> Rows;
		TMap<FString, FString> Pointers;
	};

	bool ApplyItemCopyModifiers(const FJsonObject& Modifiers, const FString& Pointer, int32 SourceIndex,
		FJsonObject& Merged, FItemCopyFieldOrigins& Origins, FReadState& State);
}
