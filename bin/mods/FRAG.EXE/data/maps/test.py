import qor

# player = qor.Mesh("player.obj")

music = qor.Sound("cave.ogg")
music.ambient(True)
qor.root().add(music)
qor.on_enter(music.play)

# def redflagcb(f,p):
#     qor.log("red flag taken");
#     snd = qor.Sound("redflagtaken.wav")
#     snd.ambient(True)
#     snd.spawn()
#     snd.detach_on_done()
#     snd.play()
#     f.discard()
# def blueflagcb(f,p):
#     qor.log("blue flag taken");
#     snd = qor.Sound("blueflagtaken.wav")
#     snd.ambient(True)
#     snd.spawn()
#     snd.detach_on_done()
#     snd.play()
#     f.discard()

class Wrapper:
    def __init__(self, value):
        self.value = value

i = 0
for w in ['ump45', 'gun_glock', 'gun_rocketrifle', 'gun_grenadelauncher', 'item_healthkit']:
    wpn = qor.Mesh(w + '.obj')
    wpn.position(qor.vec3(3.0, 0.0, i), qor.Space.PARENT)
    wpn.on_tick(lambda t, wpn=wpn: wpn.rotate(t, qor.vec3(0.0, 1.0, 0.0), qor.Space.LOCAL))
    wpn.spawn()
    i += 1

lift = qor.Mesh("lift.obj")
liftsnd = qor.Sound("elevator.wav")
liftsnd.loop(True)
lift.add(liftsnd)
lift.position(qor.vec3(-3.0, 0.0, 0.0))
lift.set_physics(qor.PhysicsType.DYNAMIC)
lift.set_physics_shape(qor.PhysicsShape.HULL)
# lift.mass(1000.0)
lift.inertia(False)
lift.spawn()
lift.generate()
lift.gravity(qor.vec3(0.0))
liftvel = Wrapper(0.0)
def golift(m):
    liftvel.value = 1.0
    liftsnd.play()
lift.on_event("use", golift)
lift.on_tick(lambda t: lift.move(qor.vec3(0.0, liftvel.value * t, 0.0), qor.Space.PARENT))

btn = qor.quad(1.0, "switch1_off.png")
btn.position(qor.vec3(0.0, 0.0, -4.0))
btn.set_physics(qor.PhysicsType.STATIC)
btn.set_physics_shape(qor.PhysicsShape.MESH)
btn.spawn()
btn.generate()
btn.state("state","off")
def toggle(m):
    lights = qor.root().hook_type("light")
    if btn.state("state") == "off":
        for l in lights:
            qor.Light(l).diffuse(qor.Color(0.0, 1.0, 0.0))
        btn.state("state","on")
    else:
        for l in lights:
            qor.Light(l).diffuse(qor.Color(1.0, 1.0, 1.0))
        btn.state("state","off")
    btn.material("switch1_"+btn.state("state")+".png")
btn.on_event("use", toggle)

# redflag = qor.Mesh("item_flag.obj")
# redflag.position(qor.vec3(2,0,0))
# qor.on_touch(redflag, 0, redflagcb)
# redflag.spawn()
# redflag.nullify()

# blueflag = qor.Mesh("item_flag.obj").prototype()
# blueflag.material("wall_solid_blue.png")
# blueflag.position(qor.vec3(-2,0,0))
# qor.on_touch(blueflag, 0, blueflagcb)
# blueflag.spawn()
# blueflag.nullify()

# player.set_physics(qor.PhysicsType.DYNAMIC)
# player.set_physics_shape(qor.PhysicsShape.HULL)
# player.on_event("use", lambda _: qor.log("test!!!"))
# player.position(qor.vec3(0.0, 0.0, -2.0))
# player.inertia(False)
# player.spawn()
# player.generate()

# def enter():
#     pass

# def logic(t):
#     pass
    
# qor.on_enter(enter)
# qor.on_tick(logic)

