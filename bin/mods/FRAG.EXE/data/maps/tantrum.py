import qor
        
lift = qor.root().hook("lift")[0]
liftsnd = qor.Sound("elevator.wav")
liftsnd.loop(True)
lift.add(liftsnd)
lift = qor.Mesh(lift)
lift.set_physics(qor.PhysicsType.KINEMATIC)
lift.set_physics_shape(qor.PhysicsShape.HULL)
# lift.mass(1000.0)
lift.inertia(False)
lift.gravity(qor.vec3(0.0))
lift.generate()
lift.state("position","up")
liftstart = lift.world_box().max().y
liftstop = lift.world_box().min().y
lifttimer = 0.0
def golift(m):
    if lift.state("position") == "up":
        lift.state("position","moving_down")
        lift.velocity(qor.vec3(0, -5, 0))
        liftsnd.play()
    # elif lift.state("position") == "down":
    #     lift.state("position","moving_up")
    #     lift.velocity(qor.vec3(0, 5, 0))
    #     liftsnd.play()
def lift_tick(t):
    global lifttimer
    p = lift.state("position")
    if p == "down":
        lifttimer += t
        if lifttimer > 0.5:
            lift.state("position","moving_up")
            lift.velocity(qor.vec3(0, 5, 0))
            liftsnd.play()
    if p == "moving_down":
        if lift.world_box().max().y <= liftstop + 0.1:
            lift.state("position","down")
            lift.velocity(qor.vec3(0, 0, 0))
            liftsnd.stop()
            lifttimer = 0.0
    elif p == "moving_up":
        if lift.world_box().max().y >= liftstart:
            lift.state("position","up")
            lift.velocity(qor.vec3(0, 0, 0))
            liftsnd.stop()
lift.on_event("use", golift)
lift.on_tick(lift_tick)


