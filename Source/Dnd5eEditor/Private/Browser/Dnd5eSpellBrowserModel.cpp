#include "Dnd5eSpellBrowserModel.h"
#include "Catalog/Dnd5eSpellCatalogObject.h"

void FDnd5eSpellBrowserModel::Rebuild(const UDnd5eSpellCatalogObject* Catalog)
{
	AllRows.Reset();
	if (IsValid(Catalog))
	{
		const TArray<FDnd5eRecordId> Ids = Catalog->GetSpellIds();
		AllRows.Reserve(Ids.Num());
		for (const FDnd5eRecordId& Id : Ids)
		{
			FDnd5eSpell Spell;
			if (!Catalog->TryGetSpell(Id, Spell)) { continue; }
			auto Row = MakeShared<FDnd5eSpellBrowserRow>();
			Row->Id = Id;
			Row->Name = Spell.Header.DisplayName;
			Row->Level = Spell.Level;
			if (!Spell.Header.Provenance.IsEmpty()) { Row->SourceId = Spell.Header.Provenance[0].SourceId; }
			AllRows.Add(Row);
		}
	}
	ApplyFilter();
}

void FDnd5eSpellBrowserModel::SetSearchText(const FString& Text)
{
	SearchText = Text.TrimStartAndEnd();
	ApplyFilter();
}

void FDnd5eSpellBrowserModel::ApplyFilter()
{
	FilteredRows.Reset();
	FilteredRows.Reserve(AllRows.Num());
	for (const FDnd5eSpellBrowserRowPtr& Row : AllRows)
	{
		if (SearchText.IsEmpty() || Row->Name.Contains(SearchText, ESearchCase::IgnoreCase)) { FilteredRows.Add(Row); }
	}
}