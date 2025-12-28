#pragma once

#include "CoreMinimal.h"

class SDockTab;
class FSpawnTabArgs;

/**
 * Helper responsible for the Blueprint cognitive complexity dashboard tab.
 * Owns the tab spawner and Slate content; the main module only calls into this.
 */
class FCognitiveComplexityDashboard
{
public:
	static void RegisterTabSpawner();
	static void UnregisterTabSpawner();
	static void OpenDashboardTab();

private:
	/** Spawns the Slate content for the dashboard tab. */
	static TSharedRef<SDockTab> SpawnDashboardTab(const FSpawnTabArgs& Args);
};


