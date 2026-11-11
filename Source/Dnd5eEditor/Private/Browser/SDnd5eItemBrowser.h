// Developed by Neko Creative Technologies

#pragma once

#include "CoreMinimal.h"
#include "Browser/Dnd5eItemBrowserModel.h"
#include "UObject/StrongObjectPtr.h"
#include "Widgets/SCompoundWidget.h"
#include "Browser/Dnd5eItemBrowserDetails.h"
#include "Widgets/Views/SListView.h"

class IDetailsView;
class UDnd5eEditorItemDatasetSubsystem;
class SScrollBox;

class SDnd5eItemBrowser : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SDnd5eItemBrowser) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual ~SDnd5eItemBrowser() override;

private:
	void RefreshCatalog();
	void HandleSearchChanged(const FText& Text);
	void HandleSelectionChanged(FDnd5eItemBrowserRowPtr Row, ESelectInfo::Type SelectInfo);
	TSharedRef<ITableRow> GenerateRow(FDnd5eItemBrowserRowPtr Row, const TSharedRef<STableViewBase>& OwnerTable);
	void ClearDetails();
	FText GetSummary() const;

	FDnd5eItemBrowserModel Model;
	TSharedPtr<SListView<FDnd5eItemBrowserRowPtr>> ListView;
	TSharedPtr<IDetailsView> DetailsView;
	TStrongObjectPtr<UObject> SelectedData;
	TWeakObjectPtr<UDnd5eEditorItemDatasetSubsystem> DatasetSubsystem;
	FDelegateHandle CatalogChangedHandle;

	FText GetDetailTitle() const;
	FText GetDetailFacts() const { return Overview.Facts; }
	FText GetDetailDescription() const { return Overview.Description; }

	FDnd5eItemBrowserDetails Overview;
	TSharedPtr<SScrollBox> OverviewScroll;
};