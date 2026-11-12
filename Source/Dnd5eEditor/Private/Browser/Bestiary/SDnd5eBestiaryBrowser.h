// Developed by Neko Creative Technologies

#pragma once

#include "CoreMinimal.h"
#include "Records/Dnd5eRecordId.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/SListView.h"

class UDnd5eEditorBestiaryDatasetSubsystem;
struct FDnd5eCreature;

class SDnd5eBestiaryBrowser : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SDnd5eBestiaryBrowser) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual ~SDnd5eBestiaryBrowser() override;

private:
#if WITH_DEV_AUTOMATION_TESTS
	friend class FDnd5eBestiaryBrowserWidgetTest;
#endif

	struct FRow
	{
		FDnd5eRecordId Id;
		FString Name;
		FString Source;
	};

	using FRowPtr = TSharedPtr<FRow>;

	void RefreshCatalog();
	void ApplyFilter();
	void HandleSearchChanged(const FText& Text);
	void HandleSelectionChanged(FRowPtr Row, ESelectInfo::Type SelectInfo);
	TSharedRef<ITableRow> GenerateRow(FRowPtr Row, const TSharedRef<STableViewBase>& OwnerTable);
	FText GetSummary() const;
	void ShowCreature(const FDnd5eCreature& Creature);

	TWeakObjectPtr<UDnd5eEditorBestiaryDatasetSubsystem> Session;
	FDelegateHandle CatalogChangedHandle;
	TArray<FRowPtr> AllRows;
	TArray<FRowPtr> VisibleRows;
	TSharedPtr<SListView<FRowPtr>> ListView;
	FString Filter;
	FText CardTitle;
	FText CardSource;
	FText CardBody;
};