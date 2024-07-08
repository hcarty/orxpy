/**
 * @file orxpy.cpp
 * @date 22-Jun-2024
 */

#define __SCROLL_IMPL__
#include "orxpy.h"
#undef __SCROLL_IMPL__

#include "Object.h"
#include "orxExtensions.h"

#ifdef __orxMSVC__

/* Requesting high performance dedicated GPU on hybrid laptops */
__declspec(dllexport) unsigned long NvOptimusEnablement = 1;
__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;

#endif // __orxMSVC__

#include "pocketpy.h"

namespace py = pkpy;

/** Update function, it has been registered to be called every tick of the core clock
 */
void orxpy::Update(const orxCLOCK_INFO &_rstClockInfo)
{
  // Should quit?
  if (orxInput_IsActive("Quit"))
  {
    // Send close event
    orxEvent_SendShort(orxEVENT_TYPE_SYSTEM, orxSYSTEM_EVENT_CLOSE);
  }
}

namespace pythonwrapper
{
#define BIND(NAME) py::PyVar NAME(py::VM *vm, py::ArgsView args)
#define ARG(type, name, index) type name = py::py_cast<type>(vm, args[index])
#define OBJECT ARG(orxOBJECT *, pstObject, 0)
#define ARG_OR_NONE(type, name, index)         \
  type name;                                   \
  if (args[index] == vm->None)                 \
  {                                            \
    name = orxNULL;                            \
  }                                            \
  else                                         \
  {                                            \
    name = py::py_cast<type>(vm, args[index]); \
  }
#define RETURN(RET) return py::py_var(vm, RET)
#define RETURN_NONE return vm->None
#define RETURN_OR_NONE(RET) \
  if (RET != orxNULL)       \
  {                         \
    RETURN(RET);            \
  }                         \
  else                      \
  {                         \
    RETURN_NONE;            \
  }

  // Type wrappers

  py::PyVar vector_init(py::VM *vm, py::ArgsView args)
  {
    orxVECTOR self = py::py_cast<orxVECTOR>(vm, args[0]);
    self.fX = py_cast<orxFLOAT>(vm, args[1]);
    self.fY = py_cast<orxFLOAT>(vm, args[2]);
    self.fZ = py_cast<orxFLOAT>(vm, args[3]);
    RETURN_NONE;
  }

  void vector(py::VM *vm, py::PyVar mod, py::PyVar type)
  {
    // Fields
    vm->bind_field(type, "x", &orxVECTOR::fX);
    vm->bind_field(type, "y", &orxVECTOR::fY);
    vm->bind_field(type, "z", &orxVECTOR::fZ);

    // __init__ method
    vm->bind(type, "__init__(self, x, y, z)", vector_init);
  }

  void object(py::VM *vm, py::PyVar mod, py::PyVar type)
  {
  }

  // Function wrappers

  BIND(create_object)
  {
    ARG(const orxSTRING, zName, 0);
    orxOBJECT *pstObject = orxObject_CreateFromConfig(zName);
    if (pstObject != orxNULL)
    {
      RETURN(pstObject);
    }
    else
    {
      RETURN_NONE;
    }
  }

  BIND(delete_object)
  {
    OBJECT;
    orxObject_Delete(pstObject);
    RETURN_NONE;
  }

  BIND(enable_object)
  {
    OBJECT;
    ARG(bool, bState, 1);
    ARG(bool, bRecursive, 2);
    if (bRecursive)
    {
      orxObject_EnableRecursive(pstObject, bState);
    }
    else
    {
      orxObject_Enable(pstObject, bState);
    }
    RETURN_NONE;
  }

  BIND(is_enabled)
  {
    OBJECT;
    RETURN(orxObject_IsEnabled(pstObject));
  }

  BIND(pause)
  {
    OBJECT;
    ARG(bool, bState, 1);
    ARG(bool, bRecursive, 2);
    if (bRecursive)
    {
      orxObject_PauseRecursive(pstObject, bState);
    }
    else
    {
      orxObject_Pause(pstObject, bState);
    }
    RETURN_NONE;
  }

  BIND(is_paused)
  {
    OBJECT;
    RETURN(orxObject_IsPaused(pstObject));
  }

  BIND(set_owner)
  {
    OBJECT;
    ARG_OR_NONE(orxOBJECT *, pstOwner, 1);
    orxObject_SetParent(pstObject, (void *)pstOwner);
    RETURN_NONE;
  }

  BIND(get_owner)
  {
    OBJECT;
    orxOBJECT *pstOwner = orxOBJECT(orxObject_GetOwner(pstObject));
    RETURN_OR_NONE(pstOwner);
  }

  BIND(find_owned_child)
  {
    OBJECT;
    ARG(const orxSTRING, zPath, 1);
    orxOBJECT *pstChild = orxObject_FindOwnedChild(pstObject, zPath);
    RETURN_OR_NONE(pstChild);
  }

  BIND(set_flip)
  {
    OBJECT;
    ARG(bool, bFlipX, 1);
    ARG(bool, bFlipY, 2);
    orxObject_SetFlip(pstObject, bFlipX, bFlipY);
    RETURN_NONE;
  }

  BIND(get_flip)
  {
    OBJECT;
    orxBOOL bFlipX, bFlipY;
    orxObject_GetFlip(pstObject, &bFlipX, &bFlipY);
    py::Tuple pyResult(2);
    pyResult[0] = VAR(bFlipX);
    pyResult[1] = VAR(bFlipY);
    RETURN(pyResult);
  }

  BIND(set_position)
  {
    OBJECT;
    ARG(bool, bWorld, 1);
    orxVECTOR vPosition = py::py_cast<orxVECTOR>(vm, args[1]);
    if (bWorld)
    {
      orxObject_SetWorldPosition(pstObject, &vPosition);
    }
    else
    {
      orxObject_SetPosition(pstObject, &vPosition);
    }
    RETURN_NONE;
  }

  BIND(get_position)
  {
    OBJECT;
    ARG(bool, bWorld, 1);
    orxVECTOR vPosition;
    if (bWorld)
    {
      orxObject_GetWorldPosition(pstObject, &vPosition);
    }
    else
    {
      orxObject_GetPosition(pstObject, &vPosition);
    }
    RETURN(vPosition);
  }

  BIND(set_parent)
  {
    OBJECT;
    ARG_OR_NONE(orxOBJECT *, pstParent, 1);
    orxObject_SetParent(pstObject, pstParent);
    RETURN_NONE;
  }

  BIND(get_parent)
  {
    OBJECT;
    orxOBJECT *pstParent = orxOBJECT(orxObject_GetParent(pstObject));
    RETURN_OR_NONE(pstParent);
  }

  BIND(find_child)
  {
    OBJECT;
    ARG(const orxSTRING, zPath, 1);
    orxOBJECT *pstChild = orxObject_FindChild(pstObject, zPath);
    RETURN_OR_NONE(pstChild);
  }

  BIND(get_name)
  {
    OBJECT;
    RETURN(orxObject_GetName(pstObject));
  }

#undef BIND
#undef ARG
#undef ARG_OR_NONE
#undef OBJECT
#undef RETURN
#undef RETURN_NONE
#undef RETURN_OR_NONE
}

void InitPy(py::VM *vm)
{
  auto mod = vm->new_module("orx");

  using namespace pythonwrapper;

  // Bind types
  vm->register_user_class<orxVECTOR>(mod, "Vector", vector);
  vm->register_user_class<orxOBJECT *>(mod, "Object", object);

  // Bind functions
  vm->bind(mod, "create_object(name: str) -> Object | None", create_object);
  vm->bind(mod, "delete_object(o: Object) -> None", delete_object);

  vm->bind(mod, "enable(o: Object, state: bool, recursive: bool = False) -> None", enable_object);
  vm->bind(mod, "is_enabled(o: Object) -> bool", is_enabled);

  vm->bind(mod, "pause(o: Object, state: bool, recursive: bool = False) -> None", pause);
  vm->bind(mod, "is_paused(o: Object) -> bool", is_paused);

  vm->bind(mod, "set_owner(o: Object, owner: Object | None) -> None", set_owner);
  vm->bind(mod, "get_owner(o: Object) -> Object | None", get_owner);

  vm->bind(mod, "find_owned_child(o: Object, path: str) -> Object | None", find_owned_child);

  vm->bind(mod, "set_flip(o: Object, flip_x: bool, flip_y: bool) -> None", set_flip);
  vm->bind(mod, "get_flip(o: Object) -> tuple[bool, bool]", get_flip);

  vm->bind(mod, "set_position(o: Object, position: Vector, world: bool = False) -> Vector", get_position);
  vm->bind(mod, "get_position(o: Object, world: bool = False) -> Vector", get_position);

  vm->bind(mod, "set_parent(o: Object, parent: Object | None) -> None", set_parent);
  vm->bind(mod, "get_parent(o: Object) -> Object | None", get_parent);

  vm->bind(mod, "find_child(o: Object, path: str) -> Object | None", find_child);

  vm->bind(mod, "get_name(o: Object) -> str", get_name);
}

/** Init function, it is called when all orx's modules have been initialized
 */
orxSTATUS orxpy::Init()
{
  // Display a small hint in console
  orxLOG("\n* This template project creates a simple scene"
         "\n* You can play with the config parameters in ../data/config/orxpy.ini"
         "\n* After changing them, relaunch the executable to see the changes.");

  // Init extensions
  InitExtensions();

  // Create the scene
  auto scene = CreateObject("Scene");

  // Done!
  return orxSTATUS_SUCCESS;
}

/** Run function, it should not contain any game logic
 */
orxSTATUS orxpy::Run()
{
  // Return orxSTATUS_FAILURE to instruct orx to quit
  return orxSTATUS_SUCCESS;
}

/** Exit function, it is called before exiting from orx
 */
void orxpy::Exit()
{
  // Exit from extensions
  ExitExtensions();

  // Let orx clean all our mess automatically. :)
}

/** BindObjects function, ScrollObject-derived classes are bound to config sections here
 */
void orxpy::BindObjects()
{
  // Bind the Object class to the Object config section
  ScrollBindObject<Object>("Object");
}

/** Bootstrap function, it is called before config is initialized, allowing for early resource storage definitions
 */
orxSTATUS orxpy::Bootstrap() const
{
  // Bootstrap extensions
  BootstrapExtensions();

  // Return orxSTATUS_FAILURE to prevent orx from loading the default config file
  return orxSTATUS_SUCCESS;
}

/** Main function
 */
int main(int argc, char **argv)
{
  // Execute our game
  orxpy::GetInstance().Execute(argc, argv);

  // Done!
  return EXIT_SUCCESS;
}
