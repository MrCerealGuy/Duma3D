# Duma3D Indoor Level Format

Demo_2 uses the text-based **Duma3D Indoor Level Format**, with the `.d3l` extension. `Engine::Game::World::IndoorLevelLoader` reads the file at application startup; `IndoorLevelBuilder` creates its room geometry, collision, doors, lighting, switches, and teleporters. The path is relative to the executable's `assets` directory unless it is absolute. Demo_2 accepts an optional level path as its first command-line argument.

## Syntax

- UTF-8 plain text, one record per line
- Fields are separated by spaces or tabs; quoted strings and spaces within a field are not supported
- Text from `#` to the end of a line is a comment
- Records may appear in any order; references are checked after the whole file is read
- Coordinates are in meters; room centers share the same floor height

The current version is 1. Its required records are:

```text
DUMA3D_LEVEL 1
name Sector_09
settings 10 14 4 3.2
```

`settings` fields are room width/depth, center-to-center spacing between linked rooms, corridor width, and wall height. Rooms linked by a corridor must be axis-aligned neighbors exactly one spacing apart. The engine creates a floor, ceiling, perimeter walls, and one doorway opening for every link.

## Records

| Record | Fields | Purpose |
|---|---|---|
| `material` | `id texture-or-- tint-r tint-g tint-b` | Defines a texture and RGB tint. Texture paths are relative to the executable. Use `-` for a solid color. |
| `room` | `id center-x center-z` | Adds a rectangular room. |
| `link` | `room-a room-b door-id` | Connects neighboring rooms with a corridor and assigns its door. |
| `door` | `id open-or-closed required-switch-or-- material-id` | Sets the initial door state, optional power requirement, and door material. Powered doors start closed. |
| `lamp` | `room side offset material-id r g b intensity behavior` | Adds a wall luminaire and point light. Side is `north`, `east`, `south`, or `west`; offset is along the wall; behavior is `steady`, `flicker`, or `sputter`. |
| `switch` | `id room side offset door-id body handle off-led on-led` | Adds a wall-mounted lever that supplies power to the named door. Offset is along the wall. Switches start off. |
| `teleporter` | `id room offset-x offset-z destination-id material-id required-switch-or--` | Adds a teleporter pad that sends the player to the destination pad. |
| `enemy` | `id room offset-x offset-z grunt-or-demon` | Adds a hostile actor at a room-relative position. Grunts move and fire slowly; demons are tougher and more aggressive. |
| `spawn` | `room offset-x offset-z yaw-degrees` | Sets player start position and heading; zero degrees faces east (+X). |

The level builder uses these conventional material IDs for shared room parts: `wall`, `floor`, `metal`, `trim`, `warning`, and `teleporter_on`. Other material IDs are chosen by the level records. A door's `required-switch` and a teleporter's `required-switch` refer to a switch ID; `-` means no power requirement.

## Example

This is the complete level currently used by Demo_2: [sector_09.d3l](assets/levels/sector_09.d3l). It defines a six-room loop, a powered security gate, six contrasting lamps, a relay switch, a paired teleporter route, and four enemies.

Load another `.d3l` file by passing its asset-relative path to Demo_2:

```bat
build\Duma3D_Demo_2.exe assets\levels\sector_09.d3l
```
