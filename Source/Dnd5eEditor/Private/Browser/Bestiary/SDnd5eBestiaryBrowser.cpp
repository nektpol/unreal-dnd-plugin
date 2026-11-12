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

#define LOCTEXT_NAMESPACE "Dnd5eBestiaryBrowser"

namespace
{
    void AppendStat(FString& Out, const TCHAR* Label, const FString& Value)
    {
        if (!Value.IsEmpty())
        {
            Dnd5eBrowserText::AppendPreview(Out, FString::Printf(TEXT("%s  %s\n"), Label, *Value));
        }
    }

    void AppendEntryGroup(FString& Out, const TCHAR* Label, const TArray<FDnd5eCreatureEntry>& Entries,
        const TArray<FDnd5eContentBlock>* Header = nullptr)
    {
        if (Entries.IsEmpty() && (!Header || Header->IsEmpty())) { return; }

        Dnd5eBrowserText::AppendPreview(Out, FString::Printf(TEXT("\n%s\n"), Label));
        if (Header && !Header->IsEmpty())
        {
            Dnd5eBrowserText::AppendPreview(Out, Dnd5eBrowserText::FormatDescription(*Header).ToString() + TEXT("\n"));
        }
        for (const FDnd5eCreatureEntry& Entry : Entries)
        {
            if (!Entry.Name.IsEmpty()) { Dnd5eBrowserText::AppendPreview(Out, Entry.Name + TEXT("\n")); }
            if (!Entry.Entries.IsEmpty())
            {
                Dnd5eBrowserText::AppendPreview(Out,
                    Dnd5eBrowserText::FormatDescription(Entry.Entries).ToString() + TEXT("\n"));
            }
        }
    }
}

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
    FString Body;

    TArray<FString> ArmorClasses;
    for (const FDnd5eCreatureArmorClass& Armor : Creature.ArmorClasses)
    {
        if (!Armor.bHasValue) { continue; }
        FString Value = FString::FromInt(Armor.Value);
        if (!Armor.Sources.IsEmpty())
        {
            Value += TEXT(" (") + Dnd5eBrowserText::FormatDescription(Armor.Sources).ToString() + TEXT(")");
        }
        ArmorClasses.Add(MoveTemp(Value));
    }
    AppendStat(Body, TEXT("Armor Class"), FString::Join(ArmorClasses, TEXT(", ")));

    FString HitPoints = Creature.HitPoints.bHasAverage ? FString::FromInt(Creature.HitPoints.Average) : FString();
    const FString& Formula = Creature.HitPoints.Formula.Expression;
    if (!Formula.IsEmpty())
    {
        HitPoints = HitPoints.IsEmpty() ? Formula : HitPoints + TEXT(" (") + Formula + TEXT(")");
    }
    AppendStat(Body, TEXT("Hit Points"), HitPoints);

    TArray<FString> Speeds;
    for (const FDnd5eCreatureSpeed& Speed : Creature.Movement.Speeds)
    {
        const FString Distance = Dnd5eBrowserText::FormatDistance(Speed.Distance);
        if (Distance.IsEmpty()) { continue; }
        const FString Mode = Speed.Mode == EDnd5eCreatureMovementMode::Walk
            ? FString() : StaticEnum<EDnd5eCreatureMovementMode>()->GetDisplayNameTextByValue(static_cast<int64>(Speed.Mode)).ToString() + TEXT(" ");
        Speeds.Add(Mode + Distance);
    }
    AppendStat(Body, TEXT("Speed"), FString::Join(Speeds, TEXT(", ")));

    const auto& Scores = Creature.AbilityScores;
    if (Scores.bHasScores)
    {
        Dnd5eBrowserText::AppendPreview(Body, FString::Printf(TEXT("\nSTR %d    DEX %d    CON %d    INT %d    WIS %d    CHA %d\n\n"),
            Scores.Strength, Scores.Dexterity, Scores.Constitution, Scores.Intelligence, Scores.Wisdom, Scores.Charisma));
    }

    TArray<FString> Senses = Creature.Senses;
    if (Creature.PassivePerceptionPresence == EDnd5eValuePresence::Present)
    {
        const FString Passive = Creature.PassivePerceptionFormula.IsEmpty()
            ? FString::FromInt(Creature.PassivePerception) : Creature.PassivePerceptionFormula;
        Senses.Add(TEXT("passive Perception ") + Passive);
    }
    AppendStat(Body, TEXT("Senses"), FString::Join(Senses, TEXT(", ")));
    AppendStat(Body, TEXT("Languages"), FString::Join(Creature.Languages, TEXT(", ")));
    AppendStat(Body, TEXT("Challenge"), Creature.ChallengeRating.Base);

    AppendEntryGroup(Body, TEXT("Traits"), Creature.Traits);
    AppendEntryGroup(Body, TEXT("Actions"), Creature.Actions);
    AppendEntryGroup(Body, TEXT("Bonus Actions"), Creature.BonusActions);
    AppendEntryGroup(Body, TEXT("Reactions"), Creature.Reactions.Entries, &Creature.Reactions.Header);
    AppendEntryGroup(Body, TEXT("Legendary Actions"), Creature.LegendaryActions.Entries, &Creature.LegendaryActions.Header);
    AppendEntryGroup(Body, TEXT("Mythic Actions"), Creature.MythicActions.Entries, &Creature.MythicActions.Header);
    CardBody = Dnd5eBrowserText::FinishPreview(MoveTemp(Body));
}

#undef LOCTEXT_NAMESPACE