#if WITH_DEV_AUTOMATION_TESTS

#include "Dnd5eTestSupport.h"
#include "Catalog/Dnd5eSpellCatalogObject.h"
#include "Readers/Dnd5eJsonSpellCatalogLibrary.h"
#include "UObject/GarbageCollection.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/StructOnScope.h"

using namespace Dnd5eTests;

namespace
{
	FDnd5eJsonSpellCatalogBuildRequest BlueprintBuildRequest()
	{
		FDnd5eJsonSpellCatalogBuildRequest Request;
		FDnd5eJsonDocument Document;
		Document.Context.DatasetNamespace = TEXT("blueprint-synthetic");
		Document.Context.RelativeFilePath = TEXT("spells/invented.json");
		Document.JsonText = TEXT(R"json({"spell":[{
			"name":"Invented Lantern","source":"SYN","level":0,"school":"V",
			"time":[{"number":1,"unit":"action"}],"range":{"type":"point","distance":{"type":"self"}},
			"components":{"v":true},"duration":[{"type":"instant"}],"entries":["An invented light uses {@dice 1d4}."]
		}]})json");
		Request.SpellDocuments.Add(MoveTemp(Document));
		return Request;
	}

	struct FParameterContract
	{
		const TCHAR* Name;
		const TCHAR* Type;
		bool bOutput = false;
		bool bConst = false;
		bool bReturn = false;
	};

	FString ParameterType(const FProperty& Property)
	{
		FString ExtendedType;
		const FString Type = Property.GetCPPType(&ExtendedType);
		return Type + ExtendedType;
	}

	bool CheckFunction(FAutomationTestBase& Test, UClass* Owner, const TCHAR* Name, bool bPure, bool bConst, bool bStatic, const TArray<FParameterContract>& Parameters)
	{
		UFunction* Function = Owner->FindFunctionByName(Name);
		if (!Test.TestNotNull(Name, Function)) { return false; }
		Test.TestTrue(TEXT("Function belongs to the documented class"), Function->GetOuterUClass() == Owner);
		Test.TestTrue(TEXT("Function is native, public, and Blueprint callable"), Function->HasAllFunctionFlags(FUNC_Native | FUNC_Public | FUNC_BlueprintCallable));
		Test.TestEqual(TEXT("Pure flag"), Function->HasAnyFunctionFlags(FUNC_BlueprintPure), bPure);
		Test.TestEqual(TEXT("Const flag"), Function->HasAnyFunctionFlags(FUNC_Const), bConst);
		Test.TestEqual(TEXT("Static flag"), Function->HasAnyFunctionFlags(FUNC_Static), bStatic);
		int32 Count = 0;
		for (TFieldIterator<FProperty> It(Function); It; ++It) { if (It->HasAnyPropertyFlags(CPF_Parm)) { ++Count; } }
		Test.TestEqual(TEXT("Complete parameter count"), Count, Parameters.Num());
		for (const auto& Expected : Parameters)
		{
			const auto* Property = FindFProperty<FProperty>(Function, Expected.Name);
			if (!Test.TestNotNull(Expected.Name, Property)) { continue; }
			Test.TestTrue(TEXT("Field is a parameter"), Property->HasAnyPropertyFlags(CPF_Parm));
			Test.TestEqual(Expected.Name, ParameterType(*Property), FString(Expected.Type));
			Test.TestEqual(TEXT("Return pin"), Property->HasAnyPropertyFlags(CPF_ReturnParm), Expected.bReturn);
			Test.TestEqual(TEXT("Const input"), Property->HasAnyPropertyFlags(CPF_ConstParm), Expected.bConst);
			const bool bOutput = !Property->HasAnyPropertyFlags(CPF_ReturnParm | CPF_ConstParm) && Property->HasAnyPropertyFlags(CPF_OutParm);
			Test.TestEqual(TEXT("Output pin direction"), bOutput, Expected.bOutput);
			if (Expected.bOutput) { Test.TestFalse(TEXT("Outputs do not request an input reference pin"), Property->HasAnyPropertyFlags(CPF_ReferenceParm)); }
		}
		return !Test.HasAnyErrors();
	}

	// Allocate the reflected parameter frame rather than guessing a generated struct's layout.
	struct FBlueprintInvocation
	{
		FAutomationTestBase& Test;
		UObject* Target;
		UFunction* Function;
		FStructOnScope Frame;

		FBlueprintInvocation(FAutomationTestBase& InTest, UObject* InTarget, const TCHAR* Name)
			: Test(InTest), Target(InTarget), Function(InTarget ? InTarget->FindFunction(FName(Name)) : nullptr), Frame(Function)
		{
			Test.TestNotNull(Name, Function);
		}

		template<typename T> T* Parameter(const TCHAR* Name, const TCHAR* ExpectedType)
		{
			const auto* Property = Function ? FindFProperty<FProperty>(Function, Name) : nullptr;
			if (!Test.TestNotNull(Name, Property)) { return nullptr; }
			if (!Test.TestEqual(Name, ParameterType(*Property), FString(ExpectedType))) { return nullptr; }
			return Property->ContainerPtrToValuePtr<T>(Frame.GetStructMemory());
		}

		void Invoke() { Target->ProcessEvent(Function, Frame.GetStructMemory()); }
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eBlueprintCatalogReflectionTest, "Dnd5e.Blueprint.SpellCatalog.Reflection", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FDnd5eBlueprintCatalogReflectionTest::RunTest(const FString& Parameters)
{
	UClass* Catalog = UDnd5eSpellCatalogObject::StaticClass();
	TestEqual(TEXT("Core owns the object"), Catalog->GetOutermost()->GetName(), FString(TEXT("/Script/Dnd5e")));
	TestTrue(TEXT("Object is a Blueprint type"), Catalog->GetBoolMetaData(TEXT("BlueprintType")));
	TestTrue(TEXT("Object is transient"), Catalog->HasAnyClassFlags(CLASS_Transient));
	TestTrue(TEXT("Object derives directly from UObject"), Catalog->GetSuperClass() == UObject::StaticClass());
	TestNull(TEXT("Mutable native catalog access is not exposed to Blueprint"), Catalog->FindFunctionByName(TEXT("GetMutableCatalog")));
	CheckFunction(*this, Catalog, TEXT("ReplaceSpells"), false, false, false, {
		{TEXT("Records"), TEXT("TArray<FDnd5eSpell>"), false, true}, {TEXT("Diagnostics"), TEXT("TArray<FDnd5eDiagnostic>"), true}, {TEXT("ReturnValue"), TEXT("bool"), false, false, true}});
	CheckFunction(*this, Catalog, TEXT("TryGetSpell"), false, true, false, {
		{TEXT("Id"), TEXT("FDnd5eRecordId"), false, true}, {TEXT("Spell"), TEXT("FDnd5eSpell"), true}, {TEXT("ReturnValue"), TEXT("bool"), false, false, true}});
	CheckFunction(*this, Catalog, TEXT("QuerySpellIds"), false, true, false, {
		{TEXT("Query"), TEXT("FDnd5eSpellQuery"), false, true}, {TEXT("Ids"), TEXT("TArray<FDnd5eRecordId>"), true},
		{TEXT("Diagnostics"), TEXT("TArray<FDnd5eDiagnostic>"), true}, {TEXT("ReturnValue"), TEXT("bool"), false, false, true}});
	CheckFunction(*this, Catalog, TEXT("GetSpellCount"), true, true, false, {{TEXT("ReturnValue"), TEXT("int32"), false, false, true}});
	CheckFunction(*this, Catalog, TEXT("GetSpellIds"), false, true, false, {{TEXT("ReturnValue"), TEXT("TArray<FDnd5eRecordId>"), false, false, true}});
	UClass* Library = UDnd5eJsonSpellCatalogLibrary::StaticClass();
	TestEqual(TEXT("JSON module owns the build library"), Library->GetOutermost()->GetName(), FString(TEXT("/Script/Dnd5eJson")));
	TestTrue(TEXT("Library derives from the engine's function library"), Library->GetSuperClass() == UBlueprintFunctionLibrary::StaticClass());
	CheckFunction(*this, Library, TEXT("BuildSpellCatalog"), false, false, true, {
		{TEXT("CatalogObject"), TEXT("UDnd5eSpellCatalogObject*")}, {TEXT("Request"), TEXT("FDnd5eJsonSpellCatalogBuildRequest"), false, true},
		{TEXT("ReturnValue"), TEXT("FDnd5eJsonSpellCatalogBuildResult"), false, false, true}});
	CheckFunction(*this, Library, TEXT("LoadSpellCatalog"), false, false, true, {
		{TEXT("CatalogObject"), TEXT("UDnd5eSpellCatalogObject*")}, {TEXT("Request"), TEXT("FDnd5eJsonSpellCatalogLoadRequest"), false, true},
		{TEXT("ReturnValue"), TEXT("FDnd5eJsonSpellCatalogBuildResult"), false, false, true}});
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eBlueprintCatalogInvocationTest, "Dnd5e.Blueprint.SpellCatalog.BuildQueryAndCopyOut", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FDnd5eBlueprintCatalogInvocationTest::RunTest(const FString& Parameters)
{
	TStrongObjectPtr<UDnd5eSpellCatalogObject> Catalog(NewObject<UDnd5eSpellCatalogObject>());
	const auto Request = BlueprintBuildRequest();
	FBlueprintInvocation Build(*this, GetMutableDefault<UDnd5eJsonSpellCatalogLibrary>(), TEXT("BuildSpellCatalog"));
	auto* Target = Build.Parameter<UDnd5eSpellCatalogObject*>(TEXT("CatalogObject"), TEXT("UDnd5eSpellCatalogObject*"));
	auto* Input = Build.Parameter<FDnd5eJsonSpellCatalogBuildRequest>(TEXT("Request"), TEXT("FDnd5eJsonSpellCatalogBuildRequest"));
	auto* Result = Build.Parameter<FDnd5eJsonSpellCatalogBuildResult>(TEXT("ReturnValue"), TEXT("FDnd5eJsonSpellCatalogBuildResult"));
	if (!Target || !Input || !Result) { return false; }
	*Target = Catalog.Get(); *Input = Request;
	Build.Invoke();
	if (!TestTrue(TEXT("Reflected build succeeds"), Result->Status == EDnd5eJsonReadStatus::Success)) { return false; }
	TestEqual(TEXT("Reflected result count"), Result->SpellCount, 1);
	TestTrue(TEXT("Build preserves supplied request storage"), Equal(FDnd5eJsonSpellCatalogBuildRequest::StaticStruct(), Input, &Request));
	FBlueprintInvocation Count(*this, Catalog.Get(), TEXT("GetSpellCount"));
	auto* StoredCount = Count.Parameter<int32>(TEXT("ReturnValue"), TEXT("int32"));
	if (!StoredCount) { return false; }
	Count.Invoke();
	TestEqual(TEXT("Count comes from the retained catalog object"), *StoredCount, 1);
	FBlueprintInvocation Query(*this, Catalog.Get(), TEXT("QuerySpellIds"));
	auto* Filter = Query.Parameter<FDnd5eSpellQuery>(TEXT("Query"), TEXT("FDnd5eSpellQuery"));
	auto* Ids = Query.Parameter<TArray<FDnd5eRecordId>>(TEXT("Ids"), TEXT("TArray<FDnd5eRecordId>"));
	auto* Diagnostics = Query.Parameter<TArray<FDnd5eDiagnostic>>(TEXT("Diagnostics"), TEXT("TArray<FDnd5eDiagnostic>"));
	auto* QuerySuccess = Query.Parameter<bool>(TEXT("ReturnValue"), TEXT("bool"));
	if (!Filter || !Ids || !Diagnostics || !QuerySuccess) { return false; }
	Query.Invoke();
	if (!TestTrue(TEXT("Default reflected query returns every ID"), *QuerySuccess && Ids->Num() == 1 && Diagnostics->IsEmpty())) { return false; }
	const FDnd5eRecordId Id = (*Ids)[0];
	FBlueprintInvocation Get(*this, Catalog.Get(), TEXT("TryGetSpell"));
	auto* SelectedId = Get.Parameter<FDnd5eRecordId>(TEXT("Id"), TEXT("FDnd5eRecordId"));
	auto* Spell = Get.Parameter<FDnd5eSpell>(TEXT("Spell"), TEXT("FDnd5eSpell"));
	auto* Found = Get.Parameter<bool>(TEXT("ReturnValue"), TEXT("bool"));
	if (!SelectedId || !Spell || !Found) { return false; }
	*SelectedId = Id;
	Get.Invoke();
	if (!TestTrue(TEXT("Reflected lookup resolves its returned ID"), *Found)) { return false; }
	TestEqual(TEXT("The retrieved record has its original name"), Spell->Header.DisplayName, FString(TEXT("Invented Lantern")));
	Spell->Header.DisplayName = TEXT("Edited copy");
	FDnd5eParagraphContent Paragraph;
	if (!TestTrue(TEXT("Typed Content survives reflected retrieval"), Spell->Description.Num() == 1 && UDnd5eContentLibrary::TryGetParagraph(Spell->Description[0], Paragraph))) { return false; }
	FDnd5eAnnotationContent Annotation;
	FDnd5eDiceExpression Dice;
	bool bSawDice = false;
	for (const auto& InlineValue : Paragraph.Inlines)
	{
		if (UDnd5eContentLibrary::TryGetAnnotation(InlineValue, Annotation) && UDnd5eContentLibrary::TryGetAnnotationDiceExpression(Annotation, Dice)) { bSawDice = Dice.Expression == TEXT("1d4"); }
	}
	TestTrue(TEXT("Annotation carries an unevaluated expression"), bSawDice);
	Spell->Description.Reset();
	Get.Invoke();
	TestTrue(TEXT("Editing the previous copy leaves catalog data intact"), *Found && Spell->Header.DisplayName == TEXT("Invented Lantern") && Spell->Description.Num() == 1);
	SelectedId->SourceKey = TEXT("missing");
	Get.Invoke();
	const FDnd5eSpell Default;
	TestTrue(TEXT("A reflected lookup miss clears previous output"), !*Found && Equal(FDnd5eSpell::StaticStruct(), Spell, &Default));
	Filter->SourceId = TEXT("-1");
	Query.Invoke();
	TestTrue(TEXT("A source ID is a publication constraint, not the level sentinel"), *QuerySuccess && Ids->IsEmpty());
	Filter->SourceId.Reset(); Filter->Level = -2;
	Query.Invoke();
	TestTrue(TEXT("Invalid queries reset IDs and return their error"), !*QuerySuccess && Ids->IsEmpty() && Diagnostics->Num() == 1 && (*Diagnostics)[0].Code == FName(TEXT("Dnd5e.Catalog.InvalidQuery")));
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eBlueprintCatalogFailureTest, "Dnd5e.Blueprint.SpellCatalog.AtomicFailureAndDiagnostics", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FDnd5eBlueprintCatalogFailureTest::RunTest(const FString& Parameters)
{
	TStrongObjectPtr<UDnd5eSpellCatalogObject> Catalog(NewObject<UDnd5eSpellCatalogObject>());
	auto Request = BlueprintBuildRequest();
	const auto Built = UDnd5eJsonSpellCatalogLibrary::BuildSpellCatalog(Catalog.Get(), Request);
	if (!TestTrue(TEXT("Seed catalog"), Built.Status == EDnd5eJsonReadStatus::Success && Catalog->GetSpellCount() == 1)) { return false; }
	const auto Before = Catalog->GetSpellIds();
	Request.SpellDocuments[0].JsonText = TEXT("{\"spell\":[null]}");
	const auto Failed = UDnd5eJsonSpellCatalogLibrary::BuildSpellCatalog(Catalog.Get(), Request);
	TestTrue(TEXT("Bad source returns failure without stale success count"), Failed.Status == EDnd5eJsonReadStatus::Failure && Failed.SpellCount == 0);
	TestTrue(TEXT("The target still owns its previous catalog"), Catalog->GetSpellCount() == 1 && Catalog->GetSpellIds() == Before);
	TestTrue(TEXT("Builder error location passes through unchanged"), Failed.Diagnostics.ContainsByPredicate([](const auto& Diagnostic) {
		return Diagnostic.Severity == EDnd5eDiagnosticSeverity::Error && Diagnostic.Code == FName(TEXT("Dnd5e.Json.InvalidSpellObject")) && Diagnostic.RelativeFilePath == TEXT("spells/invented.json") && Diagnostic.JsonPointer == TEXT("/spell/0"); }));
	const auto Missing = UDnd5eJsonSpellCatalogLibrary::BuildSpellCatalog(nullptr, Request);
	TestTrue(TEXT("Missing target returns structured failure"), Missing.Status == EDnd5eJsonReadStatus::Failure && Missing.SpellCount == 0 && Missing.Diagnostics.Num() == 1);
	if (Missing.Diagnostics.Num() == 1)
	{
		const auto& Diagnostic = Missing.Diagnostics[0];
		TestTrue(TEXT("Invalid-target diagnostic has no invented source location"), Diagnostic.Severity == EDnd5eDiagnosticSeverity::Error && Diagnostic.Code == FName(TEXT("Dnd5e.Json.InvalidCatalogObject")) && !Diagnostic.Message.IsEmpty() && Diagnostic.RelativeFilePath.IsEmpty() && Diagnostic.JsonPointer.IsEmpty());
	}
	auto* Invalid = NewObject<UDnd5eSpellCatalogObject>();
	Invalid->MarkAsGarbage();
	const auto InvalidResult = UDnd5eJsonSpellCatalogLibrary::BuildSpellCatalog(Invalid, Request);
	TestTrue(TEXT("An invalid non-null target is rejected before parsing"), InvalidResult.Status == EDnd5eJsonReadStatus::Failure && InvalidResult.Diagnostics.Num() == 1 && InvalidResult.Diagnostics[0].Code == FName(TEXT("Dnd5e.Json.InvalidCatalogObject")));
	FDnd5eSpell Original;
	if (!TestTrue(TEXT("The old record still resolves"), Catalog->TryGetSpell(Before[0], Original))) { return false; }
	TArray<FDnd5eDiagnostic> Diagnostics;
	TestFalse(TEXT("Core wrapper rejects duplicate replacement"), Catalog->ReplaceSpells({Original, Original}, Diagnostics));
	TestTrue(TEXT("Core wrapper preserves the target and its duplicate diagnostic"), Catalog->GetSpellIds() == Before && Diagnostics.Num() == 1 && Diagnostics[0].Code == FName(TEXT("Dnd5e.Catalog.DuplicateRecordId")));
	Request.SpellDocuments[0].JsonText = TEXT("{\"spell\":[]}");
	const auto Cleared = UDnd5eJsonSpellCatalogLibrary::BuildSpellCatalog(Catalog.Get(), Request);
	TestTrue(TEXT("Explicit empty document clears through the JSON library"), Cleared.Status == EDnd5eJsonReadStatus::Success && Cleared.SpellCount == 0 && Catalog->GetSpellCount() == 0);
	TestTrue(TEXT("Core wrapper also accepts an empty replacement"), Catalog->ReplaceSpells({}, Diagnostics) && Diagnostics.IsEmpty());
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eBlueprintCatalogAvailabilityTest, "Dnd5e.Blueprint.SpellCatalog.AvailabilityWarnings", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FDnd5eBlueprintCatalogAvailabilityTest::RunTest(const FString& Parameters)
{
	TStrongObjectPtr<UDnd5eSpellCatalogObject> Catalog(NewObject<UDnd5eSpellCatalogObject>());
	auto Request = BlueprintBuildRequest();
	if (!TestTrue(TEXT("Build without availability"), UDnd5eJsonSpellCatalogLibrary::BuildSpellCatalog(Catalog.Get(), Request).Status == EDnd5eJsonReadStatus::Success)) { return false; }
	FDnd5eSpellQuery Query;
	Query.bRequireAvailability = true;
	TArray<FDnd5eRecordId> Ids;
	TArray<FDnd5eDiagnostic> Diagnostics;
	TestTrue(TEXT("Incomplete availability query remains a valid request"), Catalog->QuerySpellIds(Query, Ids, Diagnostics));
	TestTrue(TEXT("Unknown availability remains visible to Blueprint callers"), Ids.IsEmpty() && Diagnostics.Num() == 1 && Diagnostics[0].Code == FName(TEXT("Dnd5e.Catalog.IncompleteAvailability")) && Diagnostics[0].Severity == EDnd5eDiagnosticSeverity::Warning);
	Request.bHasAvailabilityLookup = true;
	Request.AvailabilityLookup.Context.DatasetNamespace = TEXT("blueprint-synthetic");
	Request.AvailabilityLookup.Context.RelativeFilePath = TEXT("generated/invented.json");
	Request.AvailabilityLookup.JsonText = TEXT("{\"syn\":{\"invented lantern\":{\"class\":{\"SYN\":{\"Lantern Adept\":true}}}}}");
	const auto Result = UDnd5eJsonSpellCatalogLibrary::BuildSpellCatalog(Catalog.Get(), Request);
	if (!TestTrue(TEXT("Library forwards a supplied availability join"), Result.Status == EDnd5eJsonReadStatus::Success)) { return false; }
	Query.AvailabilityProvider.Category = Dnd5e::Category::Class;
	Query.AvailabilityProvider.Name = TEXT("Lantern Adept");
	TestTrue(TEXT("The joined association is queryable through the object"), Catalog->QuerySpellIds(Query, Ids, Diagnostics) && Ids.Num() == 1 && Diagnostics.IsEmpty());
	FDnd5eSpell Spell;
	if (!TestTrue(TEXT("Retrieve the joined record"), Ids.Num() == 1 && Catalog->TryGetSpell(Ids[0], Spell))) { return false; }
	TestTrue(TEXT("Association storage is available to reflected consumers"), Spell.bHasAvailabilityData && Spell.Availability.Num() == 1 && Spell.Availability[0].Provider.Name == TEXT("Lantern Adept"));
	Request.SpellDocuments[0].JsonText = TEXT("{\"spell\":[{\"name\":\"Invented Lantern\",\"source\":\"SYN\",\"level\":0,\"school\":\"V\",\"time\":[{\"number\":1,\"unit\":\"action\"}],\"range\":{\"type\":\"special\"},\"components\":{},\"duration\":[{\"type\":\"instant\"}],\"entries\":[\"An invented {@futureRule value}.\"]}]}");
	const auto Preserved = UDnd5eJsonSpellCatalogLibrary::BuildSpellCatalog(Catalog.Get(), Request);
	TestTrue(TEXT("Build warnings pass through rather than becoming silent success"), Preserved.Status == EDnd5eJsonReadStatus::Success && Preserved.Diagnostics.ContainsByPredicate([](const auto& Diagnostic) { return Diagnostic.Code == FName(TEXT("Dnd5e.Json.PreservedUnsupportedContent")) && Diagnostic.Severity == EDnd5eDiagnosticSeverity::Warning; }));
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eBlueprintCatalogLifetimeTest, "Dnd5e.Blueprint.SpellCatalog.OwnershipAndGarbageCollection", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FDnd5eBlueprintCatalogLifetimeTest::RunTest(const FString& Parameters)
{
	TStrongObjectPtr<UDnd5eSpellCatalogObject> First(NewObject<UDnd5eSpellCatalogObject>());
	TStrongObjectPtr<UDnd5eSpellCatalogObject> Second(NewObject<UDnd5eSpellCatalogObject>());
	TestTrue(TEXT("Every new object starts empty"), First->GetSpellCount() == 0 && Second->GetSpellCount() == 0);
	if (!TestTrue(TEXT("Build first object"), UDnd5eJsonSpellCatalogLibrary::BuildSpellCatalog(First.Get(), BlueprintBuildRequest()).Status == EDnd5eJsonReadStatus::Success)) { return false; }
	TestEqual(TEXT("Second object remains independent"), Second->GetSpellCount(), 0);
	const auto OriginalIds = First->GetSpellIds();
	auto CopiedIds = First->GetSpellIds(); CopiedIds.Reset();
	TestTrue(TEXT("Mutating copied IDs does not change the owner"), First->GetSpellIds() == OriginalIds);
	TWeakObjectPtr<UDnd5eSpellCatalogObject> Weak = First.Get();
	CollectGarbage(RF_NoFlags);
	if (!TestTrue(TEXT("A strong owner reference survives collection"), Weak.IsValid())) { return false; }
	FDnd5eSpell Spell;
	TestTrue(TEXT("Owned native records survive with their UObject"), First->GetSpellCount() == 1 && First->TryGetSpell(OriginalIds[0], Spell) && Spell.Description.Num() == 1);
	First.Reset();
	CollectGarbage(RF_NoFlags);
	TestFalse(TEXT("The object is collectible after the strong owner releases it"), Weak.IsValid());
	TestEqual(TEXT("Collecting one object does not affect another"), Second->GetSpellCount(), 0);
	return !HasAnyErrors();
}

#endif
