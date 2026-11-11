// Developed by Neko Creative Technologies
#if WITH_DEV_AUTOMATION_TESTS

#include "Dnd5eTestSupport.h"
#include "Browser/Dnd5eItemBrowserDetails.h"
#include "Browser/Dnd5eSpellBrowserDetails.h"
#include "Browser/Dnd5eItemBrowserLinks.h"
#include "Browser/Dnd5eItemBrowserSelection.h"
#include "Browser/SDnd5eItemBrowser.h"
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
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/ScopeExit.h"
#include "Readers/Items/Dnd5eJsonItemReader.h"
#include "Readers/Items/Dnd5eJsonItemDefinitionCatalogLibrary.h"
#include "Readers/Spells/Dnd5eJsonSpellReader.h"
#include "Subsystems/Dnd5eEditorDatasetSubsystem.h"
#include "Subsystems/Dnd5eEditorItemDatasetSubsystem.h"
#include "ToolMenu.h"
#include "ToolMenuSection.h"
#include "ToolMenus.h"
#include "UObject/GarbageCollection.h"
#include "UObject/StrongObjectPtr.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Text/STextBlock.h"

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
	TSharedPtr<STextBlock> FindBrowserText(const TSharedRef<SWidget>& Root, const FString& Prefix)
	{
		if (Root->GetType() == FName(TEXT("STextBlock")))
		{
			auto Text = StaticCastSharedRef<STextBlock>(Root);
			if (Text->GetText().ToString().StartsWith(Prefix)) { return Text; }
		}
		auto* Children = Root->GetChildren();
		for (int32 Index = 0; Index < Children->Num(); ++Index)
		{
			if (auto Found = FindBrowserText(Children->GetChildAt(Index), Prefix)) { return Found; }
		}
		return nullptr;
	}

	TSharedPtr<SButton> FindButtonWithCaption(const TSharedRef<SWidget>& Root, const FString& Prefix)
	{
		if (Root->GetType() == FName(TEXT("SButton")) && FindBrowserText(Root, Prefix))
		{
			return StaticCastSharedRef<SButton>(Root);
		}
		auto* Children = Root->GetChildren();
		for (int32 Index = 0; Index < Children->Num(); ++Index)
		{
			if (auto Found = FindButtonWithCaption(Children->GetChildAt(Index), Prefix)) { return Found; }
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
	TestTrue(TEXT("The separate item tab factory is registered"), FGlobalTabmanager::Get()->HasTabSpawner(TEXT("Dnd5eItemBrowser")));
	UToolMenu* Menu = UToolMenus::Get()->FindMenu(TEXT("LevelEditor.MainMenu.Tools"));
	FToolMenuSection* Section = Menu ? Menu->FindSection(TEXT("Dnd5e")) : nullptr;
	if (!TestNotNull(TEXT("Browser tools section exists"), Section)) { return false; }
	auto& Module = FModuleManager::GetModuleChecked<FDnd5eEditorModule>(TEXT("Dnd5eEditor"));
	for (FName Name : {FName(TEXT("Dnd5eSpellBrowser")), FName(TEXT("Dnd5eItemBrowser"))})
	{
		int32 Count = 0;
		for (const auto& Entry : Section->Blocks)
		{
			if (Entry.Name != Name) { continue; }
			++Count;
			TestTrue(TEXT("The browser menu uses the module's cleanup owner"), Entry.Owner == FToolMenuOwner(&Module));
		}
		TestEqual(TEXT("Each browser command has one registration"), Count, 1);
	}
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eBrowserReportTest, "Dnd5e.Editor.SpellBrowser.ReferenceReportInspection", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FDnd5eBrowserReportTest::RunTest(const FString& Parameters)
{
	auto* Session = GEditor ? GEditor->GetEditorSubsystem<UDnd5eEditorDatasetSubsystem>() : nullptr;
	if (!TestNotNull(TEXT("Editor session"), Session)) { return false; }
	auto* Property = FindFProperty<FObjectPropertyBase>(Session->GetClass(), TEXT("Catalog"));
	if (!TestNotNull(TEXT("Transient catalog property"), Property)) { return false; }
	TStrongObjectPtr<UDnd5eSpellCatalogObject> Fixture(NewObject<UDnd5eSpellCatalogObject>());
	auto Owner = BrowserRecords()[0]; Owner.Header.DisplayName = TEXT("Invented Caller");
	auto Target = BrowserRecords()[1]; Target.Header.DisplayName = TEXT("Invented Beacon"); Target.Description.Reset(); Target.HigherLevelDescription.Reset();
	FDnd5eReferenceContent Token; Token.SourceFormat = TEXT("5etools"); Token.SourceReference = TEXT("{@spell Invented Beacon|SYN-SECOND}"); Token.Display = {Dnd5eTests::Text(TEXT("A label"))};
	FDnd5eReferenceContent Declared = Token; Declared.SourceFormat = TEXT("other"); Declared.bHasTargetRecordId = true; Declared.TargetRecordId = Target.Header.RecordId;
	FDnd5eParagraphContent Body; Body.Inlines = {Dnd5eTests::Inline(Dnd5e::Content::Inline::Reference, Token), Dnd5eTests::Inline(Dnd5e::Content::Inline::Reference, Declared)};
	Owner.Description = {Dnd5eTests::Block(Dnd5e::Content::Block::Paragraph, Body), Dnd5eTests::Block(Dnd5e::Content::Block::Unsupported, FDnd5eUnsupportedContent{})}; Owner.HigherLevelDescription.Reset();
	auto Empty = Target; Empty.Header.RecordId.SourceKey = TEXT("empty"); Empty.Header.DisplayName = TEXT("Invented Empty");
	auto Failure = Empty; Failure.Header.RecordId.SourceKey = TEXT("failure"); Failure.Header.DisplayName = TEXT("Invented Failure"); Failure.Description.AddDefaulted();
	if (!Install(*this, *Fixture, {Owner, Empty, Failure, Target})) { return false; }
	FScopedBrowserCatalog Restore(*Session, *Property, Fixture.Get());
	auto Browser = SNew(SDnd5eSpellBrowser);
	auto List = StaticCastSharedPtr<SListView<FDnd5eSpellBrowserRowPtr>>(FindWidget(Browser, TEXT("SListView<FDnd5eSpellBrowserRowPtr>")));
	auto Search = StaticCastSharedPtr<SSearchBox>(FindWidget(Browser, TEXT("SSearchBox")));
	auto Details = StaticCastSharedPtr<IDetailsView>(FindWidget(Browser, TEXT("SDetailsView")));
	if (!TestTrue(TEXT("Production widgets"), List.IsValid() && Search.IsValid() && Details.IsValid())) { return false; }
	const FString NoSelection = TEXT("Select a spell to inspect its reference report.");
	TestTrue(TEXT("Initial summary prompts selection"), FindBrowserText(Browser, NoSelection).IsValid());
	List->SetSelection(List->GetItems()[0]);
	if (!TestEqual(TEXT("One selected inspector object"), Details->GetSelectedObjects().Num(), 1)) { return false; }
	auto* Selected = Cast<UDnd5eSpellBrowserSelection>(Details->GetSelectedObjects()[0].Get());
	if (!TestNotNull(TEXT("Production selection"), Selected)) { return false; }
	TestTrue(TEXT("Report computed against same catalog"), Selected->ReferenceReport.bCollected && Selected->ReferenceReport.Entries.Num() == 2 && Selected->ReferenceReport.Diagnostics.Num() == 1);
	auto Summary = FindBrowserText(Browser, TEXT("Typed references: 2"));
	if (!TestTrue(TEXT("Separate stage counts and collection warning shown"), Summary.IsValid() && Summary->GetText().ToString().Contains(TEXT("Original targets found: 1")) && Summary->GetText().ToString().Contains(TEXT("Selectors normalized: 1")) && Summary->GetText().ToString().Contains(TEXT("Normalized targets found: 1")) && Summary->GetText().ToString().Contains(TEXT("Collection diagnostics: 1")))) { return false; }
	TestFalse(TEXT("Both spell/report inspection is read-only"), Details->GetIsPropertyEditingEnabledDelegate().Execute());
	for (int32 Frame = 0; Frame < 3; ++Frame) { Details->Tick(FGeometry(), FPlatformTime::Seconds(), 1.0f / 60.0f); }
	TWeakObjectPtr<UDnd5eSpellBrowserSelection> Weak = Selected; CollectGarbage(RF_NoFlags);
	TestTrue(TEXT("Populated report survives GC"), Weak.IsValid() && Weak->ReferenceReport.Entries.Num() == 2);
	const FString Cached = Summary->GetText().ToString(); Selected->ReferenceReport.Entries.Reset();
	TestEqual(TEXT("Text getter returns cached summary rather than rebuilding report"), Summary->GetText().ToString(), Cached);
	FDnd5eSpell Again; Fixture->TryGetSpell(Owner.Header.RecordId, Again);
	TestTrue(TEXT("Selection report edits leave catalog intact"), Dnd5eTests::Equal(FDnd5eSpell::StaticStruct(), &Owner, &Again));
	List->SetSelection(List->GetItems()[1]);
	TestTrue(TEXT("Zero-reference success is distinct from no selection"), FindBrowserText(Browser, TEXT("Typed references: 0")).IsValid());
	List->SetSelection(List->GetItems()[2]);
	TestTrue(TEXT("Structural failure has explicit summary"), FindBrowserText(Browser, TEXT("Reference collection failed.")).IsValid());
	Selected = Cast<UDnd5eSpellBrowserSelection>(Details->GetSelectedObjects()[0].Get());
	TestTrue(TEXT("Failed report remains inspectable beside spell"), Selected && !Selected->ReferenceReport.bCollected && Selected->ReferenceReport.Diagnostics.Num() == 1 && Selected->Spell.Header.RecordId == Failure.Header.RecordId);
	Search->SetText(FText::FromString(TEXT("Caller")));
	TestTrue(TEXT("Search clears report object and cached summary"), Details->GetSelectedObjects().IsEmpty() && FindBrowserText(Browser, NoSelection).IsValid());
	if (!TestEqual(TEXT("Filtered caller row exists"), List->GetItems().Num(), 1)) { return false; }
	List->SetSelection(List->GetItems()[0]);
	if (!Install(*this, *Fixture, {Owner, Target})) { return false; }
	Session->OnCatalogChanged().Broadcast();
	TestTrue(TEXT("Refresh clears old report and summary"), Details->GetSelectedObjects().IsEmpty() && FindBrowserText(Browser, NoSelection).IsValid());
	List->SetSelection(List->GetItems()[0]);
	TestTrue(TEXT("Reselection builds a fresh report"), FindBrowserText(Browser, TEXT("Typed references: 2")).IsValid());
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eItemBrowserWidgetTest, "Dnd5e.Editor.ItemBrowser.WidgetSelectionAndRefresh", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FDnd5eItemBrowserWidgetTest::RunTest(const FString& Parameters)
{
	FBrowserFiles Files;
	if (!TestTrue(TEXT("Write item and group fixture"), Files.Write(TEXT(R"({
		"item":[{"name":"Invented Compass","source":"SYN","rarity":"rare","charges":2,"type":"M|SYN",
			"attachedSpells":["Invented Ember|SYN-FIRST"],"entries":["Compass overview text."]}],
		"itemGroup":[{"name":"Invented Family","source":"SYN","rarity":"rare","items":["Invented Compass|SYN"],
			"entries":["Family overview text."]}],
		"itemType":[{"name":"Invented Definition","abbreviation":"M","source":"SYN","entries":["Definition overview text."]}]
	})")))) { return false; }
	FDnd5eJsonItemCatalogLoadRequest Request;
	Request.DatasetRoot = Files.Root;
	Request.DatasetNamespace = TEXT("item-browser");
	Request.ItemFiles = {TEXT("synthetic.json")};
	Request.DefinitionFiles = Request.ItemFiles;
	const auto Loaded = UDnd5eJsonItemCatalogLoader::LoadItemCatalog(Request);
	if (!TestTrue(TEXT("Load browser fixture"), Loaded.Status == EDnd5eJsonReadStatus::Success && Loaded.ItemCatalog)) { return false; }

	auto* Session = GEditor ? GEditor->GetEditorSubsystem<UDnd5eEditorItemDatasetSubsystem>() : nullptr;
	if (!TestNotNull(TEXT("Actual editor item subsystem exists"), Session)) { return false; }
	auto* Property = FindFProperty<FObjectPropertyBase>(Session->GetClass(), TEXT("ItemCatalog"));
	if (!TestNotNull(TEXT("Transient catalog fixture can be installed"), Property)) { return false; }
	auto* DefinitionsProperty = FindFProperty<FObjectPropertyBase>(Session->GetClass(), TEXT("DefinitionCatalog"));
	if (!TestNotNull(TEXT("Definition fixture can be installed"), DefinitionsProperty)) { return false; }
	struct FRestore
	{
		UDnd5eEditorItemDatasetSubsystem& Session;
		FObjectPropertyBase& Property;
		TStrongObjectPtr<UDnd5eItemCatalogObject> Original;
		~FRestore()
		{
			Property.SetObjectPropertyValue_InContainer(&Session, Original.Get());
			Session.OnCatalogChanged().Broadcast();
		}
	};
	FRestore Restore{*Session, *Property, TStrongObjectPtr<UDnd5eItemCatalogObject>(Session->GetItemCatalog())};
	TStrongObjectPtr<UDnd5eItemDefinitionCatalogObject> OriginalDefinitions(Session->GetDefinitionCatalog());
	Property->SetObjectPropertyValue_InContainer(Session, Loaded.ItemCatalog.Get());
	DefinitionsProperty->SetObjectPropertyValue_InContainer(Session, Loaded.DefinitionCatalog.Get());
	ON_SCOPE_EXIT { DefinitionsProperty->SetObjectPropertyValue_InContainer(Session, OriginalDefinitions.Get()); };
	TSharedPtr<SDnd5eItemBrowser> Browser = SNew(SDnd5eItemBrowser);
	auto List = StaticCastSharedPtr<SListView<FDnd5eItemBrowserRowPtr>>(FindWidget(Browser.ToSharedRef(), TEXT("SListView<FDnd5eItemBrowserRowPtr>")));
	auto Search = StaticCastSharedPtr<SSearchBox>(FindWidget(Browser.ToSharedRef(), TEXT("SSearchBox")));
	auto Details = StaticCastSharedPtr<IDetailsView>(FindWidget(Browser.ToSharedRef(), TEXT("SDetailsView")));
	if (!TestTrue(TEXT("Browser lists both kinds from the current catalog"),
		List && Search && Details && List->GetItems().Num() == 2)) { return false; }

	const FDnd5eRecordId ItemId = List->GetItems()[0]->Id;
	List->SetSelection(List->GetItems()[0]);
	if (!TestEqual(TEXT("Item selection installs one details object"), Details->GetSelectedObjects().Num(), 1)) { return false; }
	auto* ItemSelection = Cast<UDnd5eItemBrowserSelection>(Details->GetSelectedObjects()[0].Get());
	if (!TestNotNull(TEXT("Item selection uses typed wrapper"), ItemSelection)) { return false; }
	TestTrue(TEXT("Exact selected item retains typed charges"), ItemSelection->Item.Header.RecordId == ItemId &&
		ItemSelection->Item.ChargeData.Charges.Fixed == 2);
	TestTrue(TEXT("Selected item populates the readable overview"),
		FindBrowserText(Browser.ToSharedRef(), TEXT("Compass overview text.\n\nDefinition overview text.")).IsValid());
	auto DefinitionButton = FindButtonWithCaption(Browser.ToSharedRef(), TEXT("Type: M | SYN"));
	if (!TestTrue(TEXT("Matched definition can be followed"), DefinitionButton && DefinitionButton->IsEnabled())) { return false; }
	DefinitionButton->SimulateClick();
	TestTrue(TEXT("Definition click shows the typed description preview"),
		FindBrowserText(Browser.ToSharedRef(), TEXT("Invented Definition\nDefinition overview text.")).IsValid());
	DefinitionButton.Reset();

	{
		auto* SpellSession = GEditor->GetEditorSubsystem<UDnd5eEditorDatasetSubsystem>();
		if (!TestNotNull(TEXT("Spell session exists"), SpellSession)) { return false; }
		auto* SpellProperty = FindFProperty<FObjectPropertyBase>(SpellSession->GetClass(), TEXT("Catalog"));
		if (!TestNotNull(TEXT("Spell fixture can be installed"), SpellProperty)) { return false; }
		TStrongObjectPtr<UDnd5eSpellCatalogObject> SpellFixture(NewObject<UDnd5eSpellCatalogObject>());
		auto Records = BrowserRecords();
		for (auto& Spell : Records) { Spell.Header.RecordId.DatasetNamespace = Request.DatasetNamespace; }
		if (!Install(*this, *SpellFixture, Records)) { return false; }
		FScopedBrowserCatalog RestoreSpell(*SpellSession, *SpellProperty, nullptr);
		SpellSession->OnCatalogChanged().Broadcast();
		auto SpellButton = FindButtonWithCaption(Browser.ToSharedRef(), TEXT("Spell: Invented Ember"));
		if (!TestTrue(TEXT("Unavailable spells remain visible and disabled"), SpellButton && !SpellButton->IsEnabled())) { return false; }
		SpellProperty->SetObjectPropertyValue_InContainer(SpellSession, SpellFixture.Get());
		SpellSession->OnCatalogChanged().Broadcast();
		SpellButton = FindButtonWithCaption(Browser.ToSharedRef(), TEXT("Spell: Invented Ember"));
		if (!TestTrue(TEXT("Spell reload enables the link without reselecting the item"), SpellButton && SpellButton->IsEnabled())) { return false; }

		const FTabId SpellTabId(TEXT("Dnd5eSpellBrowser"));
		const bool bHadTab = FGlobalTabmanager::Get()->FindExistingLiveTab(SpellTabId).IsValid();
		auto Tab = FGlobalTabmanager::Get()->TryInvokeTab(SpellTabId);
		if (!TestTrue(TEXT("Production spell tab opens"), Tab.IsValid())) { return false; }
		ON_SCOPE_EXIT { if (!bHadTab) { Tab->RequestCloseTab(); } };
		auto SpellSearch = StaticCastSharedPtr<SSearchBox>(FindWidget(Tab->GetContent(), TEXT("SSearchBox")));
		auto SpellDetails = StaticCastSharedPtr<IDetailsView>(FindWidget(Tab->GetContent(), TEXT("SDetailsView")));
		if (!TestTrue(TEXT("Spell tab has search and details"), SpellSearch && SpellDetails)) { return false; }
		SpellSearch->SetText(FText::FromString(TEXT("Frost")));
		SpellButton->SimulateClick();
		const auto& Selected = SpellDetails->GetSelectedObjects();
		const auto* Spell = Selected.Num() == 1 ? Cast<UDnd5eSpellBrowserSelection>(Selected[0].Get()) : nullptr;
		TestTrue(TEXT("Link reveals and selects the exact spell in the visible tab"),
			SpellSearch->GetText().IsEmpty() && Spell && Spell->Spell.Header.RecordId == Records[0].Header.RecordId);
		TestTrue(TEXT("Navigation also updates the visible spell card"),
			FindBrowserText(Tab->GetContent(), TEXT("Invented Ember")).IsValid() &&
			!FindBrowserText(Tab->GetContent(), TEXT("Select a spell.")).IsValid());
		SpellSearch->SetText(FText::FromString(TEXT("Frost")));
		TestTrue(TEXT("Spell search clears the previous card"),
			FindBrowserText(Tab->GetContent(), TEXT("Select a spell.")).IsValid() &&
			SpellDetails->GetSelectedObjects().IsEmpty());
	}
	TestFalse(TEXT("Details editing is disabled"), Details->GetIsPropertyEditingEnabledDelegate().Execute());
	TWeakObjectPtr<UDnd5eItemBrowserSelection> WeakItem = ItemSelection;
	CollectGarbage(RF_NoFlags);
	if (!TestTrue(TEXT("The item selection survives collection"), WeakItem.IsValid())) { return false; }
	for (int32 Frame = 0; Frame < 3; ++Frame) { Details->Tick(FGeometry(), FPlatformTime::Seconds(), 1.0f / 60.0f); }
	WeakItem->Item.Header.DisplayName = TEXT("Inspection copy");
	FDnd5eItem Original;
	TestTrue(TEXT("Selection is independent of catalog storage"), Session->GetItemCatalog()->TryGetItem(ItemId, Original) &&
		Original.Header.DisplayName == TEXT("Invented Compass"));

	Search->SetText(FText::FromString(TEXT(" FAMILY ")));
	if (!TestTrue(TEXT("Search selects the matching row set and clears stale details"),
		List->GetItems().Num() == 1 && List->GetSelectedItems().IsEmpty() && Details->GetSelectedObjects().IsEmpty() &&
		!FindBrowserText(Browser.ToSharedRef(), TEXT("Compass overview text.")).IsValid() &&
		!FindBrowserText(Browser.ToSharedRef(), TEXT("Invented Definition\n")).IsValid())) { return false; }
	List->SetSelection(List->GetItems()[0]);
	if (!TestEqual(TEXT("Group selection installs one details object"), Details->GetSelectedObjects().Num(), 1)) { return false; }
	const auto* Group = Cast<UDnd5eItemGroupBrowserSelection>(Details->GetSelectedObjects()[0].Get());
	TestTrue(TEXT("Group selection exposes its authored members"), Group && Group->Group.Members.Num() == 1 &&
		Group->Group.Details.Header.RecordId == List->GetItems()[0]->Id);
	TestTrue(TEXT("Group selection builds the overview from common details"),
		FindBrowserText(Browser.ToSharedRef(), TEXT("Family overview text.")).IsValid());
	auto MemberButton = FindButtonWithCaption(Browser.ToSharedRef(), TEXT("Member: Invented Compass"));
	if (!TestTrue(TEXT("The matched member has an enabled button"), MemberButton && MemberButton->IsEnabled())) { return false; }
	MemberButton->SimulateClick();
	TestTrue(TEXT("Following a member clears an excluding search and selects the exact item"),
		Search->GetText().IsEmpty() && List->GetSelectedItems().Num() == 1 && List->GetSelectedItems()[0]->Id == ItemId &&
		FindBrowserText(Browser.ToSharedRef(), TEXT("Compass overview text.")).IsValid());
	MemberButton.Reset();
	Search->SetText(FText::FromString(TEXT(" FAMILY ")));
	if (!TestEqual(TEXT("Group remains searchable after navigation"), List->GetItems().Num(), 1)) { return false; }
	List->SetSelection(List->GetItems()[0]);
	Session->OnCatalogChanged().Broadcast();
	TestTrue(TEXT("Reload notification clears details and preserves active search"),
		List->GetItems().Num() == 1 && Details->GetSelectedObjects().IsEmpty() && List->GetSelectedItems().IsEmpty() &&
		!FindBrowserText(Browser.ToSharedRef(), TEXT("Family overview text.")).IsValid());
	TWeakPtr<SDnd5eItemBrowser> WeakBrowser = Browser;
	List.Reset(); Search.Reset(); Details.Reset(); Browser.Reset();
	TestFalse(TEXT("Releasing browser removes its shared callback"), WeakBrowser.IsValid());
	Session->OnCatalogChanged().Broadcast();
	return !HasAnyErrors();
}

IMPLEMENT_COMPLEX_AUTOMATION_TEST(FDnd5eSrdItemBrowserTest, "Dnd5e.Editor.ItemBrowser.External", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
void FDnd5eSrdItemBrowserTest::GetTests(TArray<FString>& Names, TArray<FString>& Commands) const
{
	FString Root;
	if (FParse::Value(FCommandLine::Get(), TEXT("Dnd5eJsonSrdItemBrowserRoot="), Root))
	{
		Names.Add(TEXT("Srd51"));
		Commands.Add(TEXT("Srd51"));
	}
}

bool FDnd5eSrdItemBrowserTest::RunTest(const FString& Parameters)
{
	FDnd5eJsonItemCatalogLoadRequest Request;
	if (!FParse::Value(FCommandLine::Get(), TEXT("Dnd5eJsonSrdItemBrowserRoot="), Request.DatasetRoot)) { return false; }
	Request.DatasetNamespace = TEXT("srd51-local");
	Request.BaseFiles = {TEXT("items-base-srd51.json")};
	Request.DefinitionFiles = Request.BaseFiles;
	Request.ItemFiles = {TEXT("items-srd51.json")};
	TStrongObjectPtr<UDnd5eEditorItemDatasetSubsystem> Session(NewObject<UDnd5eEditorItemDatasetSubsystem>());
	const auto& Result = Session->LoadSelection(Request);
	if (!TestTrue(TEXT("Load the prepared SRD catalog pair"), Result.Status == EDnd5eJsonReadStatus::Success &&
		Session->GetRecordCount() == 590 && Session->GetDefinitionCount() == 34)) { return false; }
	FDnd5eItemBrowserModel Model;
	Model.Rebuild(Session->GetItemCatalog());
	TestEqual(TEXT("Every SRD record has a browser row"), Model.GetTotalCount(), 590);
	Model.SetSearchText(TEXT("gaming set"));
	TestTrue(TEXT("Search finds the SRD group"), Model.GetRows().ContainsByPredicate([](const auto& Row)
	{
		return Row->Id.Category == Dnd5e::Category::ItemGroup.GetTag() && Row->Name == TEXT("Gaming Set");
	}));
	Model.SetSearchText(TEXT(""));
	TestEqual(TEXT("Clearing search restores all SRD rows"), Model.GetRows().Num(), 590);
	int32 VerifiedEquipmentCards = 0;
	for (const auto& Row : Model.GetRows())
	{
		if (Row->SourceId != TEXT("PHB") || (Row->Name != TEXT("Longsword") && Row->Name != TEXT("Longbow") &&
			Row->Name != TEXT("Leather Armor") && Row->Name != TEXT("Chain Mail"))) { continue; }
		FDnd5eItem Item;
		if (!TestTrue(TEXT("Retrieve SRD card item"), Session->GetItemCatalog()->TryGetItem(Row->Id, Item))) { return false; }
		++VerifiedEquipmentCards;
		const auto Card = Dnd5eItemBrowserDetails::Build(Item);
		const FString Equipment = Card.Equipment.ToString();
		if (Row->Name == TEXT("Longsword"))
		{
			TestTrue(TEXT("Longsword card exposes source, price, weight and both damage expressions"),
				Card.Source.ToString() == TEXT("PHB p. 149") && Card.Facts.ToString().Contains(TEXT("15 gp")) &&
				Card.Facts.ToString().Contains(TEXT("3 lb")) && Equipment == TEXT("1d8 slashing\nVersatile (1d10)"));
		}
		else if (Row->Name == TEXT("Longbow"))
		{
			TestTrue(TEXT("Longbow card shows piercing damage and both ranges"),
				Equipment.Contains(TEXT("1d8 piercing")) && Equipment.Contains(TEXT("150 ft / 600 ft")));
		}
		else if (Row->Name == TEXT("Leather Armor"))
		{
			TestTrue(TEXT("Leather armor shows uncapped Dexterity and its own description"),
				Equipment.Contains(TEXT("11 + Dex modifier")) && Card.Description.ToString().Contains(TEXT("breastplate")));
		}
		else
		{
			TestTrue(TEXT("Chain Mail shows base AC, Strength and stealth"),
				Equipment.Contains(TEXT("AC: 16")) && Equipment.Contains(TEXT("Strength: 13")) &&
				Equipment.Contains(TEXT("Stealth disadvantage")));
		}
	}
	TestEqual(TEXT("All four PHB equipment examples were checked"), VerifiedEquipmentCards, 4);
	FString CardDefinitionsFile;
	if (FParse::Value(FCommandLine::Get(), TEXT("Dnd5eJsonSrdCardDefinitionFile="), CardDefinitionsFile))
	{
		Request.DefinitionFiles = {CardDefinitionsFile};
		if (!TestTrue(TEXT("Local enriched definitions load with the same record counts"),
			Session->LoadSelection(Request).Status == EDnd5eJsonReadStatus::Success &&
			Session->GetRecordCount() == 590 && Session->GetDefinitionCount() == 34)) { return false; }
		for (const auto& Name : {FString(TEXT("Longsword")), FString(TEXT("Cobbler's Tools"))})
		{
			Model.SetSearchText(Name);
			const auto* Row = Model.GetRows().FindByPredicate([&Name](const auto& Candidate)
			{
				return Candidate->Name == Name && Candidate->SourceId == TEXT("PHB");
			});
			FDnd5eItem Item;
			if (!TestTrue(TEXT("Read the exact local card item"), Row &&
				Session->GetItemCatalog()->TryGetItem((*Row)->Id, Item))) { return false; }
			const FString Description = Dnd5eItemBrowserDetails::Build(Item, Session->GetDefinitionCatalog()).Description.ToString();
			const TCHAR* Expected = Name == TEXT("Longsword") ? TEXT("alternate damage die") : TEXT("particular craft or trade");
			TestTrue(TEXT("The local definition contributes its rules summary"), Description.Contains(Expected));
		}
	}
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eItemOverviewValuesTest, "Dnd5e.Editor.ItemBrowser.OverviewValues", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FDnd5eItemOverviewValuesTest::RunTest(const FString& Parameters)
{
	FDnd5eItem Item;
	Item.Header.DisplayName = TEXT("Invented Device");
	Item.Header.Provenance.AddDefaulted_GetRef().SourceId = TEXT("SYN");
	Item.Rarity = Dnd5e::Item::Rarity::VeryRare;
	const auto Empty = Dnd5eItemBrowserDetails::Build(Item);
	TestTrue(TEXT("Title, source and friendly rarity display without absent numeric rows"),
		Empty.Title.ToString() == TEXT("Invented Device") && Empty.Source.ToString() == TEXT("SYN") &&
		Empty.Facts.ToString().Contains(TEXT("Very rare")) && !Empty.Facts.ToString().Contains(TEXT("Price:")) &&
		!Empty.Facts.ToString().Contains(TEXT("Weight:")) && !Empty.Facts.ToString().Contains(TEXT("Charges:")));
	FDnd5eMoney Gold;
	Gold.Currency = EDnd5eCurrency::Gold;
	Gold.Amount.Kind = EDnd5eNumberKind::Integer;
	Gold.Amount.Integer = 5;
	FDnd5eMoney Copper = Gold;
	Copper.Currency = EDnd5eCurrency::Copper;
	Copper.Amount.Integer = 1;
	Item.Price.CostPresence = EDnd5eValuePresence::Present;
	Item.Price.Cost = {Gold, Copper};
	Item.bHasWeight = true;
	Item.Weight.Amount.Kind = EDnd5eNumberKind::Integer;
	Item.Weight.Unit = EDnd5eMassUnit::Pounds;
	Item.ChargeData.Charges.Kind = EDnd5eItemChargeAmountKind::Fixed;
	const FString Fixed = Dnd5eItemBrowserDetails::Build(Item).Facts.ToString();
	TestTrue(TEXT("Money order and zero values are preserved"),
		Fixed.Contains(TEXT("Price: 5 gp, 1 cp")) && Fixed.Contains(TEXT("Weight: 0 lb")) && Fixed.Contains(TEXT("Charges: 0")));
	Item.Price.CostPresence = EDnd5eValuePresence::ExplicitNull;
	Item.ChargeData.Charges.Kind = EDnd5eItemChargeAmountKind::Dice;
	Item.ChargeData.Charges.Dice.Expression = TEXT("1d4 + 1");
	const FString Dice = Dnd5eItemBrowserDetails::Build(Item).Facts.ToString();
	TestTrue(TEXT("Explicit null and dice use their own display"),
		Dice.Contains(TEXT("No listed price")) && !Dice.Contains(TEXT("5 gp")) && Dice.Contains(TEXT("Charges: 1d4 + 1")));
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eItemOverviewContentTest, "Dnd5e.Editor.ItemBrowser.OverviewContent", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FDnd5eItemOverviewContentTest::RunTest(const FString& Parameters)
{
	FDnd5eJsonDocument Document;
	Document.Context.DatasetNamespace = TEXT("overview-test");
	Document.Context.RelativeFilePath = TEXT("preview.json");
	Document.JsonText = TEXT(R"json({"item":[{"name":"Invented Device","source":"SYN","rarity":"rare","entries":[
		{"type":"entries","name":"Setup","entries":["Use {@b the switch} and roll {@dice 1d4}.",
			{"type":"list","items":["First step",{"type":"item","name":"Second step","entries":["Nested body"]}]}]},
		{"type":"table","colLabels":["Column"],"rows":[["Cell"]]}]},
		{"name":"Supplement Fixture","source":"SYN","rarity":"rare","entries":["Supplementary paragraph."]}]})json");
	auto Main = FDnd5eJsonItemReader::ReadAtIndex(Document, 0);
	auto Supplement = FDnd5eJsonItemReader::ReadAtIndex(Document, 1);
	if (!TestTrue(TEXT("Nested source content converts"), Main.Status == EDnd5eJsonReadStatus::Success &&
		Supplement.Status == EDnd5eJsonReadStatus::Success)) { return false; }
	Main.Item.bHasSupplementaryContent = true;
	Main.Item.SupplementaryContent.Entries = MoveTemp(Supplement.Item.Description);
	const FString Description = Dnd5eItemBrowserDetails::Build(Main.Item).Description.ToString();
	TestTrue(TEXT("Preview uses typed inline text and preserves block order"),
		Description.StartsWith(TEXT("Setup\n")) && Description.Contains(TEXT("Use the switch and roll 1d4.")) &&
		Description.Find(TEXT("First step")) != INDEX_NONE && Description.Find(TEXT("Second step")) > Description.Find(TEXT("First step")) &&
		Description.Contains(TEXT("Nested body")) && !Description.Contains(TEXT("{@")));
	TestTrue(TEXT("Unsupported preview blocks defer to typed details and supplementary entries follow"),
		Description.Contains(TEXT("[See typed details for this block]")) && Description.EndsWith(TEXT("Supplementary paragraph.")));
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eItemBrowserLinkStatusTest, "Dnd5e.Editor.ItemBrowser.ReferenceStatuses", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FDnd5eItemBrowserLinkStatusTest::RunTest(const FString& Parameters)
{
	FDnd5eItem First;
	First.Header.RecordId.DatasetNamespace = TEXT("links");
	First.Header.RecordId.Category = Dnd5e::Category::Item;
	First.Header.RecordId.SourceKey = TEXT("first");
	First.Header.DisplayName = TEXT("Invented Blade");
	First.Header.Provenance.AddDefaulted_GetRef().SourceId = TEXT("SYN");
	FDnd5eItem Second = First;
	Second.Header.RecordId.SourceKey = TEXT("second");
	TStrongObjectPtr<UDnd5eItemCatalogObject> Catalog(NewObject<UDnd5eItemCatalogObject>());
	TArray<FDnd5eDiagnostic> Diagnostics;
	if (!TestTrue(TEXT("Install two records with the same authored identity"),
		Catalog->ReplaceRecords({First, Second}, {}, Diagnostics))) { return false; }
	FDnd5eItemGroup Group;
	Group.Details = First;
	auto& Missing = Group.Members.AddDefaulted_GetRef();
	Missing.Name = First.Header.DisplayName;
	auto& Ambiguous = Group.Members.AddDefaulted_GetRef();
	Ambiguous.Name = First.Header.DisplayName;
	Ambiguous.bHasSource = true;
	Ambiguous.SourceId = TEXT("SYN");
	auto& Absent = Group.Members.AddDefaulted_GetRef();
	Absent.Name = TEXT("Unloaded Item");
	Absent.bHasSource = true;
	Absent.SourceId = TEXT("SYN");

	const auto Links = Dnd5eItemBrowserLinks::Build(Group.Details, &Group, Catalog.Get(), nullptr, nullptr);
	if (!TestEqual(TEXT("All authored members stay visible"), Links.Num(), 3)) { return false; }
	TestTrue(TEXT("Omitted source never authorizes a target"), !Links[0].Target.IsSet() && Links[0].Status.ToString() == TEXT("Source omitted"));
	TestTrue(TEXT("Ambiguity never picks a candidate"), !Links[1].Target.IsSet() && Links[1].Status.ToString() == TEXT("Multiple item matches"));
	TestTrue(TEXT("Absent target stays visible"), !Links[2].Target.IsSet() && Links[2].Status.ToString() == TEXT("Target unavailable"));
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eItemDefinitionPreviewTest, "Dnd5e.Editor.ItemBrowser.DefinitionPreviewComposition", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FDnd5eItemDefinitionPreviewTest::RunTest(const FString& Parameters)
{
	FDnd5eJsonDocument Document;
	Document.Context.DatasetNamespace = TEXT("preview");
	Document.Context.RelativeFilePath = TEXT("preview.json");
	Document.JsonText = TEXT(R"({
		"itemType":[{"name":"Tools","abbreviation":"AT","source":"PHB","entries":["Type rules."]}],
		"itemProperty":[
			{"name":"Versatile","abbreviation":"V","source":"PHB","entries":["Classic property rules."]},
			{"name":"Versatile","abbreviation":"V","source":"XPHB","entries":["Modern property rules."]},
			{"name":"Other","abbreviation":"F","source":"PHB","entries":["Unqualified property rules."]}],
		"item":[{"name":"Fixture","source":"PHB","rarity":"none","type":"AT","property":["V","V|PHB","F"],
			"entries":["Direct rules."],"additionalEntries":["Retained source metadata."]},
			{"name":"Supplement","source":"PHB","rarity":"none","entries":["Supplement rules."]}]
	})");
	TStrongObjectPtr<UDnd5eItemDefinitionCatalogObject> Definitions(NewObject<UDnd5eItemDefinitionCatalogObject>());
	TArray<FDnd5eDiagnostic> Diagnostics;
	auto Main = FDnd5eJsonItemReader::ReadAtIndex(Document, 0);
	auto Supplement = FDnd5eJsonItemReader::ReadAtIndex(Document, 1);
	if (!TestTrue(TEXT("Fixture definitions and typed items load"),
		UDnd5eJsonItemDefinitionCatalogLibrary::BuildItemDefinitionCatalog(Definitions.Get(), Document, Diagnostics) &&
		Main.Status == EDnd5eJsonReadStatus::Success && Supplement.Status == EDnd5eJsonReadStatus::Success &&
		Main.Item.Properties.Num() == 3)) { return false; }

	auto& Item = Main.Item;
	Item.bHasSupplementaryContent = true;
	Item.SupplementaryContent.Entries = MoveTemp(Supplement.Item.Description);
	const FString Composed = Dnd5eItemBrowserDetails::Build(Item, Definitions.Get()).Description.ToString();
	TestEqual(TEXT("Composition preserves order and includes each exact property once"), Composed,
		FString(TEXT("Direct rules.\n\nType rules.\n\nClassic property rules.\n\nSupplement rules.")));
	TestTrue(TEXT("Default presentation does not rewrite the authored selectors"), !Item.Type.bHasSource && !Item.Properties[0].bHasSource);

	Item.Properties[0].bHasSource = true;
	Item.Properties[0].SourceId = TEXT("XPHB");
	Item.Properties.RemoveAt(1);
	const FString Explicit = Dnd5eItemBrowserDetails::Build(Item, Definitions.Get()).Description.ToString();
	TestTrue(TEXT("Explicit source selects its exact definition"),
		Explicit.Contains(TEXT("Modern property rules.")) && !Explicit.Contains(TEXT("Classic property rules.")));
	Item.Header.RecordId.DatasetNamespace = TEXT("other");
	const FString Isolated = Dnd5eItemBrowserDetails::Build(Item, Definitions.Get()).Description.ToString();
	TestEqual(TEXT("An unrelated namespace cannot contribute definition text"), Isolated,
		FString(TEXT("Direct rules.\n\nSupplement rules.")));
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eSpellCardTest, "Dnd5e.Editor.SpellBrowser.StatCard", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FDnd5eSpellCardTest::RunTest(const FString& Parameters)
{
	FDnd5eJsonSpellReadRequest Request;
	Request.Context.DatasetNamespace = TEXT("spell-card");
	Request.Context.RelativeFilePath = TEXT("spells.json");
	Request.SpellIndex = 0;
	Request.JsonText = TEXT(R"json({"spell":[{"name":"Invented Beacon","source":"SYN","page":12,"level":1,"school":"V",
		"time":[{"number":1,"unit":"action"},{"number":1,"unit":"reaction","condition":"when the lamp dims"}],
		"range":{"type":"cone","distance":{"type":"feet","amount":15}},
		"components":{"v":true,"s":true,"m":{"text":"a crystal","cost":100,"consume":true}},
		"duration":[{"type":"timed","duration":{"type":"minute","amount":1,"upTo":true},"concentration":true},{"type":"instant"}],
		"entries":["The beacon deals {@damage 1d6} radiant damage."],
		"entriesHigherLevel":[{"type":"entries","name":"At Higher Levels","entries":["The light grows brighter."]}]}]})json");
	const auto Read = FDnd5eJsonSpellReader::ReadSpell(Request);
	if (!TestTrue(TEXT("Read fixture through production parser"), Read.Status == EDnd5eJsonReadStatus::Success)) { return false; }
	const auto Card = Dnd5eSpellBrowserDetails::Build(Read.Spell);
	const FString Mechanics = Card.Mechanics.ToString();
	TestTrue(TEXT("Header shows authored source and spell classification"), Card.Source.ToString() == TEXT("SYN p. 12") &&
		Card.Classification.ToString() == TEXT("Level 1 Evocation"));
	TestTrue(TEXT("Casting options and a self-origin area are readable"),
		Mechanics.Contains(TEXT("1 action / 1 reaction")) && Mechanics.Contains(TEXT("when the lamp dims")) &&
		Mechanics.Contains(TEXT("Self (15 ft cone)")));
	TestTrue(TEXT("Materials keep their typed cost and consumption"),
		Mechanics.Contains(TEXT("V, S, M (a crystal | Cost: 100 cp | consumed)")));
	TestTrue(TEXT("Concentration applies only to its own duration option"),
		Mechanics.Contains(TEXT("Concentration, up to 1 minute / Instantaneous")));
	const FString Rules = Card.Description.ToString();
	TestTrue(TEXT("Damage displays in context and higher-level rules follow"),
		Rules.StartsWith(TEXT("The beacon deals 1d6 radiant damage.")) && Rules.Contains(TEXT("At Higher Levels")) &&
		Rules.EndsWith(TEXT("The light grows brighter.")));
	return !HasAnyErrors();
}

#endif
