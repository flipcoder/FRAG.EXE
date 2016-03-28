import qor

player = qor.Mesh("player.obj")

def flagcb(m):
    global player
    # snd = qor.Sound("redteamscores.wav")
    # snd.ambient(True)
    # snd.spawn()
    # snd.detach_on_done()
    # snd.play()

flag = qor.Mesh("item_flag.obj")
flag.set_physics(qor.PhysicsType.DYNAMIC)
flag.set_physics_shape(qor.PhysicsShape.HULL)
flag.position(qor.vec3(2,0,0))
flag.on_event("use", flagcb)
flag.spawn()
flag.generate()

player.set_physics(qor.PhysicsType.DYNAMIC)
player.set_physics_shape(qor.PhysicsShape.HULL)
player.on_event("use", lambda _: qor.log("test!!!"))
player.position(qor.vec3(0.0, 0.0, -2.0))
player.inertia(False)
player.spawn()
player.generate()
 
def enter():
    qor.log("enter")
    pass

def logic(t):
    pass
    
qor.on_enter(enter)
qor.on_tick(logic)

