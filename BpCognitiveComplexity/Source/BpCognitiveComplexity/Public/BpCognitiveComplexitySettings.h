// Copyright MurtazaHere, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BpCognitiveComplexitySettings.generated.h"

/**
 * User-configurable weights for the Blueprint cognitive complexity meter.
 *
 * The config name is "CognitiveComplexity" so defaults live in
 * Config/DefaultCognitiveComplexity.ini inside the plugin.
 */
UCLASS(config=CognitiveComplexity, defaultconfig, meta=(DisplayName="Blueprint Cognitive Complexity"))
class UBpCognitiveComplexitySettings : public UObject
{
	GENERATED_BODY()

public:
	static const UBpCognitiveComplexitySettings* Get();

#if WITH_EDITOR
	FText GetSectionText() const;
	FText GetSectionDescription() const;
	FName GetCategoryName() const;
#endif

public:
	/** Added once per function/event entry. */
	UPROPERTY(EditAnywhere, config, Category="Weights", meta=(ClampMin="0"))
	int32 BaseEntryWeight = 1;

	/** Branching flow-control: Branch, Sequence, DoOnce. */
	UPROPERTY(EditAnywhere, config, Category="Weights", meta=(ClampMin="0"))
	int32 BranchWeight = 2;

	/** Select/Switch nodes. */
	UPROPERTY(EditAnywhere, config, Category="Weights", meta=(ClampMin="0"))
	int32 SelectWeight = 1;

	/** Loop constructs: For, ForEach, While, DoN, LoopWithBreak. */
	UPROPERTY(EditAnywhere, config, Category="Weights", meta=(ClampMin="0"))
	int32 LoopWeight = 3;

	/** Latent/async nodes: Delay, Timeline, async actions. */
	UPROPERTY(EditAnywhere, config, Category="Weights", meta=(ClampMin="0"))
	int32 LatentWeight = 2;

	/** Any function call node, pure or impure. */
	UPROPERTY(EditAnywhere, config, Category="Weights", meta=(ClampMin="0"))
	int32 FunctionCallWeight = 1;

	/** Pure math/data nodes. */
	UPROPERTY(EditAnywhere, config, Category="Weights", meta=(ClampMin="0"))
	int32 MathNodeWeight = 1;

	/** Extra added per nesting level inside branches/loops/sequences. */
	UPROPERTY(EditAnywhere, config, Category="Weights", meta=(ClampMin="0"))
	int32 NestingPenalty = 1;

	/** Converts raw score to percent: Percent = Score / PercentScale * 100. */
	UPROPERTY(EditAnywhere, config, Category="Display", meta=(ClampMin="1"))
	int32 PercentScale = 100;

	/** Thresholds for badge coloring. */
	UPROPERTY(EditAnywhere, config, Category="Display", meta=(ClampMin="0"))
	int32 LightGreenThreshold = 30;

	UPROPERTY(EditAnywhere, config, Category="Display", meta=(ClampMin="0"))
	int32 GreenThreshold = 60;

	UPROPERTY(EditAnywhere, config, Category="Display", meta=(ClampMin="0"))
	int32 OrangeThreshold = 80;

	UPROPERTY(EditAnywhere, config, Category="Display", meta=(ClampMin="0"))
	int32 RedThreshold = 100;
};
