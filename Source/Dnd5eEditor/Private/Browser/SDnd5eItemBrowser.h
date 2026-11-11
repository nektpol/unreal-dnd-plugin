// Developed by Neko Creative Technologies

#pragma once

#include "CoreMinimal.h"
#include "Browser/Dnd5eItemBrowserModel.h"
#include "UObject/StrongObjectPtr.h"
#include "Widgets/SCompoundWidget.h"
#include "Browser/Dnd5eItemBrowserDetails.h"
#include "Browser/Dnd5eItemBrowserLinks.h"
#include "Widgets/Views/SListView.h"

class IDetailsView;
class UDnd5eEditorItemDatasetSubsystem;
class SScrollBox;
class SSearchBox;
class SVerticalBox;
class SSearchBox;
class UDnd5eEditorDatasetSubsystem;

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
	FText GetDetailSource() const { return Overview.Source; }
	FText GetDetailClassification() const { return Overview.Classification; }
	FText GetDetailEquipment() const { return Overview.Equipment; }
	FText GetDetailFacts() const { return Overview.Facts; }
	FText GetDetailDescription() const { return Overview.Description; }

	FDnd5eItemBrowserDetails Overview;
	TSharedPtr<SScrollBox> OverviewScroll;

	void RebuildReferences();
	FReply FollowReference(FDnd5eItemBrowserLink Link);
	bool SelectItemById(const FDnd5eRecordId& Id);

	TSharedPtr<SSearchBox> SearchBox;
	TSharedPtr<SVerticalBox> ReferenceBox;
	FText ReferencePreview;
	TWeakObjectPtr<UDnd5eEditorDatasetSubsystem> SpellDatasetSubsystem;
	FDelegateHandle SpellCatalogChangedHandle;

};
