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
	CheckFunction(*this, Catalog, TEXT("ResolveSpellReference"), false, true, false, {
		{TEXT("Reference"), TEXT("FDnd5eReferenceContent"), false, true}, {TEXT("Spell"), TEXT("FDnd5eSpell"), true},
		{TEXT("Diagnostics"), TEXT("TArray<FDnd5eDiagnostic>"), true}, {TEXT("ReturnValue"), TEXT("EDnd5eReferenceResolveStatus"), false, false, true}});
	CheckFunction(*this, Catalog, TEXT("ResolveSpellRelationship"), false, true, false, {
		{TEXT("Relationship"), TEXT("FDnd5eRecordRelationship"), false, true}, {TEXT("Spell"), TEXT("FDnd5eSpell"), true},
		{TEXT("Diagnostics"), TEXT("TArray<FDnd5eDiagnostic>"), true}, {TEXT("ReturnValue"), TEXT("EDnd5eReferenceResolveStatus"), false, false, true}});
	CheckFunction(*this, Catalog, TEXT("MatchSpellSelector"), false, true, false, {
		{TEXT("Selector"), TEXT("FDnd5eSpellSelector"), false, true}, {TEXT("CandidateIds"), TEXT("TArray<FDnd5eRecordId>"), true},
		{TEXT("Diagnostics"), TEXT("TArray<FDnd5eDiagnostic>"), true}, {TEXT("ReturnValue"), TEXT("EDnd5eSpellSelectorMatchStatus"), false, false, true}});
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
	CheckFunction(*this, Library, TEXT("NormalizeSpellReference"), false, false, true, {
		{TEXT("CatalogObject"), TEXT("UDnd5eSpellCatalogObject*")}, {TEXT("DatasetNamespace"), TEXT("FString")},
		{TEXT("Reference"), TEXT("FDnd5eReferenceContent"), false, true}, {TEXT("NormalizedReference"), TEXT("FDnd5eReferenceContent"), true},
		{TEXT("Diagnostics"), TEXT("TArray<FDnd5eDiagnostic>"), true}, {TEXT("ReturnValue"), TEXT("EDnd5eJsonSpellReferenceNormalizeStatus"), false, false, true}});
	// UHT represents const FString& inputs with NativeConst metadata, without CPF_ConstParm.
	const auto* Normalize = Library->FindFunctionByName(TEXT("NormalizeSpellReference"));
	const auto* Namespace = Normalize ? FindFProperty<FStrProperty>(Normalize, TEXT("DatasetNamespace")) : nullptr;
	TestTrue(TEXT("Namespace preserves native const input metadata"), Namespace && Namespace->HasMetaData(TEXT("NativeConst")));
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eBlueprintReferenceResolveTest, "Dnd5e.Blueprint.SpellCatalog.ResolveReference", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FDnd5eBlueprintReferenceResolveTest::RunTest(const FString& Parameters)
{
	TStrongObjectPtr<UDnd5eSpellCatalogObject> Catalog(NewObject<UDnd5eSpellCatalogObject>());
	const FDnd5eSpell Target = SyntheticSpell();
	TArray<FDnd5eDiagnostic> SetupDiagnostics;
	if (!TestTrue(TEXT("Seed a synthetic target"), Catalog->ReplaceSpells({Target}, SetupDiagnostics))) { return false; }
	FBlueprintInvocation Call(*this, Catalog.Get(), TEXT("ResolveSpellReference"));
	auto* Reference = Call.Parameter<FDnd5eReferenceContent>(TEXT("Reference"), TEXT("FDnd5eReferenceContent"));
	auto* Spell = Call.Parameter<FDnd5eSpell>(TEXT("Spell"), TEXT("FDnd5eSpell"));
	auto* Diagnostics = Call.Parameter<TArray<FDnd5eDiagnostic>>(TEXT("Diagnostics"), TEXT("TArray<FDnd5eDiagnostic>"));
	auto* Status = Call.Parameter<EDnd5eReferenceResolveStatus>(TEXT("ReturnValue"), TEXT("EDnd5eReferenceResolveStatus"));
	if (!Reference || !Spell || !Diagnostics || !Status) { return false; }
	Reference->bHasTargetRecordId = true;
	Reference->TargetRecordId = Target.Header.RecordId;
	Reference->SourceFormat = TEXT("synthetic");
	Reference->SourceReference = TEXT("opaque-token");
	Reference->Display.Add(Text(TEXT("Independent label")));
	const FDnd5eReferenceContent Before = *Reference;
	Diagnostics->AddDefaulted();
	Call.Invoke();
	TestTrue(TEXT("Reflected success returns the complete independent value"), *Status == EDnd5eReferenceResolveStatus::Resolved && Diagnostics->IsEmpty() && Equal(FDnd5eSpell::StaticStruct(), Spell, &Target));
	TestTrue(TEXT("Reflected input remains unchanged"), Equal(FDnd5eReferenceContent::StaticStruct(), Reference, &Before));
	Spell->Description.Reset();
	Call.Invoke();
	TestTrue(TEXT("A new reflected resolution restores the original nested content"), Equal(FDnd5eSpell::StaticStruct(), Spell, &Target));

	struct FFailure
	{
		FDnd5eReferenceContent Value;
		EDnd5eReferenceResolveStatus Expected;
	};
	TArray<FFailure> Failures;
	auto Missing = Before; Missing.bHasTargetRecordId = false;
	Failures.Add({Missing, EDnd5eReferenceResolveStatus::MissingTargetId});
	auto Invalid = Before; Invalid.TargetRecordId.SourceKey.Reset();
	Failures.Add({Invalid, EDnd5eReferenceResolveStatus::InvalidTargetId});
	auto Other = Before; Other.TargetRecordId.Category = Dnd5e::Category::Class;
	Failures.Add({Other, EDnd5eReferenceResolveStatus::UnsupportedCategory});
	auto Absent = Before; Absent.TargetRecordId.SourceKey = TEXT("not-loaded");
	Failures.Add({Absent, EDnd5eReferenceResolveStatus::TargetNotFound});
	const FDnd5eSpell Default;
	for (const auto& Failure : Failures)
	{
		*Reference = Failure.Value; *Spell = Target; Diagnostics->AddDefaulted();
		Call.Invoke();
		TestTrue(TEXT("Every reflected failure returns its status and clears stale spell output"), *Status == Failure.Expected && Equal(FDnd5eSpell::StaticStruct(), Spell, &Default) && Diagnostics->Num() == 1);
		TestTrue(TEXT("Failure also preserves its reference input"), Equal(FDnd5eReferenceContent::StaticStruct(), Reference, &Failure.Value));
	}
	*Reference = Before;
	Call.Invoke();
	TestTrue(TEXT("Success clears the preceding failure diagnostic"), *Status == EDnd5eReferenceResolveStatus::Resolved && Diagnostics->IsEmpty());
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eBlueprintRelationshipTest, "Dnd5e.Blueprint.SpellCatalog.ResolveRelationship", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FDnd5eBlueprintRelationshipTest::RunTest(const FString& Parameters)
{
	TStrongObjectPtr<UDnd5eSpellCatalogObject> Catalog(NewObject<UDnd5eSpellCatalogObject>());
	const auto Target = SyntheticSpell();
	TArray<FDnd5eDiagnostic> Setup;
	if (!TestTrue(TEXT("Install a synthetic relationship target"), Catalog->ReplaceSpells({Target}, Setup))) { return false; }
	FBlueprintInvocation Call(*this, Catalog.Get(), TEXT("ResolveSpellRelationship"));
	auto* Input = Call.Parameter<FDnd5eRecordRelationship>(TEXT("Relationship"), TEXT("FDnd5eRecordRelationship"));
	auto* Output = Call.Parameter<FDnd5eSpell>(TEXT("Spell"), TEXT("FDnd5eSpell"));
	auto* Diagnostics = Call.Parameter<TArray<FDnd5eDiagnostic>>(TEXT("Diagnostics"), TEXT("TArray<FDnd5eDiagnostic>"));
	auto* Status = Call.Parameter<EDnd5eReferenceResolveStatus>(TEXT("ReturnValue"), TEXT("EDnd5eReferenceResolveStatus"));
	if (!Input || !Output || !Diagnostics || !Status) { return false; }
	Input->Kind = Dnd5e::Record::Relationship::ReprintedAs;
	Input->SourceFormat = TEXT("synthetic"); Input->SourceReference = TEXT("retained-selector");
	Input->bHasTargetRecord = true; Input->TargetRecordId = Target.Header.RecordId;
	const auto Original = *Input;
	Diagnostics->AddDefaulted();
	Call.Invoke();
	TestTrue(TEXT("Reflected relationship lookup returns an independent full spell"), *Status == EDnd5eReferenceResolveStatus::Resolved && Diagnostics->IsEmpty() && Equal(FDnd5eSpell::StaticStruct(), Output, &Target));
	TestTrue(TEXT("The relationship remains unchanged"), Equal(FDnd5eRecordRelationship::StaticStruct(), Input, &Original));
	Output->Description.Reset(); Call.Invoke();
	TestTrue(TEXT("Nested output mutations cannot affect the catalog"), Equal(FDnd5eSpell::StaticStruct(), Output, &Target));
	for (int32 Case = 0; Case < 4; ++Case)
	{
		*Input = Original; *Output = Target; Diagnostics->AddDefaulted();
		EDnd5eReferenceResolveStatus Expected;
		if (Case == 0) { Input->bHasTargetRecord = false; Expected = EDnd5eReferenceResolveStatus::MissingTargetId; }
		else if (Case == 1) { Input->TargetRecordId.SourceKey.Reset(); Expected = EDnd5eReferenceResolveStatus::InvalidTargetId; }
		else if (Case == 2) { Input->TargetRecordId.Category = Dnd5e::Category::Class; Expected = EDnd5eReferenceResolveStatus::UnsupportedCategory; }
		else { Input->TargetRecordId.SourceKey = TEXT("absent"); Expected = EDnd5eReferenceResolveStatus::TargetNotFound; }
		const auto Before = *Input;
		Call.Invoke();
		const FDnd5eSpell Default;
		TestTrue(TEXT("Every reflected relationship failure clears output and returns one diagnostic"), *Status == Expected && Diagnostics->Num() == 1 && Equal(FDnd5eSpell::StaticStruct(), Output, &Default));
		TestTrue(TEXT("Failure preserves the relationship"), Equal(FDnd5eRecordRelationship::StaticStruct(), Input, &Before));
	}
	*Input = Original; Call.Invoke();
	TestTrue(TEXT("Success clears a previous warning"), *Status == EDnd5eReferenceResolveStatus::Resolved && Diagnostics->IsEmpty());
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eBlueprintSelectorTest, "Dnd5e.Blueprint.SpellCatalog.MatchSelector", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FDnd5eBlueprintSelectorTest::RunTest(const FString& Parameters)
{
	TStrongObjectPtr<UDnd5eSpellCatalogObject> Catalog(NewObject<UDnd5eSpellCatalogObject>());
	auto Target = SyntheticSpell();
	Target.Header.DisplayName = TEXT("Invented Beacon"); Target.Header.Provenance[0].SourceId = TEXT("SYN-B");
	TArray<FDnd5eDiagnostic> Setup;
	if (!TestTrue(TEXT("Install the synthetic target"), Catalog->ReplaceSpells({Target}, Setup))) { return false; }
	FBlueprintInvocation Call(*this, Catalog.Get(), TEXT("MatchSpellSelector"));
	auto* Selector = Call.Parameter<FDnd5eSpellSelector>(TEXT("Selector"), TEXT("FDnd5eSpellSelector"));
	auto* Ids = Call.Parameter<TArray<FDnd5eRecordId>>(TEXT("CandidateIds"), TEXT("TArray<FDnd5eRecordId>"));
	auto* Diagnostics = Call.Parameter<TArray<FDnd5eDiagnostic>>(TEXT("Diagnostics"), TEXT("TArray<FDnd5eDiagnostic>"));
	auto* Status = Call.Parameter<EDnd5eSpellSelectorMatchStatus>(TEXT("ReturnValue"), TEXT("EDnd5eSpellSelectorMatchStatus"));
	if (!Selector || !Ids || !Diagnostics || !Status) { return false; }
	Selector->DatasetNamespace = Target.Header.RecordId.DatasetNamespace;
	Selector->Name = TEXT("INVENTED BEACON"); Selector->SourceId = TEXT("syn-b");
	const auto Original = *Selector;
	Diagnostics->AddDefaulted(); Call.Invoke();
	TestTrue(TEXT("Reflected matching returns the stored canonical ID"), *Status == EDnd5eSpellSelectorMatchStatus::Matched && *Ids == TArray<FDnd5eRecordId>{Target.Header.RecordId} && Diagnostics->IsEmpty());
	TestTrue(TEXT("Reflected selector input is immutable"), Equal(FDnd5eSpellSelector::StaticStruct(), Selector, &Original));
	Ids->Reset(); Selector->SourceId.Reset(); Call.Invoke();
	TestTrue(TEXT("A single candidate is still MissingSource"), *Status == EDnd5eSpellSelectorMatchStatus::MissingSource && Ids->Num() == 1 && Diagnostics->Num() == 1);
	*Selector = Original; Selector->Name = TEXT("not present"); Call.Invoke();
	TestTrue(TEXT("NoMatch clears old candidates"), *Status == EDnd5eSpellSelectorMatchStatus::NoMatch && Ids->IsEmpty() && Diagnostics->Num() == 1);
	*Selector = Original; Selector->DatasetNamespace.Reset(); Ids->Add(Target.Header.RecordId); Call.Invoke();
	TestTrue(TEXT("Invalid input clears candidates and replaces diagnostics"), *Status == EDnd5eSpellSelectorMatchStatus::InvalidSelector && Ids->IsEmpty() && Diagnostics->Num() == 1 && (*Diagnostics)[0].Severity == EDnd5eDiagnosticSeverity::Error);
	auto Duplicate = Target; Duplicate.Header.RecordId.SourceKey = TEXT("different-key");
	Catalog->ReplaceSpells({Target, Duplicate}, Setup);
	*Selector = Original; Call.Invoke();
	TestTrue(TEXT("Reflected ambiguity returns all IDs without choosing one"), *Status == EDnd5eSpellSelectorMatchStatus::Ambiguous && Ids->Num() == 2 && Diagnostics->Num() == 1);
	Catalog->ReplaceSpells({Target}, Setup); Call.Invoke();
	TestTrue(TEXT("Later success clears previous failure diagnostics"), *Status == EDnd5eSpellSelectorMatchStatus::Matched && Diagnostics->IsEmpty() && Ids->Num() == 1);
	FDnd5eSpell Copy;
	TestTrue(TEXT("The returned canonical ID works with exact lookup"), Catalog->TryGetSpell((*Ids)[0], Copy) && Equal(FDnd5eSpell::StaticStruct(), &Copy, &Target));
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eBlueprintNormalizeReferenceTest, "Dnd5e.Blueprint.SpellCatalog.NormalizeReference", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FDnd5eBlueprintNormalizeReferenceTest::RunTest(const FString& Parameters)
{
	TStrongObjectPtr<UDnd5eSpellCatalogObject> Catalog(NewObject<UDnd5eSpellCatalogObject>());
	const auto Build = UDnd5eJsonSpellCatalogLibrary::BuildSpellCatalog(Catalog.Get(), BlueprintBuildRequest());
	if (!TestEqual(TEXT("Build fixture"), Build.Status, EDnd5eJsonReadStatus::Success)) { return false; }
	FBlueprintInvocation Call(*this, GetMutableDefault<UDnd5eJsonSpellCatalogLibrary>(), TEXT("NormalizeSpellReference"));
	auto* CatalogInput = Call.Parameter<UDnd5eSpellCatalogObject*>(TEXT("CatalogObject"), TEXT("UDnd5eSpellCatalogObject*"));
	auto* Namespace = Call.Parameter<FString>(TEXT("DatasetNamespace"), TEXT("FString"));
	auto* Input = Call.Parameter<FDnd5eReferenceContent>(TEXT("Reference"), TEXT("FDnd5eReferenceContent"));
	auto* Output = Call.Parameter<FDnd5eReferenceContent>(TEXT("NormalizedReference"), TEXT("FDnd5eReferenceContent"));
	auto* Diagnostics = Call.Parameter<TArray<FDnd5eDiagnostic>>(TEXT("Diagnostics"), TEXT("TArray<FDnd5eDiagnostic>"));
	auto* Status = Call.Parameter<EDnd5eJsonSpellReferenceNormalizeStatus>(TEXT("ReturnValue"), TEXT("EDnd5eJsonSpellReferenceNormalizeStatus"));
	if (!CatalogInput || !Namespace || !Input || !Output || !Diagnostics || !Status) { return false; }
	*Namespace = TEXT("blueprint-synthetic"); Input->SourceFormat = TEXT("5etools");
	Input->SourceReference = TEXT("{@spell invented lantern|syn}"); Input->Display = {Text(TEXT("A label"))};
	const auto Original = *Input;
	Call.Invoke();
	TestTrue(TEXT("Null catalog failure preserves input copy"), *Status == EDnd5eJsonSpellReferenceNormalizeStatus::InvalidCatalog && Equal(FDnd5eReferenceContent::StaticStruct(), Output, &Original));
	if (TestEqual(TEXT("Null catalog diagnostic"), Diagnostics->Num(), 1))
	{
		TestTrue(TEXT("Controlled error"), (*Diagnostics)[0].Code == FName(TEXT("Dnd5e.Json.InvalidCatalogObject")) && (*Diagnostics)[0].Severity == EDnd5eDiagnosticSeverity::Error);
	}
	*CatalogInput = Catalog.Get(); Call.Invoke();
	TestTrue(TEXT("Reflected success replaces failure"), *Status == EDnd5eJsonSpellReferenceNormalizeStatus::Normalized && Output->bHasTargetRecordId && Diagnostics->IsEmpty());
	TestTrue(TEXT("Input frame unchanged"), Equal(FDnd5eReferenceContent::StaticStruct(), Input, &Original));
	FDnd5eSpell Spell; TArray<FDnd5eDiagnostic> ResolveDiagnostics;
	TestEqual(TEXT("Reflected output resolves"), Catalog->ResolveSpellReference(*Output, Spell, ResolveDiagnostics), EDnd5eReferenceResolveStatus::Resolved);
	Input->SourceReference = TEXT("{@spell invented lantern}"); Call.Invoke();
	TestTrue(TEXT("Missing source does not promote one candidate"), *Status == EDnd5eJsonSpellReferenceNormalizeStatus::MissingSource && !Output->bHasTargetRecordId && Diagnostics->Num() == 1);
	Input->SourceReference = TEXT("{@item invented lantern|syn}"); Call.Invoke();
	TestTrue(TEXT("Unsupported replaces preceding output"), *Status == EDnd5eJsonSpellReferenceNormalizeStatus::UnsupportedReference && Equal(FDnd5eReferenceContent::StaticStruct(), Input, Output) && Diagnostics->Num() == 1);
	return !HasAnyErrors();
}

#endif
