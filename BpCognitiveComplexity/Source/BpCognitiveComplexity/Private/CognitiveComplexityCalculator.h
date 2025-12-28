#pragma once

#include "CoreMinimal.h"
#include "CognitiveComplexityTypes.h"

class UEdGraphNode;
class UEdGraph;
class UBlueprint;
class UBpCognitiveComplexitySettings;

/**
 * Traverses a Blueprint graph starting from an entry node to estimate
 * cognitive complexity according to configurable weights.
 */
class FCognitiveComplexityCalculator
{
public:
	explicit FCognitiveComplexityCalculator(const UBpCognitiveComplexitySettings& InSettings);

	/** Calculates complexity for a given entry node (event/function). */
	FCognitiveComplexityScore Calculate(const UEdGraphNode& EntryNode) const;

	/** Processes a graph and returns all entry node results. */
	TArray<FCognitiveComplexityEntryResult> ProcessGraph(const UEdGraph* Graph) const;

	/** Processes a Blueprint and returns all entry node results from all graphs. */
	TArray<FCognitiveComplexityEntryResult> ProcessBlueprint(const UBlueprint* Blueprint) const;

private:
	float WalkNode(const UEdGraphNode* Node, int32 Depth) const;

	bool IsBranchNode(const UEdGraphNode* Node) const;
	bool IsSelectNode(const UEdGraphNode* Node) const;
	bool IsLoopNode(const UEdGraphNode* Node) const;
	bool IsLatentNode(const UEdGraphNode* Node) const;
	bool IsFunctionCallNode(const UEdGraphNode* Node) const;
	bool IsMathNode(const UEdGraphNode* Node) const;
	bool IsEntryNode(const UEdGraphNode* Node) const;

	const UBpCognitiveComplexitySettings& Settings;
	mutable TSet<const UEdGraphNode*> Visited;
};
