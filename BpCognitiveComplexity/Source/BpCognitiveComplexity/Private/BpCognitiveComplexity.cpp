// Copyright Epic Games, Inc. All Rights Reserved.

#include "BpCognitiveComplexity.h"

#include "K2Node_Event.h"
#include "K2Node_FunctionEntry.h"
#include "BpCognitiveComplexitySettings.h"
#include "CognitiveComplexityCalculator.h"
#include "Editor.h"
#include "EdGraph/EdGraph.h"
#include "Engine/Blueprint.h"
#include "EdGraphUtilities.h"
#include "Styling/AppStyle.h"
#include "KismetNodes/SGraphNodeK2Default.h"
#include "Framework/Application/SlateApplication.h"
#include "Fonts/FontMeasure.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"
#include "Rendering/DrawElements.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "ToolMenus.h"
#include "CognitiveComplexityDashboard.h"

FLinearColor ComplexityColorForPercent(const UBpCognitiveComplexitySettings& Settings, float Percent)
{
	// Keep thresholds driven by settings, but always map from green -> amber -> red -> dark maroon.
	if (Percent < Settings.LightGreenThreshold)
	{
		return FLinearColor(0.10f, 0.70f, 0.25f, 0.9f);
	}
	if (Percent < Settings.GreenThreshold)
	{
		return FLinearColor(0.10f, 0.70f, 0.25f, 0.9f);
	}
	if (Percent < Settings.OrangeThreshold)
	{
		return FLinearColor(0.85f, 0.55f, 0.10f, 0.9f);
	}
	if (Percent < Settings.RedThreshold)
	{
		return FLinearColor(0.80f, 0.10f, 0.10f, 0.9f);
	}
	return FLinearColor(0.45f, 0.0f, 0.0f, 0.95f); // dark maroon
}

class SCognitiveComplexityEntryNode : public SGraphNodeK2Default
{
public:
	SLATE_BEGIN_ARGS(SCognitiveComplexityEntryNode) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, UEdGraphNode* InNode, FBpCognitiveComplexityModule* InModule)
	{
		ComplexityModule = InModule;
		SGraphNodeK2Default::Construct(SGraphNodeK2Default::FArguments(), Cast<UK2Node>(InNode));
	}

	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override
	{
		const int32 BaseLayer = SGraphNodeK2Default::OnPaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);

		if (ComplexityModule == nullptr || GraphNode == nullptr)
		{
			return BaseLayer;
		}

		const UBpCognitiveComplexitySettings* Settings = UBpCognitiveComplexitySettings::Get();
		if (Settings == nullptr)
		{
			return BaseLayer;
		}

		const FCognitiveComplexityScore Score = ComplexityModule->GetScoreForNode(*GraphNode);
		const FSlateFontInfo FontInfo = FAppStyle::GetFontStyle(TEXT("BlueprintEditor.Node.TitleFont"));
		const FString Label = ComplexityModule->GetComplexityLabelForPercent(Score.Percent, Settings);
		// Prefix with a small diamond-like glyph to subtly echo IDE complexity indicators.
		const FString Text = FString::Printf(TEXT("◆ %s (%.0f%%)"), *Label, Score.Percent);
		const FVector2D TextSize = FSlateApplication::Get().GetRenderer()->GetFontMeasureService()->Measure(Text, FontInfo);

		const FVector2D BadgePadding(6.f, 2.f);
		const FVector2D BadgeSize = TextSize + BadgePadding * 2.0f;
		const FVector2D LocalSize = AllottedGeometry.GetLocalSize();
		const FVector2D BadgePos((LocalSize.X - BadgeSize.X) * 0.5f, -BadgeSize.Y - 4.0f);
		const FVector2f BadgeSizeF(BadgeSize);
		const FVector2f BadgePosF(BadgePos);
		
		int32 CurrentLayer = BaseLayer;

		if (ComplexityModule->GetShowEntryBadges())
		{
			const FLinearColor BadgeColor = ComplexityColorForPercent(*Settings, Score.Percent);

			// Draw only colored text (no filled background) to keep the indicator subtle.
			FSlateDrawElement::MakeText(
				OutDrawElements,
				CurrentLayer + 1,
				AllottedGeometry.ToPaintGeometry(FVector2f(TextSize), FSlateLayoutTransform(BadgePosF + FVector2f(BadgePadding))),
				Text,
				FontInfo,
				ESlateDrawEffect::None,
				BadgeColor
			);

			CurrentLayer += 1;
		}

		return CurrentLayer;
	}

private:
	FBpCognitiveComplexityModule* ComplexityModule = nullptr;
};

class FCognitiveComplexityNodeFactory : public FGraphPanelNodeFactory
{
public:
	explicit FCognitiveComplexityNodeFactory(FBpCognitiveComplexityModule& InModule)
		: Module(&InModule)
	{
	}

	virtual TSharedPtr<SGraphNode> CreateNode(UEdGraphNode* Node) const override
	{
		if (Module == nullptr || Node == nullptr)
		{
			return nullptr;
		}

		if (Node->IsA<UK2Node_Event>() || Node->IsA<UK2Node_FunctionEntry>())
		{
			return SNew(SCognitiveComplexityEntryNode, Node, Module);
		}

		return nullptr;
	}

private:
	FBpCognitiveComplexityModule* Module = nullptr;
};

void FBpCognitiveComplexityModule::StartupModule()
{
	NodeFactory = MakeShared<FCognitiveComplexityNodeFactory>(*this);
	FEdGraphUtilities::RegisterVisualNodeFactory(NodeFactory);

	// Add Tools menu toggle entry for showing/hiding entry node badges
	UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateLambda([this]()
	{
		FToolMenuOwnerScoped OwnerScoped("BpCognitiveComplexity");
		UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Tools");
		FToolMenuSection& Section = Menu->FindOrAddSection("ToolsMisc");

		Section.AddSubMenu(
			"BpCognitiveComplexity_SubMenu",
			NSLOCTEXT("BpCognitiveComplexity", "SubMenuLabel", "BP Cognitive Complexity"),
			NSLOCTEXT("BpCognitiveComplexity", "SubMenuTooltip", "Blueprint cognitive complexity tools."),
			FNewToolMenuDelegate::CreateLambda([this](UToolMenu* InMenu)
			{
				FToolMenuSection& SubSection = InMenu->AddSection("BpCognitiveComplexitySection");

				SubSection.AddMenuEntry(
					"BpCognitiveComplexity_ToggleEntryBadges",
					NSLOCTEXT("BpCognitiveComplexity", "ToggleEntryBadges", "Show Entry Badges"),
					NSLOCTEXT("BpCognitiveComplexity", "ToggleEntryBadgesTooltip", "Toggle display of cognitive complexity labels above Blueprint entry nodes for this editor session."),
					FSlateIcon(),
					FUIAction(
						FExecuteAction::CreateRaw(this, &FBpCognitiveComplexityModule::ToggleShowEntryBadges),
						FCanExecuteAction(),
						FGetActionCheckState::CreateRaw(this, &FBpCognitiveComplexityModule::GetShowEntryBadgesCheckState)),
					EUserInterfaceActionType::ToggleButton
				);

				SubSection.AddMenuEntry(
					"BpCognitiveComplexity_OpenDashboard",
					NSLOCTEXT("BpCognitiveComplexity", "OpenDashboard", "Open Complexity Dashboard"),
					NSLOCTEXT("BpCognitiveComplexity", "OpenDashboardTooltip", "Open a dashboard listing Blueprint events and functions that exceed a chosen cognitive complexity threshold."),
					FSlateIcon(),
					FUIAction(
						FExecuteAction::CreateStatic(&FCognitiveComplexityDashboard::OpenDashboardTab),
						FCanExecuteAction())
				);
			})
		);
	}));

	FCognitiveComplexityDashboard::RegisterTabSpawner();
}

void FBpCognitiveComplexityModule::ShutdownModule()
{
	if (NodeFactory.IsValid())
	{
		FEdGraphUtilities::UnregisterVisualNodeFactory(NodeFactory);
		NodeFactory.Reset();
	}

	CachedScores.Empty();

	FCognitiveComplexityDashboard::UnregisterTabSpawner();
}

void FBpCognitiveComplexityModule::ToggleShowEntryBadges()
{
	bShowEntryBadges = !bShowEntryBadges;
}

FString FBpCognitiveComplexityModule::GetComplexityLabelForPercent(float Percent, const class UBpCognitiveComplexitySettings* Settings) const
{
	if (Settings == nullptr)
	{
		return TEXT("Unknown");
	}

	if (Percent < Settings->LightGreenThreshold)
	{
		return TEXT("Simple Enough");
	}
	if (Percent < Settings->GreenThreshold)
	{
		return TEXT("Still Okay");
	}
	if (Percent < Settings->OrangeThreshold)
	{
		return TEXT("Mildly Complex");
	}
	if (Percent < Settings->RedThreshold)
	{
		return TEXT("Very Complex");
	}
	if (Percent < Settings->RedThreshold * 1.6f)
	{
		return TEXT("Extremely Complex");
	}

	return TEXT("Refactor Me, please?");
}

ECheckBoxState FBpCognitiveComplexityModule::GetShowEntryBadgesCheckState() const
{
	return bShowEntryBadges ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}

FCognitiveComplexityScore FBpCognitiveComplexityModule::GetScoreForNode(const UEdGraphNode& Node) const
{
	const UBpCognitiveComplexitySettings* Settings = UBpCognitiveComplexitySettings::Get();
	if (Settings == nullptr)
	{
		return {};
	}

	const FCognitiveComplexityCalculator Calculator(*Settings);
	return Calculator.Calculate(Node);
}

void FBpCognitiveComplexityModule::InvalidateBlueprint(UBlueprint* Blueprint)
{
	if (Blueprint == nullptr)
	{
		return;
	}

	// Collect all node GUIDs that belong to this Blueprint
	TSet<FGuid> BlueprintNodeGuids;
	
	auto CollectNodeGuids = [&BlueprintNodeGuids](UEdGraph* Graph)
	{
		if (Graph == nullptr)
		{
			return;
		}

		for (UEdGraphNode* Node : Graph->Nodes)
		{
			if (Node != nullptr)
			{
				BlueprintNodeGuids.Add(Node->NodeGuid);
			}
		}
	};

	for (UEdGraph* Graph : Blueprint->UbergraphPages)
	{
		CollectNodeGuids(Graph);
	}

	for (UEdGraph* Graph : Blueprint->FunctionGraphs)
	{
		CollectNodeGuids(Graph);
	}

	// Remove cached scores for nodes in this Blueprint
	for (auto It = CachedScores.CreateIterator(); It; ++It)
	{
		if (BlueprintNodeGuids.Contains(It.Key()))
		{
			It.RemoveCurrent();
		}
	}
}

void FBpCognitiveComplexityModule::HandleBlueprintCompiled(UBlueprint* Blueprint)
{
	if (Blueprint == nullptr)
	{
		return;
	}

	InvalidateBlueprint(Blueprint);

	const UBpCognitiveComplexitySettings* Settings = UBpCognitiveComplexitySettings::Get();
	if (Settings == nullptr)
	{
		return;
	}

	const FCognitiveComplexityCalculator Calculator(*Settings);

	auto ScoreGraph = [this, &Calculator](UEdGraph* Graph)
	{
		if (Graph == nullptr)
		{
			return;
		}

		for (UEdGraphNode* Node : Graph->Nodes)
		{
			if (Node == nullptr)
			{
				continue;
			}

			if (Node->IsA<UK2Node_Event>() || Node->IsA<UK2Node_FunctionEntry>())
			{
				const FCognitiveComplexityScore Score = Calculator.Calculate(*Node);
				CachedScores.Add(Node->NodeGuid, Score);
			}
		}
	};

	for (UEdGraph* Graph : Blueprint->UbergraphPages)
	{
		ScoreGraph(Graph);
	}

	for (UEdGraph* Graph : Blueprint->FunctionGraphs)
	{
		ScoreGraph(Graph);
	}
}

IMPLEMENT_MODULE(FBpCognitiveComplexityModule, BpCognitiveComplexity)
