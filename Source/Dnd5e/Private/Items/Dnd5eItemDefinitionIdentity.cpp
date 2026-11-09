// Developed by Neko Creative Technologies

#include "Items/Dnd5eItemDefinitionIdentity.h"
#include "Dnd5eTags.h"
#include "Dnd5eItemIdentityInternal.h"

bool Dnd5e::ItemDefinitions::TryMakeId(const FString& DatasetNamespace, FGameplayTag Category,
	const FString& Key, const FString& SourceId, FDnd5eRecordId& OutId)
{
	using namespace Dnd5e::ItemIdentityInternal;

	const TCHAR* Domain = nullptr;
	if (Category == Dnd5e::Category::ItemType) { Domain = TEXT("type"); }
	else if (Category == Dnd5e::Category::ItemProperty) { Domain = TEXT("property"); }
	else if (Category == Dnd5e::Category::ItemMastery) { Domain = TEXT("mastery"); }

	FString Scope, NormalizedKey, Publication;
	if (!Domain || !NormalizeIdPart(DatasetNamespace, false, Scope) ||
		!NormalizeIdPart(Key, true, NormalizedKey) ||
		!NormalizeIdPart(SourceId, true, Publication))
	{
		OutId = {};
		return false;
	}

	FDnd5eRecordId Candidate;
	Candidate.DatasetNamespace = MoveTemp(Scope);
	Candidate.Category = Category;
	Candidate.SourceKey = FString::Printf(TEXT("dnd5e:item-definition:v1:%s:%s|%s"),
		Domain, *EscapeIdPart(MoveTemp(NormalizedKey)),
		*EscapeIdPart(MoveTemp(Publication)));
	OutId = MoveTemp(Candidate);
	return true;
}
