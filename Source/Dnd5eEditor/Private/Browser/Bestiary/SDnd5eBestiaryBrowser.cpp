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
#include "Widgets/Layout/SUniformGridPanel.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SWidgetSwitcher.h"
#include "Content/Dnd5eContentLibrary.h"
#include "Engine/Texture2D.h"
#include "HAL/FileManager.h"
#include "ImageUtils.h"
#include "Misc/Paths.h"
#include "Settings/Dnd5eEditorBestiaryDatasetSettings.h"
#include "UObject/StrongObjectPtr.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SHyperlink.h"
#include "Dnd5eTags.h"

#define LOCTEXT_NAMESPACE "Dnd5eBestiaryBrowser"

namespace
{
    struct FBestiaryArtwork
    {
        TStrongObjectPtr<UTexture2D> Texture;
        FSlateBrush Brush;
    };
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
                SAssignNew(SearchBox, SSearchBox)
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
            SNew(SVerticalBox)
            + SVerticalBox::Slot().AutoHeight().Padding(12, 8)
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
            + SVerticalBox::Slot().AutoHeight().Padding(12, 0, 12, 6)
            [
                SNew(SHorizontalBox)
                + SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 4, 0)
                [
                    SNew(SButton)
                    .Text(LOCTEXT("StatBlock", "Stat Block"))
                    .OnClicked(this, &SDnd5eBestiaryBrowser::ShowDetailsPage, 0)
                ]
                + SHorizontalBox::Slot().AutoWidth()
                [
                    SNew(SButton)
                    .Text(LOCTEXT("Info", "Info"))
                    .OnClicked(this, &SDnd5eBestiaryBrowser::ShowDetailsPage, 1)
                ]
                + SHorizontalBox::Slot().AutoWidth().Padding(4, 0, 0, 0)
                [
                    SNew(SButton)
                    .Text(LOCTEXT("Images", "Images"))
                    .OnClicked(this, &SDnd5eBestiaryBrowser::ShowDetailsPage, 2)
                ]
            ]
            + SVerticalBox::Slot().AutoHeight()
            [
                SNew(SSeparator)
            ]
            + SVerticalBox::Slot().FillHeight(1)
            [
                SAssignNew(DetailsPages, SWidgetSwitcher)
                .WidgetIndex(0)
                + SWidgetSwitcher::Slot()
                [
                    SAssignNew(StatBlockScroll, SScrollBox)
                    + SScrollBox::Slot().Padding(12)
                    [
                        SAssignNew(StatBlockBody, SVerticalBox)
                    ]
                ]
                + SWidgetSwitcher::Slot()
                [
                    SAssignNew(InfoScroll, SScrollBox)
                    + SScrollBox::Slot().Padding(12)
                    [
                        SNew(STextBlock)
                        .AutoWrapText(true)
                        .Text_Lambda([this]() { return InfoText; })
                    ]
                ]
                + SWidgetSwitcher::Slot()
                [
                    SAssignNew(ImageScroll, SScrollBox)
                    + SScrollBox::Slot().Padding(12)
                    [
                        SAssignNew(ImageBody, SVerticalBox)
                    ]
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

void SDnd5eBestiaryBrowser::HandleSelectionChanged(FRowPtr Row, ESelectInfo::Type SelectInfo)
{
    CardTitle = FText::GetEmpty();
    CardSource = FText::GetEmpty();

    const auto* DatasetSession = Session.Get();
    const auto* Catalog = DatasetSession ? DatasetSession->GetCatalog() : nullptr;
    if (!Catalog)
    {
        const FText Message = LOCTEXT("NoCatalog", "Load a Bestiary dataset from the Tools menu.");
        ShowStatBlockMessage(Message);
        SetInfoText(Message);
        SetImagesMessage(Message);
        return;
    }

    FDnd5eCreature Creature;
    if (!Row || !Catalog->TryGetCreature(Row->Id, Creature))
    {
        const FText Message = LOCTEXT("SelectCreature", "Select a creature to view its stat block.");
        ShowStatBlockMessage(Message);
        SetInfoText(Message);
        SetImagesMessage(Message);
        return;
    }

    ShowCreature(Creature);
}

void SDnd5eBestiaryBrowser::ShowCreature(const FDnd5eCreature& Creature)
{
    SetImagesMessage(LOCTEXT("NoArtwork", "No local artwork is available for this creature."));
    SelectedImages = Creature.SupplementaryContent.Images;
    if (Creature.Token.Kind != EDnd5eMediaReferenceKind::Unspecified)
    {
        FDnd5eImageContent Image;
        Image.Resource = Creature.Token;
        Image.AltText = TEXT("Creature token");

        FDnd5eContentBlock Block;
        Block.Kind = Dnd5e::Content::Block::Image;
        Block.Payload = FInstancedStruct::Make<FDnd5eImageContent>(Image);
        SelectedImages.Insert(MoveTemp(Block), 0);
    }
    if (DetailsPages->GetActiveWidgetIndex() == 2) { RebuildImages(); }
    CardTitle = FText::FromString(Creature.Header.DisplayName);
    CardSource = Dnd5eBrowserText::FormatSource(Creature.Header);
    RebuildStatBlock(Dnd5eBestiaryStatBlockText::Build(Creature), Creature.Relationships);

    const auto& Entries = Creature.SupplementaryContent.Entries;
    SetInfoText(Entries.IsEmpty()
        ? LOCTEXT("NoLore", "No lore is available for this creature in the loaded dataset.")
        : Dnd5eBrowserText::FormatDescription(Entries));
}

FReply SDnd5eBestiaryBrowser::ShowDetailsPage(int32 PageIndex)
{
    if (PageIndex == 2 && !bImagesBuilt && !SelectedImages.IsEmpty()) { RebuildImages(); }
    DetailsPages->SetActiveWidgetIndex(PageIndex);
    return FReply::Handled();
}

void SDnd5eBestiaryBrowser::SetImagesMessage(const FText& Message)
{
    SelectedImages.Reset();
    bImagesBuilt = false;
    ImageBody->ClearChildren();
    ImageScroll->ScrollToStart();
    ImageBody->AddSlot().AutoHeight()
    [
        SNew(STextBlock).AutoWrapText(true).Text(Message)
    ];
}

void SDnd5eBestiaryBrowser::RebuildImages()
{
    ImageBody->ClearChildren();
    ImageScroll->ScrollToStart();
    bImagesBuilt = true;

    const FString Root = GetDefault<UDnd5eEditorBestiaryDatasetSettings>()->ArtworkRoot.Path;
    int32 Displayed = 0;
    if (!Root.IsEmpty() && !FPaths::IsRelative(Root))
    {
        for (const FDnd5eContentBlock& Block : SelectedImages)
        {
            if (Displayed == 8) { break; }
            FDnd5eImageContent Image;
            if (!UDnd5eContentLibrary::TryGetImage(Block, Image) ||
                Image.Resource.Kind != EDnd5eMediaReferenceKind::DatasetRelative ||
                !Dnd5eMedia::IsDatasetRelativePath(Image.Resource.RelativePath))
            {
                continue;
            }

            FString Path = FPaths::Combine(Root, Image.Resource.RelativePath);
            FString Extension = FPaths::GetExtension(Path).ToLower();
            if (Extension == TEXT("webp"))
            {
                Path = FPaths::ChangeExtension(Path, TEXT("png"));
                Extension = TEXT("png");
            }
            if (Extension != TEXT("png") && Extension != TEXT("jpg") && Extension != TEXT("jpeg")) { continue; }

            const int64 Bytes = IFileManager::Get().FileSize(*Path);
            if (Bytes <= 0 || Bytes > 16 * 1024 * 1024) { continue; }

            auto Artwork = MakeShared<FBestiaryArtwork>();
            Artwork->Texture.Reset(FImageUtils::ImportFileAsTexture2D(Path));
            if (!Artwork->Texture.IsValid()) { continue; }

            Artwork->Brush.SetResourceObject(Artwork->Texture.Get());
            Artwork->Brush.DrawAs = ESlateBrushDrawType::Image;
            const float Width = static_cast<float>(Artwork->Texture->GetSizeX());
            const float Height = static_cast<float>(Artwork->Texture->GetSizeY());
            const float Scale = FMath::Min(1.0f, 640.0f / FMath::Max(Width, Height));
            Artwork->Brush.ImageSize = FVector2D(Width * Scale, Height * Scale);
            ImageBody->AddSlot().AutoHeight().HAlign(HAlign_Left).Padding(0, 0, 0, 8)
            [
                SNew(SImage).Image_Lambda([Artwork]() { return &Artwork->Brush; })
            ];
            ++Displayed;
        }
    }

    if (Displayed == 0)
    {
        ImageBody->AddSlot().AutoHeight()
        [
            SNew(STextBlock).AutoWrapText(true)
            .Text(LOCTEXT("NoArtwork", "No local artwork is available for this creature."))
        ];
    }
}

void SDnd5eBestiaryBrowser::SetInfoText(const FText& Text)
{
    InfoText = Text;
    InfoScroll->ScrollToStart();
}

void SDnd5eBestiaryBrowser::ShowStatBlockMessage(const FText& Message)
{
    StatBlockBody->ClearChildren();
    StatBlockBody->AddSlot().AutoHeight()
    [
        SNew(STextBlock).Text(Message).AutoWrapText(true)
    ];
    StatBlockScroll->ScrollToStart();
}

void SDnd5eBestiaryBrowser::RebuildStatBlock(const Dnd5eBestiaryStatBlockText::FStatBlock& Card,
    const TArray<FDnd5eRecordRelationship>& Relationships)
{
    StatBlockBody->ClearChildren();
    StatBlockScroll->ScrollToStart();

    const auto AddText = [this](const FText& Text, const FMargin& Padding)
    {
        if (Text.IsEmpty()) { return; }
        StatBlockBody->AddSlot().AutoHeight().Padding(Padding)
        [
            SNew(STextBlock).Text(Text).AutoWrapText(true)
        ];
    };

    AddText(Card.Classification, FMargin(0, 0, 0, 8));
    AddText(Card.Vitals, FMargin(0, 0, 0, 8));

    if (!Card.Abilities.IsEmpty())
    {
        TSharedRef<SUniformGridPanel> Grid = SNew(SUniformGridPanel).SlotPadding(FMargin(4));
        for (int32 Index = 0; Index < Card.Abilities.Num(); ++Index)
        {
            const auto& Ability = Card.Abilities[Index];
            Grid->AddSlot(Index % 3, Index / 3)
            [
                SNew(SVerticalBox)
                + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
                [
                    SNew(STextBlock)
                    .Font(FAppStyle::GetFontStyle("DetailsView.CategoryFontStyle"))
                    .Text(Ability.Label)
                ]
                + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
                [
                    SNew(STextBlock).Text(Ability.Value)
                ]
            ];
        }
        StatBlockBody->AddSlot().AutoHeight().Padding(0, 0, 0, 8)
        [
            Grid
        ];
    }

    AddText(Card.Facts, FMargin(0, 0, 0, 8));
    for (const auto& Section : Card.Sections)
    {
        StatBlockBody->AddSlot().AutoHeight().Padding(0, 4, 0, 4)
        [
            SNew(SSeparator)
        ];
        StatBlockBody->AddSlot().AutoHeight().Padding(0, 0, 0, 4)
        [
            SNew(STextBlock)
            .Font(FAppStyle::GetFontStyle("DetailsView.CategoryFontStyle"))
            .Text(Section.Heading)
        ];
        AddText(Section.Body, FMargin(0, 0, 0, 8));
    }

    AddReprintLinks(Relationships);

    if (!Card.Environment.IsEmpty() || !Card.References.IsEmpty())
    {
        StatBlockBody->AddSlot().AutoHeight().Padding(0, 4, 0, 4)
        [
            SNew(SSeparator)
        ];
        AddText(Card.Environment, FMargin(0, 0, 0, 4));
        AddText(Card.References, FMargin(0, 0, 0, 8));
    }

    if (StatBlockBody->GetChildren()->Num() == 0)
    {
        ShowStatBlockMessage(LOCTEXT("NoStats", "No stat block values supplied."));
    }
}

void SDnd5eBestiaryBrowser::AddReprintLinks(const TArray<FDnd5eRecordRelationship>& Relationships)
{
    const auto* DatasetSession = Session.Get();
    const auto* Catalog = DatasetSession ? DatasetSession->GetCatalog() : nullptr;
    bool bAddedHeading = false;

    for (const FDnd5eRecordRelationship& Relationship : Relationships)
    {
        if (Relationship.Kind != Dnd5e::Record::Relationship::ReprintedAs) { continue; }

        if (!bAddedHeading)
        {
            StatBlockBody->AddSlot().AutoHeight().Padding(0, 4)
            [
                SNew(SSeparator)
            ];
            StatBlockBody->AddSlot().AutoHeight().Padding(0, 0, 0, 4)
            [
                SNew(STextBlock)
                .Font(FAppStyle::GetFontStyle("DetailsView.CategoryFontStyle"))
                .Text(LOCTEXT("ReprintedAs", "Reprinted as"))
            ];
            bAddedHeading = true;
        }

        FDnd5eCreature Target;
        if (Catalog && Catalog->ResolveCreatureRelationship(Relationship, Target) == EDnd5eReferenceResolveStatus::Resolved)
        {
            const FText Label = FText::Format(LOCTEXT("ReprintTarget", "{0} ({1})"),
                FText::FromString(Target.Header.DisplayName), Dnd5eBrowserText::FormatSource(Target.Header));
            StatBlockBody->AddSlot().AutoHeight().HAlign(HAlign_Left).Padding(0, 0, 0, 4)
            [
                SNew(SHyperlink)
                .Text(Label)
                .OnNavigate(this, &SDnd5eBestiaryBrowser::NavigateToCreature, Target.Header.RecordId)
            ];
        }
        else
        {
            const FText Label = FText::Format(
                Relationship.bHasTargetRecord ? LOCTEXT("UnavailableReprint", "{0} (not loaded)") :
                    LOCTEXT("UnresolvedReprint", "{0} (unresolved reference)"),
                FText::FromString(Relationship.SourceReference));
            StatBlockBody->AddSlot().AutoHeight().Padding(0, 0, 0, 4)
            [
                SNew(STextBlock).Text(Label).AutoWrapText(true)
            ];
        }
    }
}

void SDnd5eBestiaryBrowser::NavigateToCreature(FDnd5eRecordId Id)
{
    const auto* DatasetSession = Session.Get();
    const auto* Catalog = DatasetSession ? DatasetSession->GetCatalog() : nullptr;
    FDnd5eCreature Target;
    if (!Catalog || !Catalog->TryGetCreature(Id, Target)) { return; }

    const auto FindVisibleRow = [this, &Id]()
    {
        return VisibleRows.FindByPredicate([&Id](const FRowPtr& Row) { return Row->Id == Id; });
    };

    const FRowPtr* Found = FindVisibleRow();
    if (!Found && !Filter.IsEmpty())
    {
        SearchBox->SetText(FText::GetEmpty());
        Found = FindVisibleRow();
    }
    if (!Found) { return; }

    const FRowPtr Row = *Found;
    ListView->SetSelection(Row);
    ListView->RequestScrollIntoView(Row);
}

#undef LOCTEXT_NAMESPACE