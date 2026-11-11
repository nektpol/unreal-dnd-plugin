#pragma once

#include "Modules/ModuleManager.h"

class FSpawnTabArgs;
class SDockTab;
struct FDnd5eRecordId;
class SDnd5eSpellBrowser;

class FDnd5eEditorModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	bool OpenSpellById(const FDnd5eRecordId& Id);

private:
	void RegisterMenus();
	void OpenDatasetSettings();
	void LoadDataset();
	void OpenItemDatasetSettings();
	void LoadItemDataset();
	void OpenSpellBrowser();
	TSharedRef<SDockTab> SpawnSpellBrowser(const FSpawnTabArgs& Args);
	TWeakPtr<SDockTab> SpellBrowserTab;
	TWeakPtr<SDnd5eSpellBrowser> SpellBrowserWidget;
	void OpenItemBrowser();
	TSharedRef<SDockTab> SpawnItemBrowser(const FSpawnTabArgs& Args);
	TWeakPtr<SDockTab> ItemBrowserTab;
	bool bEditorUiRegistered = false;
};