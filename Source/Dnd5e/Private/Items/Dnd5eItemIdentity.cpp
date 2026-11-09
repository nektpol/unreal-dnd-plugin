// Developed by Neko Creative Technologies

#include "Items/Dnd5eItemIdentity.h"
#include "Dnd5eItemIdentityInternal.h"
#include "Dnd5eTags.h"

bool Dnd5e::Items::TryMakeId(const FString& DatasetNamespace, const FString& Name,
	const FString& SourceId, FDnd5eRecordId& OutId)
{
	using namespace Dnd5e::ItemIdentityInternal;
	FString Scope, Key, Publication;
	if (!NormalizeIdPart(DatasetNamespace, false, Scope) ||
		!NormalizeIdPart(Name, true, Key) ||
		!NormalizeIdPart(SourceId, true, Publication))
	{
		OutId = {};
		return false;
	}

	FDnd5eRecordId Candidate;
	Candidate.DatasetNamespace = MoveTemp(Scope);
	Candidate.Category = Dnd5e::Category::Item;
	Candidate.SourceKey = FString::Printf(TEXT("dnd5e:item:v1:%s|%s"),
		*EscapeIdPart(MoveTemp(Key)), *EscapeIdPart(MoveTemp(Publication)));
	OutId = MoveTemp(Candidate);
	return true;
}