// Developed by Neko Creative Technologies

#pragma once

#include "CoreMinimal.h"
namespace Dnd5eJsonReader
{
	TArray<FString> SplitInlineArguments(const FString& Body);
	bool TryParseInlineToken(const FString& Token, FString& Tag, TArray<FString>& Parts);
}
