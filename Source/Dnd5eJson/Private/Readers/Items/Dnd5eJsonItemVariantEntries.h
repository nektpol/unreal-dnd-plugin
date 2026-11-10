// Developed by Neko Creative Technologies

#pragma once
#include "Readers/Dnd5eJsonReaderInternal.h"

namespace Dnd5eJsonReader
{
	bool RenderItemVariantEntries(const TArray<TSharedPtr<FJsonValue>>& Entries, const FJsonObject& Base,
		const FJsonObject& Inherits, const FString& BaseName, const FString& Pointer,
		TArray<TSharedPtr<FJsonValue>>& Out, FReadState& State);
}