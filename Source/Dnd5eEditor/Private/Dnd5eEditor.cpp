#include "Dnd5eEditor.h"
#include "Editor.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Commands/UIAction.h"
#include "ISettingsModule.h"
#include "MessageLogModule.h"
#include "Logging/MessageLog.h"
#include "Settings/Dnd5eEditorDatasetSettings.h"
#include "Subsystems/Dnd5eEditorDatasetSubsystem.h"
#include "Textures/SlateIcon.h"
#include "ToolMenu.h"
#include "ToolMenuSection.h"
#include "Browser/SDnd5eSpellBrowser.h"
#include "Framework/Docking/TabManager.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/SNullWidget.h"
#include "ToolMenus.h"

#define LOCTEXT_NAMESPACE "Dnd5eEditor"

namespace
{
    const FName DatasetLogName(TEXT("Dnd5eDataset"));
    const FName SpellBrowserTabName(TEXT("Dnd5eSpellBrowser"));
}

void FDnd5eEditorModule::StartupModule()
{
    if (IsRunningCommandlet() || !FSlateApplication::IsInitialized()) { return; }
    FMessageLogModule& MessageLog = FModuleManager::LoadModuleChecked<FMessageLogModule>("MessageLog");
    FGlobalTabmanager::Get()->RegisterNomadTabSpawner(SpellBrowserTabName, FOnSpawnTab::CreateRaw(this, &FDnd5eEditorModule::SpawnSpellBrowser))
        .SetDisplayName(LOCTEXT("BrowserTabTitle", "Dnd5e Spell Browser"))
        .SetMenuType(ETabSpawnerMenuType::Hidden);
    FMessageLogInitializationOptions Options;
    Options.bShowPages = true;
    Options.bAllowClear = true;
    MessageLog.RegisterLogListing(DatasetLogName, LOCTEXT("DatasetLog", "Dnd5e Dataset"), Options);
    UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FDnd5eEditorModule::RegisterMenus));
    bEditorUiRegistered = true;
}

void FDnd5eEditorModule::ShutdownModule()
{
    if (!bEditorUiRegistered) { return; }
    if (FSlateApplication::IsInitialized())
    {
        if (TSharedPtr<SDockTab> Tab = SpellBrowserTab.Pin())
        {
            Tab->SetContent(SNullWidget::NullWidget);
            Tab->RequestCloseTab();
        }
        FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(SpellBrowserTabName);
    }
    SpellBrowserTab.Reset();
    if (FModuleManager::Get().IsModuleLoaded("ToolMenus"))
    {
        UToolMenus::UnRegisterStartupCallback(this);
        UToolMenus::UnregisterOwner(this);
    }
    if (FMessageLogModule* MessageLog = FModuleManager::GetModulePtr<FMessageLogModule>("MessageLog"))
    {
        MessageLog->UnregisterLogListing(DatasetLogName);
    }
    bEditorUiRegistered = false;
}

void FDnd5eEditorModule::RegisterMenus()
{
    FToolMenuOwnerScoped Owner(this);
    UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Tools");
    FToolMenuSection& Section = Menu->FindOrAddSection("Dnd5e");
    Section.AddMenuEntry("Dnd5eDatasetSettings", LOCTEXT("Settings", "Dnd5e Dataset Settings"), LOCTEXT("SettingsTip", "Choose external dataset files and remember this local selection."), FSlateIcon(), FUIAction(FExecuteAction::CreateRaw(this, &FDnd5eEditorModule::OpenDatasetSettings)));
    Section.AddMenuEntry("Dnd5eLoadDataset", LOCTEXT("Load", "Load Dnd5e Dataset"), LOCTEXT("LoadTip", "Load the selected files into the editor catalog and show diagnostics."), FSlateIcon(), FUIAction(FExecuteAction::CreateRaw(this, &FDnd5eEditorModule::LoadDataset)));
    Section.AddMenuEntry("Dnd5eSpellBrowser", LOCTEXT("Browser", "Dnd5e Spell Browser"), LOCTEXT("BrowserTip", "Search and inspect spells in the current editor catalog."), FSlateIcon(), FUIAction(FExecuteAction::CreateRaw(this, &FDnd5eEditorModule::OpenSpellBrowser)));
}

void FDnd5eEditorModule::OpenDatasetSettings()
{
    const auto* Settings = GetDefault<UDnd5eEditorDatasetSettings>();
    FModuleManager::LoadModuleChecked<ISettingsModule>("Settings").ShowViewer(Settings->GetContainerName(), Settings->GetCategoryName(), Settings->GetSectionName());
}

void FDnd5eEditorModule::LoadDataset()
{
    FMessageLog Log(DatasetLogName);
    Log.SuppressLoggingToOutputLog();
    Log.NewPage(LOCTEXT("LoadPage", "Dataset load"));
    auto* Subsystem = GEditor ? GEditor->GetEditorSubsystem<UDnd5eEditorDatasetSubsystem>() : nullptr;
    if (!Subsystem)
    {
        Log.Error(LOCTEXT("Unavailable", "The editor dataset service is unavailable."));
        Log.Open(EMessageSeverity::Info, true);
        return;
    }
    auto* Settings = GetMutableDefault<UDnd5eEditorDatasetSettings>();
    Settings->SaveConfig();
    const auto Request = Settings->MakeLoadRequest();
    const auto& Result = Subsystem->LoadSelection(Request);
    if (Result.Status == EDnd5eJsonReadStatus::Success)
    {
        Log.Info(FText::Format(LOCTEXT("Loaded", "Loaded {0} spell records. Diagnostics: {1}."), FText::AsNumber(Result.SpellCount), FText::AsNumber(Result.Diagnostics.Num())));
    }
    else
    {
        Log.Error(FText::Format(LOCTEXT("Failed", "Load failed. The active editor catalog still contains {0} spell records."), FText::AsNumber(Subsystem->GetSpellCount())));
    }
    for (const FDnd5eDiagnostic& Diagnostic : Result.Diagnostics)
    {
        FString Message = FString::Printf(TEXT("[%s] %s"), *Diagnostic.Code.ToString(), *Diagnostic.Message);
        if (!Diagnostic.RelativeFilePath.IsEmpty()) { Message += TEXT(" | ") + Diagnostic.RelativeFilePath; }
        if (!Diagnostic.JsonPointer.IsEmpty()) { Message += TEXT(" ") + Diagnostic.JsonPointer; }
        const FText Text = FText::FromString(Message);
        if (Diagnostic.Severity == EDnd5eDiagnosticSeverity::Error) { Log.Error(Text); }
        else if (Diagnostic.Severity == EDnd5eDiagnosticSeverity::Warning) { Log.Warning(Text); }
        else { Log.Info(Text); }
    }
    Log.Open(EMessageSeverity::Info, true);
}

void FDnd5eEditorModule::OpenSpellBrowser()
{
    FGlobalTabmanager::Get()->TryInvokeTab(SpellBrowserTabName);
}

TSharedRef<SDockTab> FDnd5eEditorModule::SpawnSpellBrowser(const FSpawnTabArgs& Args)
{
    TSharedRef<SDockTab> Tab = SNew(SDockTab).TabRole(ETabRole::NomadTab)
    [
        SNew(SDnd5eSpellBrowser)
    ];
    SpellBrowserTab = Tab;
    return Tab;
}

IMPLEMENT_MODULE(FDnd5eEditorModule, Dnd5eEditor)

#undef LOCTEXT_NAMESPACE