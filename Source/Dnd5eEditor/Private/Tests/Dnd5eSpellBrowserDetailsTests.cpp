#if WITH_DEV_AUTOMATION_TESTS

#include "Dnd5eTestSupport.h"
#include "Browser/Dnd5eSpellBrowserSelection.h"
#include "DetailsViewArgs.h"
#include "Framework/Application/SlateApplication.h"
#include "IDetailsView.h"
#include "Modules/ModuleManager.h"
#include "PropertyEditorModule.h"
#include "PropertyPath.h"
#include "UObject/GarbageCollection.h"
#include "UObject/StrongObjectPtr.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eSpellBrowserDetailsTest, "Dnd5e.Editor.SpellBrowser.DetailsSelectionLifecycle", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FDnd5eSpellBrowserDetailsTest::RunTest(const FString& Parameters)
{
	if (!TestTrue(TEXT("Slate is initialized"), FSlateApplication::IsInitialized())) { return false; }
	FDetailsViewArgs Args;
	Args.bUpdatesFromSelection = false;
	Args.bLockable = false;
	Args.bAllowSearch = true;
	Args.bHideSelectionTip = true;
	Args.NameAreaSettings = FDetailsViewArgs::HideNameArea;
	auto& PropertyEditor = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	TSharedRef<IDetailsView> View = PropertyEditor.CreateDetailView(Args);
	View->SetIsPropertyEditingEnabledDelegate(FIsPropertyEditingEnabled::CreateLambda([]() { return false; }));
	TestFalse(TEXT("Details editing is disabled"), View->GetIsPropertyEditingEnabledDelegate().Execute());

	auto TickDetails = [&View]()
	{
		for (int32 Frame = 0; Frame < 3; ++Frame) { View->Tick(FGeometry(), FPlatformTime::Seconds(), 1.0f / 60.0f); }
	};
	TickDetails();
	TStrongObjectPtr<UDnd5eSpellBrowserSelection> SelectedData;
	for (int32 Selection = 0; Selection < 8; ++Selection)
	{
		View->SetObject(nullptr);
		SelectedData.Reset();
		FDnd5eSpell Spell = Dnd5eTests::SyntheticSpell();
		Spell.Header.DisplayName = FString::Printf(TEXT("Invented selection %d"), Selection);
		SelectedData.Reset(NewObject<UDnd5eSpellBrowserSelection>());
		SelectedData->Spell = MoveTemp(Spell);
		View->SetObject(SelectedData.Get());
		// The struct-details path crashed here for a single nested paragraph in UE 5.8.4.
		// Exercise actual PropertyEditor updates, rather than merely constructing its widget.
		TickDetails();
		TestEqual(TEXT("The selected value survives details updates"), SelectedData->Spell.Header.DisplayName, FString::Printf(TEXT("Invented selection %d"), Selection));
		TestFalse(TEXT("The reflected detail tree is populated"), View->GetPropertiesInOrderDisplayed().IsEmpty());
		if (Selection == 3)
		{
			TWeakObjectPtr<UDnd5eSpellBrowserSelection> WeakSelection = SelectedData.Get();
			CollectGarbage(RF_NoFlags);
			TestTrue(TEXT("Slate's strong selection holder survives garbage collection"), WeakSelection.IsValid());
			TickDetails();
		}
	}
	View->SetObject(nullptr);
	SelectedData.Reset();
	TickDetails();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eBrowserReportSchemaTest, "Dnd5e.Editor.SpellBrowser.ReportSelectionSchema", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FDnd5eBrowserReportSchemaTest::RunTest(const FString& Parameters)
{
	auto* Type = UDnd5eSpellBrowserSelection::StaticClass();
	TestEqual(TEXT("Private editor owns wrapper"), Type->GetOutermost()->GetName(), FString(TEXT("/Script/Dnd5eEditor")));
	TestTrue(TEXT("Selection class remains transient"), Type->HasAnyClassFlags(CLASS_Transient));
	int32 Count = 0;
	for (TFieldIterator<FProperty> It(Type, EFieldIterationFlags::None); It; ++It) { ++Count; }
	TestEqual(TEXT("Exactly spell and report fields"), Count, 2);
	const auto* Report = FindFProperty<FStructProperty>(Type, TEXT("ReferenceReport"));
	if (!TestNotNull(TEXT("Reflected report"), Report)) { return false; }
	TestTrue(TEXT("Correct report type"), Report->Struct == FDnd5eJsonSpellReferenceReport::StaticStruct());
	TestTrue(TEXT("Visible, noneditable, transient report"), Report->HasAllPropertyFlags(CPF_Edit | CPF_EditConst | CPF_Transient));
	TestFalse(TEXT("Report is not config or save-game data"), Report->HasAnyPropertyFlags(CPF_Config | CPF_SaveGame));
	TStrongObjectPtr<UDnd5eSpellBrowserSelection> Selection(NewObject<UDnd5eSpellBrowserSelection>());
	TestTrue(TEXT("Fresh selection has default report"), !Selection->ReferenceReport.bCollected && Selection->ReferenceReport.Entries.IsEmpty() && Selection->ReferenceReport.Diagnostics.IsEmpty());
	return !HasAnyErrors();
}

#endif
