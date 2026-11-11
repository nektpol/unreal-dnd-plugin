// Developed by Neko Creative Technologies

#pragma once

#include "CoreMinimal.h"
#include "Browser/Dnd5eSpellBrowserModel.h"
#include "Browser/Dnd5eSpellBrowserSelection.h"
#include "UObject/StrongObjectPtr.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/SListView.h"

class IDetailsView;
class UDnd5eEditorDatasetSubsystem;
class SSearchBox;

class SDnd5eSpellBrowser : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SDnd5eSpellBrowser) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual ~SDnd5eSpellBrowser() override;

	bool SelectSpellById(const FDnd5eRecordId& Id);

private:
	void RefreshCatalog();
	void HandleSearchChanged(const FText& Text);
	void HandleSelectionChanged(FDnd5eSpellBrowserRowPtr Item, ESelectInfo::Type SelectInfo);
	TSharedRef<ITableRow> GenerateRow(FDnd5eSpellBrowserRowPtr Item, const TSharedRef<STableViewBase>& OwnerTable);
	void ClearDetails();
	FText GetSummary() const;
	FText GetReferenceSummary() const;

	FDnd5eSpellBrowserModel Model;
	TSharedPtr<SListView<FDnd5eSpellBrowserRowPtr>> ListView;
	TSharedPtr<IDetailsView> DetailsView;
	TStrongObjectPtr<UDnd5eSpellBrowserSelection> SelectedData;
	TWeakObjectPtr<UDnd5eEditorDatasetSubsystem> DatasetSubsystem;
	FDelegateHandle CatalogChangedHandle;
	FText ReferenceSummary;

	TSharedPtr<SSearchBox> SearchBox;
};
