#include "BpCognitiveComplexitySettings.h"

#include "Internationalization/Text.h"

const UBpCognitiveComplexitySettings* UBpCognitiveComplexitySettings::Get()
{
	return GetDefault<UBpCognitiveComplexitySettings>();
}

#if WITH_EDITOR
FText UBpCognitiveComplexitySettings::GetSectionText() const
{
	return NSLOCTEXT("BpCognitiveComplexity", "SettingsSection", "Blueprint Cognitive Complexity");
}

FText UBpCognitiveComplexitySettings::GetSectionDescription() const
{
	return NSLOCTEXT("BpCognitiveComplexity", "SettingsSectionDescription", "Configure scoring weights and display thresholds for the Blueprint cognitive complexity badges.");
}

FName UBpCognitiveComplexitySettings::GetCategoryName() const
{
	return "Plugins";
}
#endif
