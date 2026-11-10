// Developed by Neko Creative Technologies

#pragma once
#include "Readers/Dnd5eJsonReaderInternal.h"

namespace Dnd5eJsonReader
{
	bool ReadPreserveMap(const FJsonObject& Copy, const FString& Pointer, TSharedPtr<FJsonObject>& Out, FReadState& State);
	bool IsProtectedCopyField(const FString& Key);
	bool IsPreserved(const TSharedPtr<FJsonObject>& Preserve, const TCHAR* Key);
	bool ContainsCopyVariable(const FJsonValue& Value);
}