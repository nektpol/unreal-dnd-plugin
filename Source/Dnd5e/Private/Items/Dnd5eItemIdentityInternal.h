// Developed by Neko Creative Technologies

#pragma once
#include "CoreMinimal.h"

namespace Dnd5e::ItemIdentityInternal
{
	inline bool NormalizeIdPart(const FString& Input, bool bFoldCase, FString& Out)
	{
		if (Input.Len() > 1024) { return false; }
		for (TCHAR Ch : Input)
		{
			if (FChar::IsControl(Ch)) { return false; }
		}

		Out = Input.TrimStartAndEnd();
		if (bFoldCase) { Out.ToLowerInline(); }
		return !Out.IsEmpty();
	}

	inline FString EscapeIdPart(FString Value)
	{
		Value.ReplaceInline(TEXT("%"), TEXT("%25"), ESearchCase::CaseSensitive);
		Value.ReplaceInline(TEXT("|"), TEXT("%7C"), ESearchCase::CaseSensitive);
		Value.ReplaceInline(TEXT(":"), TEXT("%3A"), ESearchCase::CaseSensitive);
		return Value;
	}
}