#include "Records/Dnd5eRecordId.h"
#include "Misc/Crc.h"

namespace
{
	bool ExactText(const FString& A, const FString& B)
	{
		return A.Len() == B.Len() && FMemory::Memcmp(*A, *B, A.Len() * sizeof(TCHAR)) == 0;
	}

	uint32 HashText(const FString& Text) { return FCrc::MemCrc32(*Text, Text.Len() * sizeof(TCHAR)); }
}

bool FDnd5eRecordId::operator==(const FDnd5eRecordId& Other) const
{
	return Category == Other.Category && ExactText(DatasetNamespace, Other.DatasetNamespace) && ExactText(SourceKey, Other.SourceKey);
}

uint32 GetTypeHash(const FDnd5eRecordId& Id)
{
	return HashCombineFast(HashCombineFast(HashText(Id.DatasetNamespace), GetTypeHash(Id.Category)), HashText(Id.SourceKey));
}
