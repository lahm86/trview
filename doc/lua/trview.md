# trview

The trview library provides information about the program in general.

# Properties

| Name | Type | Mode | Description |
| ---- | ---- | ---- | ----------- |
| level | [Level](level.md) | RW | The current level, or `nil` if no level loaded. Can be set to a `Level` instance. Will raise an error if the level could not be loaded or the user cancelled the operation.   |
| recent_files | string[] | R | The table of recently opened filenames |

# Functions

| Name | Returns | Parameters | Description |
| ---- | ------- | ---------- | ----------- |
| load | [Level](level.md) | `string` filename | Load a level from the file provided. This will not automatically load the level in the viewer. |