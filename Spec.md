# FRAG.EXE Spec for Mappers

## Gamespec

See [game.json](https://github.com/flipcoder/FRAG.EXE/blob/master/bin/mods/FRAG.EXE/data/game.json) for weapon and
pickup type specifics. This is called the gamespec.  Each mode can further modify these settings.  Servers
can even further modify these (They are called gamespec, modespec, and serverspec).  Any remaining values can
be accessed via map scripting.

## Node Name Prefixes

Prefixes follow blender naming convention.
Example: Spawns may be called spawn, spawn.001, or spawn.base.

Empty nodes should be placed with the origin directly on the ground.

The following prefixes are recognized:

- spawn
- redspawn
- bluespawn
- redflag
- blueflag
- All weapon names are weapon spawns (example. ump45)
- All pickup names are pickup spawns (example. medkit)

## Node properties/tags

Properties are keys with values that store  information for each node set in blender.
The specs may or may not make use of these.
For example, there may be a flag called "penetration" with a value of
0.2 for items that bullets are able to penetrate while reducing the damage by 20%.
This is something that Gamespec would make use of, since it is specific to the game.
However, the serverspec may override this by disabling bullet penetration.

A single property called tags is a comma-separated list of string values.
An example of a tag may be "explosive" for items that can explode.

Here is an example of a mesh using properties and tags correctly:

```
Node name: Barrel.001
Properties:
    penetration: 0.2
    tags: explosive,ctf
```

The above example includes a second flag, which may mean to only include this
if the mode is CTF.

## Events

The following events can be used in scripting to change map state when things occur:

- use - When a player presses the use key on a surface.
- start - When the map/round starts.
- enter - When a player is touching a surface.
- leave - When a player stops touching a surface.
- damage - When a player does damage to a surface.
- tick - Every tick

## Scripting

Node names, properties, and events are all accessible through scripting.

If you want something to happen when you press a switch, you need to

```
def activate(player, switch):
    # ...

def enter():
    switches = qor.hook("#switch") # get all nodes tagged as switches
    for s in switches:
        s.on_event("use", lambda player: activate(player, s))
```

