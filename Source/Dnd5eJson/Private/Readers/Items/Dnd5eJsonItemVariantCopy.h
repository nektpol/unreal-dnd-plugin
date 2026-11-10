// Developed by Neko Creative Technologies

#pragma once
#include "Readers/Dnd5eJsonReaderInternal.h"

namespace Dnd5eJsonReader
{
	struct FResolvedItemVariantTemplate
	{
		TSharedPtr<FJsonObject> Row;
		FString IdentitySource;
		TArray<int32> SourceRowIndices;
	};

	bool ResolveItemVariantTemplate(const TArray<TSharedPtr<FJsonValue>>& Rows, int32 Index,
		FResolvedItemVariantTemplate& Out, FReadState& State);
}