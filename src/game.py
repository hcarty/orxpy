import object

def update(o: object.Object):
  pos = object.get_position(o)
  pos.x += 1
  object.set_position(o, pos)
