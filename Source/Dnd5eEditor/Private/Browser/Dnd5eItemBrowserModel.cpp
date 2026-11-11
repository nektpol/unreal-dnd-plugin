// Developed by Neko Creative Technologies

#include "Browser/Dnd5eItemBrowserModel.h"
#include "Catalog/Dnd5eItemCatalogObject.h"
#include "Dnd5eTags.h"

namespace
{
	FDnd5eItemBrowserRowPtr MakeItemBrowserRow(const FDnd5eRecordId& Id, const FDnd5eItem& Item)
	{
		auto Row = MakeShared<FDnd5eItemBrowserRow>();
		Row->Id = Id;
		Row->Name = Item.Header.DisplayName;
		Row->Rarity = Item.Rarity;
		if (!Item.Header.Provenance.IsEmpty()) { Row->SourceId = Item.Header.Provenance[0].SourceId; }
		return Row;
	}
}

void FDnd5eItemBrowserModel::Rebuild(const UDnd5eItemCatalogObject* Catalog)
{
	AllRows.Reset();
	TArray<FDnd5eRecordId> Ids;
	TArray<FDnd5eDiagnostic> Diagnostics;
	if (IsValid(Catalog) && Catalog->QueryRecordIds({}, Ids, Diagnostics))
	{
		AllRows.Reserve(Ids.Num());
		for (const FDnd5eRecordId& Id : Ids)
		{
			if (Id.Category == Dnd5e::Category::ItemGroup.GetTag())
			{
				FDnd5eItemGroup Group;
				if (Catalog->TryGetGroup(Id, Group)) { AllRows.Add(MakeItemBrowserRow(Id, Group.Details)); }
			}
			else
			{
				FDnd5eItem Item;
				if (Catalog->TryGetItem(Id, Item)) { AllRows.Add(MakeItemBrowserRow(Id, Item)); }
			}
		}
	}
	ApplyFilter();
}

void FDnd5eItemBrowserModel::SetSearchText(const FString& Text)
{
	SearchText = Text.TrimStartAndEnd();
	ApplyFilter();
}

void FDnd5eItemBrowserModel::ApplyFilter()
{
	FilteredRows.Reset();
	FilteredRows.Reserve(AllRows.Num());
	for (const auto& Row : AllRows)
	{
		if (SearchText.IsEmpty() || Row->Name.Contains(SearchText, ESearchCase::IgnoreCase))
		{
			FilteredRows.Add(Row);
		}
	}
}