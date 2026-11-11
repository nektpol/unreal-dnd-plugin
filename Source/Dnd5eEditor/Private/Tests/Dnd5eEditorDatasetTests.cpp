#if WITH_DEV_AUTOMATION_TESTS

#include "Dnd5eTestSupport.h"
#include "Dnd5eEditor.h"
#include "Settings/Dnd5eEditorDatasetSettings.h"
#include "Settings/Dnd5eEditorItemDatasetSettings.h"
#include "Subsystems/Dnd5eEditorDatasetSubsystem.h"
#include "Subsystems/Dnd5eEditorItemDatasetSubsystem.h"
#include "Catalog/Dnd5eSpellCatalogObject.h"
#include "Framework/Application/SlateApplication.h"
#include "HAL/FileManager.h"
#include "ISettingsModule.h"
#include "ISettingsContainer.h"
#include "ISettingsCategory.h"
#include "ISettingsSection.h"
#include "MessageLogModule.h"
#include "Misc/App.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/FileHelper.h"
#include "Misc/Guid.h"
#include "Misc/Paths.h"
#include "ToolMenu.h"
#include "ToolMenuSection.h"
#include "ToolMenus.h"
#include "UObject/GarbageCollection.h"
#include "UObject/StrongObjectPtr.h"

using namespace Dnd5eTests;

namespace
{
	struct FEditorDatasetFiles
	{
		FString Base = FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Automation/Dnd5eEditorFixtures")));
		FString Root = FPaths::Combine(Base, FGuid::NewGuid().ToString(EGuidFormats::Digits));

		~FEditorDatasetFiles()
		{
			FString Resolved = FPaths::ConvertRelativePathToFull(Root);
			if (FPaths::CollapseRelativeDirectories(Resolved) && FPaths::IsUnderDirectory(Resolved, Base) && !FPaths::IsSamePath(Resolved, Base))
			{
				IFileManager::Get().DeleteDirectory(*Resolved, false, true);
			}
		}

		bool WriteSpell() const
		{
			const FString Directory = FPaths::Combine(Root, TEXT("spells"));
			if (!IFileManager::Get().MakeDirectory(*Directory, true)) { return false; }
			const FString Text = TEXT(R"json({"spell":[{
				"name":"Invented Compass","source":"SYN","level":0,"school":"V",
				"time":[{"number":1,"unit":"action"}],"range":{"type":"point","distance":{"type":"self"}},
				"components":{"v":true},"duration":[{"type":"instant"}],"entries":["An invented compass produces light."]
			}]})json");
			return FFileHelper::SaveStringToFile(Text, *FPaths::Combine(Directory, TEXT("synthetic.json")), FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
		}

		FDnd5eJsonSpellCatalogLoadRequest Request() const
		{
			FDnd5eJsonSpellCatalogLoadRequest Result;
			Result.DatasetRoot = Root;
			Result.DatasetNamespace = TEXT("editor-synthetic");
			Result.SpellFiles = {TEXT("spells/synthetic.json")};
			return Result;
		}
	};

	void ResetSelection(UDnd5eEditorDatasetSettings& Settings)
	{
		// Instances can inherit real user config from their CDO; overwrite the fixture, never the CDO.
		Settings.DatasetRoot.Path.Reset();
		Settings.DatasetNamespace.Reset();
		Settings.SpellFiles.Reset();
		Settings.bHasAvailabilityLookup = false;
		Settings.AvailabilityFile.FilePath.Reset();
	}

	void AddPickerFile(UDnd5eEditorDatasetSettings& Settings, const FString& Filename)
	{
		FFilePath File;
		File.FilePath = Filename;
		Settings.SpellFiles.Add(MoveTemp(File));
	}

	TArray<FString> SelectionSnapshot(const UDnd5eEditorDatasetSettings& Settings)
	{
		TArray<FString> Result = {Settings.DatasetRoot.Path, Settings.DatasetNamespace, Settings.bHasAvailabilityLookup ? TEXT("true") : TEXT("false"), Settings.AvailabilityFile.FilePath};
		for (const auto& File : Settings.SpellFiles) { Result.Add(File.FilePath); }
		return Result;
	}

	bool SameSelection(const TArray<FString>& A, const TArray<FString>& B)
	{
		if (A.Num() != B.Num()) { return false; }
		for (int32 Index = 0; Index < A.Num(); ++Index) { if (!A[Index].Equals(B[Index], ESearchCase::CaseSensitive)) { return false; } }
		return true;
	}

	FString FieldType(const FProperty& Property)
	{
		FString Extended;
		const FString Type = Property.GetCPPType(&Extended);
		return Type + Extended;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eEditorDatasetSchemaTest, "Dnd5e.Editor.Dataset.Schema", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FDnd5eEditorDatasetSchemaTest::RunTest(const FString& Parameters)
{
	UClass* SettingsClass = UDnd5eEditorDatasetSettings::StaticClass();
	TestEqual(TEXT("Settings live in the editor module"), SettingsClass->GetOutermost()->GetName(), FString(TEXT("/Script/Dnd5eEditor")));
	TestTrue(TEXT("Settings derive from UDeveloperSettings"), SettingsClass->GetSuperClass() == UDeveloperSettings::StaticClass());
	TestEqual(TEXT("Settings use per-project user config"), SettingsClass->ClassConfigName, FName(TEXT("EditorPerProjectUserSettings")));
	TestFalse(TEXT("Settings cannot default to a shared DefaultConfig destination"), SettingsClass->HasAnyClassFlags(CLASS_DefaultConfig));
	const TPair<const TCHAR*, const TCHAR*> Fields[] = {
		{TEXT("DatasetRoot"), TEXT("FDirectoryPath")}, {TEXT("DatasetNamespace"), TEXT("FString")},
		{TEXT("SpellFiles"), TEXT("TArray<FFilePath>")}, {TEXT("bHasAvailabilityLookup"), TEXT("bool")}, {TEXT("AvailabilityFile"), TEXT("FFilePath")}};
	int32 OwnFields = 0;
	for (TFieldIterator<FProperty> It(SettingsClass); It; ++It) { if (It->GetOwnerStruct() == SettingsClass) { ++OwnFields; } }
	TestEqual(TEXT("Exactly five editor selection fields"), OwnFields, 5);
	for (const auto& Field : Fields)
	{
		const FProperty* Property = FindFProperty<FProperty>(SettingsClass, Field.Key);
		if (!TestNotNull(Field.Key, Property)) { continue; }
		TestEqual(Field.Key, FieldType(*Property), FString(Field.Value));
		TestTrue(TEXT("Selection is editable config data"), Property->HasAllPropertyFlags(CPF_Edit | CPF_Config));
		TestFalse(TEXT("Selection is not a transient runtime result"), Property->HasAnyPropertyFlags(CPF_Transient));
	}
	const auto* FilesProperty = FindFProperty<FProperty>(SettingsClass, TEXT("SpellFiles"));
	if (FilesProperty) { TestEqual(TEXT("Picker filter is JSON"), FilesProperty->GetMetaData(TEXT("FilePathFilter")), FString(TEXT("json"))); }
	const auto* Settings = GetDefault<UDnd5eEditorDatasetSettings>();
	TestTrue(TEXT("Stable settings viewer location"), Settings->GetContainerName() == TEXT("Editor") && Settings->GetCategoryName() == TEXT("General") && Settings->GetSectionName() == TEXT("Dnd5eDataset"));
	TestTrue(TEXT("Settings support automatic registration"), Settings->SupportsAutoRegistration());
	UClass* SubsystemClass = UDnd5eEditorDatasetSubsystem::StaticClass();
	TestEqual(TEXT("Catalog owner lives in the editor module"), SubsystemClass->GetOutermost()->GetName(), FString(TEXT("/Script/Dnd5eEditor")));
	TestTrue(TEXT("Owner is an editor subsystem"), SubsystemClass->GetSuperClass() == UEditorSubsystem::StaticClass() && SubsystemClass->HasAnyClassFlags(CLASS_Transient));
	for (const TCHAR* Name : {TEXT("Catalog"), TEXT("LastLoadResult")})
	{
		const auto* Property = FindFProperty<FProperty>(SubsystemClass, Name);
		if (!TestNotNull(Name, Property)) { continue; }
		TestTrue(TEXT("Session state is transient and never config"), Property->HasAnyPropertyFlags(CPF_Transient) && !Property->HasAnyPropertyFlags(CPF_Config));
	}
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eEditorDatasetConversionTest, "Dnd5e.Editor.Dataset.PathConversion", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FDnd5eEditorDatasetConversionTest::RunTest(const FString& Parameters)
{
	FEditorDatasetFiles Files;
	if (!TestTrue(TEXT("Create synthetic input"), Files.WriteSpell())) { return false; }
	TStrongObjectPtr<UDnd5eEditorDatasetSettings> Settings(NewObject<UDnd5eEditorDatasetSettings>());
	ResetSelection(*Settings);
	Settings->DatasetRoot.Path = Files.Root;
	Settings->DatasetNamespace = TEXT("editor-synthetic");
	AddPickerFile(*Settings, FPaths::Combine(Files.Root, TEXT("spells/synthetic.json")));
	AddPickerFile(*Settings, TEXT("spells/already-relative.json"));
	Settings->AvailabilityFile.FilePath = TEXT("../inactive.json");
	const auto Before = SelectionSnapshot(*Settings);
	auto Request = Settings->MakeLoadRequest();
	TestTrue(TEXT("Namespace and root are explicitly preserved"), Request.DatasetRoot.Equals(Files.Root, ESearchCase::CaseSensitive) && Request.DatasetNamespace == TEXT("editor-synthetic"));
	TestTrue(TEXT("Picker conversion preserves order"), Request.SpellFiles.Num() == 2 && Request.SpellFiles[0] == TEXT("spells/synthetic.json") && Request.SpellFiles[1] == TEXT("spells/already-relative.json"));
	TestTrue(TEXT("Inactive lookup is omitted from the request"), !Request.bHasAvailabilityLookup && Request.AvailabilityFile.IsEmpty());
	TestTrue(TEXT("Conversion leaves remembered picker values unchanged"), SameSelection(Before, SelectionSnapshot(*Settings)));
	Settings->bHasAvailabilityLookup = true;
	Settings->AvailabilityFile.FilePath = FPaths::Combine(Files.Root, TEXT("generated/lookup.json"));
	Request = Settings->MakeLoadRequest();
	TestTrue(TEXT("Active lookup becomes dataset-relative"), Request.bHasAvailabilityLookup && Request.AvailabilityFile == TEXT("generated/lookup.json"));
	Settings->bHasAvailabilityLookup = false;
	Settings->SpellFiles.Reset();
	AddPickerFile(*Settings, FPaths::Combine(Files.Base, TEXT("outside.json")));
	Request = Settings->MakeLoadRequest();
	FDnd5eSpellCatalog Catalog;
	const auto Rejected = FDnd5eJsonSpellCatalogLoader::LoadCatalog(Request, Catalog);
	TestTrue(TEXT("Outside-root picker selection remains rejectable, not omitted"), Request.SpellFiles.Num() == 1 && Rejected.Status == EDnd5eJsonReadStatus::Failure && Rejected.Diagnostics.ContainsByPredicate([](const auto& Value) { return Value.Code == FName(TEXT("Dnd5e.Json.InvalidContext")); }));
#if PLATFORM_WINDOWS
	Settings->SpellFiles.Reset();
	const TCHAR OtherDrive = FChar::ToUpper(Files.Root[0]) == TEXT('Z') ? TEXT('Y') : TEXT('Z');
	AddPickerFile(*Settings, FString::Printf(TEXT("%c:/not-opened/synthetic.json"), OtherDrive));
	Request = Settings->MakeLoadRequest();
	TestTrue(TEXT("A different drive remains an absolute rejected selection"), Request.SpellFiles.Num() == 1 && !FPaths::IsRelative(Request.SpellFiles[0]) && FDnd5eJsonSpellCatalogLoader::LoadCatalog(Request, Catalog).Status == EDnd5eJsonReadStatus::Failure);
#endif
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eEditorDatasetConfigTest, "Dnd5e.Editor.Dataset.ConfigIsolation", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FDnd5eEditorDatasetConfigTest::RunTest(const FString& Parameters)
{
	FEditorDatasetFiles Files;
	if (!TestTrue(TEXT("Create isolated fixture directory"), Files.WriteSpell())) { return false; }
	const auto DefaultBefore = SelectionSnapshot(*GetDefault<UDnd5eEditorDatasetSettings>());
	const bool bUserFileExisted = FPaths::FileExists(GEditorPerProjectIni);
	TArray<uint8> UserBefore;
	if (bUserFileExisted && !TestTrue(TEXT("Snapshot existing user config privately"), FFileHelper::LoadFileToArray(UserBefore, *GEditorPerProjectIni))) { return false; }
	TStrongObjectPtr<UDnd5eEditorDatasetSettings> Source(NewObject<UDnd5eEditorDatasetSettings>());
	ResetSelection(*Source);
	Source->DatasetRoot.Path = Files.Root;
	Source->DatasetNamespace = TEXT("config-fixture Ω");
	AddPickerFile(*Source, FPaths::Combine(Files.Root, TEXT("spells/synthetic.json")));
	Source->bHasAvailabilityLookup = true;
	Source->AvailabilityFile.FilePath = FPaths::Combine(Files.Root, TEXT("generated/lookup.json"));
	const auto Expected = SelectionSnapshot(*Source);
	const FString Ini = FPaths::Combine(Files.Root, TEXT("isolated-settings.ini"));
	// Explicit filename AND no copy to the real CDO: SaveConfig defaults would otherwise mutate user state.
	Source->SaveConfig(CPF_Config, *Ini, GConfig, false);
	GConfig->Flush(false, *Ini);
	FString Saved;
	if (!TestTrue(TEXT("Synthetic config exists on disk"), FFileHelper::LoadFileToString(Saved, *Ini))) { return false; }
	TestTrue(TEXT("Only selection, never loaded records/results, is saved"), Saved.Contains(TEXT("DatasetNamespace")) && Saved.Contains(TEXT("SpellFiles")) && !Saved.Contains(TEXT("LastLoadResult=")) && !Saved.Contains(TEXT("Catalog=")));
	GConfig->UnloadFile(Ini);
	TStrongObjectPtr<UDnd5eEditorDatasetSettings> Reloaded(NewObject<UDnd5eEditorDatasetSettings>());
	ResetSelection(*Reloaded);
	Reloaded->LoadConfig(UDnd5eEditorDatasetSettings::StaticClass(), *Ini, 0);
	TestTrue(TEXT("All five properties reload from the explicit temporary file"), SameSelection(Expected, SelectionSnapshot(*Reloaded)));
	TestTrue(TEXT("Real settings default object remains unchanged"), SameSelection(DefaultBefore, SelectionSnapshot(*GetDefault<UDnd5eEditorDatasetSettings>())));
	TestEqual(TEXT("Real config file presence remains unchanged"), FPaths::FileExists(GEditorPerProjectIni), bUserFileExisted);
	if (bUserFileExisted)
	{
		TArray<uint8> UserAfter;
		if (!TestTrue(TEXT("Read original user config privately after test"), FFileHelper::LoadFileToArray(UserAfter, *GEditorPerProjectIni))) { return false; }
		TestTrue(TEXT("Real user config bytes remain unchanged"), UserBefore == UserAfter);
	}
	GConfig->UnloadFile(Ini);
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eEditorDatasetSessionTest, "Dnd5e.Editor.Dataset.SessionAndAtomicLoading", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FDnd5eEditorDatasetSessionTest::RunTest(const FString& Parameters)
{
	FEditorDatasetFiles Files;
	if (!TestTrue(TEXT("Create synthetic input"), Files.WriteSpell())) { return false; }
	TStrongObjectPtr<UDnd5eEditorDatasetSubsystem> Session(NewObject<UDnd5eEditorDatasetSubsystem>());
	TStrongObjectPtr<UDnd5eEditorDatasetSubsystem> Other(NewObject<UDnd5eEditorDatasetSubsystem>());
	TestTrue(TEXT("Session starts unloaded without dataset reads"), Session->GetCatalog() == nullptr && Session->GetSpellCount() == 0 && Session->GetLastLoadResult().Status == EDnd5eJsonReadStatus::NotAttempted);
	const auto Request = Files.Request();
	const auto Loaded = Session->LoadSelection(Request);
	if (!TestTrue(TEXT("Explicit selection installs the synthetic collection"), Loaded.Status == EDnd5eJsonReadStatus::Success && Loaded.SpellCount == 1 && Session->GetSpellCount() == 1)) { return false; }
	UDnd5eSpellCatalogObject* Original = Session->GetCatalog();
	const auto Ids = Original->GetSpellIds();
	TestTrue(TEXT("Session owns its own catalog object"), Original->GetOuter() == Session.Get() && Other->GetCatalog() == nullptr);
	auto Missing = Request; Missing.SpellFiles[0] = TEXT("spells/missing.json");
	const auto Failed = Session->LoadSelection(Missing);
	TestTrue(TEXT("Failure result is preserved separately from active count"), Failed.Status == EDnd5eJsonReadStatus::Failure && Failed.SpellCount == 0 && Session->GetSpellCount() == 1 && Session->GetLastLoadResult().Status == EDnd5eJsonReadStatus::Failure);
	TestTrue(TEXT("Failure keeps the same owner object and IDs"), Session->GetCatalog() == Original && Session->GetCatalog()->GetSpellIds() == Ids);
	TestTrue(TEXT("The complete diagnostic remains available to UI"), Failed.Diagnostics.Num() == 1 && Session->GetLastLoadResult().Diagnostics.Num() == 1 && Failed.Diagnostics[0].Code == FName(TEXT("Dnd5e.Json.Load.FileReadFailed")) && Failed.Diagnostics[0].RelativeFilePath == TEXT("spells/missing.json"));
	const auto BeforeSettings = SelectionSnapshot(*GetDefault<UDnd5eEditorDatasetSettings>());
	Session->LoadSelection(Request);
	TestTrue(TEXT("Loading an explicit request never rewrites remembered selection"), SameSelection(BeforeSettings, SelectionSnapshot(*GetDefault<UDnd5eEditorDatasetSettings>())));
	TestTrue(TEXT("Successful retry uses the same catalog and replaces last failure"), Session->GetCatalog() == Original && Session->GetLastLoadResult().Status == EDnd5eJsonReadStatus::Success);
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eEditorDatasetLifetimeTest, "Dnd5e.Editor.Dataset.LifetimeAndDeinitialize", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FDnd5eEditorDatasetLifetimeTest::RunTest(const FString& Parameters)
{
	FEditorDatasetFiles Files;
	if (!TestTrue(TEXT("Create synthetic input"), Files.WriteSpell())) { return false; }
	TStrongObjectPtr<UDnd5eEditorDatasetSubsystem> Session(NewObject<UDnd5eEditorDatasetSubsystem>());
	if (!TestTrue(TEXT("Load a catalog"), Session->LoadSelection(Files.Request()).Status == EDnd5eJsonReadStatus::Success)) { return false; }
	TWeakObjectPtr<UDnd5eSpellCatalogObject> Weak = Session->GetCatalog();
	CollectGarbage(RF_NoFlags);
	if (!TestTrue(TEXT("Reflected ownership retains the child catalog"), Weak.IsValid())) { return false; }
	TestEqual(TEXT("Records survive collection with their owner"), Session->GetSpellCount(), 1);
	Session->Deinitialize();
	TestTrue(TEXT("Deinitialization releases session state"), Session->GetCatalog() == nullptr && Session->GetSpellCount() == 0 && Session->GetLastLoadResult().Status == EDnd5eJsonReadStatus::NotAttempted && Session->GetLastLoadResult().Diagnostics.IsEmpty());
	CollectGarbage(RF_NoFlags);
	TestFalse(TEXT("Released catalog is collectible even while the empty session remains"), Weak.IsValid());
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eEditorDatasetMenuTest, "Dnd5e.Editor.Dataset.MenuRegistration", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FDnd5eEditorDatasetMenuTest::RunTest(const FString& Parameters)
{
	ISettingsModule& SettingsModule = FModuleManager::LoadModuleChecked<ISettingsModule>(TEXT("Settings"));
	const auto Container = SettingsModule.GetContainer(TEXT("Editor"));
	if (!TestTrue(TEXT("Editor settings container exists"), Container.IsValid())) { return false; }
	const auto Category = Container->GetCategory(TEXT("General"));
	if (!TestTrue(TEXT("General category exists"), Category.IsValid())) { return false; }
	const auto Section = Category->GetSection(TEXT("Dnd5eDataset"));
	if (!TestTrue(TEXT("Settings auto-register at their stable location"), Section.IsValid())) { return false; }
	TestTrue(TEXT("Settings registration points to the default selection object"), Section->GetSettingsObject().Get() == GetDefault<UDnd5eEditorDatasetSettings>());
	const auto ItemSection = Category->GetSection(TEXT("Dnd5eItemDataset"));
	TestTrue(TEXT("Item settings register separately from spell settings"),
		ItemSection.IsValid() && ItemSection->GetSettingsObject().Get() == GetDefault<UDnd5eEditorItemDatasetSettings>());
	UToolMenu* Menu = FSlateApplication::IsInitialized() ? UToolMenus::Get()->FindMenu(TEXT("LevelEditor.MainMenu.Tools")) : nullptr;
	FToolMenuSection* Tools = Menu ? Menu->FindSection(TEXT("Dnd5e")) : nullptr;
	if (!Tools && !FApp::CanEverRender())
	{
		AddInfo(TEXT("This non-rendering worker has no level-editor Tools menu; picker/menu presentation is covered by the user's manual editor check."));
		return !HasAnyErrors();
	}
	if (!TestNotNull(TEXT("Dataset tools section exists"), Tools)) { return false; }
	auto& Module = FModuleManager::GetModuleChecked<FDnd5eEditorModule>(TEXT("Dnd5eEditor"));
	for (FName Name : {FName(TEXT("Dnd5eDatasetSettings")), FName(TEXT("Dnd5eLoadDataset")),
		FName(TEXT("Dnd5eItemDatasetSettings")), FName(TEXT("Dnd5eLoadItemDataset"))})
	{
		int32 Count = 0;
		for (const auto& Entry : Tools->Blocks)
		{
			if (Entry.Name != Name) { continue; }
			++Count;
			TestTrue(TEXT("Entry owner matches the module's shutdown cleanup identity"), Entry.Owner == FToolMenuOwner(&Module));
		}
		TestEqual(TEXT("Each command has one registered entry"), Count, 1);
	}
	if (const auto* LogModule = FModuleManager::GetModulePtr<FMessageLogModule>(TEXT("MessageLog")))
	{
		TestTrue(TEXT("Dataset results listing is registered"), LogModule->IsRegisteredLogListing(TEXT("Dnd5eDataset")));
	}
	else { AddError(TEXT("Message Log module is unavailable in an interactive editor.")); }
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eEditorItemSettingsTest, "Dnd5e.Editor.ItemDataset.Settings", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FDnd5eEditorItemSettingsTest::RunTest(const FString& Parameters)
{
	FEditorDatasetFiles Files;
	TStrongObjectPtr<UDnd5eEditorItemDatasetSettings> Settings(NewObject<UDnd5eEditorItemDatasetSettings>());
	Settings->DatasetRoot.Path = Files.Root;
	Settings->DatasetNamespace = TEXT("items-test");
	FFilePath Base, Primary, Relative, Supplement;
	Base.FilePath = FPaths::Combine(Files.Root, TEXT("base.json"));
	Primary.FilePath = FPaths::Combine(Files.Root, TEXT("items/first.json"));
	Relative.FilePath = TEXT("items/second.json");
	Supplement.FilePath = TEXT("fluff.json");
	Settings->BaseFiles = {Base};
	Settings->DefinitionFiles = {Base};
	Settings->ItemFiles = {Primary, Relative};
	Settings->SupplementFiles = {Supplement};

	const auto Request = Settings->MakeLoadRequest();
	TestTrue(TEXT("Settings preserve roles, array order and the selected dataset context"),
		Request.DatasetRoot == Files.Root && Request.DatasetNamespace == TEXT("items-test") &&
		Request.BaseFiles == TArray<FString>{TEXT("base.json")} && Request.DefinitionFiles == Request.BaseFiles &&
		Request.ItemFiles == TArray<FString>{TEXT("items/first.json"), TEXT("items/second.json")} &&
		Request.SupplementFiles == TArray<FString>{TEXT("fluff.json")} && Request.VariantSelections.IsEmpty());
	TestTrue(TEXT("Item settings use their own per-project user section"),
		Settings->GetSectionName() == TEXT("Dnd5eItemDataset") &&
		Settings->GetClass()->ClassConfigName == TEXT("EditorPerProjectUserSettings") && Settings->SupportsAutoRegistration());
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eEditorItemSessionTest, "Dnd5e.Editor.ItemDataset.Session", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FDnd5eEditorItemSessionTest::RunTest(const FString& Parameters)
{
	FEditorDatasetFiles Files;
	if (!TestTrue(TEXT("Write item and definition fixtures"), IFileManager::Get().MakeDirectory(*Files.Root, true) &&
		FFileHelper::SaveStringToFile(TEXT(R"({"baseitem":[{"name":"Blade","source":"SYN","rarity":"none","type":"M|SYN"}],
			"itemType":[{"name":"Melee","source":"SYN","abbreviation":"M"}]})"), *FPaths::Combine(Files.Root, TEXT("base.json")))))
	{
		return false;
	}

	TStrongObjectPtr<UDnd5eEditorItemDatasetSubsystem> Subsystem(NewObject<UDnd5eEditorItemDatasetSubsystem>());
	FDnd5eJsonItemCatalogLoadRequest Request;
	Request.DatasetRoot = Files.Root;
	Request.DatasetNamespace = TEXT("first");
	Request.BaseFiles = {TEXT("base.json")};
	Request.DefinitionFiles = Request.BaseFiles;
	int32 Broadcasts = 0;
	Subsystem->OnCatalogChanged().AddLambda([&]
	{
		++Broadcasts;
		TestTrue(TEXT("Listeners see both catalogs after publication"),
			Subsystem->GetItemCatalog() && Subsystem->GetDefinitionCatalog() &&
			Subsystem->GetRecordCount() == 1 && Subsystem->GetDefinitionCount() == 1);
	});
	TestTrue(TEXT("Empty session has zero counts"), Subsystem->GetRecordCount() == 0 && Subsystem->GetDefinitionCount() == 0);
	if (!TestTrue(TEXT("Initial load publishes once"),
		Subsystem->LoadSelection(Request).Status == EDnd5eJsonReadStatus::Success && Broadcasts == 1)) { return false; }
	TWeakObjectPtr<UDnd5eItemCatalogObject> Items(Subsystem->GetItemCatalog());
	TWeakObjectPtr<UDnd5eItemDefinitionCatalogObject> Definitions(Subsystem->GetDefinitionCatalog());
	Request.ItemFiles = {TEXT("missing.json")};
	TestTrue(TEXT("Failed load reports diagnostics and leaves active objects and notification count alone"),
		Subsystem->LoadSelection(Request).Status == EDnd5eJsonReadStatus::Failure &&
		!Subsystem->GetLastLoadResult().Diagnostics.IsEmpty() && Broadcasts == 1 &&
		Subsystem->GetItemCatalog() == Items.Get() && Subsystem->GetDefinitionCatalog() == Definitions.Get());
	CollectGarbage(RF_NoFlags);
	TestTrue(TEXT("Active references survive garbage collection after a failed attempt"), Items.IsValid() && Definitions.IsValid());
	Request.ItemFiles.Reset();
	Request.DatasetNamespace = TEXT("second");
	TestTrue(TEXT("Later success replaces both and publishes once"),
		Subsystem->LoadSelection(Request).Status == EDnd5eJsonReadStatus::Success && Broadcasts == 2 &&
		Subsystem->GetItemCatalog() != Items.Get() && Subsystem->GetDefinitionCatalog() != Definitions.Get());
	Subsystem->Deinitialize();
	TestTrue(TEXT("Deinitialization releases both catalogs and resets the last result"),
		!Subsystem->GetItemCatalog() && !Subsystem->GetDefinitionCatalog() && Broadcasts == 2 &&
		Subsystem->GetLastLoadResult().Status == EDnd5eJsonReadStatus::NotAttempted && !Subsystem->OnCatalogChanged().IsBound());
	return !HasAnyErrors();
}

#endif
