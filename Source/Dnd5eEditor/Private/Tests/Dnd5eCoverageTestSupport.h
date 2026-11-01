// Original synthetic fixtures; no external database content is embedded.
#pragma once
#include "Dnd5eTestSupport.h"
#include "Readers/Dnd5eJsonSpellReader.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"

namespace Dnd5eCoverageTests
{
	inline FDnd5eJsonDocument Document(const FString& Entries = TEXT("[\"Invented text\"]"), const FString& Extra = TEXT(""))
	{
		FDnd5eJsonDocument Value; Value.Context.DatasetNamespace = TEXT("coverage-synthetic"); Value.Context.RelativeFilePath = TEXT("primary.json");
		Value.JsonText = FString::Printf(TEXT(R"json({"spell":[{"name":"Invented Beacon","source":"SYN","level":0,"school":"V","time":[{"number":1,"unit":"action"}],"range":{"type":"point","distance":{"type":"self"}},"components":{"v":true},"duration":[{"type":"instant"}],"entries":%s%s}]})json"), *Entries, *Extra);
		return Value;
	}
	inline FDnd5eJsonSpellReadResult Read(const FDnd5eJsonDocument& Value)
	{
		FDnd5eJsonSpellReadRequest Request; Request.JsonText = Value.JsonText; Request.Context = Value.Context; Request.SpellIndex = 0;
		return FDnd5eJsonSpellReader::ReadSpell(Request);
	}
	inline FDnd5eJsonDocument Token(const FString& Text)
	{
		FString Json; TArray<TSharedPtr<FJsonValue>> Values{MakeShared<FJsonValueString>(Text)};
		FJsonSerializer::Serialize(Values, TJsonWriterFactory<>::Create(&Json)); return Document(Json);
	}
	inline FDnd5eJsonDocument Supplement(const FString& Rows)
	{
		auto Value = Document(); Value.Context.RelativeFilePath = TEXT("fluff.json"); Value.JsonText = TEXT("{\"spellFluff\":") + Rows + TEXT("}"); return Value;
	}
	inline bool FirstInline(const FDnd5eSpell& Spell, FDnd5eInlineContent& Out)
	{
		FDnd5eParagraphContent Paragraph;
		if (Spell.Description.Num() != 1 || !UDnd5eContentLibrary::TryGetParagraph(Spell.Description[0], Paragraph) || Paragraph.Inlines.Num() != 1) { return false; }
		Out = Paragraph.Inlines[0]; return true;
	}
}
