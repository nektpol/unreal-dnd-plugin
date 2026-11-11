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
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SExpandableArea.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Dnd5eEditor.h"
#include "Subsystems/Dnd5eEditorDatasetSubsystem.h"
#include "Widgets/Input/SButton.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Layout/SSeparator.h"

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
						SAssignNew(SearchBox, SSearchBox).HintText(LOCTEXT("Search", "Search item and group names"))
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
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().FillHeight(1).Padding(8, 0, 0, 8)
					[
						SAssignNew(OverviewScroll, SScrollBox)
						+ SScrollBox::Slot().Padding(0, 0, 0, 8)
						+ SScrollBox::Slot().Padding(0, 0, 0, 4)
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot().FillWidth(1).Padding(0, 0, 12, 0)
							[
								SNew(STextBlock)
								.Text(this, &SDnd5eItemBrowser::GetDetailTitle)
								.Font(FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), 18))
								.AutoWrapText(true)
							]
							+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
							[
								SNew(STextBlock)
								.Text(this, &SDnd5eItemBrowser::GetDetailSource)
								.Visibility_Lambda([this]() { return Overview.Source.IsEmpty() ? EVisibility::Collapsed : EVisibility::Visible; })
							]
						]
						+ SScrollBox::Slot().Padding(0, 0, 0, 8)
						[
							SNew(STextBlock)
							.Text(this, &SDnd5eItemBrowser::GetDetailClassification)
							.AutoWrapText(true)
							.Visibility_Lambda([this]() { return Overview.Classification.IsEmpty() ? EVisibility::Collapsed : EVisibility::Visible; })
						]
						+ SScrollBox::Slot().Padding(0, 0, 0, 8)
						[
							SNew(SHorizontalBox)
							.Visibility_Lambda([this]()
							{
								return Overview.Facts.IsEmpty() && Overview.Equipment.IsEmpty() ? EVisibility::Collapsed : EVisibility::Visible;
							})
							+ SHorizontalBox::Slot().FillWidth(1).Padding(0, 0, 12, 0)
							[
								SNew(STextBlock)
								.Text(this, &SDnd5eItemBrowser::GetDetailFacts)
								.AutoWrapText(true)
							]
							+ SHorizontalBox::Slot().FillWidth(1)
							[
								SNew(STextBlock)
								.Text(this, &SDnd5eItemBrowser::GetDetailEquipment)
								.Justification(ETextJustify::Right)
								.AutoWrapText(true)
							]
						]
						+ SScrollBox::Slot().Padding(0, 0, 0, 8)
						[
							SNew(SSeparator)
							.Visibility_Lambda([this]() { return Overview.Title.IsEmpty() ? EVisibility::Collapsed : EVisibility::Visible; })
						]
						+ SScrollBox::Slot()
						[
							SNew(STextBlock).Text(this, &SDnd5eItemBrowser::GetDetailDescription).AutoWrapText(true)
							.Visibility_Lambda([this]() { return Overview.Description.IsEmpty() ? EVisibility::Collapsed : EVisibility::Visible; })
						]
						+ SScrollBox::Slot().Padding(0, 12, 0, 0)
						[
							SAssignNew(ReferenceBox, SVerticalBox)
						]
						+ SScrollBox::Slot().Padding(0, 8, 0, 0)
						[
							SNew(STextBlock)
							.Text_Lambda([this]() { return ReferencePreview; })
							.AutoWrapText(true)
							.Visibility_Lambda([this]() { return ReferencePreview.IsEmpty() ? EVisibility::Collapsed : EVisibility::Visible; })
						]
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(8, 0, 0, 0)
					[
						SNew(SExpandableArea).InitiallyCollapsed(true)
						.HeaderContent()
						[
							SNew(STextBlock).Text(LOCTEXT("TypedDetails", "Typed details"))
						]
						.BodyContent()
						[
							SNew(SBox).MaxDesiredHeight(280)
							[
								DetailsView.ToSharedRef()
							]
						]
					]
				]
			]
		]
	];

	DatasetSubsystem = GEditor ? GEditor->GetEditorSubsystem<UDnd5eEditorItemDatasetSubsystem>() : nullptr;
	if (auto* Subsystem = DatasetSubsystem.Get())
	{
		CatalogChangedHandle = Subsystem->OnCatalogChanged().AddSP(this, &SDnd5eItemBrowser::RefreshCatalog);
	}
	SpellDatasetSubsystem = GEditor ? GEditor->GetEditorSubsystem<UDnd5eEditorDatasetSubsystem>() : nullptr;
	if (auto* Spells = SpellDatasetSubsystem.Get())
	{
		SpellCatalogChangedHandle = Spells->OnCatalogChanged().AddSP(this, &SDnd5eItemBrowser::RebuildReferences);
	}
	RefreshCatalog();
}

SDnd5eItemBrowser::~SDnd5eItemBrowser()
{
	if (auto* Subsystem = DatasetSubsystem.Get()) { Subsystem->OnCatalogChanged().Remove(CatalogChangedHandle); }
	if (auto* Spells = SpellDatasetSubsystem.Get()) { Spells->OnCatalogChanged().Remove(SpellCatalogChangedHandle); }
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
	const FText Rarity = Dnd5eItemBrowserDetails::FormatRarity(Row->Rarity);
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
	const auto* Definitions = Subsystem ? Subsystem->GetDefinitionCatalog() : nullptr;
	if (Row->Id.Category == Dnd5e::Category::ItemGroup.GetTag())
	{
		FDnd5eItemGroup Group;
		if (!Catalog->TryGetGroup(Row->Id, Group)) { return; }
		auto* Selection = NewObject<UDnd5eItemGroupBrowserSelection>();
		Selection->Group = MoveTemp(Group);
		Overview = Dnd5eItemBrowserDetails::Build(Selection->Group.Details, Definitions);
		SelectedData.Reset(Selection);
	}
	else
	{
		FDnd5eItem Item;
		if (!Catalog->TryGetItem(Row->Id, Item)) { return; }
		auto* Selection = NewObject<UDnd5eItemBrowserSelection>();
		Selection->Item = MoveTemp(Item);
		Overview = Dnd5eItemBrowserDetails::Build(Selection->Item, Definitions);
		SelectedData.Reset(Selection);
	}
	DetailsView->SetObject(SelectedData.Get());
	RebuildReferences();
}

void SDnd5eItemBrowser::ClearDetails()
{
	DetailsView->SetObject(nullptr);
	SelectedData.Reset();
	ReferencePreview = FText::GetEmpty();
	if (ReferenceBox.IsValid()) { ReferenceBox->ClearChildren(); }
	Overview = {};
	if (OverviewScroll.IsValid()) { OverviewScroll->ScrollToStart(); }
}

FText SDnd5eItemBrowser::GetSummary() const
{
	if (!DatasetSubsystem.IsValid()) { return LOCTEXT("Unavailable", "The editor item dataset service is unavailable."); }
	if (Model.GetTotalCount() == 0) { return LOCTEXT("Empty", "No items loaded. Use Tools → Load Dnd5e Item Dataset."); }
	return FText::Format(LOCTEXT("Count", "Showing {0} of {1} item and group records."),
		FText::AsNumber(Model.GetRows().Num()), FText::AsNumber(Model.GetTotalCount()));
}

FText SDnd5eItemBrowser::GetDetailTitle() const
{
	return Overview.Title.IsEmpty() ? LOCTEXT("SelectItem", "Select an item or group.") : Overview.Title;
}

void SDnd5eItemBrowser::RebuildReferences()
{
	if (!ReferenceBox.IsValid()) { return; }
	ReferenceBox->ClearChildren();
	ReferencePreview = FText::GetEmpty();
	const auto* Group = Cast<UDnd5eItemGroupBrowserSelection>(SelectedData.Get());
	const auto* Item = Cast<UDnd5eItemBrowserSelection>(SelectedData.Get());
	const FDnd5eItem* Record = Group ? &Group->Group.Details : (Item ? &Item->Item : nullptr);
	if (!Record) { return; }

	const auto* ItemSession = DatasetSubsystem.Get();
	const auto* SpellSession = SpellDatasetSubsystem.Get();
	const auto Links = Dnd5eItemBrowserLinks::Build(*Record, Group ? &Group->Group : nullptr,
		ItemSession ? ItemSession->GetItemCatalog() : nullptr,
		ItemSession ? ItemSession->GetDefinitionCatalog() : nullptr,
		SpellSession ? SpellSession->GetCatalog() : nullptr);
	for (const auto& Link : Links)
	{
		const FText Caption = FText::Format(LOCTEXT("LinkCaption", "{0} — {1}"), Link.Label, Link.Status);
		ReferenceBox->AddSlot().AutoHeight().Padding(0, 0, 0, 4)
		[
			SNew(SButton)
			.IsEnabled(Link.Target.IsSet())
			.OnClicked(this, &SDnd5eItemBrowser::FollowReference, Link)
			[
				SNew(STextBlock).Text(Caption).AutoWrapText(true)
			]
		];
	}
}

FReply SDnd5eItemBrowser::FollowReference(FDnd5eItemBrowserLink Link)
{
	if (!Link.Target.IsSet()) { return FReply::Handled(); }
	const FDnd5eRecordId Id = Link.Target.GetValue();
	switch (Link.Kind)
	{
	case EDnd5eItemBrowserLinkKind::Member:
		if (SelectItemById(Id)) { return FReply::Handled(); }
		break;
	case EDnd5eItemBrowserLinkKind::Spell:
		if (FModuleManager::GetModuleChecked<FDnd5eEditorModule>("Dnd5eEditor").OpenSpellById(Id))
		{
			return FReply::Handled();
		}
		break;
	case EDnd5eItemBrowserLinkKind::Definition:
		if (const auto* Session = DatasetSubsystem.Get())
		{
			if (const auto* Catalog = Session->GetDefinitionCatalog())
			{
				FText Title, Description;
				if (Dnd5eItemBrowserLinks::ReadDefinition(*Catalog, Id, Title, Description))
				{
					ReferencePreview = FText::Format(LOCTEXT("DefinitionPreview", "{0}\n{1}"), Title, Description);
					return FReply::Handled();
				}
			}
		}
		break;
	}
	ReferencePreview = LOCTEXT("ReferenceUnavailable", "The target is no longer available in the active catalog.");
	return FReply::Handled();
}

bool SDnd5eItemBrowser::SelectItemById(const FDnd5eRecordId& Id)
{
	const auto* Session = DatasetSubsystem.Get();
	const auto* Catalog = Session ? Session->GetItemCatalog() : nullptr;
	FDnd5eItem Record;
	if (!Catalog || !Catalog->TryGetItem(Id, Record)) { return false; }
	auto FindRow = [&]() { return Model.GetRows().FindByPredicate([&Id](const auto& Row) { return Row->Id == Id; }); };
	const auto* Found = FindRow();
	if (!Found)
	{
		SearchBox->SetText(FText::GetEmpty());
		Found = FindRow();
	}
	if (!Found) { return false; }
	const FDnd5eItemBrowserRowPtr Row = *Found;
	ListView->SetSelection(Row);
	ListView->RequestScrollIntoView(Row);
	return true;
}

#undef LOCTEXT_NAMESPACE
