// Developed by Neko Creative Technologies
#if WITH_DEV_AUTOMATION_TESTS
#include "Dnd5eTestSupport.h"
#include "Catalog/Dnd5eItemDefinitionCatalogObject.h"
#include "Items/Dnd5eItemDefinitionIdentity.h"
#include "Items/Dnd5eItemDefinitionLibrary.h"
#include "UObject/StructOnScope.h"

namespace
{
	bool MakeDefinitionHeader(FDnd5eRecordHeader& Header, FGameplayTag Category, const TCHAR* Key, const TCHAR* Source)
	{
		FDnd5eRecordProvenance Origin;
		Origin.SourceId = Source;
		Origin.RelativeFilePath = TEXT("synthetic.json");
		Origin.JsonPointer = TEXT("/definitions/0");
		Header.Provenance.Add(Origin);
		return Dnd5e::ItemDefinitions::TryMakeId(TEXT("guide"), Category, Key, Source, Header.RecordId);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eDefinitionIdentityTest, "Dnd5e.Items.Catalog.Identity", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FDnd5eDefinitionIdentityTest::RunTest(const FString& Parameters)
{
	using Dnd5e::ItemDefinitions::TryMakeId;
	FDnd5eRecordId Id, Other;
	const FGameplayTag Property = Dnd5e::Category::ItemProperty;
	TestTrue(TEXT("Construct canonical identity"), TryMakeId(TEXT(" guide "), Property, TEXT(" P|9:% "), TEXT(" SYN-A "), Id));
	TestEqual(TEXT("Escape reserved characters"), Id.SourceKey, FString(TEXT("dnd5e:item-definition:v1:property:p%7C9%3A%25|syn-a")));
	TestEqual(TEXT("Trim scope"), Id.DatasetNamespace, FString(TEXT("guide")));
	TestTrue(TEXT("Construct literal escape spelling"), TryMakeId(TEXT("guide"), Property, TEXT("p%7c9:%"), TEXT("syn-a"), Other));
	TestTrue(TEXT("Literal escape spelling has a different identity"), Id != Other);
	TestTrue(TEXT("Construct a distinct category identity"), TryMakeId(TEXT("guide"), Dnd5e::Category::ItemType, TEXT("p|9:%"), TEXT("syn-a"), Other));
	TestTrue(TEXT("Category separates otherwise identical keys"), Id != Other);
	TestFalse(TEXT("Reject control characters"), TryMakeId(TEXT("guide"), Property, TEXT("p\n9"), TEXT("syn-a"), Other));
	TestTrue(TEXT("Failure clears previous identity"), Other == FDnd5eRecordId{});
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eDefinitionCatalogTest, "Dnd5e.Items.Catalog.AtomicLookup", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FDnd5eDefinitionCatalogTest::RunTest(const FString& Parameters)
{
	FDnd5eItemTypeDefinition Type;
	Type.Abbreviation = TEXT("P9");
	FDnd5eItemPropertyDefinition A, B;
	A.Abbreviation = B.Abbreviation = TEXT("P9");
	A.TemplateText = TEXT("Original {{item.reload}}");
	FDnd5eItemMasteryDefinition Mastery;
	Mastery.Header.DisplayName = TEXT("Invented Deflection");
	if (!MakeDefinitionHeader(Type.Header, Dnd5e::Category::ItemType, TEXT("P9"), TEXT("SYN-A")) ||
		!MakeDefinitionHeader(A.Header, Dnd5e::Category::ItemProperty, TEXT("P9"), TEXT("SYN-A")) ||
		!MakeDefinitionHeader(B.Header, Dnd5e::Category::ItemProperty, TEXT("P9"), TEXT("SYN-B")) ||
		!MakeDefinitionHeader(Mastery.Header, Dnd5e::Category::ItemMastery, TEXT("Invented Deflection"), TEXT("SYN-A")))
	{
		AddError(TEXT("Synthetic ID construction failed."));
		return false;
	}

	FDnd5eItemDefinitionCatalog Catalog;
	TArray<FDnd5eDiagnostic> Diagnostics;
	if (!TestTrue(TEXT("Populate three domains"), Catalog.ReplaceDefinitions({Type}, {A, B}, {Mastery}, Diagnostics))) { return false; }
	const TArray<FDnd5eRecordId> OriginalIds = {Type.Header.RecordId, A.Header.RecordId, B.Header.RecordId, Mastery.Header.RecordId};
	TestTrue(TEXT("Preserve supplied domain and row order"), Catalog.GetDefinitionIds() == OriginalIds);
	FDnd5eItemDefinitionSelector Selector;
	Selector.Category = Dnd5e::Category::ItemProperty;
	Selector.Abbreviation = TEXT(" p9 ");
	Selector.SourceId = TEXT("syn-a");
	TArray<FDnd5eRecordId> Candidates;
	TestTrue(TEXT("Inactive publication gives inspection candidates"), Catalog.MatchDefinitionSelector(TEXT("guide"), Selector, Candidates, Diagnostics) == EDnd5eItemDefinitionMatchStatus::MissingSource && Candidates == TArray<FDnd5eRecordId>{A.Header.RecordId, B.Header.RecordId});
	Selector.bHasSource = true;
	TestTrue(TEXT("Explicit source selects one canonical property"), Catalog.MatchDefinitionSelector(TEXT("guide"), Selector, Candidates, Diagnostics) == EDnd5eItemDefinitionMatchStatus::Matched && Candidates == TArray<FDnd5eRecordId>{A.Header.RecordId} && Diagnostics.IsEmpty());
	TestTrue(TEXT("Namespace is case-sensitive and miss clears candidates"), Catalog.MatchDefinitionSelector(TEXT("Guide"), Selector, Candidates, Diagnostics) == EDnd5eItemDefinitionMatchStatus::NoMatch && Candidates.IsEmpty());
	Selector.SourceId.Reset();
	TestTrue(TEXT("Empty active publication is invalid"), Catalog.MatchDefinitionSelector(TEXT("guide"), Selector, Candidates, Diagnostics) == EDnd5eItemDefinitionMatchStatus::InvalidSelector && Candidates.IsEmpty());

	FDnd5eRecordSelector Named;
	Named.Category = Dnd5e::Category::ItemMastery;
	Named.Name = TEXT("invented deflection");
	Named.bHasSource = true;
	Named.SourceId = TEXT("syn-a");
	TestTrue(TEXT("Mastery matches by name"), Catalog.MatchMasterySelector(TEXT("guide"), Named, Candidates, Diagnostics) == EDnd5eItemDefinitionMatchStatus::Matched && Candidates == TArray<FDnd5eRecordId>{Mastery.Header.RecordId});
	Named.Qualifier.InitializeAs<FDnd5eClassFeatureSelector>();
	TestTrue(TEXT("Reject unsupported qualifier"), Catalog.MatchMasterySelector(TEXT("guide"), Named, Candidates, Diagnostics) == EDnd5eItemDefinitionMatchStatus::InvalidSelector && Candidates.IsEmpty());

	FDnd5eItemPropertyDefinition Out;
	TestTrue(TEXT("Retrieve a property copy"), Catalog.TryGetProperty(A.Header.RecordId, Out));
	Out.TemplateText = TEXT("Changed");
	TestTrue(TEXT("Copy edits stay outside catalog"), Catalog.TryGetProperty(A.Header.RecordId, Out) && Out.TemplateText == A.TemplateText);
	TestFalse(TEXT("Type ID cannot retrieve a property"), Catalog.TryGetProperty(Type.Header.RecordId, Out));
	TestTrue(TEXT("Miss clears old result"), Out.TemplateText.IsEmpty());
	TestFalse(TEXT("Reject duplicate definitions"), Catalog.ReplaceDefinitions({}, {A, A}, {}, Diagnostics));
	TestTrue(TEXT("Duplicate diagnostic includes origin"), Diagnostics.Num() == 1 && Diagnostics[0].Code == TEXT("Dnd5e.ItemDefinitions.DuplicateRecordId") && Diagnostics[0].JsonPointer == TEXT("/definitions/0"));
	auto InvalidMastery = Mastery;
	InvalidMastery.Header.DisplayName = TEXT("Different key");
	TestFalse(TEXT("Reject a stale ID in the final domain"), Catalog.ReplaceDefinitions({Type}, {B}, {InvalidMastery}, Diagnostics));
	TestTrue(TEXT("Both failures preserve previous records and ordering"), Catalog.GetDefinitionIds() == OriginalIds && Catalog.TryGetProperty(A.Header.RecordId, Out));
	TestTrue(TEXT("Empty replacement clears all domains"), Catalog.ReplaceDefinitions({}, {}, {}, Diagnostics) && Catalog.GetDefinitionCount() == 0 && Diagnostics.IsEmpty());
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eDefinitionReflectionTest, "Dnd5e.Items.Catalog.ReflectedGetter", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FDnd5eDefinitionReflectionTest::RunTest(const FString& Parameters)
{
	auto* Object = NewObject<UDnd5eItemDefinitionCatalogObject>();
	FDnd5eItemPropertyDefinition Property;
	Property.Abbreviation = TEXT("P9");
	if (!TestTrue(TEXT("Blueprint factory supplies ID"), UDnd5eItemDefinitionLibrary::TryMakeDefinitionId(TEXT("guide"), Dnd5e::Category::ItemProperty, Property.Abbreviation, TEXT("SYN"), Property.Header.RecordId))) { return false; }
	FDnd5eRecordProvenance Origin;
	Origin.SourceId = TEXT("SYN");
	Property.Header.Provenance.Add(Origin);
	TArray<FDnd5eDiagnostic> Diagnostics;
	if (!TestTrue(TEXT("Populate wrapper"), Object->ReplaceDefinitions({}, {Property}, {}, Diagnostics))) { return false; }

	UFunction* Function = Object->FindFunction(TEXT("TryGetProperty"));
	if (!TestNotNull(TEXT("Reflected getter"), Function)) { return false; }
	const auto* Id = FindFProperty<FStructProperty>(Function, TEXT("Id"));
	const auto* Out = FindFProperty<FStructProperty>(Function, TEXT("Out"));
	const auto* Result = FindFProperty<FBoolProperty>(Function, TEXT("ReturnValue"));
	if (!Id || !Out || !Result) { AddError(TEXT("Reflected getter parameters are missing.")); return false; }
	TestTrue(TEXT("Getter is callable with an output pin"), Function->HasAnyFunctionFlags(FUNC_BlueprintCallable) && Out->HasAnyPropertyFlags(CPF_OutParm) && !Out->HasAnyPropertyFlags(CPF_ReferenceParm));
	FStructOnScope Frame(Function);
	auto* Memory = Frame.GetStructMemory();
	*Id->ContainerPtrToValuePtr<FDnd5eRecordId>(Memory) = Property.Header.RecordId;
	Object->ProcessEvent(Function, Memory);
	TestTrue(TEXT("Reflected getter returns the stored record"), Result->GetPropertyValue_InContainer(Memory) && Out->ContainerPtrToValuePtr<FDnd5eItemPropertyDefinition>(Memory)->Abbreviation == Property.Abbreviation);
	*Id->ContainerPtrToValuePtr<FDnd5eRecordId>(Memory) = {};
	Object->ProcessEvent(Function, Memory);
	TestTrue(TEXT("Reflected miss resets output"), !Result->GetPropertyValue_InContainer(Memory) && Out->ContainerPtrToValuePtr<FDnd5eItemPropertyDefinition>(Memory)->Abbreviation.IsEmpty());
	return !HasAnyErrors();
}
#endif
