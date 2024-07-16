import object

scene: object.Object
moving_right: bool = True

def orx_init():
  global scene
  scene = object.create_object("Scene")

def orx_update(_dt: float):
  global moving_right
  logo = object.find_child(scene, "Object")
  if logo is not None:
    pos = object.get_position(logo)
    if moving_right:
      pos.x += 1
      if pos.x > 100:
        moving_right = False
    else:
      pos.x -= 1
      if pos.x < -100:
        moving_right = True
    object.set_position(logo, pos)

def orx_exit():
  return
