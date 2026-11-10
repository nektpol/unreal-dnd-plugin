// Developed by Neko Creative Technologies
#if WITH_DEV_AUTOMATION_TESTS

#include "Dnd5eTestSupport.h"
#include "Catalog/Dnd5eItemDefinitionCatalog.h"
#include "Readers/Spells/Dnd5eJsonSpellReferenceNormalizer.h"
#include "UObject/Package.h"

using namespace Dnd5eTests;

IMPLEMENT_COMPLEX_AUTOMATION_TEST(FDnd5eModelContractTest, "Dnd5e.Model.Contract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

void FDnd5eModelContractTest::GetTests(TArray<FString>& Names, TArray<FString>& Commands) const
{
	for (const FModelContract& Case : ModelContracts()) { Names.Add(Case.Name); Commands.Add(Case.Name); }
}

bool FDnd5eModelContractTest::RunTest(const FString& Parameters)
{
	const FModelContract* Case = FindContract(Parameters);
	if (!TestNotNull(TEXT("The requested public model is in the contract"), Case)) { return false; }
	UScriptStruct* Type = Case->Type();
	if (!TestNotNull(TEXT("The public struct is reflected and linked across modules"), Type)) { return false; }
	TestTrue(TEXT("The value is available as a Blueprint type"), Type->GetBoolMetaData(TEXT("BlueprintType")));
	TestEqual(TEXT("The struct belongs to its declared runtime module"), Type->GetOutermost()->GetName(), FString(Case->Module));
	int32 ReflectedCount = 0;
	for (TFieldIterator<FProperty> It(Type); It; ++It) { ++ReflectedCount; }
	TestEqual(TEXT("No public field was silently omitted or added"), ReflectedCount, Case->Fields.Num());
	for (const FFieldContract& Field : Case->Fields)
	{
		const FProperty* Property = Type->FindPropertyByName(Field.Name);
		if (!TestNotNull(*FString::Printf(TEXT("%s is reflected"), Field.Name), Property)) { continue; }
		FString ExtendedType;
		const FString CppType = Property->GetCPPType(&ExtendedType) + ExtendedType;
		TestEqual(*FString::Printf(TEXT("%s keeps its public type and array element type"), Field.Name),
			CppType, FString(Field.CppType));
		TestTrue(*FString::Printf(TEXT("%s is editable"), Field.Name), Property->HasAnyPropertyFlags(CPF_Edit) == !Field.bReadOnly);
		TestTrue(*FString::Printf(TEXT("%s is Blueprint-visible"), Field.Name), Property->HasAnyPropertyFlags(CPF_BlueprintVisible));
		TestTrue(*FString::Printf(TEXT("%s keeps its Blueprint access contract"), Field.Name), Property->HasAnyPropertyFlags(CPF_BlueprintReadOnly) == Field.bReadOnly);
	}
	FInstancedStruct Defaults(Type);
	CheckDefaults(*this, Type, Defaults.GetMemory(), Case->Name);
	return !HasAnyErrors();
}

IMPLEMENT_COMPLEX_AUTOMATION_TEST(FDnd5eModelRoundTripTest, "Dnd5e.Model.CopyAndSerialization",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

void FDnd5eModelRoundTripTest::GetTests(TArray<FString>& Names, TArray<FString>& Commands) const
{
	for (const FModelContract& Case : ModelContracts()) { Names.Add(Case.Name); Commands.Add(Case.Name); }
}

bool FDnd5eModelRoundTripTest::RunTest(const FString& Parameters)
{
	const FModelContract* Case = FindContract(Parameters);
	if (!TestNotNull(TEXT("The requested model exists"), Case)) { return false; }
	UScriptStruct* Type = Case->Type();
	FInstancedStruct Original = MakeStorageProbe(Type);
	const FInstancedStruct Snapshot = Original;
	FInstancedStruct Copy = Original;
	TestTrue(TEXT("Ordinary value copy initially preserves every reflected field"),
		Equal(Type, Original.GetMemory(), Copy.GetMemory()));
	TestTrue(TEXT("The copy has an editable leaf"), MutateOneLeaf(Type, Copy.GetMutableMemory()));
	TestFalse(TEXT("The edit really changed the copy"), Equal(Type, Original.GetMemory(), Copy.GetMemory()));
	TestTrue(TEXT("Editing a stored leaf in the copy preserves the original"),
		Equal(Type, Original.GetMemory(), Snapshot.GetMemory()));
	FInstancedStruct Restored(Type);
	RoundTrip(*this, Type, Original.GetMemory(), Restored.GetMutableMemory());
	TestTrue(TEXT("Serialization is read-only for the original"),
		Equal(Type, Original.GetMemory(), Snapshot.GetMemory()));
	return !HasAnyErrors();
}

namespace
{
	struct FEnumContract
	{
		const TCHAR* Name;
		UEnum* (*Type)();
		TArray<FString> Members;
	};
	const TArray<FEnumContract>& EnumContracts()
	{
		static const TArray<FEnumContract> Cases =
		{
			{ TEXT("EDnd5eItemDefinitionMatchStatus"), &StaticEnum<EDnd5eItemDefinitionMatchStatus>, { TEXT("NotAttempted"), TEXT("Matched"), TEXT("MissingSource"), TEXT("InvalidSelector"), TEXT("NoMatch") } },
			{ TEXT("EDnd5eWeaponCategory"), &StaticEnum<EDnd5eWeaponCategory>, { TEXT("Unspecified"), TEXT("Simple"), TEXT("Martial") } },
			{ TEXT("EDnd5eArmorClassValueKind"), &StaticEnum<EDnd5eArmorClassValueKind>, { TEXT("Unspecified"), TEXT("Base"), TEXT("Bonus") } },
			{ TEXT("EDnd5eArmorClassDexterityMode"), &StaticEnum<EDnd5eArmorClassDexterityMode>, { TEXT("Unspecified"), TEXT("NotAdded"), TEXT("Uncapped"), TEXT("Capped") } },
			{ TEXT("EDnd5eSize"), &StaticEnum<EDnd5eSize>, { TEXT("Unspecified"), TEXT("Tiny"), TEXT("Small"), TEXT("Medium"), TEXT("Large"), TEXT("Huge"), TEXT("Gargantuan") } },
			{ TEXT("EDnd5eMassUnit"), &StaticEnum<EDnd5eMassUnit>, { TEXT("Unspecified"), TEXT("Ounces"), TEXT("Pounds"), TEXT("Grams"), TEXT("Kilograms") } },
			{ TEXT("EDnd5eValuePresence"), &StaticEnum<EDnd5eValuePresence>, { TEXT("Unspecified"), TEXT("ExplicitNull"), TEXT("Present") } },
			{ TEXT("EDnd5eItemChargeAmountKind"), &StaticEnum<EDnd5eItemChargeAmountKind>, { TEXT("Unspecified"), TEXT("ExplicitNull"), TEXT("Fixed"), TEXT("Dice") } },
			{ TEXT("EDnd5eItemRechargeTrigger"), &StaticEnum<EDnd5eItemRechargeTrigger>, { TEXT("Unspecified"), TEXT("Dawn"), TEXT("Dusk"), TEXT("Midnight"), TEXT("ShortRest"), TEXT("LongRest"), TEXT("Decade"), TEXT("Special") } },
			{ TEXT("EDnd5eAttunementMode"), &StaticEnum<EDnd5eAttunementMode>, { TEXT("Unspecified"), TEXT("NotRequired"), TEXT("Required"), TEXT("Optional") } },
			{ TEXT("EDnd5eSourceInclusion"), &StaticEnum<EDnd5eSourceInclusion>, { TEXT("Unspecified"), TEXT("Excluded"), TEXT("Included"), TEXT("IncludedWithAlternateName") } },
			{ TEXT("EDnd5eDataAvailability"), &StaticEnum<EDnd5eDataAvailability>, { TEXT("Unknown"), TEXT("Absent"), TEXT("Present") } },
			{ TEXT("EDnd5eListMarkerStyle"), &StaticEnum<EDnd5eListMarkerStyle>, { TEXT("Unspecified"), TEXT("Bullet"), TEXT("Decimal"), TEXT("LowerRoman"), TEXT("UpperRoman"), TEXT("None") } },
			{ TEXT("EDnd5eContentAlignment"), &StaticEnum<EDnd5eContentAlignment>, { TEXT("Unspecified"), TEXT("Left"), TEXT("Center"), TEXT("Right"), TEXT("Justify") } },
			{ TEXT("EDnd5ePageReferenceKind"), &StaticEnum<EDnd5ePageReferenceKind>, { TEXT("Unspecified"), TEXT("Number"), TEXT("Text") } },
			{ TEXT("EDnd5eSourceFragmentKind"), &StaticEnum<EDnd5eSourceFragmentKind>, { TEXT("Unspecified"), TEXT("Text"), TEXT("Json") } },
			{ TEXT("EDnd5eNumberKind"), &StaticEnum<EDnd5eNumberKind>, { TEXT("Unspecified"), TEXT("Integer"), TEXT("Decimal"), TEXT("Literal") } },
			{ TEXT("EDnd5eMediaReferenceKind"), &StaticEnum<EDnd5eMediaReferenceKind>, { TEXT("Unspecified"), TEXT("DatasetRelative"), TEXT("ExternalUrl") } },
			{ TEXT("EDnd5eAreaDimensionKind"), &StaticEnum<EDnd5eAreaDimensionKind>, { TEXT("Unspecified"), TEXT("Radius"), TEXT("Diameter"), TEXT("Length"), TEXT("Width"), TEXT("Height"), TEXT("EdgeLength"), TEXT("Extent") } },
			{ TEXT("EDnd5eDistanceUnit"), &StaticEnum<EDnd5eDistanceUnit>, { TEXT("Unspecified"), TEXT("Inches"), TEXT("Feet"), TEXT("Yards"), TEXT("Miles") } },
			{ TEXT("EDnd5eTimeUnit"), &StaticEnum<EDnd5eTimeUnit>, { TEXT("Unspecified"), TEXT("Turn"), TEXT("Round"), TEXT("Minute"), TEXT("Hour"), TEXT("Day"), TEXT("Week"), TEXT("Month"), TEXT("Year") } },
			{ TEXT("EDnd5eDurationKind"), &StaticEnum<EDnd5eDurationKind>, { TEXT("Unspecified"), TEXT("Instantaneous"), TEXT("Timed"), TEXT("Permanent"), TEXT("Special") } },
			{ TEXT("EDnd5eCurrency"), &StaticEnum<EDnd5eCurrency>, { TEXT("Unspecified"), TEXT("Copper"), TEXT("Silver"), TEXT("Electrum"), TEXT("Gold"), TEXT("Platinum") } },
			{ TEXT("EDnd5eMaterialConsumption"), &StaticEnum<EDnd5eMaterialConsumption>, { TEXT("Unspecified"), TEXT("Consumed"), TEXT("Optional"), TEXT("NotConsumed") } },
			{ TEXT("EDnd5eDiagnosticSeverity"), &StaticEnum<EDnd5eDiagnosticSeverity>, { TEXT("Unspecified"), TEXT("Info"), TEXT("Warning"), TEXT("Error") } },
			{ TEXT("EDnd5eJsonReadStatus"), &StaticEnum<EDnd5eJsonReadStatus>, { TEXT("NotAttempted"), TEXT("Success"), TEXT("Failure") } },
			{ TEXT("EDnd5eReferenceResolveStatus"), &StaticEnum<EDnd5eReferenceResolveStatus>, { TEXT("NotAttempted"), TEXT("Resolved"), TEXT("MissingTargetId"), TEXT("InvalidTargetId"), TEXT("UnsupportedCategory"), TEXT("TargetNotFound") } },
			{ TEXT("EDnd5eSpellSelectorMatchStatus"), &StaticEnum<EDnd5eSpellSelectorMatchStatus>, { TEXT("NotAttempted"), TEXT("Matched"), TEXT("MissingSource"), TEXT("InvalidSelector"), TEXT("NoMatch"), TEXT("Ambiguous") } },
			{ TEXT("EDnd5eJsonSpellReferenceNormalizeStatus"), &StaticEnum<EDnd5eJsonSpellReferenceNormalizeStatus>, { TEXT("NotAttempted"), TEXT("Normalized"), TEXT("UnsupportedReference"), TEXT("MissingSource"), TEXT("InvalidSelector"), TEXT("NoMatch"), TEXT("Ambiguous"), TEXT("InvalidCatalog") } },
		};
		return Cases;
	}
}

IMPLEMENT_COMPLEX_AUTOMATION_TEST(FDnd5eEnumContractTest, "Dnd5e.Model.EnumContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

void FDnd5eEnumContractTest::GetTests(TArray<FString>& Names, TArray<FString>& Commands) const
{
	for (const FEnumContract& Case : EnumContracts()) { Names.Add(Case.Name); Commands.Add(Case.Name); }
}

bool FDnd5eEnumContractTest::RunTest(const FString& Parameters)
{
	const FEnumContract* Case = EnumContracts().FindByPredicate([&](const FEnumContract& Item) { return Parameters == Item.Name; });
	if (!TestNotNull(TEXT("The enum contract exists"), Case)) { return false; }
	UEnum* Type = Case->Type();
	if (!TestNotNull(TEXT("The enum is reflected and linked"), Type)) { return false; }
	TestTrue(TEXT("The enum is exposed to Blueprints"), Type->GetBoolMetaData(TEXT("BlueprintType")));
	// UHT appends one hidden _MAX entry. Public values are the fixed names below.
	TestEqual(TEXT("The public enum vocabulary has not silently changed"), Type->NumEnums() - 1, Case->Members.Num());
	for (int32 Index = 0; Index < Case->Members.Num(); ++Index)
	{
		TestEqual(*Case->Members[Index], Type->GetNameStringByValue(Index), Case->Members[Index]);
	}
	return !HasAnyErrors();
}

#endif
