// Developed by Neko Creative Technologies
#if WITH_DEV_AUTOMATION_TESTS

#include "Dnd5eTestSupport.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"
#include "Serialization/ObjectAndNameAsStringProxyArchive.h"

namespace Dnd5eTests
{
	// Committed golden schema: intentional public API changes require reviewing it.
	#include "Dnd5eModelContracts.inl"

	const FModelContract* FindContract(const FString& Name)
	{
		return ModelContracts().FindByPredicate([&](const FModelContract& Case) { return Name == Case.Name; });
	}

	bool Equal(UScriptStruct* Type, const void* Left, const void* Right)
	{
		return Type->CompareScriptStruct(Left, Right, 0);
	}

	bool CheckDefaults(FAutomationTestBase& Test, UScriptStruct* Type, const void* Memory,
		const FString& Path)
	{
		const FModelContract* Contract = ModelContracts().FindByPredicate(
			[&](const FModelContract& Case) { return Case.Type() == Type; });
		if (!Test.TestNotNull(*FString::Printf(TEXT("%s has a default contract"), *Path), Contract))
		{
			return false;
		}
		for (const FFieldContract& Field : Contract->Fields)
		{
			const FProperty* Property = Type->FindPropertyByName(Field.Name);
			const FString Label = Path + TEXT(".") + Field.Name;
			if (!Test.TestNotNull(*Label, Property)) { continue; }
			const void* Value = Property->ContainerPtrToValuePtr<void>(Memory);
			if (const FBoolProperty* Bool = CastField<FBoolProperty>(Property))
			{
				Test.TestEqual(*Label, Bool->GetPropertyValue(Value), FString(Field.Default) == TEXT("true"));
			}
			else if (const FEnumProperty* Enum = CastField<FEnumProperty>(Property))
			{
				const int64 Number = Enum->GetUnderlyingProperty()->GetSignedIntPropertyValue(Value);
				FString Expected = Field.Default;
				int32 Separator;
				if (Expected.FindLastChar(TEXT(':'), Separator)) { Expected = Expected.Mid(Separator + 1); }
				Test.TestEqual(*Label, Enum->GetEnum()->GetNameStringByValue(Number), Expected);
			}
			else if (const FNumericProperty* Number = CastField<FNumericProperty>(Property))
			{
				if (Number->IsInteger())
				{
					Test.TestEqual(*Label, Number->GetSignedIntPropertyValue(Value), FCString::Atoi64(Field.Default));
				}
				else
				{
					Test.TestEqual(*Label, Number->GetFloatingPointPropertyValue(Value), FCString::Atod(Field.Default));
				}
			}
			else if (const FStrProperty* String = CastField<FStrProperty>(Property))
			{
				Test.TestTrue(*Label, String->GetPropertyValue(Value).IsEmpty());
			}
			else if (const FArrayProperty* Array = CastField<FArrayProperty>(Property))
			{
				Test.TestEqual(*Label, FScriptArrayHelper(Array, Value).Num(), 0);
			}
			else if (const FStructProperty* Struct = CastField<FStructProperty>(Property))
			{
				if (Struct->Struct == FGameplayTag::StaticStruct())
				{
					Test.TestFalse(*Label, static_cast<const FGameplayTag*>(Value)->IsValid());
				}
				else if (Struct->Struct == FGameplayTagContainer::StaticStruct())
				{
					Test.TestTrue(*Label, static_cast<const FGameplayTagContainer*>(Value)->IsEmpty());
				}
				else if (Struct->Struct == FInstancedStruct::StaticStruct())
				{
					Test.TestFalse(*Label, static_cast<const FInstancedStruct*>(Value)->IsValid());
				}
				else { CheckDefaults(Test, Struct->Struct, Value, Label); }
			}
			else { Test.AddError(Label + TEXT(": add an explicit default contract for this property kind")); }
		}
		return !Test.HasAnyErrors();
	}

	namespace
	{
		void SeedStruct(UScriptStruct* Type, void* Memory, int32 Seed, int32 Depth);

		void SeedProperty(FProperty* Property, void* Value, int32 Seed, int32 Depth)
		{
			check(Depth < 24); // The probe handles only bounded synthetic data, never input files.
			if (FBoolProperty* Bool = CastField<FBoolProperty>(Property))
			{
				// Dirty default-true flags as well as presence flags. Otherwise a
				// partial reset could leave quotation/striping flags stale unnoticed.
				Bool->SetPropertyValue(Value, !Bool->GetPropertyValue(Value));
			}
			else if (FEnumProperty* Enum = CastField<FEnumProperty>(Property))
			{
				Enum->GetUnderlyingProperty()->SetIntPropertyValue(Value, static_cast<int64>(1));
			}
			else if (FNumericProperty* Number = CastField<FNumericProperty>(Property))
			{
				if (Number->IsInteger()) { Number->SetIntPropertyValue(Value, static_cast<int64>(Seed + 17)); }
				else { Number->SetFloatingPointPropertyValue(Value, 0.123456789012345 + Seed); }
			}
			else if (FStrProperty* String = CastField<FStrProperty>(Property))
			{
				String->SetPropertyValue(Value, FString::Printf(TEXT("Synthetic %d: Ω 雪 \"quoted\"\nsecond line"), Seed));
			}
			else if (FArrayProperty* Array = CastField<FArrayProperty>(Property))
			{
				FScriptArrayHelper Items(Array, Value);
				Items.Resize(2);
				for (int32 Index = 0; Index < Items.Num(); ++Index)
				{
					SeedProperty(Array->Inner, Items.GetRawPtr(Index), Seed + Index + 1, Depth + 1);
				}
			}
			else if (FStructProperty* Struct = CastField<FStructProperty>(Property))
			{
				if (Struct->Struct == FGameplayTag::StaticStruct())
				{
					*static_cast<FGameplayTag*>(Value) = Dnd5e::Damage::Type::Fire;
				}
				else if (Struct->Struct == FGameplayTagContainer::StaticStruct())
				{
					auto& Tags = *static_cast<FGameplayTagContainer*>(Value);
					Tags.AddTag(Dnd5e::Damage::Type::Fire);
					Tags.AddTag(Dnd5e::Damage::Type::Cold);
				}
				else if (Struct->Struct == FInstancedStruct::StaticStruct())
				{
					FDnd5eTextContent TextValue;
					TextValue.Text = FString::Printf(TEXT("Nested synthetic payload %d"), Seed);
					*static_cast<FInstancedStruct*>(Value) = FInstancedStruct::Make<FDnd5eTextContent>(TextValue);
				}
				else { SeedStruct(Struct->Struct, Value, Seed, Depth + 1); }
			}
			else { checkNoEntry(); }
		}

		void SeedStruct(UScriptStruct* Type, void* Memory, int32 Seed, int32 Depth)
		{
			int32 Index = 0;
			for (TFieldIterator<FProperty> It(Type); It; ++It)
			{
				SeedProperty(*It, It->ContainerPtrToValuePtr<void>(Memory), Seed + Index++, Depth);
			}
		}

		bool MutateProperty(FProperty* Property, void* Value)
		{
			if (FStrProperty* String = CastField<FStrProperty>(Property))
			{
				String->SetPropertyValue(Value, String->GetPropertyValue(Value) + TEXT(" / changed"));
				return true;
			}
			if (FArrayProperty* Array = CastField<FArrayProperty>(Property))
			{
				FScriptArrayHelper Items(Array, Value);
				return Items.Num() > 0 && MutateProperty(Array->Inner, Items.GetRawPtr(0));
			}
			if (FStructProperty* Struct = CastField<FStructProperty>(Property))
			{
				if (Struct->Struct == FInstancedStruct::StaticStruct())
				{
					auto& Payload = *static_cast<FInstancedStruct*>(Value);
					return Payload.IsValid() && MutateOneLeaf(const_cast<UScriptStruct*>(Payload.GetScriptStruct()), Payload.GetMutableMemory());
				}
				if (Struct->Struct == FGameplayTag::StaticStruct())
				{
					*static_cast<FGameplayTag*>(Value) = Dnd5e::Damage::Type::Thunder;
					return true;
				}
				if (Struct->Struct == FGameplayTagContainer::StaticStruct())
				{
					static_cast<FGameplayTagContainer*>(Value)->AddTag(Dnd5e::Damage::Type::Thunder);
					return true;
				}
				return MutateOneLeaf(Struct->Struct, Value);
			}
			if (FBoolProperty* Bool = CastField<FBoolProperty>(Property))
			{
				Bool->SetPropertyValue(Value, !Bool->GetPropertyValue(Value));
				return true;
			}
			if (FNumericProperty* Number = CastField<FNumericProperty>(Property))
			{
				if (Number->IsInteger()) { Number->SetIntPropertyValue(Value, Number->GetSignedIntPropertyValue(Value) + 1); }
				else { Number->SetFloatingPointPropertyValue(Value, Number->GetFloatingPointPropertyValue(Value) + 1.0); }
				return true;
			}
			return false;
		}
	}

	FInstancedStruct MakeStorageProbe(UScriptStruct* Type)
	{
		FInstancedStruct Probe(Type);
		SeedStruct(Type, Probe.GetMutableMemory(), 1, 0);
		return Probe;
	}

	bool MutateOneLeaf(UScriptStruct* Type, void* Memory)
	{
		for (TFieldIterator<FProperty> It(Type); It; ++It)
		{
			if (MutateProperty(*It, It->ContainerPtrToValuePtr<void>(Memory))) { return true; }
		}
		return false;
	}

	bool RoundTrip(FAutomationTestBase& Test, UScriptStruct* Type, const void* Source, void* Destination)
	{
		TArray<uint8> Bytes;
		FMemoryWriter Writer(Bytes, true);
		FObjectAndNameAsStringProxyArchive Save(Writer, false);
		Type->SerializeItem(Save, const_cast<void*>(Source), nullptr);
		if (!Test.TestFalse(TEXT("Saving the reflected struct succeeds"), Save.IsError())) { return false; }
		FMemoryReader Reader(Bytes, true);
		FObjectAndNameAsStringProxyArchive Load(Reader, true);
		Type->SerializeItem(Load, Destination, nullptr);
		Test.TestFalse(TEXT("Loading the reflected struct succeeds"), Load.IsError());
		Test.TestEqual(TEXT("The entire binary payload was consumed"), Reader.Tell(), static_cast<int64>(Bytes.Num()));
		return Test.TestTrue(TEXT("All reflected data survives binary round trip"), Equal(Type, Source, Destination));
	}

	FDnd5eInlineContent Text(const TCHAR* Value)
	{
		FDnd5eTextContent Payload;
		Payload.Text = Value;
		return Inline(Dnd5e::Content::Inline::Text, Payload);
	}

	FDnd5eContentBlock Paragraph(const TCHAR* Value)
	{
		FDnd5eParagraphContent Payload;
		Payload.Inlines.Add(Text(Value));
		return Block(Dnd5e::Content::Block::Paragraph, Payload);
	}
}

#endif
