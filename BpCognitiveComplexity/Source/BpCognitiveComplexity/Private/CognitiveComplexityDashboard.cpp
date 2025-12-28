#include "CognitiveComplexityDashboard.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "BpCognitiveComplexitySettings.h"
#include "CognitiveComplexityCalculator.h"
#include "CognitiveComplexityTypes.h"
#include "EdGraph/EdGraph.h"
#include "Editor.h"
#include "Engine/Blueprint.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Docking/TabManager.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Styling/AppStyle.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Input/SSpinBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Views/SListView.h"


static const FName DashboardTabId(TEXT("BpCognitiveComplexity_Dashboard"));

struct FCognitiveComplexityEntryRow
{
	FString BlueprintName;
	FString BlueprintPath;
	FGuid NodeGuid;
	FString EntryDisplayName;
	float RawScore = 0.0f;
	float Percent = 0.0f;
};

typedef TSharedPtr<FCognitiveComplexityEntryRow> FCognitiveComplexityEntryRowPtr;

class SCognitiveComplexityDashboard : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SCognitiveComplexityDashboard)
		{
		}

	SLATE_END_ARGS()

	SCognitiveComplexityDashboard() = default;

	void Construct(const FArguments& InArgs)
	{
		ThresholdPercent = 60.0f;

		ChildSlot
		[
			SNew(SVerticalBox)
			
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(4.0f)
			[
				SNew(SHorizontalBox)

				+SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(0.0f, 0.0f, 8.0f, 0.0f)
				[
					SNew(STextBlock)
					.Text(NSLOCTEXT("BpCognitiveComplexity", "DashboardThresholdLabel", "Complexity threshold (%):"))
				]

				+SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(0.0f, 0.0f, 8.0f, 0.0f)
				[
					SNew(SSpinBox<float>)
					.MinValue(0.0f)
					.MaxValue(500.0f)
					.Delta(5.0f)
					.Value(this, &SCognitiveComplexityDashboard::GetThresholdValue)
					.OnValueChanged(this, &SCognitiveComplexityDashboard::OnThresholdChanged)
				]

				+SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				[
					SNew(SButton)
					.OnClicked(this, &SCognitiveComplexityDashboard::OnRescanClicked)
					.Text(NSLOCTEXT("BpCognitiveComplexity", "DashboardRescan", "Rescan Blueprints"))
				]
			]

			// Results list
			+SVerticalBox::Slot()
			.FillHeight(1.0f)
			.Padding(4.0f)
			[
				SAssignNew(ListViewWidget, SListView<FCognitiveComplexityEntryRowPtr>)
				.ListItemsSource(&Rows)
				.SelectionMode(ESelectionMode::Single)
				.OnGenerateRow(this, &SCognitiveComplexityDashboard::OnGenerateRow)
				.OnMouseButtonDoubleClick(this, &SCognitiveComplexityDashboard::OnItemDoubleClicked)
			]
		];

		Refresh();
	}

private:
	float GetThresholdValue() const
	{
		return ThresholdPercent;
	}

	void OnThresholdChanged(float NewValue)
	{
		ThresholdPercent = NewValue;
		Refresh();
	}

	FReply OnRescanClicked()
	{
		Refresh();
		return FReply::Handled();
	}

	TSharedRef<ITableRow> OnGenerateRow(FCognitiveComplexityEntryRowPtr InItem,
	                                    const TSharedRef<STableViewBase>& OwnerTable) const
	{
		check(InItem.IsValid());

		const FText BlueprintText = FText::FromString(
			FString::Printf(TEXT("%s (%s)"), *InItem->BlueprintName, *InItem->BlueprintPath));
		const FText EntryText = FText::FromString(InItem->EntryDisplayName);
		const FText PercentText = FText::AsNumber(FMath::RoundToInt(InItem->Percent));

		return SNew(STableRow<FCognitiveComplexityEntryRowPtr>, OwnerTable)
			[
				SNew(SHorizontalBox)

				+ SHorizontalBox::Slot()
				.FillWidth(0.45f)
				[
					SNew(STextBlock)
					.Text(BlueprintText)
					.ToolTipText(BlueprintText)
				]

				+ SHorizontalBox::Slot()
				.FillWidth(0.35f)
				[
					SNew(STextBlock)
					.Text(EntryText)
					.ToolTipText(EntryText)
				]

				+ SHorizontalBox::Slot()
				.FillWidth(0.20f)
				.HAlign(HAlign_Right)
				[
					SNew(STextBlock)
					.Text(PercentText)
				]
			];
	}

	void OnItemDoubleClicked(FCognitiveComplexityEntryRowPtr InItem)
	{
		if (!InItem.IsValid())
		{
			return;
		}

		// Build full object path: /Game/Path/BPName.BPName
		FString NormalizedPath = InItem->BlueprintPath;
		// Strip any trailing slash
		while (NormalizedPath.EndsWith(TEXT("/")))
		{
			NormalizedPath.LeftChopInline(1, false);
		}

		const FString ObjectPathString = FString::Printf(
			TEXT("%s/%s.%s"),
			*NormalizedPath,
			*InItem->BlueprintName,
			*InItem->BlueprintName);

		UBlueprint* Blueprint = LoadObject<UBlueprint>(nullptr, *ObjectPathString);
		if (!Blueprint)
		{
			return;
		}

		// Make sure the Blueprint editor is open/focused for this asset.
		FKismetEditorUtilities::GetIBlueprintEditorForObject(Blueprint, true);

		UEdGraphNode* TargetNode = nullptr;

		auto FindNodeInGraph = [&InItem, &TargetNode](UEdGraph* Graph)
		{
			if (!Graph || TargetNode != nullptr)
			{
				return;
			}

			for (UEdGraphNode* Node : Graph->Nodes)
			{
				if (Node && Node->NodeGuid == InItem->NodeGuid)
				{
					TargetNode = Node;
					return;
				}
			}
		};

		for (UEdGraph* Graph : Blueprint->UbergraphPages)
		{
			FindNodeInGraph(Graph);
			if (TargetNode)
			{
				break;
			}
		}

		if (!TargetNode)
		{
			for (UEdGraph* Graph : Blueprint->FunctionGraphs)
			{
				FindNodeInGraph(Graph);
				if (TargetNode)
				{
					break;
				}
			}
		}

		if (TargetNode)
		{
			// Focus the Blueprint editor on the specific node (event or function entry).
			FKismetEditorUtilities::BringKismetToFocusAttentionOnObject(TargetNode);
		}
	}

	void Refresh()
	{
		Rows.Reset();

		const UBpCognitiveComplexitySettings* Settings = UBpCognitiveComplexitySettings::Get();
		if (!Settings)
		{
			if (ListViewWidget.IsValid())
			{
				ListViewWidget->RequestListRefresh();
			}
			return;
		}

		FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(
			"AssetRegistry");
		TArray<FAssetData> BlueprintAssets;
		AssetRegistryModule.Get().
		                    GetAssetsByClass(UBlueprint::StaticClass()->GetClassPathName(), BlueprintAssets, true);

		const FCognitiveComplexityCalculator Calculator(*Settings);

		for (const FAssetData& AssetData : BlueprintAssets)
		{
			// Only consider user/content Blueprints, skip engine/editor/plugin script assets.
			const FString PackagePathString = AssetData.PackagePath.ToString();
			if (!PackagePathString.StartsWith(TEXT("/Game")))
			{
				continue;
			}

			const UBlueprint* Blueprint = Cast<UBlueprint>(AssetData.GetAsset());
			if (!Blueprint)
			{
				continue;
			}

			const FString BlueprintName = Blueprint->GetName();
			const FString BlueprintPath = PackagePathString;

			// Ask the calculator to process the Blueprint
			const TArray<FCognitiveComplexityEntryResult> Results = Calculator.ProcessBlueprint(Blueprint);

			// Populate rows from results, filtering by threshold
			for (const FCognitiveComplexityEntryResult& Result : Results)
			{
				if (Result.Score.Percent >= ThresholdPercent)
				{
					FCognitiveComplexityEntryRowPtr Row = MakeShared<FCognitiveComplexityEntryRow>();
					Row->BlueprintName = BlueprintName;
					Row->BlueprintPath = BlueprintPath;
					Row->NodeGuid = Result.NodeGuid;
					Row->EntryDisplayName = Result.EntryDisplayName;
					Row->RawScore = Result.Score.RawScore;
					Row->Percent = Result.Score.Percent;

					Rows.Add(Row);
				}
			}
		}

		Rows.Sort([](const FCognitiveComplexityEntryRowPtr& A, const FCognitiveComplexityEntryRowPtr& B)
		{
			if (!A.IsValid() || !B.IsValid())
			{
				return A.IsValid();
			}

			// Sort by complexity (Percent) descending, then by Blueprint name and entry name for stability.
			if (!FMath::IsNearlyEqual(A->Percent, B->Percent))
			{
				return A->Percent > B->Percent;
			}

			if (A->BlueprintName != B->BlueprintName)
			{
				return A->BlueprintName < B->BlueprintName;
			}

			return A->EntryDisplayName < B->EntryDisplayName;
		});

		if (ListViewWidget.IsValid())
		{
			ListViewWidget->RequestListRefresh();
		}
	}

private:
	float ThresholdPercent;
	TArray<FCognitiveComplexityEntryRowPtr> Rows;
	TSharedPtr<SListView<FCognitiveComplexityEntryRowPtr>> ListViewWidget;
};

void FCognitiveComplexityDashboard::RegisterTabSpawner()
{
	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(
		                        DashboardTabId,
		                        FOnSpawnTab::CreateStatic(&FCognitiveComplexityDashboard::SpawnDashboardTab))
	                        .SetDisplayName(NSLOCTEXT("BpCognitiveComplexity", "DashboardTabTitle",
	                                                  "BP Cognitive Complexity"))
	                        .SetTooltipText(NSLOCTEXT("BpCognitiveComplexity", "DashboardTabTooltip",
	                                                  "Blueprint cognitive complexity dashboard."))
	                        .SetMenuType(ETabSpawnerMenuType::Hidden);
}

void FCognitiveComplexityDashboard::UnregisterTabSpawner()
{
	if (FGlobalTabmanager::Get()->HasTabSpawner(DashboardTabId))
	{
		FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(DashboardTabId);
	}
}

void FCognitiveComplexityDashboard::OpenDashboardTab()
{
	FGlobalTabmanager::Get()->TryInvokeTab(FTabId(DashboardTabId));
}

TSharedRef<SDockTab> FCognitiveComplexityDashboard::SpawnDashboardTab(const FSpawnTabArgs& Args)
{
	return SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		[
			SNew(SCognitiveComplexityDashboard)
		];
}
