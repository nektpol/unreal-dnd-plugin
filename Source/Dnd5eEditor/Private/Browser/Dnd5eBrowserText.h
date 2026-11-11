// Developed by Neko Creative Technologies
#pragma once

#include "CoreMinimal.h"

struct FDnd5eContentBlock;
struct FDnd5eDistance;
struct FDnd5eNumber;
struct FDnd5eRecordHeader;
enum class EDnd5eCurrency : uint8;

namespace Dnd5eBrowserText
{
	inline constexpr int32 MaxPreviewChars = 32768;

	FString FormatNumber(const FDnd5eNumber& Number);
	FString FormatCurrency(EDnd5eCurrency Currency);
	FString FormatDistance(const FDnd5eDistance& Distance);
	FText FormatSource(const FDnd5eRecordHeader& Header);
	void AppendPreview(FString& Out, const FString& Text);
	void AppendBlocks(const TArray<FDnd5eContentBlock>& Blocks, FString& Out, int32 Depth);
	FText FinishPreview(FString Description);
	FText FormatDescription(const TArray<FDnd5eContentBlock>& Blocks);
}