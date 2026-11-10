// Developed by Neko Creative Technologies

#pragma once
#include "Readers/Dnd5eJsonReaderInternal.h"

namespace Dnd5eJsonReader
{
	bool TransformItemEntryStrings(const TSharedPtr<FJsonValue>& Value, const FString& Pointer,
		TSharedPtr<FJsonValue>& Out, FReadState& State,
		TFunctionRef<bool(const FString&, const FString&, FString&, FReadState&)> Render, int32 Depth = 0);
}