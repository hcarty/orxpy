def push_set(name: str) -> None: ...
def pop_set() -> None: ...
def enable_set(name: str, enable: bool = True) -> None: ...
def is_set_enabled(name: str) -> bool: ...
def is_active(name: str) -> bool: ...
def has_been_activated(name: str) -> bool: ...
def has_been_deactivated(name: str) -> bool: ...
def get_value(name: str) -> float: ...
def set_value(name: str, value: float, permanent: bool = False) -> None: ...
def reset_value(name: str) -> None: ...
