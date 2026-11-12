// Developed by Neko Creative Technologies

#if WITH_DEV_AUTOMATION_TESTS

#include "Browser/Bestiary/SDnd5eBestiaryBrowser.h"
#include "Dnd5eTags.h"
#include "Editor.h"
#include "Framework/Docking/TabManager.h"
#include "Misc/AutomationTest.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Misc/ScopeExit.h"
#include "Settings/Dnd5eEditorBestiaryDatasetSettings.h"
#include "Subsystems/Dnd5eEditorBestiaryDatasetSubsystem.h"
#include "ToolMenu.h"
#include "ToolMenus.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/UnrealType.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
	SWidget* FindWidgetByType(SWidget& Widget, FName Type)
	{
		if (Widget.GetType() == Type) { return &Widget; }
		FChildren* Children = Widget.GetChildren();
		for (int32 Index = 0; Index < Children->Num(); ++Index)
		{
			if (SWidget* Found = FindWidgetByType(*Children->GetChildAt(Index), Type)) { return Found; }
		}
		return nullptr;
	}

	bool ContainsText(SWidget& Widget, const TCHAR* Expected)
	{
		if (Widget.GetType() == TEXT("STextBlock") &&
			static_cast<const STextBlock&>(Widget).GetText().ToString().Contains(Expected))
		{
			return true;
		}

		FChildren* Children = Widget.GetChildren();
		for (int32 Index = 0; Index < Children->Num(); ++Index)
		{
			if (ContainsText(*Children->GetChildAt(Index), Expected)) { return true; }
		}
		return false;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eBestiaryBrowserWidgetTest, "Dnd5e.Bestiary.Editor.BrowserSelectionAndRefresh",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FDnd5eBestiaryBrowserWidgetTest::RunTest(const FString& Parameters)
{
	const FName BrowserId(TEXT("Dnd5eBestiaryBrowser"));
	TestTrue(TEXT("Bestiary tab spawner is registered"), FGlobalTabmanager::Get()->HasTabSpawner(BrowserId));
	auto* Menu = UToolMenus::Get()->FindMenu(TEXT("LevelEditor.MainMenu.Tools"));
	auto* Section = Menu ? Menu->FindSection(TEXT("Dnd5e")) : nullptr;
	TestTrue(TEXT("Tools menu contains the Bestiary browser"), Section && Section->FindEntry(BrowserId));

	auto* Session = GEditor ? GEditor->GetEditorSubsystem<UDnd5eEditorBestiaryDatasetSubsystem>() : nullptr;
	if (!TestNotNull(TEXT("The editor Bestiary session exists"), Session)) { return false; }
	auto* CatalogProperty = FindFProperty<FObjectPropertyBase>(Session->GetClass(), TEXT("Catalog"));
	if (!TestNotNull(TEXT("The transient catalog property exists"), CatalogProperty)) { return false; }

	const TStrongObjectPtr<UDnd5eCreatureCatalogObject> Original(Session->GetCatalog());
	const TStrongObjectPtr<UDnd5eCreatureCatalogObject> Fixture(NewObject<UDnd5eCreatureCatalogObject>());
	ON_SCOPE_EXIT
	{
		CatalogProperty->SetObjectPropertyValue_InContainer(Session, Original.Get());
		Session->OnCatalogChanged().Broadcast();
	};
	CatalogProperty->SetObjectPropertyValue_InContainer(Session, Fixture.Get());

	FDnd5eCreature First;
	First.Header.DisplayName = TEXT("Invented Scout");
	First.Header.RecordId.DatasetNamespace = TEXT("browser-test");
	First.Header.RecordId.Category = Dnd5e::Category::Creature;
	First.Header.RecordId.SourceKey = TEXT("scout-first");
	First.Header.Provenance.AddDefaulted_GetRef().SourceId = TEXT("SYN-FIRST");
	First.HitPoints.bHasAverage = true;
	First.HitPoints.Average = 7;

	FDnd5eCreature Second = First;
	Second.Header.RecordId.SourceKey = TEXT("scout-second");
	Second.Header.Provenance[0].SourceId = TEXT("SYN-SECOND");
	Second.HitPoints.Average = 15;
	First.AbilityScores.bHasScores = true;
	First.AbilityScores.Strength = 7;
	First.Actions.AddDefaulted_GetRef().Name = TEXT("Invented Lunge");

	TArray<FDnd5eDiagnostic> Diagnostics;
	if (!TestTrue(TEXT("Install two same-name records with distinct sources"), Fixture->GetMutableCatalog().ReplaceCreatures({First, Second}, Diagnostics)))
	{
		return false;
	}

	const TSharedRef<SDnd5eBestiaryBrowser> Browser = SNew(SDnd5eBestiaryBrowser);
	if (!TestEqual(TEXT("Both same-name rows appear"), Browser->ListView->GetItems().Num(), 2)) { return false; }

	Browser->ListView->SetSelection(Browser->ListView->GetItems()[0]);
	TestTrue(TEXT("First row shows the matching source and HP"), Browser->CardSource.ToString() == TEXT("SYN-FIRST") &&
		ContainsText(*Browser, TEXT("Hit Points  7")));
	SWidget* Grid = FindWidgetByType(*Browser, TEXT("SUniformGridPanel"));
	TestTrue(TEXT("Selected creature has six ability cells and a separate action section"),
		Grid && Grid->GetChildren()->Num() == 6 && ContainsText(*Grid, TEXT("7 (-2)")) &&
		ContainsText(*Browser, TEXT("Actions")) && ContainsText(*Browser, TEXT("Invented Lunge")));

	Browser->ListView->SetSelection(Browser->ListView->GetItems()[1]);
	TestTrue(TEXT("Second row retrieves its own record by exact ID"), Browser->CardSource.ToString() == TEXT("SYN-SECOND") &&
		ContainsText(*Browser, TEXT("Hit Points  15")));
	TestTrue(TEXT("Changing selection removes the former ability grid and action section"),
		!FindWidgetByType(*Browser, TEXT("SUniformGridPanel")) && !ContainsText(*Browser, TEXT("Invented Lunge")));

	Browser->HandleSearchChanged(FText::FromString(TEXT("  syn-first  ")));
	TestTrue(TEXT("Source search trims and ignores case, and clears the old selection"),
		Browser->VisibleRows.Num() == 1 && Browser->VisibleRows[0]->Id == First.Header.RecordId &&
		Browser->ListView->GetSelectedItems().IsEmpty() && Browser->CardTitle.IsEmpty());

	if (!TestTrue(TEXT("Replace the active catalog with the other source"), Fixture->GetMutableCatalog().ReplaceCreatures({Second}, Diagnostics)))
	{
		return false;
	}
	Session->OnCatalogChanged().Broadcast();
	TestTrue(TEXT("A publication refreshes the rows while preserving the filter"),
		Browser->AllRows.Num() == 1 && Browser->VisibleRows.IsEmpty() && Browser->Filter == TEXT("syn-first"));

	Browser->HandleSearchChanged(FText::GetEmpty());
	TestTrue(TEXT("Clearing the filter exposes the replacement row"),
		Browser->VisibleRows.Num() == 1 && Browser->VisibleRows[0]->Id == Second.Header.RecordId);
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eBestiarySrdPreviewTest, "Dnd5e.Bestiary.Local.SrdPreview",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FDnd5eBestiarySrdPreviewTest::RunTest(const FString& Parameters)
{
	FString Root;
	if (!FParse::Value(FCommandLine::Get(), TEXT("Dnd5eSrdBestiaryRoot="), Root))
	{
		AddInfo(TEXT("Supply -Dnd5eSrdBestiaryRoot to verify the local SRD preview exports and host settings."));
		return true;
	}

	const auto Selected = GetDefault<UDnd5eEditorBestiaryDatasetSettings>()->MakeLoadRequest();
	TestTrue(TEXT("The host editor selects SRD 5.1 with no template"),
		FPaths::IsSamePath(Selected.DatasetRoot, FPaths::Combine(Root, TEXT("srd51"))) &&
		Selected.DatasetNamespace == TEXT("srd51-local") && Selected.TemplateFile.IsEmpty());
	const TStrongObjectPtr<UDnd5eEditorBestiaryDatasetSubsystem> Session(NewObject<UDnd5eEditorBestiaryDatasetSubsystem>());

	const auto Verify = [this, &Session](const FDnd5eJsonCreatureCatalogLoadRequest& Request, int32 ExpectedCount,
		const TCHAR* Name, const TCHAR* Source)
	{
		const auto& Result = Session->LoadSelection(Request);
		if (!TestTrue(FString::Printf(TEXT("%s publishes all %d creatures"), *Request.DatasetNamespace, ExpectedCount),
			Result.Status == EDnd5eJsonReadStatus::Success && Session->GetCreatureCount() == ExpectedCount))
		{
			for (const auto& Diagnostic : Result.Diagnostics)
			{
				if (Diagnostic.Severity == EDnd5eDiagnosticSeverity::Error)
				{
					AddError(Diagnostic.RelativeFilePath + TEXT(" ") + Diagnostic.JsonPointer + TEXT(" ") + Diagnostic.Message);
				}
			}
			return;
		}

		FDnd5eCreatureQuery Query;
		Query.Name = Name;
		Query.SourceId = Source;
		const auto* Catalog = Session->GetCatalog();
		const auto Ids = Catalog->QueryCreatureIds(Query);
		FDnd5eCreature Creature;
		TestTrue(FString::Printf(TEXT("%s can be retrieved with its stat block"), Name), Ids.Num() == 1 &&
			Catalog->TryGetCreature(Ids[0], Creature) && Creature.AbilityScores.bHasScores && !Creature.Actions.IsEmpty());
	};

	Verify(Selected, 323, TEXT("Goblin"), TEXT("MM"));
	FDnd5eJsonCreatureCatalogLoadRequest Revised;
	Revised.DatasetRoot = FPaths::Combine(Root, TEXT("srd52"));
	Revised.DatasetNamespace = TEXT("srd52-local");
	Verify(Revised, 336, TEXT("Goblin Warrior"), TEXT("XMM"));
	return !HasAnyErrors();
}

#endif
