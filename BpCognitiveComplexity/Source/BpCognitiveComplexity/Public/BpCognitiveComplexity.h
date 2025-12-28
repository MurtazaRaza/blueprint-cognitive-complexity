// Copyright MurtazaHere, Inc. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "CognitiveComplexityTypes.h"

enum class ECheckBoxState : uint8;

class FBpCognitiveComplexityModule : public IModuleInterface
{
public:
	static FBpCognitiveComplexityModule& Get()
	{
		return FModuleManager::LoadModuleChecked<FBpCognitiveComplexityModule>("BpCognitiveComplexity");
	}

	static bool IsAvailable()
	{
		return FModuleManager::Get().IsModuleLoaded("BpCognitiveComplexity");
	}

	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	
	FCognitiveComplexityScore GetScoreForNode(const class UEdGraphNode& Node) const;
	void InvalidateBlueprint(class UBlueprint* Blueprint);
	bool GetShowEntryBadges() const { return bShowEntryBadges; }
	void ToggleShowEntryBadges();
	FString GetComplexityLabelForPercent(float Percent, const class UBpCognitiveComplexitySettings* Settings) const;
	ECheckBoxState GetShowEntryBadgesCheckState() const;

private:
	void HandleBlueprintCompiled(class UBlueprint* Blueprint);
	
	TSharedPtr<class FCognitiveComplexityNodeFactory> NodeFactory;
	mutable TMap<FGuid, FCognitiveComplexityScore> CachedScores;
	FDelegateHandle CompileHandle;
	bool bShowEntryBadges = true;
};
