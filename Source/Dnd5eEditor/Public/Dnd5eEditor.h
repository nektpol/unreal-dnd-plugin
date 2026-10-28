#pragma once

#include "Modules/ModuleManager.h"

class FDnd5eEditorModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
