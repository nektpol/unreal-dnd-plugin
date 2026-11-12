// Developed by Neko Creative Technologies

#include "Browser/Bestiary/SDnd5eBestiaryBrowser.h"
#include "Browser/Dnd5eBrowserText.h"
#include "Editor.h"
#include "Styling/AppStyle.h"
#include "Subsystems/Dnd5eEditorBestiaryDatasetSubsystem.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Layout/SSplitter.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Views/STableRow.h"
#include "Browser/Bestiary/Dnd5eBestiaryStatBlockText.h"

#define LOCTEXT_NAMESPACE "Dnd5eBestiaryBrowser"


void SDnd5eBestiaryBrowser::Construct(const FArguments& InArgs)
{
    ChildSlot
    [
        SNew(SSplitter)
        + SSplitter::Slot().Value(0.35f)
        [
            SNew(SVerticalBox)
            + SVerticalBox::Slot().AutoHeight().Padding(6)
            [
                SNew(STextBlock).Text(this, &SDnd5eBestiaryBrowser::GetSummary)
            ]
            + SVerticalBox::Slot().AutoHeight().Padding(6, 0, 6, 6)
            [
                SNew(SSearchBox)
                .HintText(LOCTEXT("Search", "Search creature name or source"))
                .OnTextChanged(this, &SDnd5eBestiaryBrowser::HandleSearchChanged)
            ]
            + SVerticalBox::Slot().FillHeight(1).Padding(6, 0, 6, 6)
            [
                SAssignNew(ListView, SListView<FRowPtr>)
                .ListItemsSource(&VisibleRows)
                .SelectionMode(ESelectionMode::Single)
                .OnGenerateRow(this, &SDnd5eBestiaryBrowser::GenerateRow)
                .OnSelectionChanged(this, &SDnd5eBestiaryBrowser::HandleSelectionChanged)
            ]
        ]
        + SSplitter::Slot().Value(0.65f)
        [
            SNew(SScrollBox)
            + SScrollBox::Slot().Padding(12)
            [
                SNew(SVerticalBox)
                + SVerticalBox::Slot().AutoHeight()
                [
                    SNew(STextBlock).Text(LOCTEXT("StatBlock", "Stat Block"))
                ]
                + SVerticalBox::Slot().AutoHeight().Padding(0, 8)
                [
                    SNew(SHorizontalBox)
                    + SHorizontalBox::Slot().FillWidth(1)
                    [
                        SNew(STextBlock)
                        .Font(FAppStyle::GetFontStyle("DetailsView.CategoryFontStyle"))
                        .Text_Lambda([this]() { return CardTitle; })
                    ]
                    + SHorizontalBox::Slot().AutoWidth().Padding(8, 0, 0, 0)
                    [
                        SNew(STextBlock).Text_Lambda([this]() { return CardSource; })
                    ]
                ]
                + SVerticalBox::Slot().AutoHeight()
                [
                    SNew(SSeparator)
                ]
                + SVerticalBox::Slot().AutoHeight().Padding(0, 8)
                [
                    SNew(STextBlock).AutoWrapText(true).Text_Lambda([this]() { return CardBody; })
                ]
            ]
        ]
    ];

    Session = GEditor ? GEditor->GetEditorSubsystem<UDnd5eEditorBestiaryDatasetSubsystem>() : nullptr;
    if (auto* DatasetSession = Session.Get())
    {
        CatalogChangedHandle = DatasetSession->OnCatalogChanged().AddSP(this, &SDnd5eBestiaryBrowser::RefreshCatalog);
    }
    RefreshCatalog();
}

SDnd5eBestiaryBrowser::~SDnd5eBestiaryBrowser()
{
    if (auto* DatasetSession = Session.Get())
    {
        DatasetSession->OnCatalogChanged().Remove(CatalogChangedHandle);
    }
}

void SDnd5eBestiaryBrowser::RefreshCatalog()
{
    AllRows.Reset();
    const auto* DatasetSession = Session.Get();
    const auto* Catalog = DatasetSession ? DatasetSession->GetCatalog() : nullptr;
    if (Catalog)
    {
        const auto Ids = Catalog->QueryCreatureIds({});
        AllRows.Reserve(Ids.Num());
        for (const FDnd5eRecordId& Id : Ids)
        {
            FDnd5eCreature Creature;
            if (Catalog->TryGetCreature(Id, Creature))
            {
                auto Row = MakeShared<FRow>();
                Row->Id = Id;
                Row->Name = Creature.Header.DisplayName;
                Row->Source = Dnd5eBrowserText::FormatSource(Creature.Header).ToString();
                AllRows.Add(MoveTemp(Row));
            }
        }
    }
    ApplyFilter();
}

void SDnd5eBestiaryBrowser::ApplyFilter()
{
    ListView->ClearSelection();
    HandleSelectionChanged(nullptr, ESelectInfo::Direct);

    VisibleRows.Reset();
    for (const FRowPtr& Row : AllRows)
    {
        if (Filter.IsEmpty() || Row->Name.Contains(Filter) || Row->Source.Contains(Filter))
        {
            VisibleRows.Add(Row);
        }
    }
    ListView->RequestListRefresh();
}

void SDnd5eBestiaryBrowser::HandleSearchChanged(const FText& Text)
{
    Filter = Text.ToString().TrimStartAndEnd();
    ApplyFilter();
}

void SDnd5eBestiaryBrowser::HandleSelectionChanged(FRowPtr Row, ESelectInfo::Type SelectInfo)
{
    CardTitle = FText::GetEmpty();
    CardSource = FText::GetEmpty();
    CardBody = FText::GetEmpty();

    const auto* DatasetSession = Session.Get();
    const auto* Catalog = DatasetSession ? DatasetSession->GetCatalog() : nullptr;
    if (!Catalog)
    {
        CardBody = LOCTEXT("NoCatalog", "Load a Bestiary dataset from the Tools menu.");
        return;
    }

    FDnd5eCreature Creature;
    if (!Row || !Catalog->TryGetCreature(Row->Id, Creature))
    {
        CardBody = LOCTEXT("SelectCreature", "Select a creature to view its stat block.");
        return;
    }

    ShowCreature(Creature);
}

TSharedRef<ITableRow> SDnd5eBestiaryBrowser::GenerateRow(FRowPtr Row, const TSharedRef<STableViewBase>& OwnerTable)
{
    return SNew(STableRow<FRowPtr>, OwnerTable)
    [
        SNew(STextBlock).Text(FText::Format(LOCTEXT("Row", "{0}  |  {1}"),
            FText::FromString(Row->Name), FText::FromString(Row->Source)))
    ];
}

FText SDnd5eBestiaryBrowser::GetSummary() const
{
    return FText::Format(LOCTEXT("Summary", "Showing {0} of {1} creatures"),
        FText::AsNumber(VisibleRows.Num()), FText::AsNumber(AllRows.Num()));
}

void SDnd5eBestiaryBrowser::ShowCreature(const FDnd5eCreature& Creature)
{
    CardTitle = FText::FromString(Creature.Header.DisplayName);
    CardSource = Dnd5eBrowserText::FormatSource(Creature.Header);
    CardBody = Dnd5eBestiaryStatBlockText::Format(Creature);
}

#undef LOCTEXT_NAMESPACE