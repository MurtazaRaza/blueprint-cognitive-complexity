#include "CognitiveComplexityCalculator.h"

#include "BpCognitiveComplexitySettings.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraph/EdGraphPin.h"
#include "EdGraphSchema_K2.h"
#include "Engine/Blueprint.h"
#include "K2Node.h"
#include "K2Node_CallFunction.h"
#include "K2Node_CommutativeAssociativeBinaryOperator.h"
#include "K2Node_ExecutionSequence.h"
#include "K2Node_IfThenElse.h"
#include "K2Node_MathExpression.h"
#include "K2Node_Select.h"
#include "K2Node_SwitchEnum.h"
#include "K2Node_SwitchInteger.h"
#include "K2Node_SwitchName.h"
#include "K2Node_SwitchString.h"
#include "K2Node_Timeline.h"
#include "K2Node_BaseAsyncTask.h"
#include "K2Node_Event.h"
#include "K2Node_FunctionEntry.h"

namespace CognitiveComplexity
{
	static FName GetNodeClassName(const UEdGraphNode* Node)
	{
		return Node ? Node->GetClass()->GetFName() : NAME_None;
	}

	static bool NodeClassIsOneOf(const UEdGraphNode* Node, std::initializer_list<FName> Names)
	{
		const FName ClassName = GetNodeClassName(Node);
		for (FName Name : Names)
		{
			if (ClassName == Name)
			{
				return true;
			}
		}
		return false;
	}

	static bool HasExecOutputs(const UEdGraphNode* Node)
	{
		for (const UEdGraphPin* Pin : Node->Pins)
		{
			if (Pin && Pin->Direction == EGPD_Output && Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Exec)
			{
				return true;
			}
		}
		return false;
	}
}

FCognitiveComplexityCalculator::FCognitiveComplexityCalculator(const UBpCognitiveComplexitySettings& InSettings)
	: Settings(InSettings)
{
}

FCognitiveComplexityScore FCognitiveComplexityCalculator::Calculate(const UEdGraphNode& EntryNode) const
{
	Visited.Reset();

	const float RawScore = WalkNode(&EntryNode, /*Depth*/0);
	const float Percent = (RawScore / static_cast<float>(Settings.PercentScale)) * 100.0f;

	FCognitiveComplexityScore Result;
	Result.RawScore = RawScore;
	Result.Percent = Percent;
	return Result;
}

float FCognitiveComplexityCalculator::WalkNode(const UEdGraphNode* Node, int32 Depth) const
{
	if (Node == nullptr || Visited.Contains(Node))
	{
		return 0.0f;
	}

	Visited.Add(Node);

	float Score = 0.0f;

	// Base weight for entry nodes.
	if (IsEntryNode(Node))
	{
		Score += Settings.BaseEntryWeight;
	}

	// Node-specific weights.
	if (IsLoopNode(Node))
	{
		Score += Settings.LoopWeight;
	}

	if (IsBranchNode(Node))
	{
		Score += Settings.BranchWeight;
	}

	if (IsSelectNode(Node))
	{
		Score += Settings.SelectWeight;
	}

	if (IsLatentNode(Node))
	{
		Score += Settings.LatentWeight;
	}

	if (IsFunctionCallNode(Node))
	{
		Score += Settings.FunctionCallWeight;
	}
	else if (IsMathNode(Node))
	{
		Score += Settings.MathNodeWeight;
	}

	// Nesting penalty.
	Score += static_cast<float>(Settings.NestingPenalty * Depth);

	// Traverse exec outputs.
	const bool bIncreaseDepth = IsLoopNode(Node) || IsBranchNode(Node) || IsSelectNode(Node);
	const int32 NextDepth = bIncreaseDepth ? Depth + 1 : Depth;

	for (const UEdGraphPin* Pin : Node->Pins)
	{
		if (Pin == nullptr || Pin->Direction != EGPD_Output || Pin->PinType.PinCategory != UEdGraphSchema_K2::PC_Exec)
		{
			continue;
		}

		for (const UEdGraphPin* Linked : Pin->LinkedTo)
		{
			if (Linked == nullptr)
			{
				continue;
			}

			const UEdGraphNode* LinkedNode = Linked->GetOwningNode();
			Score += WalkNode(LinkedNode, NextDepth);
		}
	}

	return Score;
}

bool FCognitiveComplexityCalculator::IsBranchNode(const UEdGraphNode* Node) const
{
	return CognitiveComplexity::NodeClassIsOneOf(Node, { TEXT("K2Node_IfThenElse"), TEXT("K2Node_ExecutionSequence"), TEXT("K2Node_MultiGate") });
}

bool FCognitiveComplexityCalculator::IsSelectNode(const UEdGraphNode* Node) const
{
	return CognitiveComplexity::NodeClassIsOneOf(Node, { TEXT("K2Node_Select"), TEXT("K2Node_SwitchInteger"), TEXT("K2Node_SwitchEnum"), TEXT("K2Node_SwitchName"), TEXT("K2Node_SwitchString"), TEXT("K2Node_Switch") });
}

bool FCognitiveComplexityCalculator::IsLoopNode(const UEdGraphNode* Node) const
{
	const FName ClassName = CognitiveComplexity::GetNodeClassName(Node);

	// Native loop-style nodes.
	if (ClassName.ToString().Contains(TEXT("ForLoop")) || ClassName.ToString().Contains(TEXT("WhileLoop")))
	{
		return true;
	}

	// ForEach nodes shipped as separate classes.
	if (CognitiveComplexity::NodeClassIsOneOf(Node, { TEXT("K2Node_MapForEach"), TEXT("K2Node_SetForEach"), TEXT("K2Node_ForEachElementInEnum") }))
	{
		return true;
	}

	// Macro-based loop implementations (e.g. standard library ForLoop/WhileLoop macros).
	if (ClassName == TEXT("K2Node_MacroInstance"))
	{
		const FString Title = Node->GetNodeTitle(ENodeTitleType::FullTitle).ToString();
		if (Title.Contains(TEXT("ForLoop")) || Title.Contains(TEXT("ForEach")) || Title.Contains(TEXT("While")))
		{
			return true;
		}
	}

	return false;
}

bool FCognitiveComplexityCalculator::IsLatentNode(const UEdGraphNode* Node) const
{
	// Check for latent/async nodes - using class name string comparison as fallback for classes without headers
	const FName ClassName = CognitiveComplexity::GetNodeClassName(Node);
	const FString ClassNameStr = ClassName.ToString();

	if (ClassName == TEXT("K2Node_Timeline") || ClassName == TEXT("K2Node_BaseAsyncTask") || ClassName == TEXT("K2Node_AsyncAction"))
	{
		return true;
	}

	return ClassNameStr.Contains(TEXT("LatentGameplayCall")) || ClassNameStr.Contains(TEXT("AsyncTask"));
}

bool FCognitiveComplexityCalculator::IsFunctionCallNode(const UEdGraphNode* Node) const
{
	return Node->IsA<UK2Node_CallFunction>();
}

bool FCognitiveComplexityCalculator::IsMathNode(const UEdGraphNode* Node) const
{
	// Math expression nodes or generic pure nodes with no exec pins.
	const UK2Node* K2Node = Cast<UK2Node>(Node);
	return Node->IsA<UK2Node_MathExpression>() || Node->IsA<UK2Node_CommutativeAssociativeBinaryOperator>() || (K2Node && !CognitiveComplexity::HasExecOutputs(Node) && K2Node->IsNodePure());
}

bool FCognitiveComplexityCalculator::IsEntryNode(const UEdGraphNode* Node) const
{
	return CognitiveComplexity::NodeClassIsOneOf(Node, { TEXT("K2Node_Event"), TEXT("K2Node_FunctionEntry") });
}

TArray<FCognitiveComplexityEntryResult> FCognitiveComplexityCalculator::ProcessGraph(const UEdGraph* Graph) const
{
	TArray<FCognitiveComplexityEntryResult> Results;

	if (!Graph)
	{
		return Results;
	}

	for (UEdGraphNode* Node : Graph->Nodes)
	{
		if (!Node)
		{
			continue;
		}

		if (IsEntryNode(Node))
		{
			const FCognitiveComplexityScore Score = Calculate(*Node);
			
			FCognitiveComplexityEntryResult Result;
			Result.NodeGuid = Node->NodeGuid;
			Result.EntryDisplayName = Node->GetNodeTitle(ENodeTitleType::ListView).ToString();
			Result.Score = Score;
			
			Results.Add(Result);
		}
	}

	return Results;
}

TArray<FCognitiveComplexityEntryResult> FCognitiveComplexityCalculator::ProcessBlueprint(const UBlueprint* Blueprint) const
{
	TArray<FCognitiveComplexityEntryResult> Results;

	if (!Blueprint)
	{
		return Results;
	}

	// Process all Ubergraph pages
	for (UEdGraph* Graph : Blueprint->UbergraphPages)
	{
		Results.Append(ProcessGraph(Graph));
	}

	// Process all function graphs
	for (UEdGraph* Graph : Blueprint->FunctionGraphs)
	{
		Results.Append(ProcessGraph(Graph));
	}

	return Results;
}
