// Developed by Neko Creative Technologies

#include "Dnd5eJsonInlineSyntax.h"

namespace Dnd5eJsonReader
{
	bool TryParseInlineToken(const FString& Token, FString& Tag, TArray<FString>& Parts)
	{
		Tag.Reset(); Parts.Reset();
		if (Token.Len() > 65536 || !Token.StartsWith(TEXT("{@"), ESearchCase::CaseSensitive) || !Token.EndsWith(TEXT("}"))) { return false; }
		int32 Depth = 0;
		for (int32 Index = 0; Index < Token.Len(); ++Index)
		{
			if (Token[Index] == TEXT('{')) { if (++Depth > 64) { return false; } }
			else if (Token[Index] == TEXT('}'))
			{
				if (--Depth < 0 || (Depth == 0 && Index != Token.Len() - 1)) { return false; }
			}
		}
		if (Depth != 0) { return false; }
		int32 BodyStart = 2;
		while (BodyStart < Token.Len() - 1 && !FChar::IsWhitespace(Token[BodyStart])) { ++BodyStart; }
		if (BodyStart == 2 || BodyStart >= Token.Len() - 1) { return false; }
		Tag = Token.Mid(2, BodyStart - 2);
		Parts = SplitInlineArguments(Token.Mid(BodyStart + 1, Token.Len() - BodyStart - 2));
		return true;
	}

	TArray<FString> SplitInlineArguments(const FString& Body)
	{
		TArray<FString> Parts;
		int32 Depth = 0, Start = 0;
		for (int32 Index = 0; Index < Body.Len(); ++Index)
		{
			if (Body[Index] == TEXT('{')) { ++Depth; }
			else if (Body[Index] == TEXT('}')) { --Depth; }
			else if (Body[Index] == TEXT('|') && Depth == 0) { Parts.Add(Body.Mid(Start, Index - Start)); Start = Index + 1; }
		}
		Parts.Add(Body.Mid(Start));
		return Parts;
	}
}
