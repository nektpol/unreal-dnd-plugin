// Developed by Neko Creative Technologies

#pragma once

#include "CoreMinimal.h"
#include "Readers/Dnd5eJsonReaderInternal.h"

struct FDnd5eCreatureCopyRow
{
	TSharedPtr<FJsonObject> Object;
	FString RelativeFilePath;
	FString JsonPointer;
};

struct FDnd5eCreatureCopyIndex
{
	TMap<FString, FDnd5eCreatureCopyRow> Rows;
	TSet<FString> AmbiguousKeys;
	TMap<FString, FDnd5eCreatureCopyRow> TemplateRows;
	TSet<FString> AmbiguousTemplateKeys;
	bool bHasTemplateDocument = false;
};

void IndexCreatureCopyRow(FDnd5eCreatureCopyIndex& Index, const FDnd5eCreatureCopyRow& Row);

bool ResolveCreatureCopy(const FDnd5eCreatureCopyIndex& Index,
	const FDnd5eCreatureCopyRow& Child, TSharedPtr<FJsonObject>& OutObject,
	TArray<FDnd5eCreatureCopyRow>& OutAncestors, Dnd5eJsonReader::FReadState& State);