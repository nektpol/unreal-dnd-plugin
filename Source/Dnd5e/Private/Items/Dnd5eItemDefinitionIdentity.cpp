// Developed by Neko Creative Technologies

#include "Items/Dnd5eItemDefinitionIdentity.h"
#include "Dnd5eTags.h"

namespace
{
	bool NormalizeDefinitionIdPart(const FString& Input, bool bFoldCase, FString& Out)
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

	FString EscapeDefinitionIdPart(FString Value)
	{
		Value.ReplaceInline(TEXT("%"), TEXT("%25"), ESearchCase::CaseSensitive);
		Value.ReplaceInline(TEXT("|"), TEXT("%7C"), ESearchCase::CaseSensitive);
		Value.ReplaceInline(TEXT(":"), TEXT("%3A"), ESearchCase::CaseSensitive);
		return Value;
	}
}

bool Dnd5e::ItemDefinitions::TryMakeId(const FString& DatasetNamespace, FGameplayTag Category,
	const FString& Key, const FString& SourceId, FDnd5eRecordId& OutId)
{
	const TCHAR* Domain = nullptr;
	if (Category == Dnd5e::Category::ItemType) { Domain = TEXT("type"); }
	else if (Category == Dnd5e::Category::ItemProperty) { Domain = TEXT("property"); }
	else if (Category == Dnd5e::Category::ItemMastery) { Domain = TEXT("mastery"); }

	FString Scope, NormalizedKey, Publication;
	if (!Domain || !NormalizeDefinitionIdPart(DatasetNamespace, false, Scope) ||
		!NormalizeDefinitionIdPart(Key, true, NormalizedKey) ||
		!NormalizeDefinitionIdPart(SourceId, true, Publication))
	{
		OutId = {};
		return false;
	}

	FDnd5eRecordId Candidate;
	Candidate.DatasetNamespace = MoveTemp(Scope);
	Candidate.Category = Category;
	Candidate.SourceKey = FString::Printf(TEXT("dnd5e:item-definition:v1:%s:%s|%s"),
		Domain, *EscapeDefinitionIdPart(MoveTemp(NormalizedKey)),
		*EscapeDefinitionIdPart(MoveTemp(Publication)));
	OutId = MoveTemp(Candidate);
	return true;
}