#pragma once

#include "Modules/ModuleManager.h"

class FSpawnTabArgs;
class SDockTab;

class FDnd5eEditorModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	void RegisterMenus();
	void OpenDatasetSettings();
	void LoadDataset();
	void OpenItemDatasetSettings();
	void LoadItemDataset();
	void OpenSpellBrowser();
	TSharedRef<SDockTab> SpawnSpellBrowser(const FSpawnTabArgs& Args);
	TWeakPtr<SDockTab> SpellBrowserTab;
	bool bEditorUiRegistered = false;
};