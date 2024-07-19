import input
import vector

def get_input() -> vector.Vector:
  return vector.Vector(input.get_value("Right") - input.get_value("Left"), input.get_value("Down") - input.get_value("Up"), 0)
