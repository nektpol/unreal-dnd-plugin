// Developed by Neko Creative Technologies

#include "Values/Dnd5eMediaReference.h"
namespace Dnd5eMedia
{
	bool IsDatasetRelativePath(const FString& Path)
	{
		if (Path.IsEmpty() || Path != Path.TrimStartAndEnd() || Path.StartsWith(TEXT("/")) || Path.Contains(TEXT("\\")) || Path.Contains(TEXT(":"))) { return false; }
		for (int32 Index = 0; Index < Path.Len(); ++Index)
		{
			const uint32 Unit = static_cast<uint32>(Path[Index]);
			if (FChar::IsControl(Path[Index]) || FCString::Strchr(TEXT("*?<>|\""), Path[Index])) { return false; }
			if (Unit >= 0xD800 && Unit <= 0xDBFF)
			{
				if (++Index == Path.Len()) { return false; }
				const uint32 Next = static_cast<uint32>(Path[Index]); if (Next < 0xDC00 || Next > 0xDFFF) { return false; }
			}
			else if (Unit >= 0xDC00 && Unit <= 0xDFFF) { return false; }
		}
		TArray<FString> Segments; Path.ParseIntoArray(Segments, TEXT("/"), false);
		for (const auto& Segment : Segments) { if (Segment.IsEmpty() || Segment == TEXT(".") || Segment == TEXT("..")) { return false; } }
		return true;
	}
}
