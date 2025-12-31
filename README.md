# Blueprint Cognitive Complexity

A Unreal Engine plugin that helps you write better Blueprints by displaying cognitive complexity badges on Blueprint entry nodes (Events and Functions). The plugin analyzes your Blueprint graphs and provides visual feedback to help identify overly complex code that may benefit from refactoring.

## Features

- **Visual Complexity Badges**: Displays cognitive complexity labels and percentages directly on Blueprint entry nodes (Events and Functions)
- **Color-Coded Indicators**: Uses color coding (green → amber → red → dark maroon) to quickly identify complexity levels
- **Complexity Dashboard**: A dedicated dashboard tab that lists all Blueprint events and functions exceeding configurable complexity thresholds
- **Configurable Weights**: Customize how different node types contribute to complexity scores
- **Real-Time Updates**: Complexity scores are calculated and cached when Blueprints are compiled
- **Toggle Badge Display**: Easily show or hide complexity badges via the Tools menu

## Installation

1. Copy the `BpCognitiveComplexity` folder to your project's `Plugins` directory
2. Restart Unreal Engine
3. The plugin should be automatically enabled (check **Edit → Plugins** if needed)

## Usage

### Viewing Complexity Badges

1. Open any Blueprint in the editor
2. Complexity badges will automatically appear above Event and Function entry nodes
3. Each badge shows:
   - A complexity label (e.g., "Simple Enough", "Mildly Complex", "Very Complex")
   - The complexity percentage
4. You can customize how individual nodes affect the scoring in the config file

### Toggling Badge Display

1. Go to **Tools → BP Cognitive Complexity → Show Entry Badges**
2. Toggle the checkbox to show or hide badges for the current editor session

### Using the Complexity Dashboard

1. Go to **Tools → BP Cognitive Complexity → Open Complexity Dashboard**
2. The dashboard will display all Blueprint entry points that exceed a configurable threshold
3. Results are sorted by complexity (highest first)
4. Click on any entry to navigate directly to that Blueprint

## Configuration

The plugin can be configured through the Unreal Engine Editor Settings:

1. Go to **Edit → Project Settings**
2. Navigate to **Plugins → Blueprint Cognitive Complexity**

### Weight Settings

Configure how different node types contribute to complexity:

- **Base Entry Weight** (default: 1): Base complexity added for each function/event entry
- **Branch Weight** (default: 2): Weight for branching nodes (Branch, Sequence, DoOnce)
- **Select Weight** (default: 1): Weight for Select/Switch nodes
- **Loop Weight** (default: 3): Weight for loop constructs (For, ForEach, While, DoN, LoopWithBreak)
- **Latent Weight** (default: 2): Weight for latent/async nodes (Delay, Timeline, async actions)
- **Function Call Weight** (default: 1): Weight for any function call node
- **Math Node Weight** (default: 1): Weight for pure math/data nodes
- **Nesting Penalty** (default: 1): Extra complexity added per nesting level inside branches/loops/sequences

### Display Settings

- **Percent Scale** (default: 100): Converts raw score to percentage (Percent = Score / PercentScale * 100)
- **Light Green Threshold** (default: 30): Below this percentage, complexity is considered "Simple Enough"
- **Green Threshold** (default: 60): Below this percentage, complexity is "Still Okay"
- **Orange Threshold** (default: 80): Below this percentage, complexity is "Mildly Complex"
- **Red Threshold** (default: 100): Below this percentage, complexity is "Very Complex"
- Above the Red Threshold, complexity is "Extremely Complex" or "Refactor Me, please?"

### Configuration File

Settings can also be edited directly in:
```
Plugins/BpCognitiveComplexity/Config/DefaultCognitiveComplexity.ini
```

## How It Works

The plugin calculates cognitive complexity by:

1. **Traversing the Blueprint Graph**: Starting from entry nodes (Events and Functions), the calculator walks through all connected nodes
2. **Applying Weights**: Each node type contributes to the complexity score based on configurable weights
3. **Accounting for Nesting**: Nodes nested inside branches, loops, or sequences receive additional complexity penalties
4. **Displaying Badges**: Entry nodes display color-coded badges showing the complexity label and percentage

### Complexity Labels

- **Simple Enough** (< 30%): Green - Code is straightforward and easy to understand
- **Still Okay** (30-60%): Green - Acceptable complexity level
- **Mildly Complex** (60-80%): Amber - Consider breaking into smaller functions
- **Very Complex** (80-100%): Red - Should be refactored
- **Extremely Complex** (100-160%): Red - High priority for refactoring
- **Refactor Me, please?** (> 160%): Dark Maroon - Critical refactoring needed

## Requirements

- Unreal Engine 5.0 or later
- Editor module (plugin only works in the editor, not in packaged builds)

## Technical Details

- **Module Type**: Editor-only module
- **Loading Phase**: Default
- **Category**: Development

## Author

Created by **MurtazaHere**

## License

See individual file headers for copyright information.

## Contributing

This plugin is designed to help improve Blueprint code quality by making complexity visible. If you encounter issues or have suggestions, please report them through your preferred issue tracking system.

---

**Note**: This plugin analyzes Blueprint graphs to estimate cognitive complexity. The scores are meant as guidance to help identify potentially problematic code, not as absolute measures of code quality. Use your judgment when deciding whether to refactor based on complexity scores.

