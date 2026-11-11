// Developed by Neko Creative Technologies

#include "Readers/Bestiary/Dnd5eJsonCreatureCopyResolver.h"

namespace
{
	using namespace Dnd5eJsonReader;

	constexpr int32 MaxCreatureCopyLinks = 32;

	FString MakeLookupKey(const FString& Name, const FString& Source)
	{
		return EscapeIdentityComponent(Name.ToLower()) + TEXT("|") +
			EscapeIdentityComponent(Source.ToLower());
	}

	bool TryReadLookupKey(const FJsonObject& Object, FString& Out)
	{
		const auto NameValue = FindExactField(Object, TEXT("name"));
		const auto SourceValue = FindExactField(Object, TEXT("source"));
		FString Name;
		FString Source;
		if (!NameValue.IsValid() || NameValue->Type != EJson::String || !NameValue->TryGetString(Name) ||
			!SourceValue.IsValid() || SourceValue->Type != EJson::String || !SourceValue->TryGetString(Source) ||
			!IsValidIdentityText(Name) || !IsValidIdentityText(Source))
		{
			return false;
		}
		Out = MakeLookupKey(Name, Source);
		return true;
	}

	bool RequiresPreserve(const FString& Key)
	{
		static const TCHAR* Fields[] = {
			TEXT("page"), TEXT("otherSources"), TEXT("referenceSources"), TEXT("srd"), TEXT("srd52"),
			TEXT("basicRules"), TEXT("basicRules2024"), TEXT("reprintedAs"), TEXT("hasFluff"),
			TEXT("hasFluffImages"), TEXT("hasToken"), TEXT("tokenCredit"), TEXT("tokenCustom"),
			TEXT("foundryTokenScale"), TEXT("altArt"), TEXT("_versions"), TEXT("legendaryGroup"),
			TEXT("environment"), TEXT("soundClip"), TEXT("variant"), TEXT("dragonCastingColor"), TEXT("familiar"),
		};
		for (const TCHAR* Field : Fields)
		{
			if (Key.Equals(Field, ESearchCase::CaseSensitive)) { return true; }
		}
		return false;
	}

	bool HasPreservePermission(const TSharedPtr<FJsonObject>& Preserve, const TCHAR* Field)
	{
		if (!Preserve.IsValid()) { return false; }
		const auto Value = FindExactField(*Preserve, Field);
		return Value.IsValid() && Value->Type == EJson::Boolean && Value->AsBool();
	}

	struct FCreatureCopyStep
	{
		FDnd5eCreatureCopyRow Row;
		TSharedPtr<FJsonObject> Preserve;
	};

	TSharedPtr<FJsonObject> MergeCreatureCopy(const FCreatureCopyStep& Child, const FJsonObject& Parent)
	{
		const auto Merged = MakeShared<FJsonObject>();
		for (const auto& Pair : Child.Row.Object->Values)
		{
			const FString Key(Pair.Key.Len(), *Pair.Key);
			if (!Key.Equals(TEXT("_copy"), ESearchCase::CaseSensitive))
			{
				Merged->SetField(Key, Pair.Value);
			}
		}
		const bool bPreserveAll = HasPreservePermission(Child.Preserve, TEXT("*"));
		for (const auto& Pair : Parent.Values)
		{
			const FString Key(Pair.Key.Len(), *Pair.Key);
			const auto OwnValue = FindExactField(*Child.Row.Object, *Key);
			if (OwnValue.IsValid())
			{
				if (OwnValue->Type == EJson::Null) { Merged->RemoveField(Key); }
			}
			else if (bPreserveAll || !RequiresPreserve(Key) || HasPreservePermission(Child.Preserve, *Key))
			{
				Merged->SetField(Key, Pair.Value);
			}
		}
		return Merged;
	}

	bool CollectCreatureCopyChain(const FDnd5eCreatureCopyIndex& Index,
		const FDnd5eCreatureCopyRow& Child, TArray<FCreatureCopyStep>& Chain, FReadState& State)
	{
		TSet<FString> Visited;
		const FDnd5eCreatureCopyRow* Current = &Child;
		for (;;)
		{
			auto Context = State.Context;
			Context.RelativeFilePath = Current->RelativeFilePath;
			FReadState RowState{Context, State.JsonText, State.Diagnostics};
			const FString CopyPointer = AppendPointer(Current->JsonPointer, TEXT("_copy"));

			FString Key;
			if (!Current->Object.IsValid() || !TryReadLookupKey(*Current->Object, Key))
			{
				return Fail(RowState, Codes::InvalidFieldValue, Current->JsonPointer,
					TEXT("Expected a creature name and explicit source."));
			}
			if (Index.AmbiguousKeys.Contains(Key))
			{
				return Fail(RowState, Codes::InvalidFieldValue, Current->JsonPointer,
					TEXT("Creature identity is ambiguous."));
			}
			Visited.Add(Key);

			FCreatureCopyStep Step{*Current, nullptr};
			const auto CopyValue = FindExactField(*Current->Object, TEXT("_copy"));
			if (!CopyValue.IsValid())
			{
				Chain.Add(MoveTemp(Step));
				return true;
			}
			if (Chain.Num() >= MaxCreatureCopyLinks)
			{
				return Fail(RowState, Codes::ResourceLimitExceeded, CopyPointer,
					TEXT("Too many links in the creature copy chain."));
			}

			TSharedPtr<FJsonObject> Copy;
			FString ParentName;
			FString ParentSource;
			if (!RequireObject(CopyValue, CopyPointer, Copy, RowState) ||
				!CheckAllowedKeys(*Copy, {TEXT("name"), TEXT("source"), TEXT("_preserve"), TEXT("_mod"), TEXT("_templates")},
					CopyPointer, RowState) ||
				!ReadRequiredString(*Copy, TEXT("name"), CopyPointer, ParentName, RowState) ||
				!ValidateIdentityText(ParentName, AppendPointer(CopyPointer, TEXT("name")), RowState) ||
				!ReadRequiredString(*Copy, TEXT("source"), CopyPointer, ParentSource, RowState) ||
				!ValidateIdentityText(ParentSource, AppendPointer(CopyPointer, TEXT("source")), RowState))
			{
				return false;
			}
			for (const TCHAR* Deferred : {TEXT("_mod"), TEXT("_templates")})
			{
				if (FindExactField(*Copy, Deferred).IsValid())
				{
					return Fail(RowState, Codes::UnsupportedRequiredValue, AppendPointer(CopyPointer, Deferred),
						TEXT("Creature copy operation is deferred."));
				}
			}
			if (const auto Preserve = FindExactField(*Copy, TEXT("_preserve")))
			{
				const FString PreservePointer = AppendPointer(CopyPointer, TEXT("_preserve"));
				if (!RequireObject(Preserve, PreservePointer, Step.Preserve, RowState)) { return false; }
				for (const auto& Pair : Step.Preserve->Values)
				{
					if (!Pair.Value.IsValid() || Pair.Value->Type != EJson::Boolean)
					{
						const FString Field(Pair.Key.Len(), *Pair.Key);
						return Fail(RowState, Codes::InvalidFieldValue, AppendPointer(PreservePointer, Field),
							TEXT("Expected a boolean preserve flag."));
					}
				}
			}

			const FString ParentKey = MakeLookupKey(ParentName, ParentSource);
			if (Visited.Contains(ParentKey))
			{
				return Fail(RowState, Codes::InvalidFieldValue, CopyPointer, TEXT("Creature copy cycle."));
			}
			if (Index.AmbiguousKeys.Contains(ParentKey))
			{
				return Fail(RowState, Codes::UnresolvedReference, CopyPointer, TEXT("Creature copy parent is ambiguous."));
			}
			const FDnd5eCreatureCopyRow* Parent = Index.Rows.Find(ParentKey);
			if (!Parent)
			{
				return Fail(RowState, Codes::UnresolvedReference, CopyPointer, TEXT("Creature copy parent was not found."));
			}
			Chain.Add(MoveTemp(Step));
			Current = Parent;
		}
	}
}

void IndexCreatureCopyRow(FDnd5eCreatureCopyIndex& Index, const FDnd5eCreatureCopyRow& Row)
{
	FString Key;
	if (!Row.Object.IsValid() || !TryReadLookupKey(*Row.Object, Key) || Index.AmbiguousKeys.Contains(Key)) { return; }
	if (Index.Rows.Remove(Key) != 0)
	{
		Index.AmbiguousKeys.Add(Key);
		return;
	}
	Index.Rows.Add(MoveTemp(Key), Row);
}

bool ResolveCreatureCopy(const FDnd5eCreatureCopyIndex& Index, const FDnd5eCreatureCopyRow& Child,
	TSharedPtr<FJsonObject>& OutObject, TArray<FDnd5eCreatureCopyRow>& OutAncestors,
	Dnd5eJsonReader::FReadState& State)
{
	TArray<FCreatureCopyStep> Chain;
	if (!CollectCreatureCopyChain(Index, Child, Chain, State)) { return false; }

	TSharedPtr<FJsonObject> Assembled = Chain.Last().Row.Object;
	for (int32 IndexInChain = Chain.Num() - 2; IndexInChain >= 0; --IndexInChain)
	{
		Assembled = MergeCreatureCopy(Chain[IndexInChain], *Assembled);
	}

	TArray<FDnd5eCreatureCopyRow> Ancestors;
	for (int32 IndexInChain = 1; IndexInChain < Chain.Num(); ++IndexInChain)
	{
		Ancestors.Add(Chain[IndexInChain].Row);
	}
	OutObject = MoveTemp(Assembled);
	OutAncestors = MoveTemp(Ancestors);
	return true;
}