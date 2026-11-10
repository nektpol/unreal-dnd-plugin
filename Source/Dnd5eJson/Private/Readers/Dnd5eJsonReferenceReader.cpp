
// Developed by Neko Creative Technologies

#include "Readers/Dnd5eJsonReaderInternal.h"
#include "Dnd5eTags.h"
#include "Items/Dnd5eItemIdentity.h"

namespace Dnd5eJsonReader
{
	namespace
	{
		FGameplayTag ReferenceCategory(const FString& Tag)
		{
			if (Tag.Equals(TEXT("spell"), ESearchCase::CaseSensitive)) { return Dnd5e::Category::Spell; }
			if (Tag.Equals(TEXT("item"), ESearchCase::CaseSensitive)) { return Dnd5e::Category::Item; }
			if (Tag.Equals(TEXT("creature"), ESearchCase::CaseSensitive)) { return Dnd5e::Category::Creature; }
			if (Tag.Equals(TEXT("condition"), ESearchCase::CaseSensitive)) { return Dnd5e::Category::Condition; }
			if (Tag.Equals(TEXT("status"), ESearchCase::CaseSensitive)) { return Dnd5e::Category::Status; }
			if (Tag.Equals(TEXT("action"), ESearchCase::CaseSensitive)) { return Dnd5e::Category::Action; }
			if (Tag.Equals(TEXT("skill"), ESearchCase::CaseSensitive)) { return Dnd5e::Category::Skill; }
			if (Tag.Equals(TEXT("sense"), ESearchCase::CaseSensitive)) { return Dnd5e::Category::Sense; }
			if (Tag.Equals(TEXT("hazard"), ESearchCase::CaseSensitive)) { return Dnd5e::Category::Hazard; }
			if (Tag.Equals(TEXT("variantrule"), ESearchCase::CaseSensitive)) { return Dnd5e::Category::Rule; }
			if (Tag.Equals(TEXT("race"), ESearchCase::CaseSensitive)) { return Dnd5e::Category::Species; }
			if (Tag.Equals(TEXT("feat"), ESearchCase::CaseSensitive)) { return Dnd5e::Category::Feat; }
			if (Tag.Equals(TEXT("background"), ESearchCase::CaseSensitive)) { return Dnd5e::Category::Background; }
			if (Tag.Equals(TEXT("classFeature"), ESearchCase::CaseSensitive)) { return Dnd5e::Category::ClassFeature; }
			if (Tag.Equals(TEXT("itemEntry"), ESearchCase::CaseSensitive)) { return Dnd5e::Category::ItemEntry; }
			return {};
		}
	}

	bool IsRecordReferenceTag(const FString& Tag) { return ReferenceCategory(Tag).IsValid(); }

	bool TryReadRecordSelector(const FString& Tag, const TArray<FString>& Parts, FDnd5eRecordSelector& Selector, FString& Display)
	{
		Selector = {}; Display.Reset();
		FDnd5eRecordSelector Candidate; Candidate.Category = ReferenceCategory(Tag);
		const bool Feature = Candidate.Category == Dnd5e::Category::ClassFeature;
		if (!Candidate.Category.IsValid() || Parts.IsEmpty() || Parts.Num() > (Feature ? 6 : 3) || (Feature && Parts.Num() < 4)) { return false; }
		Candidate.Name = Parts[0].TrimStartAndEnd();
		if (!IsValidIdentityText(Candidate.Name) || Candidate.Name.Contains(TEXT("{")) || Candidate.Name.Contains(TEXT("}"))) { return false; }
		if (Feature)
		{
			FDnd5eClassFeatureSelector Qualifier;
			Qualifier.ClassName = Parts[1].TrimStartAndEnd(); Qualifier.ClassSourceId = Parts[2].TrimStartAndEnd();
			Qualifier.bHasClassSource = !Qualifier.ClassSourceId.IsEmpty();
			if (!IsValidIdentityText(Qualifier.ClassName) || Qualifier.ClassName.Contains(TEXT("{")) || Qualifier.ClassName.Contains(TEXT("}")) ||
				(Qualifier.bHasClassSource && (!IsValidIdentityText(Qualifier.ClassSourceId) || Qualifier.ClassSourceId.Contains(TEXT("{")) || Qualifier.ClassSourceId.Contains(TEXT("}")))) ||
				!TryParseBoundedIntegerToken(Parts[3].TrimStartAndEnd(), 1, 20, Qualifier.Level)) { return false; }
			Candidate.SourceId = Parts.Num() > 4 ? Parts[4].TrimStartAndEnd() : FString{};
			Candidate.Qualifier = FInstancedStruct::Make<FDnd5eClassFeatureSelector>(MoveTemp(Qualifier));
			Display = Parts.Num() > 5 && !Parts[5].IsEmpty() ? Parts[5] : Candidate.Name;
		}
		else
		{
			Candidate.SourceId = Parts.Num() > 1 ? Parts[1].TrimStartAndEnd() : FString{};
			Display = Parts.Num() > 2 && !Parts[2].IsEmpty() ? Parts[2] : Candidate.Name;
		}
		Candidate.bHasSource = !Candidate.SourceId.IsEmpty();
		if (Candidate.bHasSource && (!IsValidIdentityText(Candidate.SourceId) || Candidate.SourceId.Contains(TEXT("{")) || Candidate.SourceId.Contains(TEXT("}")))) { return false; }
		Selector = MoveTemp(Candidate); return true;
	}

	bool ReadRecordReference(const FString& Tag, const TArray<FString>& Parts, const FString& Token, const FString& Pointer,
		TArray<FDnd5eInlineContent>& Inlines, FReadState& State, int32 Depth)
	{
		FDnd5eReferenceContent Reference; FString Display;
		if (!TryReadRecordSelector(Tag, Parts, Reference.TargetSelector, Display)) { UnsupportedInline(Token, Pointer, Inlines, State); return true; }
		Reference.bHasTargetSelector = true; Reference.SourceFormat = TEXT("5etools"); Reference.SourceReference = Token;
		if (!ReadInlines(Display, Pointer, Reference.Display, State, Depth + 1)) { return false; }
		if (Reference.TargetSelector.Category == Dnd5e::Category::Spell && Reference.TargetSelector.bHasSource)
		{
			Reference.bHasTargetRecordId = true;
			Reference.TargetRecordId = MakeSpellRecordId(Reference.TargetSelector.Name, Reference.TargetSelector.SourceId, State.Context.DatasetNamespace);
		}
		else if (Reference.TargetSelector.Category == Dnd5e::Category::ItemEntry && Reference.TargetSelector.bHasSource &&
			Dnd5e::Items::TryMakeEntryTemplateId(State.Context.DatasetNamespace, Reference.TargetSelector.Name,
			Reference.TargetSelector.SourceId, Reference.TargetRecordId))
		{
			Reference.bHasTargetRecordId = true;
		}
		else
		{
			AddDiagnostic(State, EDnd5eDiagnosticSeverity::Warning, Codes::UnresolvedReference, Pointer, TEXT("Typed selector retained without an effective target ID; source/default or domain lookup remains unresolved."));
		}
		FDnd5eInlineContent Inline; Inline.Kind = Dnd5e::Content::Inline::Reference;
		Inline.Payload = FInstancedStruct::Make<FDnd5eReferenceContent>(MoveTemp(Reference)); Inline.Metadata = MakeContentMetadata(Pointer, State);
		Inlines.Add(MoveTemp(Inline)); return true;
	}
}
