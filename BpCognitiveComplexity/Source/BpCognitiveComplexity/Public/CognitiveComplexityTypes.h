// Copyright MurtazaHere, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

struct FCognitiveComplexityScore
{
	float RawScore = 0.0f;
	float Percent = 0.0f;
};

struct FCognitiveComplexityEntryResult
{
	FGuid NodeGuid;
	FString EntryDisplayName;
	FCognitiveComplexityScore Score;
};