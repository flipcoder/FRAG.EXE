import qor

player = qor.Mesh("player.obj")

music = qor.Sound("cave.ogg")
music.ambient(True)
qor.root().add(music)
qor.on_enter(music.play)

def flagcb(f,p):
    qor.log("red flag taken");
    snd = qor.Sound("redflagtaken.wav")
    snd.ambient(True)
    snd.spawn()
    snd.detach_on_done()
    snd.play()
    f.detach()

flag = qor.Mesh("item_flag.obj")
flag.set_physics(qor.PhysicsType.DYNAMIC)
flag.set_physics_shape(qor.PhysicsShape.HULL)
flag.position(qor.vec3(2,0,0))
flag.on_event("use", flagcb)
flag.spawn()
qor.on_touch(flag, 0, flagcb)

player.set_physics(qor.PhysicsType.DYNAMIC)
player.set_physics_shape(qor.PhysicsShape.HULL)
player.on_event("use", lambda _: qor.log("test!!!"))
player.position(qor.vec3(0.0, 0.0, -2.0))
player.inertia(False)
player.spawn()
player.generate()

# def enter():
#     pass

# def logic(t):
#     pass
    
# qor.on_enter(enter)
# qor.on_tick(logic)

