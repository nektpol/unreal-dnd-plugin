// Developed by Neko Creative Technologies
#pragma once

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Content/Dnd5eContentLibrary.h"
#include "Dnd5eTags.h"
#include "Content/Dnd5eContentValidationLibrary.h"
#include "Spells/Dnd5eSpell.h"
#include "Spells/Dnd5eSpellContentLibrary.h"
#include "Values/Dnd5eMoney.h"
#include "Values/Dnd5eTimeSpan.h"
#include "Diagnostics/Dnd5eDiagnostic.h"
#include "Readers/Dnd5eJsonReadContext.h"
#include "Readers/Dnd5eJsonSpellReadRequest.h"
#include "Readers/Dnd5eJsonSpellReadResult.h"
#include "Readers/Dnd5eJsonSpellAvailabilityReader.h"
#include "Readers/Dnd5eJsonSpellCatalogBuilder.h"
#include "Readers/Dnd5eJsonSpellCatalogLoader.h"
#include "Readers/Dnd5eJsonSpellReferenceReporter.h"
#include "Catalog/Dnd5eSpellCatalog.h"
#include "UObject/UnrealType.h"

namespace Dnd5eTests
{
	struct FFieldContract
	{
		const TCHAR* Name;
		const TCHAR* CppType;
		const TCHAR* Default;
		bool bReadOnly = false;
	};

	struct FModelContract
	{
		const TCHAR* Name;
		UScriptStruct* (*Type)();
		TArray<FFieldContract> Fields;
		const TCHAR* Module = TEXT("/Script/Dnd5e");
	};

	// These are frozen expectations reviewed against the public model contract,
	// not values discovered from the implementation while a test is running.
	const TArray<FModelContract>& ModelContracts();
	const FModelContract* FindContract(const FString& Name);
	bool CheckDefaults(FAutomationTestBase& Test, UScriptStruct* Type, const void* Memory,
		const FString& Path);

	// Storage probes exercise every reflected member, including inactive optional
	// fields. They are deliberately NOT advertised as validated D&D definitions.
	FInstancedStruct MakeStorageProbe(UScriptStruct* Type);
	bool MutateOneLeaf(UScriptStruct* Type, void* Memory);
	bool RoundTrip(FAutomationTestBase& Test, UScriptStruct* Type, const void* Source, void* Destination);
	bool Equal(UScriptStruct* Type, const void* Left, const void* Right);

	FDnd5eInlineContent Text(const TCHAR* Value);
	FDnd5eContentBlock Paragraph(const TCHAR* Value);
	FDnd5eSpell SyntheticSpell();

	template<typename TPayload>
	FDnd5eContentBlock Block(FGameplayTag Kind, const TPayload& Payload)
	{
		FDnd5eContentBlock Result;
		Result.Kind = Kind;
		Result.Payload = FInstancedStruct::Make<TPayload>(Payload);
		return Result;
	}

	template<typename TPayload>
	FDnd5eInlineContent Inline(FGameplayTag Kind, const TPayload& Payload)
	{
		FDnd5eInlineContent Result;
		Result.Kind = Kind;
		Result.Payload = FInstancedStruct::Make<TPayload>(Payload);
		return Result;
	}
}

#endif
