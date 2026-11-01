// Developed by Neko Creative Technologies

#include "Readers/Dnd5eJsonReaderInternal.h"
#include "Dnd5eTags.h"
#include "Values/Dnd5eD20Roll.h"
#include "Values/Dnd5eScaledRoll.h"

namespace Dnd5eJsonReader
{
	namespace
	{
		bool ParseSignedInt32(const FString& Text, int32& Out)
		{
			if (Text.IsEmpty()) { return false; }
			int32 Index = 0;
			const bool Negative = Text[0] == TEXT('-');
			if (Negative || Text[0] == TEXT('+')) { ++Index; }
			if (Index == Text.Len()) { return false; }
			const int64 Limit = Negative ? 2147483648LL : 2147483647LL;
			int64 Amount = 0;
			for (; Index < Text.Len(); ++Index)
			{
				if (Text[Index] < TEXT('0') || Text[Index] > TEXT('9')) { return false; }
				const int32 Digit = Text[Index] - TEXT('0');
				if (Amount > (Limit - Digit) / 10) { return false; }
				Amount = Amount * 10 + Digit;
			}
			Out = static_cast<int32>(Negative ? -Amount : Amount);
			return true;
		}

		FDnd5eNumber IntegerValue(int32 Amount, const FString& Literal)
		{
			FDnd5eNumber Number;
			Number.Kind = EDnd5eNumberKind::Integer;
			Number.Integer = Amount; Number.SourceLiteral = Literal;
			return Number;
		}

		void AppendAnnotation(FDnd5eAnnotationContent&& Annotation, const FString& Pointer, TArray<FDnd5eInlineContent>& Inlines, FReadState& State)
		{
			FDnd5eInlineContent Inline;
			Inline.Kind = Dnd5e::Content::Inline::Annotation;
			Inline.Payload = FInstancedStruct::Make<FDnd5eAnnotationContent>(MoveTemp(Annotation));
			Inline.Metadata = MakeContentMetadata(Pointer, State);
			Inlines.Add(MoveTemp(Inline));
		}

		bool TryParseScaleInputs(const FString& Text, TArray<int32>& Inputs)
		{
			Inputs.Reset();
			if (Text.TrimStartAndEnd().IsEmpty()) { return false; }
			TArray<FString> Pieces;
			Text.ParseIntoArray(Pieces, TEXT(","), false);
			TArray<int32> Candidate;
			for (const FString& Piece : Pieces)
			{
				TArray<FString> Bounds;
				Piece.TrimStartAndEnd().ParseIntoArray(Bounds, TEXT("-"), false);
				if (Bounds.IsEmpty() || Bounds.Num() > 2) { return false; }
				int32 Minimum = 0;
				int32 Maximum = 0;
				if (!TryParseBoundedIntegerToken(Bounds[0].TrimStartAndEnd(), 1, 9, Minimum)) { return false; }
				if (Bounds.Num() == 1) { Maximum = Minimum; }
				else if (!TryParseBoundedIntegerToken(Bounds[1].TrimStartAndEnd(), 1, 9, Maximum) || Maximum < Minimum) { return false; }
				for (int32 Number = Minimum; Number <= Maximum; ++Number) { Candidate.AddUnique(Number); }
			}
			if (Candidate.IsEmpty()) { return false; }
			Candidate.Sort();
			Inputs = MoveTemp(Candidate);
			return true;
		}

		bool TryParseScaledRoll(const TArray<FString>& Parts, FDnd5eScaledRoll& Value)
		{
			Value = {};
			if (Parts.Num() < 3 || Parts.Num() > 5 || Parts[0].TrimStartAndEnd().IsEmpty() ||
				Parts[1].TrimStartAndEnd().IsEmpty() || Parts[2].TrimStartAndEnd().IsEmpty()) { return false; }
			FDnd5eScaledRoll Candidate;
			TArray<FString> Bases;
			Parts[0].ParseIntoArray(Bases, TEXT(";"), false);
			if (Bases.IsEmpty()) { return false; }
			for (const FString& Base : Bases)
			{
				if (Base.TrimStartAndEnd().IsEmpty()) { return false; }
				FDnd5eDiceExpression Expression;
				Expression.Expression = Base;
				Expression.Syntax = TEXT("5etools");
				Candidate.BaseExpressions.Add(MoveTemp(Expression));
			}
			if (!TryParseScaleInputs(Parts[1], Candidate.InputValues)) { return false; }
			Candidate.SourceProgression = Parts[1];
			Candidate.Increment.Expression = Parts[2];
			Candidate.Increment.Syntax = TEXT("5etools");
			Candidate.SourceRenderMode = Parts.Num() > 3 ? Parts[3] : FString{};
			if (Candidate.SourceRenderMode.IsEmpty()) { Candidate.InputKind = Dnd5e::Scaling::Input::SpellSlotLevel; }
			else if (Candidate.SourceRenderMode.Equals(TEXT("psi"), ESearchCase::CaseSensitive)) { Candidate.InputKind = Dnd5e::Scaling::Input::PsiPoints; }
			else { return false; }
			Value = MoveTemp(Candidate);
			return true;
		}
	}

	bool ReadAdditionalAnnotation(const FString& Tag, const TArray<FString>& Parts, const FString& Token, const FString& Pointer,
		TArray<FDnd5eInlineContent>& Inlines, FReadState& State, int32 Depth, bool& bHandled)
	{
		const bool ScaledDice = Tag.Equals(TEXT("scaledice"), ESearchCase::CaseSensitive);
		const bool ScaledDamage = Tag.Equals(TEXT("scaledamage"), ESearchCase::CaseSensitive);
		if (ScaledDice || ScaledDamage)
		{
			bHandled = true;
			FDnd5eScaledRoll Value;
			if (!TryParseScaledRoll(Parts, Value))
			{
				UnsupportedInline(Token, Pointer, Inlines, State);
				return true;
			}
			FDnd5eAnnotationContent Annotation;
			Annotation.Kind = ScaledDamage ? Dnd5e::Content::Annotation::ScaledDamageRoll : Dnd5e::Content::Annotation::ScaledDiceRoll;
			Annotation.SourceFormat = TEXT("5etools");
			Annotation.SourceAnnotation = Token;
			Annotation.Value = FInstancedStruct::Make<FDnd5eScaledRoll>(MoveTemp(Value));
			const FString Display = Parts.Num() > 4 && !Parts[4].IsEmpty() ? Parts[4] : Parts[2];
			if (!ReadInlines(Display, Pointer, Annotation.Display, State, Depth + 1)) { return false; }
			AppendAnnotation(MoveTemp(Annotation), Pointer, Inlines, State);
			return true;
		}
		const bool DC = Tag.Equals(TEXT("dc"), ESearchCase::CaseSensitive);
		const bool Hit = Tag.Equals(TEXT("hit"), ESearchCase::CaseSensitive);
		const bool D20 = Tag.Equals(TEXT("d20"), ESearchCase::CaseSensitive);
		const bool Chance = Tag.Equals(TEXT("chance"), ESearchCase::CaseSensitive);
		bHandled = DC || Hit || D20 || Chance;
		if (!bHandled) { return true; }
		const int32 MaximumParts = Chance ? 5 : (DC ? 2 : 3);
		int32 Amount = 0;
		if (Parts.IsEmpty() || Parts.Num() > MaximumParts || !ParseSignedInt32(Parts[0], Amount) ||
			(DC && Amount < 0) || (Chance && (Amount < 0 || Amount > 100)))
		{
			UnsupportedInline(Token, Pointer, Inlines, State);
			return true;
		}
		FDnd5eAnnotationContent Annotation;
		Annotation.SourceFormat = TEXT("5etools"); Annotation.SourceAnnotation = Token;
		FString Display;
		if (DC)
		{
			Annotation.Kind = Dnd5e::Content::Annotation::DifficultyClass;
			Annotation.Value = FInstancedStruct::Make<FDnd5eNumber>(IntegerValue(Amount, Parts[0]));
			Display = TEXT("DC ") + (Parts.Num() > 1 && !Parts[1].IsEmpty() ? Parts[1] : Parts[0]);
		}
		else if (Hit || D20)
		{
			FDnd5eD20Roll Roll; Roll.Modifier = IntegerValue(Amount, Parts[0]);
			if (Parts.Num() > 2) { Roll.RollName = Parts[2]; }
			Annotation.Kind = Hit ? Dnd5e::Content::Annotation::AttackRoll : Dnd5e::Content::Annotation::D20Roll;
			Annotation.Value = FInstancedStruct::Make<FDnd5eD20Roll>(Roll);
			Display = Parts.Num() > 1 && !Parts[1].IsEmpty() ? Parts[1] : (Amount >= 0 ? TEXT("+") + FString::FromInt(Amount) : FString::FromInt(Amount));
		}
		else
		{
			FDnd5eChanceContentValue Value; Value.Percent = Amount;
			if (Parts.Num() > 2) { Value.RollName = Parts[2]; }
			if (Parts.Num() > 3 && !ReadInlines(Parts[3], Pointer, Value.Success, State, Depth + 1)) { return false; }
			if (Parts.Num() > 4 && !ReadInlines(Parts[4], Pointer, Value.Failure, State, Depth + 1)) { return false; }
			Annotation.Kind = Dnd5e::Content::Annotation::PercentChance;
			Annotation.Value = FInstancedStruct::Make<FDnd5eChanceContentValue>(Value);
			Display = Parts.Num() > 1 && !Parts[1].IsEmpty() ? Parts[1] : Parts[0];
		}
		if (!ReadInlines(Display, Pointer, Annotation.Display, State, Depth + 1)) { return false; }
		AppendAnnotation(MoveTemp(Annotation), Pointer, Inlines, State);
		return true;
	}
}
