// Developed by Neko Creative Technologies

#pragma once

#include "CoreMinimal.h"
#include "Spells/Dnd5eSpell.h"
#include "Diagnostics/Dnd5eDiagnostic.h"
#include "Content/Dnd5eContentType.h"
#include "Records/Dnd5eReferenceResolution.h"
#include "Dnd5eSpellCatalog.generated.h"

USTRUCT(BlueprintType)
struct DND5E_API FDnd5eSpellQuery
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|SpellQuery")
	FString DatasetNamespace;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|SpellQuery")
	FString SourceId;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|SpellQuery")
	FString Name;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|SpellQuery")
	int32 Level = INDEX_NONE;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|SpellQuery")
	FGameplayTagContainer Schools;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|SpellQuery")
	FGameplayTagContainer Traits;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|SpellQuery")
	bool bIncludeAliases = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|SpellQuery")
	bool bRequireAvailability = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|SpellQuery")
	FDnd5eSpellAvailabilityProvider AvailabilityProvider;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|SpellQuery")
	FString AvailabilitySubgroup;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|SpellQuery")
	FString AvailabilityDefinedInSource;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dnd5e|SpellQuery")
	bool bIncludeVariantAvailability = true;
};

// The caller owns synchronization when replacing a catalog shared with other threads.
class DND5E_API FDnd5eSpellCatalog
{
public:
	bool ReplaceSpells(const TArray<FDnd5eSpell>& Records, TArray<FDnd5eDiagnostic>& Diagnostics);
	bool TryGetSpell(const FDnd5eRecordId& Id, FDnd5eSpell& Spell) const;
	bool QuerySpellIds(const FDnd5eSpellQuery& Query, TArray<FDnd5eRecordId>& Ids, TArray<FDnd5eDiagnostic>& Diagnostics) const;
	int32 GetSpellCount() const { return Spells.Num(); }
	TArray<FDnd5eRecordId> GetSpellIds() const { return OrderedIds; }
	EDnd5eReferenceResolveStatus ResolveSpellReference(const FDnd5eReferenceContent& Reference, FDnd5eSpell& Spell, TArray<FDnd5eDiagnostic>& Diagnostics) const;

private:
	TMap<FDnd5eRecordId, FDnd5eSpell> Spells;
	TArray<FDnd5eRecordId> OrderedIds;
};
