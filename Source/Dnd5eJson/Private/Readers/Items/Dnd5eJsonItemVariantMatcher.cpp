// Developed by Neko Creative Technologies

#include "Readers/Items/Dnd5eJsonItemVariantMatcher.h"
#include "Readers/Dnd5eJsonReaderInternal.h"
#include "Dnd5eJsonItemVariantMatcherInternal.h"
#include "Readers/Items/Dnd5eJsonItemVariantCopy.h"

namespace
{
	using namespace Dnd5eJsonReader;

	constexpr int32 MaxVariantRows = 100000;
	constexpr int32 MaxRequirementGroups = 512;
	constexpr int32 MaxPredicateFields = 32;
	constexpr int32 MaxPredicateAlternatives = 64;
	constexpr int64 MaxValueComparisons = 1000000;

	enum class EVariantEdition : uint8 { Unspecified, Classic, One };

	struct FVariantClause
	{
		FString Field;
		TArray<TSharedPtr<FJsonValue>> Allowed;
	};

	bool ReadVariantArray(const TCHAR* Field, TSharedPtr<FJsonObject>& Root,
		const TArray<TSharedPtr<FJsonValue>>*& Rows, FReadState& State)
	{
		TSharedPtr<FJsonValue> RootValue;
		if (!ValidateReadContext(State) || !ValidateDocumentText(State) || !PreflightJson(State) ||
			!ParseJsonDocument(RootValue, State) || !RequireObject(RootValue, TEXT(""), Root, State) ||
			!ReadRequiredArray(*Root, Field, TEXT(""), Rows, State)) { return false; }
		return Rows->Num() <= MaxVariantRows ||
			Fail(State, Codes::ResourceLimitExceeded, AppendPointer(TEXT(""), Field), TEXT("Too many item rows."));
	}

	bool ReadVariantEdition(const FJsonObject& Object, const FString& Pointer,
		EVariantEdition& Out, FReadState& State)
	{
		Out = EVariantEdition::Unspecified;
		const auto Value = FindExactField(Object, TEXT("edition"));
		if (!Value.IsValid() || Value->Type == EJson::Null) { return true; }

		FString Edition;
		if (Value->Type == EJson::String && Value->TryGetString(Edition))
		{
			if (Edition.Equals(TEXT("classic"), ESearchCase::CaseSensitive)) { Out = EVariantEdition::Classic; return true; }
			if (Edition.Equals(TEXT("one"), ESearchCase::CaseSensitive)) { Out = EVariantEdition::One; return true; }
		}
		return Fail(State, Codes::UnsupportedRequiredValue, AppendPointer(Pointer, TEXT("edition")),
			TEXT("Expected classic, one or an unspecified edition."));
	}

	bool IsEditionEligible(EVariantEdition Base, EVariantEdition Variant, EDnd5eJsonItemVariantEditionPolicy Policy)
	{
		if (Base == Variant || Base == EVariantEdition::Unspecified) { return true; }
		if (Policy == EDnd5eJsonItemVariantEditionPolicy::PreferClassic)
		{
			return Base == EVariantEdition::Classic && Variant != EVariantEdition::One;
		}
		return Base == EVariantEdition::One && Variant != EVariantEdition::Classic;
	}

	bool ReadVariantClauses(const FJsonObject& Object, const FString& Pointer,
		TArray<FVariantClause>& Out, FReadState& State)
	{
		if (Object.Values.Num() > MaxPredicateFields)
		{
			return Fail(State, Codes::ResourceLimitExceeded, Pointer, TEXT("Too many fields in the item variant predicate."));
		}
		TArray<FString> Keys;
		for (const auto& Pair : Object.Values) { Keys.Emplace(Pair.Key.Len(), *Pair.Key); }
		Keys.Sort([](const FString& A, const FString& B) { return A.Compare(B, ESearchCase::CaseSensitive) < 0; });
		for (const FString& Key : Keys)
		{
			const FString FieldPointer = AppendPointer(Pointer, Key);
			const auto Value = FindExactField(Object, *Key);
			FVariantClause Clause;
			Clause.Field = Key;
			if (Value->Type == EJson::Array) { Clause.Allowed = Value->AsArray(); }
			else { Clause.Allowed.Add(Value); }
			if (Clause.Allowed.Num() > MaxPredicateAlternatives)
			{
				return Fail(State, Codes::ResourceLimitExceeded, FieldPointer, TEXT("Too many predicate alternatives."));
			}
			for (int32 Index = 0; Index < Clause.Allowed.Num(); ++Index)
			{
				const auto Type = Clause.Allowed[Index]->Type;
				if (Type != EJson::String && Type != EJson::Boolean)
				{
					const FString ValuePointer = Value->Type == EJson::Array
						? AppendPointer(FieldPointer, FString::FromInt(Index)) : FieldPointer;
					return Fail(State, Codes::UnsupportedRequiredValue, ValuePointer,
						TEXT("Item variant predicates support strings and Booleans."));
				}
			}
			Out.Add(MoveTemp(Clause));
		}
		return true;
	}

	bool ReadVariantRules(const FJsonObject& Object, const FString& Pointer,
		TArray<TArray<FVariantClause>>& Requirements, TArray<FVariantClause>& Exclusions, FReadState& State)
	{
		const TArray<TSharedPtr<FJsonValue>>* Rows = nullptr;
		if (!ReadRequiredArray(Object, TEXT("requires"), Pointer, Rows, State)) { return false; }
		const FString RequiresPointer = AppendPointer(Pointer, TEXT("requires"));
		if (Rows->Num() > MaxRequirementGroups)
		{
			return Fail(State, Codes::ResourceLimitExceeded, RequiresPointer, TEXT("Too many requirement groups."));
		}
		for (int32 Index = 0; Index < Rows->Num(); ++Index)
		{
			const FString GroupPointer = AppendPointer(RequiresPointer, FString::FromInt(Index));
			TSharedPtr<FJsonObject> Group;
			if (!RequireObject((*Rows)[Index], GroupPointer, Group, State) ||
				!ReadVariantClauses(*Group, GroupPointer, Requirements.AddDefaulted_GetRef(), State)) { return false; }
		}

		const auto Value = FindExactField(Object, TEXT("excludes"));
		if (!Value.IsValid() || Value->Type == EJson::Null) { return true; }
		const FString ExcludesPointer = AppendPointer(Pointer, TEXT("excludes"));
		TSharedPtr<FJsonObject> Excludes;
		return RequireObject(Value, ExcludesPointer, Excludes, State) &&
			ReadVariantClauses(*Excludes, ExcludesPointer, Exclusions, State);
	}

	bool MatchesVariantScalar(const FJsonValue& Candidate, const FJsonValue& Allowed)
	{
		if (Candidate.Type != Allowed.Type) { return false; }
		if (Allowed.Type == EJson::Boolean) { return Candidate.AsBool() == Allowed.AsBool(); }
		return Candidate.AsString().Equals(Allowed.AsString(), ESearchCase::CaseSensitive);
	}

	bool TestVariantClause(const FJsonObject& Base, const FVariantClause& Clause, const FString& Pointer,
		int64& Comparisons, bool& bMatches, FReadState& State)
	{
		const auto Value = FindExactField(Base, *Clause.Field);
		const bool bArray = Value.IsValid() && Value->Type == EJson::Array;
		const int32 CandidateCount = bArray ? Value->AsArray().Num() : 1;
		Comparisons += FMath::Max<int64>(1, static_cast<int64>(CandidateCount) * Clause.Allowed.Num());
		if (Comparisons > MaxValueComparisons)
		{
			return Fail(State, Codes::ResourceLimitExceeded, AppendPointer(Pointer, Clause.Field),
				TEXT("Item variant matching exceeded its comparison budget."));
		}

		bMatches = false;
		if (!Value.IsValid()) { return true; }
		const auto MatchesAny = [&Clause](const TSharedPtr<FJsonValue>& Candidate)
		{
			return Clause.Allowed.ContainsByPredicate([&](const TSharedPtr<FJsonValue>& Allowed)
			{
				return MatchesVariantScalar(*Candidate, *Allowed);
			});
		};
		bMatches = bArray ? Value->AsArray().ContainsByPredicate(MatchesAny) : MatchesAny(Value);
		return true;
	}

	bool TestVariantGroup(const FJsonObject& Base, const TArray<FVariantClause>& Group, const FString& Pointer,
		bool bEvery, int64& Comparisons, bool& bMatches, FReadState& State)
	{
		for (const auto& Clause : Group)
		{
			bool bClauseMatches = false;
			if (!TestVariantClause(Base, Clause, Pointer, Comparisons, bClauseMatches, State)) { return false; }
			if (bClauseMatches != bEvery)
			{
				bMatches = !bEvery;
				return true;
			}
		}
		bMatches = bEvery;
		return true;
	}
}

FDnd5eJsonItemVariantMatchResult Dnd5eJsonReader::MatchItemVariant(const FDnd5eJsonDocument& VariantDocument,
	int32 VariantIndex, const FDnd5eJsonDocument& BaseItemDocument, EDnd5eJsonItemVariantEditionPolicy Policy,
	FItemVariantMatchInputs* OutInputs)
{
	using namespace Dnd5eJsonReader;
	FDnd5eJsonItemVariantMatchResult Result;
	Result.Status = EDnd5eJsonReadStatus::Failure;
	FReadState VariantState{VariantDocument.Context, VariantDocument.JsonText, Result.Diagnostics};
	FReadState BaseState{BaseItemDocument.Context, BaseItemDocument.JsonText, Result.Diagnostics};

	if (Policy != EDnd5eJsonItemVariantEditionPolicy::SourceDefault &&
		Policy != EDnd5eJsonItemVariantEditionPolicy::PreferClassic)
	{
		Fail(VariantState, Codes::InvalidFieldValue, TEXT(""), TEXT("Unknown item variant edition policy."));
		return Result;
	}

	TSharedPtr<FJsonObject> VariantRoot;
	const TArray<TSharedPtr<FJsonValue>>* Variants = nullptr;
	if (!ReadVariantArray(TEXT("magicvariant"), VariantRoot, Variants, VariantState)) { return Result; }
	const FString VariantPointer = AppendPointer(TEXT("/magicvariant"), FString::FromInt(VariantIndex));
	FResolvedItemVariantTemplate Resolved;
	if (!ResolveItemVariantTemplate(*Variants, VariantIndex, Resolved, VariantState)) { return Result; }
	TSharedPtr<FJsonObject> Variant = MoveTemp(Resolved.Row);

	FString Name, Source;
	TSharedPtr<FJsonObject> Inherits;
	const FString InheritsPointer = AppendPointer(VariantPointer, TEXT("inherits"));
	EVariantEdition VariantEdition = EVariantEdition::Unspecified;
	TArray<TArray<FVariantClause>> Requirements;
	TArray<FVariantClause> Exclusions;
	if (!ReadRequiredString(*Variant, TEXT("name"), VariantPointer, Name, VariantState) ||
		!ValidateIdentityText(Name, AppendPointer(VariantPointer, TEXT("name")), VariantState) ||
		!ReadRequiredObject(*Variant, TEXT("inherits"), VariantPointer, Inherits, VariantState) ||
		!ReadRequiredString(*Inherits, TEXT("source"), InheritsPointer, Source, VariantState) ||
		!ValidateIdentityText(Source, AppendPointer(InheritsPointer, TEXT("source")), VariantState) ||
		!ReadVariantEdition(*Variant, VariantPointer, VariantEdition, VariantState) ||
		!ReadVariantRules(*Variant, VariantPointer, Requirements, Exclusions, VariantState)) { return Result; }

	TSharedPtr<FJsonObject> BaseRoot;
	const TArray<TSharedPtr<FJsonValue>>* Bases = nullptr;
	if (!ReadVariantArray(TEXT("baseitem"), BaseRoot, Bases, BaseState)) { return Result; }
	if (!VariantDocument.Context.DatasetNamespace.Equals(BaseItemDocument.Context.DatasetNamespace, ESearchCase::CaseSensitive))
	{
		Fail(BaseState, Codes::InvalidFieldValue, TEXT(""), TEXT("The base and variant documents need the same dataset namespace."));
		return Result;
	}

	TArray<int32> Matches;
	int64 Comparisons = 0;
	for (int32 Index = 0; Index < Bases->Num(); ++Index)
	{
		const FString Pointer = AppendPointer(TEXT("/baseitem"), FString::FromInt(Index));
		TSharedPtr<FJsonObject> Base;
		if (!RequireObject((*Bases)[Index], Pointer, Base, BaseState)) { return Result; }
		EVariantEdition BaseEdition = EVariantEdition::Unspecified;
		if (!ReadVariantEdition(*Base, Pointer, BaseEdition, BaseState)) { return Result; }

		if (const auto Pack = FindExactField(*Base, TEXT("packContents")); Pack.IsValid() && Pack->Type != EJson::Null)
		{
			if (Pack->Type != EJson::Array)
			{
				Fail(BaseState, Codes::InvalidFieldValue, AppendPointer(Pointer, TEXT("packContents")), TEXT("Expected a pack contents array."));
				return Result;
			}
			continue;
		}
		if (!IsEditionEligible(BaseEdition, VariantEdition, Policy)) { continue; }

		bool bRequired = false;
		for (const auto& Group : Requirements)
		{
			if (!TestVariantGroup(*Base, Group, Pointer, true, Comparisons, bRequired, BaseState)) { return Result; }
			if (bRequired) { break; }
		}
		if (!bRequired) { continue; }

		bool bExcluded = false;
		if (!TestVariantGroup(*Base, Exclusions, Pointer, false, Comparisons, bExcluded, BaseState)) { return Result; }
		if (!bExcluded) { Matches.Add(Index); }
	}

	Result.VariantName = MoveTemp(Name);
	Result.VariantSourceId = MoveTemp(Source);
	Result.MatchingBaseIndices = MoveTemp(Matches);
	Result.Status = EDnd5eJsonReadStatus::Success;
	if (OutInputs)
	{
		*OutInputs = {MoveTemp(VariantRoot), MoveTemp(Variant), MoveTemp(BaseRoot), Bases,
			MoveTemp(Resolved.IdentitySource), MoveTemp(Resolved.SourceRowIndices)};
	}
	return Result;
}

FDnd5eJsonItemVariantMatchResult FDnd5eJsonItemVariantMatcher::MatchAtIndex(const FDnd5eJsonDocument& VariantDocument,
	int32 VariantIndex, const FDnd5eJsonDocument& BaseItemDocument, EDnd5eJsonItemVariantEditionPolicy Policy)
{
	return Dnd5eJsonReader::MatchItemVariant(VariantDocument, VariantIndex, BaseItemDocument, Policy, nullptr);
}
