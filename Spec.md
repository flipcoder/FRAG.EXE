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
- glock
- shotgun
- ump45
- grenadelauncher
- sniper
- medkit

See "game.json" for the current list of all default weapons, items with their stats.

## Node properties/tags

- physics (string)
    - (empty) - disables physics
    - static
    - dynamic
    - actor
    - ghost
- penetration (float [0,1] def 0) - fraction of resultant bullet damage after penetrating
- visible (bool)
- hurt (float) - amount of damage to inflict on player per second when touching, negative values heal
- tags:
    - fluid

Properties are keys with values that store information for each node set in blender.
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
    physics: dynamic
    tags: explosive,ctf
```

## Events

The following events can be used in scripting to change map state when things occur,
along with the data passed in when the event occurs

- use - When a player presses the use key on a surface.
- enter - When the map/round starts.
- touch - When a player starts touching an object.
- untouch - When a player stops touching an object.
- damage - When a player does damage to a object.
    - player
    - object
    - damage - amount of damage
    - projectile (bool)
- tick - Every tick
- see - when an object starts being visible to a player
- unsee - when an object stops being visible to a player

## Scripting

The map script first executes during the loading screen.  You set up events here
and associate them with functions.  The first event you'll want to associate is
enter().  This occurs *after* the loading screen during the first tick.

```
def enter():
    # this stuff happens after loading screen
    # ...
    
# this stuff happens during loading screen
qor.on_enter(enter)
```

Node names, properties, and events are all accessible through scripting.

If you want something to happen when you press a switch, you need to set up
those events with every switch.  You can find all the switches on a map by
*hooking*.

```
def activate(player, switch):
    # ...

switches = qor.hook("#switch") # get all nodes tagged as switches
for s in switches:
    s.on_event("use", lambda data: activate(data["player"], s))
```

The concept of a switch, as far as FRAG is concerned, is specific to your map.

You may consider reading through
[QorBook](https://github.com/flipcoder/qor/blob/master/QorBook.md)
to learn more about scripting.  Specifically the sections about different node
types since these are what you will be dealing with.

