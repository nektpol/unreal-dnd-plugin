// Developed by Neko Creative Technologies

#include "Browser/SDnd5eItemBrowser.h"
#include "Browser/Dnd5eItemBrowserSelection.h"
#include "DetailsViewArgs.h"
#include "Dnd5eTags.h"
#include "Editor.h"
#include "IDetailsView.h"
#include "Modules/ModuleManager.h"
#include "PropertyEditorModule.h"
#include "Subsystems/Dnd5eEditorItemDatasetSubsystem.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SSplitter.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Views/STableRow.h"

#define LOCTEXT_NAMESPACE "Dnd5eItemBrowser"

void SDnd5eItemBrowser::Construct(const FArguments& InArgs)
{
	FDetailsViewArgs DetailsArgs;
	DetailsArgs.bUpdatesFromSelection = false;
	DetailsArgs.bLockable = false;
	DetailsArgs.bAllowSearch = true;
	DetailsArgs.bHideSelectionTip = true;
	DetailsArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;
	auto& PropertyEditor = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	DetailsView = PropertyEditor.CreateDetailView(DetailsArgs);
	DetailsView->SetIsPropertyEditingEnabledDelegate(FIsPropertyEditingEnabled::CreateLambda([]() { return false; }));

	ChildSlot
	[
		SNew(SBorder).Padding(8)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 8)
			[
				SNew(STextBlock).Text(this, &SDnd5eItemBrowser::GetSummary)
			]
			+ SVerticalBox::Slot().FillHeight(1)
			[
				SNew(SSplitter)
				+ SSplitter::Slot().Value(0.4f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 8, 8)
					[
						SNew(SSearchBox).HintText(LOCTEXT("Search", "Search item and group names"))
						.OnTextChanged(this, &SDnd5eItemBrowser::HandleSearchChanged)
					]
					+ SVerticalBox::Slot().FillHeight(1).Padding(0, 0, 8, 0)
					[
						SAssignNew(ListView, SListView<FDnd5eItemBrowserRowPtr>)
						.ListItemsSource(&Model.GetRows())
						.SelectionMode(ESelectionMode::Single)
						.OnGenerateRow(this, &SDnd5eItemBrowser::GenerateRow)
						.OnSelectionChanged(this, &SDnd5eItemBrowser::HandleSelectionChanged)
					]
				]
				+ SSplitter::Slot().Value(0.6f)
				[
					DetailsView.ToSharedRef()
				]
			]
		]
	];

	DatasetSubsystem = GEditor ? GEditor->GetEditorSubsystem<UDnd5eEditorItemDatasetSubsystem>() : nullptr;
	if (auto* Subsystem = DatasetSubsystem.Get())
	{
		CatalogChangedHandle = Subsystem->OnCatalogChanged().AddSP(this, &SDnd5eItemBrowser::RefreshCatalog);
	}
	RefreshCatalog();
}

SDnd5eItemBrowser::~SDnd5eItemBrowser()
{
	if (auto* Subsystem = DatasetSubsystem.Get()) { Subsystem->OnCatalogChanged().Remove(CatalogChangedHandle); }
}

void SDnd5eItemBrowser::RefreshCatalog()
{
	ListView->ClearSelection();
	ClearDetails();
	const auto* Subsystem = DatasetSubsystem.Get();
	Model.Rebuild(Subsystem ? Subsystem->GetItemCatalog() : nullptr);
	ListView->RequestListRefresh();
}

void SDnd5eItemBrowser::HandleSearchChanged(const FText& Text)
{
	ListView->ClearSelection();
	ClearDetails();
	Model.SetSearchText(Text.ToString());
	ListView->RequestListRefresh();
}

TSharedRef<ITableRow> SDnd5eItemBrowser::GenerateRow(FDnd5eItemBrowserRowPtr Row, const TSharedRef<STableViewBase>& OwnerTable)
{
	const FText Source = Row->SourceId.IsEmpty() ? LOCTEXT("NoSource", "Source unspecified") : FText::FromString(Row->SourceId);
	const FText Rarity = Row->Rarity.IsValid() ? FText::FromName(Row->Rarity.GetTagName()) : LOCTEXT("NoRarity", "Rarity unspecified");
	const FText Kind = Row->Id.Category == Dnd5e::Category::ItemGroup.GetTag() ? LOCTEXT("Group", "Group") : LOCTEXT("Item", "Item");
	const FText Label = FText::Format(LOCTEXT("Row", "{0} | {1} | {2} | {3}"), FText::FromString(Row->Name), Kind, Source, Rarity);
	return SNew(STableRow<FDnd5eItemBrowserRowPtr>, OwnerTable).Padding(4)
	[
		SNew(STextBlock).Text(Label).ToolTipText(Label)
	];
}

void SDnd5eItemBrowser::HandleSelectionChanged(FDnd5eItemBrowserRowPtr Row, ESelectInfo::Type SelectInfo)
{
	ClearDetails();
	const auto* Subsystem = DatasetSubsystem.Get();
	const auto* Catalog = Subsystem ? Subsystem->GetItemCatalog() : nullptr;
	if (!Row.IsValid() || !IsValid(Catalog)) { return; }
	if (Row->Id.Category == Dnd5e::Category::ItemGroup.GetTag())
	{
		FDnd5eItemGroup Group;
		if (!Catalog->TryGetGroup(Row->Id, Group)) { return; }
		auto* Selection = NewObject<UDnd5eItemGroupBrowserSelection>();
		Selection->Group = MoveTemp(Group);
		SelectedData.Reset(Selection);
	}
	else
	{
		FDnd5eItem Item;
		if (!Catalog->TryGetItem(Row->Id, Item)) { return; }
		auto* Selection = NewObject<UDnd5eItemBrowserSelection>();
		Selection->Item = MoveTemp(Item);
		SelectedData.Reset(Selection);
	}
	DetailsView->SetObject(SelectedData.Get());
}

void SDnd5eItemBrowser::ClearDetails()
{
	DetailsView->SetObject(nullptr);
	SelectedData.Reset();
}

FText SDnd5eItemBrowser::GetSummary() const
{
	if (!DatasetSubsystem.IsValid()) { return LOCTEXT("Unavailable", "The editor item dataset service is unavailable."); }
	if (Model.GetTotalCount() == 0) { return LOCTEXT("Empty", "No items loaded. Use Tools → Load Dnd5e Item Dataset."); }
	return FText::Format(LOCTEXT("Count", "Showing {0} of {1} item and group records."),
		FText::AsNumber(Model.GetRows().Num()), FText::AsNumber(Model.GetTotalCount()));
}

#undef LOCTEXT_NAMESPACE