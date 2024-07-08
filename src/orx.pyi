from dataclasses import dataclass

@dataclass
class Vector:
    x: float
    y: float

@dataclass
class Object:
    def get_name() -> str: ...
    def get_position() -> Vector: ...
