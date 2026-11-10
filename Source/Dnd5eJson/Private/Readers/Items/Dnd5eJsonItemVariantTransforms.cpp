// Developed by Neko Creative Technologies

#include "Readers/Items/Dnd5eJsonItemVariantTransforms.h"

namespace
{
	using namespace Dnd5eJsonReader;

	constexpr double MaxCalculatedAmount = 9007199254740991.0;
	constexpr int32 MaxTraitValues = 512;

	bool ApplyAmountExpression(const FJsonObject& Base, const FJsonObject& Inherits,
		const TCHAR* ExpressionKey, const TCHAR* AmountKey, const FString& BasePointer,
		const FString& InheritsPointer, FJsonObject& Row, FReadState& BaseState, FReadState& VariantState)
	{
		const auto Value = FindExactField(Inherits, ExpressionKey);
		if (!Value.IsValid()) { return true; }
		const FString Pointer = AppendPointer(InheritsPointer, ExpressionKey);
		if (FindExactField(Inherits, AmountKey).IsValid())
		{
			return Fail(VariantState, Codes::UnsupportedRequiredValue, Pointer,
				TEXT("A variant expression cannot also supply a literal override for its output."));
		}

		if ((Row.HasField(AmountKey) && !FindExactField(Row, AmountKey).IsValid()) ||
			(Row.HasField(ExpressionKey) && !FindExactField(Row, ExpressionKey).IsValid()))
		{
			return Fail(VariantState, Codes::InvalidFieldValue, Pointer, TEXT("Variant expression conflicts with a differently cased field."));
		}
		FString Text;
		const FString Prefix = FString::Printf(TEXT("[[baseItem.%s]]"), AmountKey);
		if (Value->Type != EJson::String || !Value->TryGetString(Text) ||
			!Text.TrimStartAndEnd().StartsWith(Prefix, ESearchCase::CaseSensitive))
		{
			return Fail(VariantState, Codes::UnsupportedRequiredValue, Pointer, TEXT("Unsupported item variant expression."));
		}
		Text.TrimStartAndEndInline();
		const FString Tail = Text.Mid(Prefix.Len()).TrimStartAndEnd();
		int32 Operand = 0;
		if (Tail.IsEmpty() || (Tail[0] != TEXT('+') && Tail[0] != TEXT('*')) ||
			!TryParseBoundedIntegerToken(Tail.Mid(1).TrimStartAndEnd(), 0, MAX_int32, Operand))
		{
			return Fail(VariantState, Codes::UnsupportedRequiredValue, Pointer, TEXT("Expected + or * followed by a nonnegative integer."));
		}

		const auto Input = FindExactField(Base, AmountKey);
		if (!Input.IsValid() || Input->Type == EJson::Null)
		{
			if (FindExactField(Row, AmountKey).IsValid()) { Row.RemoveField(AmountKey); }
			Row.SetField(ExpressionKey, Value);
			AddDiagnostic(VariantState, EDnd5eDiagnosticSeverity::Warning, Codes::PreservedUnsupportedMetadata,
				Pointer, TEXT("The base amount is missing. The original expression was retained without an evaluated amount."));
			return true;
		}

		double Amount = 0.0;
		FString Literal;
		if (Input->Type != EJson::Number || !Input->TryGetNumber(Amount) ||
			!Input->TryGetString(Literal) || Literal.StartsWith(TEXT("-")) ||
			!FMath::IsFinite(Amount) || Amount > MaxCalculatedAmount)
		{
			return Fail(BaseState, Codes::InvalidFieldValue, AppendPointer(BasePointer, AmountKey),
				TEXT("Expected a supported nonnegative base amount."));
		}
		const double Result = Tail[0] == TEXT('+') ? Amount + Operand : Amount * Operand;
		if (!FMath::IsFinite(Result) || Result > MaxCalculatedAmount)
		{
			return Fail(VariantState, Codes::ResourceLimitExceeded, Pointer, TEXT("The calculated amount exceeds the supported range."));
		}
		if (FindExactField(Row, ExpressionKey).IsValid()) { Row.RemoveField(ExpressionKey); }
		Row.SetNumberField(AmountKey, Result);
		return true;
	}

	bool ApplyBarding(const FJsonObject& Base, const FJsonObject& Inherits, const FString& BasePointer,
		const FString& InheritsPointer, FJsonObject& Row, FReadState& BaseState, FReadState& VariantState)
	{
		const auto Value = FindExactField(Inherits, TEXT("barding"));
		if (!Value.IsValid()) { return true; }
		const FString Pointer = AppendPointer(InheritsPointer, TEXT("barding"));
		bool bEnabled = false;
		if (Value->Type != EJson::Boolean || !Value->TryGetBool(bEnabled) || !bEnabled ||
			FindExactField(Inherits, TEXT("bardingType")).IsValid())
		{
			return Fail(VariantState, Codes::UnsupportedRequiredValue, Pointer, TEXT("Expected barding true without a separate bardingType."));
		}
		FString Type;
		if (!ReadRequiredString(Base, TEXT("type"), BasePointer, Type, BaseState) ||
			!ValidateIdentityText(Type, AppendPointer(BasePointer, TEXT("type")), BaseState)) { return false; }
		if (Row.HasField(TEXT("bardingType")) && !FindExactField(Row, TEXT("bardingType")).IsValid())
		{
			return Fail(VariantState, Codes::InvalidFieldValue, Pointer, TEXT("Barding conflicts with a differently cased field."));
		}
		Row.SetStringField(TEXT("bardingType"), Type);
		return true;
	}

	bool ContainsExact(const TArray<FString>& Values, const FString& Text)
	{
		return Values.ContainsByPredicate([&Text](const FString& Value)
		{
			return Value.Equals(Text, ESearchCase::CaseSensitive);
		});
	}

	bool ReadTraitArray(const TSharedPtr<FJsonValue>& Value, const FString& Pointer,
		TArray<FString>& Out, FReadState& State)
	{
		const TArray<TSharedPtr<FJsonValue>>* Rows = nullptr;
		if (!RequireArray(Value, Pointer, Rows, State)) { return false; }
		if (Rows->Num() > MaxTraitValues)
		{
			return Fail(State, Codes::ResourceLimitExceeded, Pointer, TEXT("Too many variant trait values."));
		}
		for (int32 Index = 0; Index < Rows->Num(); ++Index)
		{
			FString Text;
			const auto& Entry = (*Rows)[Index];
			if (Entry->Type != EJson::String || !Entry->TryGetString(Text) || !IsValidIdentityText(Text))
			{
				return Fail(State, Codes::UnsupportedRequiredValue, AppendPointer(Pointer, FString::FromInt(Index)),
					TEXT("This variant merge supports flat arrays of nonempty strings."));
			}
			Out.Add(MoveTemp(Text));
		}
		return true;
	}

	void WriteMergedTraits(FJsonObject& Row, const TCHAR* Field, const TArray<FString>& Base, const TArray<FString>& Added)
	{
		TArray<FString> Unique;
		for (const TArray<FString>* List : {&Base, &Added})
		{
			for (const FString& Text : *List)
			{
				if (!ContainsExact(Unique, Text)) { Unique.Add(Text); }
			}
		}
		TArray<TSharedPtr<FJsonValue>> Values;
		Values.Reserve(Unique.Num());
		for (FString& Text : Unique) { Values.Add(MakeShared<FJsonValueString>(MoveTemp(Text))); }
		Row.SetArrayField(Field, Values);
	}

	struct FDamageTraitMerge
	{
		const TCHAR* Field;
		TArray<FString> Base;
		TArray<FString> Added;
		bool bHasBaseArray = false;
		bool bHasAddedArray = false;
	};

	bool ApplyDamageTraits(const FJsonObject& Base, const FJsonObject& Inherits, const FString& BasePointer,
		const FString& InheritsPointer, FJsonObject& Row, FReadState& BaseState, FReadState& VariantState)
	{
		FDamageTraitMerge Traits[] = {{TEXT("vulnerable")}, {TEXT("resist")}, {TEXT("immune")}};
		const bool bHasDirective = FindExactField(Inherits, TEXT("vulnerable")).IsValid() ||
			FindExactField(Inherits, TEXT("resist")).IsValid() || FindExactField(Inherits, TEXT("immune")).IsValid();
		if (!bHasDirective) { return true; }

		for (auto& Trait : Traits)
		{
			const auto Added = FindExactField(Inherits, Trait.Field);
			if (Added.IsValid() && Added->Type == EJson::Null) { continue; }

			const auto Existing = FindExactField(Base, Trait.Field);
			Trait.bHasBaseArray = Existing.IsValid() && Existing->Type != EJson::Null;
			if (Trait.bHasBaseArray &&
				!ReadTraitArray(Existing, AppendPointer(BasePointer, Trait.Field), Trait.Base, BaseState)) { return false; }
			Trait.bHasAddedArray = Added.IsValid();
			if (Trait.bHasAddedArray &&
				!ReadTraitArray(Added, AppendPointer(InheritsPointer, Trait.Field), Trait.Added, VariantState)) { return false; }
		}

		for (const auto& Trait : Traits)
		{
			if (!Trait.bHasAddedArray) { continue; }
			for (auto& Other : Traits)
			{
				if (&Trait == &Other || !Other.bHasBaseArray) { continue; }
				Other.Base.RemoveAll([&Trait](const FString& Text) { return ContainsExact(Trait.Added, Text); });
				Other.bHasBaseArray = !Other.Base.IsEmpty();
			}
		}

		for (const auto& Trait : Traits)
		{
			if (Trait.bHasBaseArray || Trait.bHasAddedArray)
			{
				if (Row.HasField(Trait.Field) && !FindExactField(Row, Trait.Field).IsValid())
				{
					return Fail(VariantState, Codes::InvalidFieldValue, AppendPointer(InheritsPointer, Trait.Field),
						TEXT("A merged damage trait conflicts with a differently cased field."));
				}
				WriteMergedTraits(Row, Trait.Field, Trait.Base, Trait.Added);
			}
			else if (FindExactField(Row, Trait.Field).IsValid())
			{
				Row.RemoveField(Trait.Field);
			}
		}
		return true;
	}

	bool ApplyConditionImmunities(const FJsonObject& Base, const FJsonObject& Inherits, const FString& BasePointer,
		const FString& InheritsPointer, FJsonObject& Row, FReadState& BaseState, FReadState& VariantState)
	{
		const TCHAR* Field = TEXT("conditionImmune");
		const auto Added = FindExactField(Inherits, Field);
		if (!Added.IsValid()) { return true; }
		const auto Existing = FindExactField(Base, Field);
		TArray<FString> BaseValues, AddedValues;
		if (Existing.IsValid() && Existing->Type != EJson::Null &&
			!ReadTraitArray(Existing, AppendPointer(BasePointer, Field), BaseValues, BaseState)) { return false; }
		if (!ReadTraitArray(Added, AppendPointer(InheritsPointer, Field), AddedValues, VariantState)) { return false; }
		if (Row.HasField(Field) && !FindExactField(Row, Field).IsValid())
		{
			return Fail(VariantState, Codes::InvalidFieldValue, AppendPointer(InheritsPointer, Field),
				TEXT("Merged conditions conflict with a differently cased field."));
		}
		WriteMergedTraits(Row, Field, BaseValues, AddedValues);
		return true;
	}
}

bool Dnd5eJsonReader::ApplyItemVariantTransforms(const FJsonObject& Base, const FJsonObject& Inherits,
	const FString& BasePointer, const FString& InheritsPointer, FJsonObject& Row,
	FReadState& BaseState, FReadState& VariantState)
{
	return ApplyAmountExpression(Base, Inherits, TEXT("valueExpression"), TEXT("value"), BasePointer,
			InheritsPointer, Row, BaseState, VariantState) &&
		ApplyAmountExpression(Base, Inherits, TEXT("weightExpression"), TEXT("weight"), BasePointer,
			InheritsPointer, Row, BaseState, VariantState) &&
		ApplyBarding(Base, Inherits, BasePointer, InheritsPointer, Row, BaseState, VariantState) &&
		ApplyDamageTraits(Base, Inherits, BasePointer, InheritsPointer, Row, BaseState, VariantState) &&
		ApplyConditionImmunities(Base, Inherits, BasePointer, InheritsPointer, Row, BaseState, VariantState);
}