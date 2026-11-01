// Developed by Neko Creative Technologies

#include "Browser/SDnd5eSpellBrowser.h"
#include "Browser/Dnd5eSpellBrowserSelection.h"
#include "Catalog/Dnd5eSpellCatalogObject.h"
#include "DetailsViewArgs.h"
#include "Editor.h"
#include "IDetailsView.h"
#include "Modules/ModuleManager.h"
#include "PropertyEditorModule.h"
#include "Spells/Dnd5eSpell.h"
#include "Subsystems/Dnd5eEditorDatasetSubsystem.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SSplitter.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Views/STableRow.h"
#include "Readers/Spells/Dnd5eJsonSpellReferenceReporter.h"

#define LOCTEXT_NAMESPACE "Dnd5eSpellBrowser"

namespace
{
	FText MakeReferenceSummary(const FDnd5eJsonSpellReferenceReport& Report)
	{
		if (!Report.bCollected)
		{
			return LOCTEXT("ReferenceCollectionFailed", "Reference collection failed. Expand Reference Report → Diagnostics for details.");
		}
		int32 OriginalResolved = 0;
		int32 Normalized = 0;
		int32 NormalizedResolved = 0;
		for (const auto& Entry : Report.Entries)
		{
			OriginalResolved += Entry.OriginalResolutionStatus == EDnd5eReferenceResolveStatus::Resolved;
			Normalized += Entry.NormalizationStatus == EDnd5eJsonSpellReferenceNormalizeStatus::Normalized;
			NormalizedResolved += Entry.NormalizedResolutionStatus == EDnd5eReferenceResolveStatus::Resolved;
		}
		return FText::Format(
			LOCTEXT("ReferenceReportSummary", "Typed references: {0} | Original targets found: {1} | Selectors normalized: {2} | Normalized targets found: {3} | Collection diagnostics: {4}. Expand Reference Report to inspect each stage."),
			FText::AsNumber(Report.Entries.Num()), FText::AsNumber(OriginalResolved), FText::AsNumber(Normalized),
			FText::AsNumber(NormalizedResolved), FText::AsNumber(Report.Diagnostics.Num()));
	}
}

void SDnd5eSpellBrowser::Construct(const FArguments& InArgs)
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
				SNew(STextBlock).Text(this, &SDnd5eSpellBrowser::GetSummary)
			]
			+ SVerticalBox::Slot().FillHeight(1)
			[
				SNew(SSplitter)
				+ SSplitter::Slot().Value(0.35f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 8, 8)
					[
						SNew(SSearchBox)
						.HintText(LOCTEXT("SearchHint", "Search spell names"))
						.OnTextChanged(this, &SDnd5eSpellBrowser::HandleSearchChanged)
					]
					+ SVerticalBox::Slot().FillHeight(1).Padding(0, 0, 8, 0)
					[
						SAssignNew(ListView, SListView<FDnd5eSpellBrowserRowPtr>)
						.ListItemsSource(&Model.GetRows())
						.SelectionMode(ESelectionMode::Single)
						.OnGenerateRow(this, &SDnd5eSpellBrowser::GenerateRow)
						.OnSelectionChanged(this, &SDnd5eSpellBrowser::HandleSelectionChanged)
					]
				]
				+ SSplitter::Slot().Value(0.65f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight().Padding(8, 0, 0, 8)
					[
						SNew(STextBlock).Text(LOCTEXT("DetailsHint", "Read-only spell details — select a row to inspect."))
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(8, 0, 0, 8)
					[
						SNew(STextBlock)
						.Text(this, &SDnd5eSpellBrowser::GetReferenceSummary)
						.AutoWrapText(true)
					]
					+ SVerticalBox::Slot().FillHeight(1)
					[
						DetailsView.ToSharedRef()
					]
				]
			]
		]
	];

	DatasetSubsystem = GEditor ? GEditor->GetEditorSubsystem<UDnd5eEditorDatasetSubsystem>() : nullptr;
	if (auto* Subsystem = DatasetSubsystem.Get())
	{
		CatalogChangedHandle = Subsystem->OnCatalogChanged().AddSP(this, &SDnd5eSpellBrowser::RefreshCatalog);
	}
	RefreshCatalog();
}

SDnd5eSpellBrowser::~SDnd5eSpellBrowser()
{
	if (auto* Subsystem = DatasetSubsystem.Get()) { Subsystem->OnCatalogChanged().Remove(CatalogChangedHandle); }
}

void SDnd5eSpellBrowser::RefreshCatalog()
{
	ListView->ClearSelection();
	ClearDetails();
	const auto* Subsystem = DatasetSubsystem.Get();
	Model.Rebuild(Subsystem ? Subsystem->GetCatalog() : nullptr);
	ListView->RequestListRefresh();
}

void SDnd5eSpellBrowser::HandleSearchChanged(const FText& Text)
{
	ListView->ClearSelection();
	ClearDetails();
	Model.SetSearchText(Text.ToString());
	ListView->RequestListRefresh();
}

TSharedRef<ITableRow> SDnd5eSpellBrowser::GenerateRow(FDnd5eSpellBrowserRowPtr Item, const TSharedRef<STableViewBase>& OwnerTable)
{
	const FText Source = Item->SourceId.IsEmpty() ? LOCTEXT("NoSource", "Source unspecified") : FText::FromString(Item->SourceId);
	const FText Label = FText::Format(LOCTEXT("Row", "{0} | Level {1} | {2}"), FText::FromString(Item->Name), FText::AsNumber(Item->Level), Source);
	return SNew(STableRow<FDnd5eSpellBrowserRowPtr>, OwnerTable).Padding(4)
	[
		SNew(STextBlock).Text(Label).ToolTipText(Label)
	];
}

void SDnd5eSpellBrowser::HandleSelectionChanged(FDnd5eSpellBrowserRowPtr Item, ESelectInfo::Type SelectInfo)
{
	ClearDetails();
	const auto* Subsystem = DatasetSubsystem.Get();
	auto* Catalog = Subsystem ? Subsystem->GetCatalog() : nullptr;
	if (!Item.IsValid() || !IsValid(Catalog)) { return; }
	FDnd5eSpell Spell;
	if (!Catalog->TryGetSpell(Item->Id, Spell)) { return; }
	SelectedData.Reset(NewObject<UDnd5eSpellBrowserSelection>());
	SelectedData->Spell = MoveTemp(Spell);
	SelectedData->ReferenceReport = FDnd5eJsonSpellReferenceReporter::BuildReport(Catalog->GetMutableCatalog(), SelectedData->Spell);
	ReferenceSummary = MakeReferenceSummary(SelectedData->ReferenceReport);
	DetailsView->SetObject(SelectedData.Get());
}

void SDnd5eSpellBrowser::ClearDetails()
{
	DetailsView->SetObject(nullptr);
	SelectedData.Reset();
	ReferenceSummary = LOCTEXT("NoReferenceSelection", "Select a spell to inspect its reference report.");
}

FText SDnd5eSpellBrowser::GetSummary() const
{
	if (!DatasetSubsystem.IsValid()) { return LOCTEXT("Unavailable", "The editor dataset service is unavailable."); }
	if (Model.GetTotalCount() == 0) { return LOCTEXT("Empty", "No spells loaded. Use Tools → Load Dnd5e Dataset to load your selection."); }
	return FText::Format(LOCTEXT("Count", "Showing {0} of {1} spell records."), FText::AsNumber(Model.GetRows().Num()), FText::AsNumber(Model.GetTotalCount()));
}

FText SDnd5eSpellBrowser::GetReferenceSummary() const
{
	return ReferenceSummary;
}

#undef LOCTEXT_NAMESPACE
