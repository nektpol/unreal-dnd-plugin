// Developed by Neko Creative Technologies

#include "Items/Dnd5eItemIdentity.h"
#include "Dnd5eItemIdentityInternal.h"
#include "Dnd5eTags.h"

namespace
{
	bool TryMakeItemRecordId(const FString& DatasetNamespace, const FString& Name, const FString& SourceId,
		const FGameplayTag& Category, const TCHAR* Prefix, FDnd5eRecordId& OutId)
	{
		using namespace Dnd5e::ItemIdentityInternal;
		FString Scope, Key, Publication;
		if (!NormalizeIdPart(DatasetNamespace, false, Scope) || !NormalizeIdPart(Name, true, Key) ||
			!NormalizeIdPart(SourceId, true, Publication))
		{
			OutId = {};
			return false;
		}

		FDnd5eRecordId Candidate;
		Candidate.DatasetNamespace = MoveTemp(Scope);
		Candidate.Category = Category;
		Candidate.SourceKey = FString::Printf(TEXT("%s%s|%s"), Prefix,
			*EscapeIdPart(MoveTemp(Key)), *EscapeIdPart(MoveTemp(Publication)));
		OutId = MoveTemp(Candidate);
		return true;
	}
}

bool Dnd5e::Items::TryMakeId(const FString& DatasetNamespace, const FString& Name,
	const FString& SourceId, FDnd5eRecordId& OutId)
{
	return TryMakeItemRecordId(DatasetNamespace, Name, SourceId, Dnd5e::Category::Item, TEXT("dnd5e:item:v1:"), OutId);
}

bool Dnd5e::Items::TryMakeGroupId(const FString& DatasetNamespace, const FString& Name,
	const FString& SourceId, FDnd5eRecordId& OutId)
{
	return TryMakeItemRecordId(DatasetNamespace, Name, SourceId, Dnd5e::Category::ItemGroup, TEXT("dnd5e:item-group:v1:"), OutId);
}