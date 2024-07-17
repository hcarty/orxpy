import input
import object
import vector

logo: object.Object

def get_input() -> vector.Vector:
  return vector.Vector(input.get_value("Right") - input.get_value("Left"), input.get_value("Down") - input.get_value("Up"), 0)

def orx_init():
  global logo
  scene = object.create_object("Scene")
  child = object.find_child(scene, "Object")
  if child is None:
    raise ValueError("No logo defined in the scene!")
  logo = child

def orx_update(_dt: float):
  pos = object.get_position(logo)
  movement = get_input()
  pos.x += movement.x
  pos.y += movement.y
  object.set_position(logo, pos)

def orx_exit():
  return
