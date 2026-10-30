// Developed by Neko Creative Technologies
#if WITH_DEV_AUTOMATION_TESTS

#include "Dnd5eTestSupport.h"
#include "Browser/Dnd5eSpellBrowserModel.h"
#include "Browser/SDnd5eSpellBrowser.h"
#include "Catalog/Dnd5eSpellCatalogObject.h"
#include "Dnd5eEditor.h"
#include "Editor.h"
#include "Framework/Docking/TabManager.h"
#include "HAL/FileManager.h"
#include "IDetailsView.h"
#include "Misc/FileHelper.h"
#include "Misc/Guid.h"
#include "Misc/Paths.h"
#include "Subsystems/Dnd5eEditorDatasetSubsystem.h"
#include "ToolMenu.h"
#include "ToolMenuSection.h"
#include "ToolMenus.h"
#include "UObject/GarbageCollection.h"
#include "UObject/StrongObjectPtr.h"
#include "Widgets/Input/SSearchBox.h"

namespace
{
	TArray<FDnd5eSpell> BrowserRecords()
	{
		FDnd5eSpell First = Dnd5eTests::SyntheticSpell();
		First.Header.DisplayName = TEXT("Invented Ember");
		First.Header.RecordId.SourceKey = TEXT("CaseKey");
		First.Header.Provenance[0].SourceId = TEXT("SYN-FIRST");
		FDnd5eSpell Second = First;
		Second.Header.RecordId.SourceKey = TEXT("casekey");
		Second.Header.Provenance[0].SourceId = TEXT("SYN-SECOND");
		Second.Level = 0;
		FDnd5eSpell Third = First;
		Third.Header.RecordId.SourceKey = TEXT("third");
		Third.Header.DisplayName = TEXT("Invented Frost");
		Third.Header.Provenance.Reset();
		return {First, Second, Third};
	}

	bool Install(FAutomationTestBase& Test, UDnd5eSpellCatalogObject& Catalog, const TArray<FDnd5eSpell>& Records)
	{
		TArray<FDnd5eDiagnostic> Diagnostics;
		return Test.TestTrue(TEXT("Install independent synthetic records"), Catalog.ReplaceSpells(Records, Diagnostics));
	}

	struct FBrowserFiles
	{
		FString Base = FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Automation/Dnd5eBrowserFixtures")));
		FString Root = FPaths::Combine(Base, FGuid::NewGuid().ToString(EGuidFormats::Digits));

		~FBrowserFiles()
		{
			FString Resolved = FPaths::ConvertRelativePathToFull(Root);
			if (FPaths::CollapseRelativeDirectories(Resolved) && FPaths::IsUnderDirectory(Resolved, Base) && !FPaths::IsSamePath(Resolved, Base))
			{
				IFileManager::Get().DeleteDirectory(*Resolved, false, true);
			}
		}

		bool Write(const FString& Json) const
		{
			return IFileManager::Get().MakeDirectory(*Root, true) && FFileHelper::SaveStringToFile(Json, *FPaths::Combine(Root, TEXT("synthetic.json")), FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
		}

		FDnd5eJsonSpellCatalogLoadRequest Request() const
		{
			FDnd5eJsonSpellCatalogLoadRequest Result;
			Result.DatasetRoot = Root;
			Result.DatasetNamespace = TEXT("browser-synthetic");
			Result.SpellFiles = {TEXT("synthetic.json")};
			return Result;
		}
	};

	TSharedPtr<SWidget> FindWidget(const TSharedRef<SWidget>& Root, FName Type)
	{
		if (Root->GetType() == Type) { return Root; }
		FChildren* Children = Root->GetChildren();
		for (int32 Index = 0; Index < Children->Num(); ++Index)
		{
			if (TSharedPtr<SWidget> Found = FindWidget(Children->GetChildAt(Index), Type)) { return Found; }
		}
		return nullptr;
	}

	// Install only a disposable in-memory catalog. Preserve the real subsystem's
	// original object and result; never load or save its personal selection settings.
	struct FScopedBrowserCatalog
	{
		UDnd5eEditorDatasetSubsystem& Session;
		FObjectPropertyBase& Property;
		TStrongObjectPtr<UDnd5eSpellCatalogObject> Original;

		FScopedBrowserCatalog(UDnd5eEditorDatasetSubsystem& InSession, FObjectPropertyBase& InProperty, UDnd5eSpellCatalogObject* Fixture)
			: Session(InSession), Property(InProperty), Original(InSession.GetCatalog())
		{
			Property.SetObjectPropertyValue_InContainer(&Session, Fixture);
		}

		~FScopedBrowserCatalog()
		{
			Property.SetObjectPropertyValue_InContainer(&Session, Original.Get());
			Session.OnCatalogChanged().Broadcast();
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eBrowserMetadataTest, "Dnd5e.Editor.SpellBrowser.ModelMetadataAndIdentity", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FDnd5eBrowserMetadataTest::RunTest(const FString& Parameters)
{
	TStrongObjectPtr<UDnd5eSpellCatalogObject> Catalog(NewObject<UDnd5eSpellCatalogObject>());
	const auto Records = BrowserRecords();
	if (!Install(*this, *Catalog, Records)) { return false; }
	FDnd5eSpellBrowserModel Model;
	Model.Rebuild(Catalog.Get());
	if (!TestEqual(TEXT("All records become distinct rows"), Model.GetRows().Num(), 3)) { return false; }
	TestEqual(TEXT("Total count is independent of filtering"), Model.GetTotalCount(), 3);
	for (int32 Index = 0; Index < Records.Num(); ++Index)
	{
		TestTrue(TEXT("Source order and exact identity are preserved"), Model.GetRows()[Index]->Id == Records[Index].Header.RecordId);
		TestEqual(TEXT("Name is display metadata"), Model.GetRows()[Index]->Name, Records[Index].Header.DisplayName);
		TestEqual(TEXT("Level includes a valid zero"), Model.GetRows()[Index]->Level, Records[Index].Level);
	}
	TestEqual(TEXT("Primary provenance supplies the source label"), Model.GetRows()[0]->SourceId, FString(TEXT("SYN-FIRST")));
	TestEqual(TEXT("Matching names retain their distinct source labels"), Model.GetRows()[1]->SourceId, FString(TEXT("SYN-SECOND")));
	TestTrue(TEXT("Missing provenance does not invent a source"), Model.GetRows()[2]->SourceId.IsEmpty());
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eBrowserSearchTest, "Dnd5e.Editor.SpellBrowser.ModelNameSearch", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FDnd5eBrowserSearchTest::RunTest(const FString& Parameters)
{
	TStrongObjectPtr<UDnd5eSpellCatalogObject> Catalog(NewObject<UDnd5eSpellCatalogObject>());
	if (!Install(*this, *Catalog, BrowserRecords())) { return false; }
	FDnd5eSpellBrowserModel Model;
	Model.Rebuild(Catalog.Get());
	const auto First = Model.GetRows()[0];
	for (const FString& Search : {FString(TEXT("ember")), FString(TEXT("EMBER")), FString(TEXT("  ember \t"))})
	{
		Model.SetSearchText(Search);
		TestEqual(TEXT("Case-insensitive trimmed substrings match both variants"), Model.GetRows().Num(), 2);
		if (!Model.GetRows().IsEmpty()) { TestTrue(TEXT("Search reuses the cached metadata rows"), Model.GetRows()[0] == First); }
	}
	Model.SetSearchText(TEXT("vented fro"));
	TestEqual(TEXT("A middle substring matches"), Model.GetRows().Num(), 1);
	Model.SetSearchText(TEXT("does not exist"));
	TestTrue(TEXT("No matches clear the visible rows"), Model.GetRows().IsEmpty());
	TestEqual(TEXT("Filtering does not change total records"), Model.GetTotalCount(), 3);
	Model.SetSearchText(TEXT(" \t "));
	TestEqual(TEXT("Whitespace-only search restores every record"), Model.GetRows().Num(), 3);
	Model.SetSearchText(TEXT("SYN-FIRST"));
	TestTrue(TEXT("Publication labels are not name matches"), Model.GetRows().IsEmpty());
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eBrowserRebuildTest, "Dnd5e.Editor.SpellBrowser.ModelRebuildAndOwnership", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FDnd5eBrowserRebuildTest::RunTest(const FString& Parameters)
{
	FDnd5eSpellBrowserModel Model;
	Model.Rebuild(nullptr);
	TestTrue(TEXT("Unloaded model is empty"), Model.GetRows().IsEmpty() && Model.GetTotalCount() == 0);
	TStrongObjectPtr<UDnd5eSpellCatalogObject> Catalog(NewObject<UDnd5eSpellCatalogObject>());
	if (!Install(*this, *Catalog, BrowserRecords())) { return false; }
	Model.SetSearchText(TEXT("frost"));
	Model.Rebuild(Catalog.Get());
	TestEqual(TEXT("Search survives the initial rebuild"), Model.GetRows().Num(), 1);
	FDnd5eSpell Replacement = BrowserRecords()[0];
	Replacement.Header.DisplayName = TEXT("Invented Frost replacement");
	if (!Install(*this, *Catalog, {Replacement})) { return false; }
	Model.Rebuild(Catalog.Get());
	if (!TestEqual(TEXT("Replacement discards stale rows"), Model.GetRows().Num(), 1)) { return false; }
	TestEqual(TEXT("Rebuild replaces old display metadata"), Model.GetRows()[0]->Name, Replacement.Header.DisplayName);
	FDnd5eSpell Copy;
	if (!TestTrue(TEXT("Catalog still retrieves the original record"), Catalog->TryGetSpell(Model.GetRows()[0]->Id, Copy))) { return false; }
	Model.GetRows()[0]->Name = TEXT("UI metadata only");
	Catalog->TryGetSpell(Replacement.Header.RecordId, Copy);
	TestEqual(TEXT("UI metadata changes cannot mutate a catalog record"), Copy.Header.DisplayName, Replacement.Header.DisplayName);
	Model.Rebuild(nullptr);
	TestTrue(TEXT("Disconnecting clears both cached and visible rows"), Model.GetRows().IsEmpty() && Model.GetTotalCount() == 0);
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eBrowserNotificationTest, "Dnd5e.Editor.SpellBrowser.CatalogNotifications", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FDnd5eBrowserNotificationTest::RunTest(const FString& Parameters)
{
	FBrowserFiles Files;
	const FString Json = TEXT(R"json({"spell":[{"name":"Invented Spark","source":"SYN","level":0,"school":"V","time":[{"number":1,"unit":"action"}],"range":{"type":"point","distance":{"type":"self"}},"components":{"v":true},"duration":[{"type":"instant"}],"entries":["An invented spark."]}]})json");
	if (!TestTrue(TEXT("Write an original local fixture"), Files.Write(Json))) { return false; }
	TStrongObjectPtr<UDnd5eEditorDatasetSubsystem> Session(NewObject<UDnd5eEditorDatasetSubsystem>());
	TArray<int32> Counts;
	TArray<EDnd5eJsonReadStatus> Statuses;
	const auto Handle = Session->OnCatalogChanged().AddLambda([&]()
	{
		Counts.Add(Session->GetSpellCount());
		Statuses.Add(Session->GetLastLoadResult().Status);
	});
	const auto Loaded = Session->LoadSelection(Files.Request());
	TestTrue(TEXT("Successful load broadcasts after installing records and result"), Loaded.Status == EDnd5eJsonReadStatus::Success && Counts == TArray<int32>{1} && Statuses == TArray<EDnd5eJsonReadStatus>{EDnd5eJsonReadStatus::Success});
	auto Missing = Files.Request(); Missing.SpellFiles[0] = TEXT("missing.json");
	TestTrue(TEXT("A failed load leaves active records and sends no notification"), Session->LoadSelection(Missing).Status == EDnd5eJsonReadStatus::Failure && Session->GetSpellCount() == 1 && Counts.Num() == 1);
	TestTrue(TEXT("Write a known-empty collection"), Files.Write(TEXT("{\"spell\":[]}")));
	TestTrue(TEXT("Successful empty replacement also notifies"), Session->LoadSelection(Files.Request()).Status == EDnd5eJsonReadStatus::Success && Counts == TArray<int32>{1, 0});
	Session->Deinitialize();
	TestTrue(TEXT("Deinitialization broadcasts the already-cleared state"), Counts == TArray<int32>{1, 0, 0} && Statuses.Num() == 3 && Statuses.Last() == EDnd5eJsonReadStatus::NotAttempted && Session->GetCatalog() == nullptr);
	Session->OnCatalogChanged().Remove(Handle);
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eBrowserWidgetTest, "Dnd5e.Editor.SpellBrowser.WidgetSelectionAndRefresh", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FDnd5eBrowserWidgetTest::RunTest(const FString& Parameters)
{
	auto* Session = GEditor ? GEditor->GetEditorSubsystem<UDnd5eEditorDatasetSubsystem>() : nullptr;
	if (!TestNotNull(TEXT("Actual editor subsystem exists"), Session)) { return false; }
	auto* Property = FindFProperty<FObjectPropertyBase>(Session->GetClass(), TEXT("Catalog"));
	if (!TestNotNull(TEXT("Transient catalog fixture can be installed"), Property)) { return false; }
	TStrongObjectPtr<UDnd5eSpellCatalogObject> Fixture(NewObject<UDnd5eSpellCatalogObject>());
	const auto Records = BrowserRecords();
	if (!Install(*this, *Fixture, Records)) { return false; }
	FScopedBrowserCatalog Restore(*Session, *Property, Fixture.Get());
	TSharedPtr<SDnd5eSpellBrowser> Browser = SNew(SDnd5eSpellBrowser);
	auto List = StaticCastSharedPtr<SListView<FDnd5eSpellBrowserRowPtr>>(FindWidget(Browser.ToSharedRef(), TEXT("SListView<FDnd5eSpellBrowserRowPtr>")));
	auto Search = StaticCastSharedPtr<SSearchBox>(FindWidget(Browser.ToSharedRef(), TEXT("SSearchBox")));
	auto Details = StaticCastSharedPtr<IDetailsView>(FindWidget(Browser.ToSharedRef(), TEXT("SDetailsView")));
	if (!TestTrue(TEXT("Actual browser list, search, and object details widgets exist"), List.IsValid() && Search.IsValid() && Details.IsValid())) { return false; }
	if (!TestEqual(TEXT("Browser opens on the current catalog"), List->GetItems().Num(), 3)) { return false; }
	List->SetSelection(List->GetItems()[0]);
	if (!TestEqual(TEXT("Selecting the actual list populates the actual inspector"), Details->GetSelectedObjects().Num(), 1)) { return false; }
	auto* Selection = Cast<UDnd5eSpellBrowserSelection>(Details->GetSelectedObjects()[0].Get());
	if (!TestNotNull(TEXT("The browser uses the production selection wrapper"), Selection)) { return false; }
	TestEqual(TEXT("Selected record is copied by exact ID"), Selection->Spell.Header.RecordId, Records[0].Header.RecordId);
	TestFalse(TEXT("The browser disables detail editing"), Details->GetIsPropertyEditingEnabledDelegate().Execute());
	TWeakObjectPtr<UDnd5eSpellBrowserSelection> WeakSelection = Selection;
	CollectGarbage(RF_NoFlags);
	if (!TestTrue(TEXT("The actual browser retains its selected object through collection"), WeakSelection.IsValid())) { return false; }
	for (int32 Frame = 0; Frame < 3; ++Frame) { Details->Tick(FGeometry(), FPlatformTime::Seconds(), 1.0f / 60.0f); }
	Selection->Spell.Header.DisplayName = TEXT("Inspection copy only");
	FDnd5eSpell Original;
	Fixture->TryGetSpell(Records[0].Header.RecordId, Original);
	TestEqual(TEXT("Selection changes do not modify the catalog"), Original.Header.DisplayName, Records[0].Header.DisplayName);
	Search->SetText(FText::FromString(TEXT("  FROST ")));
	TestEqual(TEXT("Search updates the actual visible list"), List->GetItems().Num(), 1);
	TestTrue(TEXT("Searching clears selection and details"), List->GetSelectedItems().IsEmpty() && Details->GetSelectedObjects().IsEmpty());
	if (List->GetItems().IsEmpty()) { return false; }
	List->SetSelection(List->GetItems()[0]);
	FDnd5eSpell Replacement = Records[0]; Replacement.Header.DisplayName = TEXT("Invented Frost replacement");
	if (!Install(*this, *Fixture, {Replacement})) { return false; }
	Session->OnCatalogChanged().Broadcast();
	TestTrue(TEXT("A catalog notification refreshes metadata and clears stale details"), List->GetItems().Num() == 1 && List->GetItems()[0]->Name == Replacement.Header.DisplayName && Details->GetSelectedObjects().IsEmpty());
	TestEqual(TEXT("Refresh preserves search text"), Search->GetText().ToString(), FString(TEXT("  FROST ")));
	List->SetSelection(List->GetItems()[0]);
	WeakSelection = Cast<UDnd5eSpellBrowserSelection>(Details->GetSelectedObjects()[0].Get());
	TWeakPtr<SDnd5eSpellBrowser> WeakBrowser = Browser;
	List.Reset(); Search.Reset(); Details.Reset(); Browser.Reset();
	TestFalse(TEXT("Closing releases the browser widget"), WeakBrowser.IsValid());
	CollectGarbage(RF_NoFlags);
	TestFalse(TEXT("Closing releases its selected UObject"), WeakSelection.IsValid());
	Session->OnCatalogChanged().Broadcast();
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eBrowserRegistrationTest, "Dnd5e.Editor.SpellBrowser.MenuAndTabRegistration", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FDnd5eBrowserRegistrationTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("The stable browser tab factory is registered"), FGlobalTabmanager::Get()->HasTabSpawner(TEXT("Dnd5eSpellBrowser")));
	UToolMenu* Menu = UToolMenus::Get()->FindMenu(TEXT("LevelEditor.MainMenu.Tools"));
	FToolMenuSection* Section = Menu ? Menu->FindSection(TEXT("Dnd5e")) : nullptr;
	if (!TestNotNull(TEXT("Browser tools section exists"), Section)) { return false; }
	auto& Module = FModuleManager::GetModuleChecked<FDnd5eEditorModule>(TEXT("Dnd5eEditor"));
	int32 Count = 0;
	for (const auto& Entry : Section->Blocks)
	{
		if (Entry.Name != FName(TEXT("Dnd5eSpellBrowser"))) { continue; }
		++Count;
		TestTrue(TEXT("The browser menu uses the module's cleanup owner"), Entry.Owner == FToolMenuOwner(&Module));
	}
	TestEqual(TEXT("Exactly one browser command is registered"), Count, 1);
	return !HasAnyErrors();
}

#endif
