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

#define orxPY_KZ_CONFIG_SECTION "Python"

#define orxPY_KZ_DEFAULT_INIT "orx_init"
#define orxPY_KZ_DEFAULT_UPDATE "orx_update"
#define orxPY_KZ_DEFAULT_EXIT "orx_exit"

struct orxPYTHON_CALLBACKS
{
  py::PyVar pyInit = nullptr;
  py::PyVar pyUpdate = nullptr;
  py::PyVar pyExit = nullptr;
};

static py::VM *pVM = nullptr;
static orxPYTHON_CALLBACKS stPyCallbacks{};

void orxPy_InitCallbacks(py::VM *vm, orxPYTHON_CALLBACKS *pstPyCallbacks)
{
  // Value defined in the main module referenced by name
  py::NameDict &dAttrs = vm->_main->attr();

  // Get functions by name
  orxConfig_PushSection(orxPY_KZ_CONFIG_SECTION);
  const orxSTRING zName = orxSTRING_EMPTY;

  zName = orxConfig_HasValue("Init") ? orxConfig_GetString("Init") : orxPY_KZ_DEFAULT_INIT;
  if (dAttrs.contains(zName))
  {
    pstPyCallbacks->pyInit = dAttrs[zName];
  }

  zName = orxConfig_HasValue("Update") ? orxConfig_GetString("Update") : orxPY_KZ_DEFAULT_UPDATE;
  if (dAttrs.contains(zName))
  {
    pstPyCallbacks->pyUpdate = dAttrs[zName];
  }

  zName = orxConfig_HasValue("Exit") ? orxConfig_GetString("Exit") : orxPY_KZ_DEFAULT_EXIT;
  if (dAttrs.contains(zName))
  {
    pstPyCallbacks->pyExit = dAttrs[zName];
  }

  orxConfig_PopSection();
}

orxSTATUS orxPy_Call(py::VM *vm, py::PyVar pyCallable)
{
  orxSTATUS eResult = orxSTATUS_FAILURE;

  if (pyCallable != nullptr && vm != nullptr)
  {
    try
    {
      vm->call(pyCallable);
      eResult = orxSTATUS_SUCCESS;
    }
    catch (py::Exception &py_exc)
    {
      orxLOG("%s", py_exc.summary().data);
    }
  }

  return eResult;
}

orxSTATUS orxPy_Call1(py::VM *vm, py::PyVar pyCallable, py::PyVar pyArg)
{
  orxSTATUS eResult = orxSTATUS_FAILURE;

  if (pyCallable != nullptr && vm != nullptr)
  {
    try
    {
      vm->call(pyCallable, pyArg);
      eResult = orxSTATUS_SUCCESS;
    }
    catch (py::Exception &py_exc)
    {
      orxLOG("%s", py_exc.summary().data);
    }
  }

  return eResult;
}

/** Update function, it has been registered to be called every tick of the core clock
 */
void orxpy::Update(const orxCLOCK_INFO &_rstClockInfo)
{
  orxPy_Call1(pVM, stPyCallbacks.pyUpdate, py::py_var(pVM, _rstClockInfo.fDT));

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

  // Function wrappers

  // Object functions

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

  BIND(get_guid)
  {
    OBJECT;
    RETURN(orxStructure_GetGUID(orxSTRUCTURE(pstObject)));
  }

  BIND(from_guid)
  {
    ARG(orxU64, u64GUID, 0);
    orxOBJECT *pstObject = orxOBJECT(orxStructure_Get(u64GUID));
    RETURN_OR_NONE(pstObject);
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
    orxVECTOR vPosition = py::py_cast<orxVECTOR>(vm, args[1]);
    ARG(bool, bWorld, 2);
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

  BIND(attach)
  {
    OBJECT;
    ARG(orxOBJECT *, pstParent, 1);
    orxObject_Attach(pstObject, (void *)pstParent);
    RETURN_NONE;
  }

  BIND(detach)
  {
    OBJECT;
    orxObject_Detach(pstObject);
    RETURN_NONE;
  }

  BIND(log_parents)
  {
    OBJECT;
    orxObject_LogParents(pstObject);
    RETURN_NONE;
  }

  BIND(set_anim_frequency)
  {
    OBJECT;
    ARG(orxFLOAT, fFrequency, 1);
    ARG(bool, bRecursive, 2);
    if (bRecursive)
    {
      orxObject_SetAnimFrequencyRecursive(pstObject, fFrequency);
    }
    else
    {
      orxObject_SetAnimFrequency(pstObject, fFrequency);
    }
    RETURN_NONE;
  }

  BIND(get_anim_frequency)
  {
    OBJECT;
    RETURN(orxObject_GetAnimFrequency(pstObject));
  }

  BIND(set_anim_time)
  {
    OBJECT;
    ARG(orxFLOAT, fTime, 1);
    ARG(bool, bRecursive, 2);
    if (bRecursive)
    {
      orxObject_SetAnimTimeRecursive(pstObject, fTime);
    }
    else
    {
      orxObject_SetAnimTime(pstObject, fTime);
    }
    RETURN_NONE;
  }

  BIND(get_anim_time)
  {
    OBJECT;
    RETURN(orxObject_GetAnimTime(pstObject));
  }

  BIND(set_current_anim)
  {
    OBJECT;
    ARG(orxSTRING, zName, 1);
    ARG(bool, bRecursive, 2);
    if (bRecursive)
    {
      orxObject_SetCurrentAnimRecursive(pstObject, zName);
    }
    else
    {
      orxObject_SetCurrentAnim(pstObject, zName);
    }
    RETURN_NONE;
  }

  BIND(get_current_anim)
  {
    OBJECT;
    RETURN(orxObject_GetCurrentAnim(pstObject));
  }

  BIND(set_target_anim)
  {
    OBJECT;
    ARG(orxSTRING, zName, 1);
    ARG(bool, bRecursive, 2);
    if (bRecursive)
    {
      orxObject_SetTargetAnimRecursive(pstObject, zName);
    }
    else
    {
      orxObject_SetTargetAnim(pstObject, zName);
    }
    RETURN_NONE;
  }

  BIND(get_target_anim)
  {
    OBJECT;
    RETURN(orxObject_GetTargetAnim(pstObject));
  }

  BIND(set_speed)
  {
    OBJECT;
    ARG(orxVECTOR, vSpeed, 1);
    ARG(bool, bRelative, 2);
    if (bRelative)
    {
      orxObject_SetRelativeSpeed(pstObject, &vSpeed);
    }
    else
    {
      orxObject_SetSpeed(pstObject, &vSpeed);
    }
    RETURN_NONE;
  }

  BIND(get_speed)
  {
    OBJECT;
    ARG(bool, bRelative, 1);
    orxVECTOR vSpeed = orxVECTOR_0;
    if (bRelative)
    {
      orxObject_GetRelativeSpeed(pstObject, &vSpeed);
    }
    else
    {
      orxObject_GetSpeed(pstObject, &vSpeed);
    }
    RETURN(vSpeed);
  }

  BIND(set_angular_velocity)
  {
    OBJECT;
    ARG(orxFLOAT, fVelocity, 1);
    orxObject_SetAngularVelocity(pstObject, fVelocity);
    RETURN_NONE;
  }

  BIND(get_angular_velocity)
  {
    OBJECT;
    RETURN(orxObject_GetAngularVelocity(pstObject));
  }

  BIND(set_custom_gravity)
  {
    OBJECT;
    ARG(orxVECTOR, vGravity, 1);
    orxObject_SetCustomGravity(pstObject, &vGravity);
    RETURN_NONE;
  }

  BIND(get_custom_gravity)
  {
    OBJECT;
    orxVECTOR vGravity = orxVECTOR_0;
    orxObject_GetCustomGravity(pstObject, &vGravity);
    RETURN(vGravity);
  }

  BIND(get_mass)
  {
    OBJECT;
    RETURN(orxObject_GetMass(pstObject));
  }

  BIND(get_mass_center)
  {
    OBJECT;
    orxVECTOR vCenter = orxVECTOR_0;
    orxObject_GetMassCenter(pstObject, &vCenter);
    RETURN(vCenter);
  }

  BIND(apply_torque)
  {
    OBJECT;
    ARG(orxFLOAT, fTorque, 1);
    orxObject_ApplyTorque(pstObject, fTorque);
    RETURN_NONE;
  }

  BIND(apply_force)
  {
    OBJECT;
    ARG(orxVECTOR, vForce, 1);
    ARG(orxVECTOR, vPoint, 2);
    orxObject_ApplyForce(pstObject, &vForce, &vPoint);
    RETURN_NONE;
  }

  BIND(apply_impulse)
  {
    OBJECT;
    ARG(orxVECTOR, vImpulse, 1);
    ARG(orxVECTOR, vPoint, 2);
    orxObject_ApplyImpulse(pstObject, &vImpulse, &vPoint);
    RETURN_NONE;
  }

  BIND(raycast)
  {
    ARG(orxVECTOR, vBegin, 0);
    ARG(orxVECTOR, vEnd, 1);
    ARG(orxU16, u16SelfFlags, 2);
    ARG(orxU16, u16CheckMask, 3);
    ARG(orxBOOL, bEarlyExit, 4);
    orxVECTOR vContact;
    orxVECTOR vNormal;
    orxOBJECT *pstDetected = orxObject_Raycast(&vBegin, &vEnd, u16SelfFlags, u16CheckMask, bEarlyExit, &vContact, &vNormal);
    if (pstDetected != orxNULL)
    {
      RETURN(py::Tuple(py::py_var(vm, pstDetected), py::py_var(vm, vContact), py::py_var(vm, vNormal)));
    }
    else
    {
      RETURN_NONE;
    }
  }

  BIND(set_text_string)
  {
    OBJECT;
    ARG(orxSTRING, zString, 1);
    orxObject_SetTextString(pstObject, zString);
    RETURN_NONE;
  }

  BIND(get_text_string)
  {
    OBJECT;
    RETURN(orxObject_GetTextString(pstObject));
  }

  BIND(add_fx)
  {
    OBJECT;
    ARG(orxSTRING, zName, 1);
    ARG(bool, bRecursive, 2);
    ARG(bool, bUnique, 3);
    ARG(orxFLOAT, fPropagationDelay, 4);
    if (bRecursive && bUnique)
    {
      orxObject_AddUniqueFXRecursive(pstObject, zName, fPropagationDelay);
    }
    else if (bRecursive)
    {
      orxObject_AddFXRecursive(pstObject, zName, fPropagationDelay);
    }
    else if (bUnique)
    {
      orxObject_AddUniqueFX(pstObject, zName);
    }
    else
    {
      orxObject_AddFX(pstObject, zName);
    }
    RETURN_NONE;
  }

  BIND(remove_fx)
  {
    OBJECT;
    ARG(orxSTRING, zName, 1);
    ARG(bool, bRecursive, 2);
    if (bRecursive)
    {
      orxObject_RemoveFXRecursive(pstObject, zName);
    }
    else
    {
      orxObject_RemoveFX(pstObject, zName);
    }
    RETURN_NONE;
  }

  BIND(remove_all_fxs)
  {
    OBJECT;
    ARG(bool, bRecursive, 1);
    if (bRecursive)
    {
      orxObject_RemoveAllFXsRecursive(pstObject);
    }
    else
    {
      orxObject_RemoveAllFXs(pstObject);
    }
    RETURN_NONE;
  }

  BIND(add_sound)
  {
    OBJECT;
    ARG(orxSTRING, zName, 1);
    orxObject_AddSound(pstObject, zName);
    RETURN_NONE;
  }

  BIND(remove_sound)
  {
    OBJECT;
    ARG(orxSTRING, zName, 1);
    orxObject_RemoveSound(pstObject, zName);
    RETURN_NONE;
  }

  BIND(remove_all_sounds)
  {
    OBJECT;
    orxObject_RemoveAllSounds(pstObject);
    RETURN_NONE;
  }

  BIND(set_volume)
  {
    OBJECT;
    ARG(orxFLOAT, fVolume, 1);
    orxObject_SetVolume(pstObject, fVolume);
    RETURN_NONE;
  }

  BIND(set_pitch)
  {
    OBJECT;
    ARG(orxFLOAT, fPitch, 1);
    orxObject_SetPitch(pstObject, fPitch);
    RETURN_NONE;
  }

  BIND(set_panning)
  {
    OBJECT;
    ARG(orxFLOAT, fPanning, 1);
    ARG(orxBOOL, bMix, 2);
    orxObject_SetPanning(pstObject, fPanning, bMix);
    RETURN_NONE;
  }

  BIND(play)
  {
    OBJECT;
    orxObject_Play(pstObject);
    RETURN_NONE;
  }

  BIND(stop)
  {
    OBJECT;
    orxObject_Stop(pstObject);
    RETURN_NONE;
  }

  BIND(add_filter)
  {
    OBJECT;
    ARG(orxSTRING, zName, 1);
    orxObject_AddFilter(pstObject, zName);
    RETURN_NONE;
  }

  BIND(remove_last_filter)
  {
    OBJECT;
    orxObject_RemoveLastFilter(pstObject);
    RETURN_NONE;
  }

  BIND(remove_all_filters)
  {
    OBJECT;
    orxObject_RemoveAllFilters(pstObject);
    RETURN_NONE;
  }

  BIND(add_shader)
  {
    OBJECT;
    ARG(orxSTRING, zName, 1);
    ARG(bool, bRecursive, 2);
    if (bRecursive)
    {
      orxObject_AddShaderRecursive(pstObject, zName);
    }
    else
    {
      orxObject_AddShader(pstObject, zName);
    }
    RETURN_NONE;
  }

  BIND(remove_shader)
  {
    OBJECT;
    ARG(orxSTRING, zName, 1);
    ARG(bool, bRecursive, 2);
    if (bRecursive)
    {
      orxObject_RemoveShaderRecursive(pstObject, zName);
    }
    else
    {
      orxObject_RemoveShader(pstObject, zName);
    }
    RETURN_NONE;
  }

  BIND(enable_shader)
  {
    OBJECT;
    ARG(orxBOOL, bEnabled, 1);
    orxObject_EnableShader(pstObject, bEnabled);
    RETURN_NONE;
  }

  BIND(is_shader_enabled)
  {
    OBJECT;
    RETURN(orxObject_IsShaderEnabled(pstObject));
  }

  BIND(add_time_line_track)
  {
    OBJECT;
    ARG(orxSTRING, zName, 1);
    ARG(bool, bRecursive, 2);
    if (bRecursive)
    {
      orxObject_AddTimeLineTrackRecursive(pstObject, zName);
    }
    else
    {
      orxObject_AddTimeLineTrack(pstObject, zName);
    }
    RETURN_NONE;
  }

  BIND(remove_time_line_track)
  {
    OBJECT;
    ARG(orxSTRING, zName, 1);
    ARG(bool, bRecursive, 2);
    if (bRecursive)
    {
      orxObject_RemoveTimeLineTrackRecursive(pstObject, zName);
    }
    else
    {
      orxObject_RemoveTimeLineTrack(pstObject, zName);
    }
    RETURN_NONE;
  }

  BIND(enable_time_line)
  {
    OBJECT;
    ARG(orxBOOL, bEnable, 1);
    orxObject_EnableTimeLine(pstObject, bEnable);
    RETURN_NONE;
  }

  BIND(is_time_line_enabled)
  {
    OBJECT;
    RETURN(orxObject_IsTimeLineEnabled(pstObject));
  }

  BIND(get_name)
  {
    OBJECT;
    RETURN(orxObject_GetName(pstObject));
  }

  BIND(set_rgb)
  {
    OBJECT;
    ARG(orxVECTOR, vRGB, 1);
    ARG(bool, bRecursive, 2);
    if (bRecursive)
    {
      orxObject_SetRGBRecursive(pstObject, &vRGB);
    }
    else
    {
      orxObject_SetRGB(pstObject, &vRGB);
    }
    RETURN_NONE;
  }

  BIND(get_rgb)
  {
    OBJECT;
    orxVECTOR vRGB = orxVECTOR_0;
    orxObject_GetRGB(pstObject, &vRGB);
    RETURN(vRGB);
  }

  BIND(set_alpha)
  {
    OBJECT;
    ARG(orxFLOAT, fAlpha, 1);
    ARG(bool, bRecursive, 2);
    if (bRecursive)
    {
      orxObject_SetAlphaRecursive(pstObject, fAlpha);
    }
    else
    {
      orxObject_SetAlpha(pstObject, fAlpha);
    }
    RETURN_NONE;
  }

  BIND(get_alpha)
  {
    OBJECT;
    RETURN(orxObject_GetAlpha(pstObject));
  }

  BIND(set_life_time)
  {
    OBJECT;
    py::PyVar pyLifeTime = args[1];
    if (py::is_float(pyLifeTime) || py::is_int(pyLifeTime))
    {
      ARG(orxFLOAT, fLifeTime, 1);
      orxObject_SetLifeTime(pstObject, fLifeTime);
    }
    else if (pyLifeTime == vm->None)
    {
      orxObject_SetLifeTime(pstObject, -orxFLOAT_1);
    }
    else
    {
      ARG(orxSTRING, zLifeTime, 1);
      orxObject_SetLiteralLifeTime(pstObject, zLifeTime);
    }
    RETURN_NONE;
  }

  BIND(get_life_time)
  {
    OBJECT;
    orxFLOAT fLifeTime = orxObject_GetLifeTime(pstObject);
    if (fLifeTime < orxFLOAT_0)
    {
      RETURN_NONE;
    }
    else
    {
      RETURN(fLifeTime);
    }
  }

  BIND(get_active_time)
  {
    OBJECT;
    RETURN(orxObject_GetActiveTime(pstObject));
  }

  BIND(reset_active_time)
  {
    OBJECT;
    ARG(bool, bRecursive, 1);
    if (bRecursive)
    {
      orxObject_ResetActiveTimeRecursive(pstObject);
    }
    else
    {
      orxObject_ResetActiveTime(pstObject);
    }
    RETURN_NONE;
  }

  // Config functions

  BIND(push_section)
  {
    ARG(orxSTRING, zSection, 0);
    orxConfig_PushSection(zSection);
    RETURN_NONE;
  }

  BIND(pop_section)
  {
    orxConfig_PopSection();
    RETURN_NONE;
  }

#define BIND_CONFIG_SET_GET(type, orxtype, name) \
  BIND(set_##type)                               \
  {                                              \
    ARG(orxSTRING, zKey, 0);                     \
    ARG(orxtype, name, 1);                       \
    orxConfig_Set##name(zKey, name);             \
    RETURN_NONE;                                 \
  }                                              \
  BIND(get_##type)                               \
  {                                              \
    ARG(orxSTRING, zKey, 0);                     \
    RETURN(orxConfig_Get##name);                 \
  }

  BIND_CONFIG_SET_GET(bool, orxBOOL, Bool)
  BIND_CONFIG_SET_GET(int, orxS64, S64)
  BIND_CONFIG_SET_GET(uint, orxU64, U64)
  BIND_CONFIG_SET_GET(float, orxFLOAT, Float)
  BIND_CONFIG_SET_GET(string, orxSTRING, String)

  BIND(set_vector)
  {
    ARG(orxSTRING, zKey, 0);
    ARG(orxVECTOR, vValue, 1);
    orxConfig_SetVector(zKey, &vValue);
    RETURN_NONE;
  }

  BIND(get_vector)
  {
    ARG(orxSTRING, zKey, 0);
    orxVECTOR vValue = orxVECTOR_0;
    orxConfig_GetVector(zKey, &vValue);
    RETURN(vValue);
  }

#undef BIND_SET_GET

  BIND(has_section)
  {
    ARG(orxSTRING, zSection, 0);
    RETURN(orxConfig_HasSection(zSection));
  }

  BIND(has_value)
  {
    ARG(orxSTRING, zKey, 0);
    ARG(bool, bCheckSpelling, 1);
    if (bCheckSpelling)
    {
      RETURN(orxConfig_HasValue(zKey));
    }
    else
    {
      RETURN(orxConfig_HasValueNoCheck(zKey));
    }
    orxASSERT(orxFALSE);
  }

  BIND(clear_section)
  {
    ARG(orxSTRING, zSection, 0);
    orxConfig_ClearSection(zSection);
    RETURN_NONE;
  }

  BIND(clear_value)
  {
    ARG(orxSTRING, zKey, 0);
    orxConfig_ClearValue(zKey);
    RETURN_NONE;
  }

  BIND(evaluate)
  {
    ARG(orxSTRING, zCommand, 0);
    orxCOMMAND_VAR stResult;
    if (args[0] == vm->None)
    {
      orxCommand_Evaluate(zCommand, &stResult);
    }
    else
    {
      ARG(orxU64, u64GUID, 1);
      orxCommand_EvaluateWithGUID(zCommand, u64GUID, &stResult);
    }
    RETURN_NONE;
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

  void object(py::VM *vm, py::PyVar mod, py::PyVar type) {}

#undef BIND
#undef ARG
#undef ARG_OR_NONE
#undef OBJECT
#undef RETURN
#undef RETURN_NONE
#undef RETURN_OR_NONE
}

orxSTATUS orxPy_ExecSource(py::VM *vm, const orxSTRING zPath)
{
  orxSTATUS eResult = orxSTATUS_SUCCESS;

  // Load map resource
  auto resourceLocation = orxResource_Locate("Python", zPath);
  orxASSERT(resourceLocation != orxNULL);
  orxHANDLE hResource = orxResource_Open(resourceLocation, orxFALSE);
  orxS64 s64Size = orxResource_GetSize(hResource);
  orxCHAR *pBuffer;
  pBuffer = (orxCHAR *)orxMemory_Allocate(s64Size, orxMEMORY_TYPE_MAIN);
  orxS64 s64Read = orxResource_Read(hResource, s64Size, pBuffer, orxNULL, orxNULL);
  orxResource_Close(hResource);
  orxASSERT(s64Size == s64Read);

  py::PyVar pExecResult = vm->exec(pBuffer, zPath, py::EXEC_MODE);
  if (pExecResult == nullptr)
  {
    eResult = orxSTATUS_FAILURE;
  }

  orxMemory_Free(pBuffer);
  return eResult;
}

void orxPy_AddVectorModule(py::VM *vm)
{
  // Register command module
  py::PyVar mod = vm->new_module("vector");

  using namespace pythonwrapper;

  // Bind types
  vm->register_user_class<orxVECTOR>(mod, "Vector", vector);
}

void orxPy_AddObjectModule(py::VM *vm)
{
  // Register object module
  py::PyVar mod = vm->new_module("object");

  using namespace pythonwrapper;

  // Register object type
  vm->register_user_class<orxOBJECT *>(mod, "Object", object);

  // Bind object functions
  vm->bind(mod, "create_object(name: str) -> Object | None", create_object);
  vm->bind(mod, "delete_object(o: Object) -> None", delete_object);

  vm->bind(mod, "get_guid(o: Object) -> int", get_guid);
  vm->bind(mod, "from_guid(guid: int) -> Object | None", from_guid);

  vm->bind(mod, "enable(o: Object, state: bool, recursive: bool = False) -> None", enable_object);
  vm->bind(mod, "is_enabled(o: Object) -> bool", is_enabled);

  vm->bind(mod, "pause(o: Object, state: bool, recursive: bool = False) -> None", pause);
  vm->bind(mod, "is_paused(o: Object) -> bool", is_paused);

  vm->bind(mod, "set_owner(o: Object, owner: Object | None) -> None", set_owner);
  vm->bind(mod, "get_owner(o: Object) -> Object | None", get_owner);

  vm->bind(mod, "find_owned_child(o: Object, path: str) -> Object | None", find_owned_child);

  vm->bind(mod, "set_flip(o: Object, flip_x: bool, flip_y: bool) -> None", set_flip);
  vm->bind(mod, "get_flip(o: Object) -> tuple[bool, bool]", get_flip);

  vm->bind(mod, "set_position(o: Object, position: Vector, world: bool = False) -> None", set_position);
  vm->bind(mod, "get_position(o: Object, world: bool = False) -> Vector", get_position);

  vm->bind(mod, "set_parent(o: Object, parent: Object | None) -> None", set_parent);
  vm->bind(mod, "get_parent(o: Object) -> Object | None", get_parent);

  vm->bind(mod, "find_child(o: Object, path: str) -> Object | None", find_child);

  vm->bind(mod, "attach(o: Object, parent: Object) -> None", attach);
  vm->bind(mod, "detach(o: Object) -> None", detach);

  vm->bind(mod, "log_parents(o: Object) -> None", log_parents);

  vm->bind(mod, "set_anim_frequency(o: Object, frequency: float, recursive: bool = False) -> None", set_anim_frequency);
  vm->bind(mod, "get_anim_frequency(o: Object) -> float", get_anim_frequency);

  vm->bind(mod, "set_anim_time(o: Object, time: float, recursive: bool = False) -> None", set_anim_time);
  vm->bind(mod, "get_anim_time(o: Object) -> float", get_anim_time);

  vm->bind(mod, "set_current_anim(o: Object, name: str, recursive: bool = False) -> None", set_current_anim);
  vm->bind(mod, "get_current_anim(o: Object) -> str", get_current_anim);

  vm->bind(mod, "set_target_anim(o: Object, name: str, recursive: bool = False) -> None", set_target_anim);
  vm->bind(mod, "get_target_anim(o: Object) -> str", get_target_anim);

  vm->bind(mod, "set_speed(o: Object, speed: Vector, relative = False) -> None", set_speed);
  vm->bind(mod, "get_speed(o: Object, relative: bool = False) -> Vector", get_speed);

  vm->bind(mod, "set_angular_velocity(o: Object, velocity: float) -> None", set_angular_velocity);
  vm->bind(mod, "get_angular_velocity(o: Object) -> float", get_angular_velocity);

  vm->bind(mod, "set_custom_gravity(o: Object, dir: Vector) -> None", set_custom_gravity);
  vm->bind(mod, "get_custom_gravity(o: Object) -> Vector", get_custom_gravity);

  vm->bind(mod, "get_mass(o: Object) -> float", get_mass);
  vm->bind(mod, "get_mass_center(o: Object) -> Vector", get_mass_center);

  vm->bind(mod, "apply_torque(o: Object, torque: float) -> None", apply_torque);
  vm->bind(mod, "apply_force(o: Object, force: Vector, point: Vector) -> None", apply_force);
  vm->bind(mod, "apply_impulse(o: Object, impulse: Vector, point: Vector) -> None", apply_impulse);

  vm->bind(mod, "raycast(begin: Vector, end: Vector, self_flags: int, check_mask: int, early_exit: bool = False) -> tuple[Object, Vector, Vector] | None", raycast);

  vm->bind(mod, "set_text_string(o: Object, s: str) -> None", set_text_string);
  vm->bind(mod, "get_text_string(o: Object) -> str", get_text_string);

  vm->bind(mod, "add_fx(o: Object, name: str, recursive: bool = False, unique: bool = True, propagation_delay: float = 0) -> None", add_fx);
  vm->bind(mod, "remove_fx(o: Object, name: str, recursive: bool = False) -> None", remove_fx);
  vm->bind(mod, "remove_all_fxs(o: Object, recursive: bool = False) -> None", remove_all_fxs);

  vm->bind(mod, "add_sound(o: Object, name: str) -> None", add_sound);
  vm->bind(mod, "remove_sound(o: Object, name: str) -> None", remove_sound);
  vm->bind(mod, "remove_all_sounds(o: Object) -> None", remove_all_sounds);

  vm->bind(mod, "set_volume(o: Object, volume: float) -> None", set_volume);
  vm->bind(mod, "set_pitch(o: Object, pitch: float) -> None", set_pitch);
  vm->bind(mod, "set_panning(o: Object, panning: float, mix: bool) -> None", set_panning);

  vm->bind(mod, "play(o: Object) -> None", play);
  vm->bind(mod, "stop(o: Object) -> None", stop);

  vm->bind(mod, "add_filter(o: Object, name: str) -> None", add_filter);
  vm->bind(mod, "remove_last_filter(o: Object) -> None", remove_last_filter);
  vm->bind(mod, "remove_all_filters(o: Object) -> None", remove_all_filters);

  vm->bind(mod, "add_shader(o: Object, name: str, recursive: bool = False) -> None", add_shader);
  vm->bind(mod, "remove_shader(o: Object, name: str, recursive: bool = False) -> None", remove_shader);
  vm->bind(mod, "enable_shader(o: Object, enabled: bool = True) -> None", enable_shader);
  vm->bind(mod, "is_shader_enabled(o: Object) -> bool", is_shader_enabled);

  vm->bind(mod, "add_time_line_track(o: Object, name: str, recusive: bool = False) -> None", add_time_line_track);
  vm->bind(mod, "remove_time_line_track(o: Object, name: str, recursive: bool = False) -> None", remove_time_line_track);
  vm->bind(mod, "enable_time_line(o: Object, enabled: bool = True) -> None", enable_time_line);
  vm->bind(mod, "is_time_line_enabled(o: Object) -> bool", is_time_line_enabled);

  vm->bind(mod, "get_name(o: Object) -> str", get_name);

  vm->bind(mod, "set_rgb(o: Object, rgb: Vector, recursive: bool = False) -> None", set_rgb);
  vm->bind(mod, "get_rgb(o: Object) -> Vector", get_rgb);

  vm->bind(mod, "set_alpha(o: Object, alpha: float, recursive: bool = False) -> None", set_alpha);
  vm->bind(mod, "get_alpha(o: Object) -> float", get_alpha);

  vm->bind(mod, "set_life_time(o: Object, life_time: float | str | None) -> None", set_life_time);
  vm->bind(mod, "get_life_time(o: Object) -> float | None", get_life_time);

  vm->bind(mod, "get_active_time(o: Object) -> float", get_active_time);
  vm->bind(mod, "reset_active_time(o: Object, recursive: bool = False) -> None", reset_active_time);
}

void orxPy_AddConfigModule(py::VM *vm)
{
  // Register config module
  py::PyVar mod = vm->new_module("config");

  using namespace pythonwrapper;

  // Bind config functions
  vm->bind(mod, "push_section(name: str) -> None", push_section);
  vm->bind(mod, "pop_section() -> None", pop_section);

  vm->bind(mod, "set_bool(key: str, value: bool) -> None", set_bool);
  vm->bind(mod, "get_bool(key: str, index: int | None = None) -> bool", get_bool);
  vm->bind(mod, "set_int(key: str, value: int) -> None", set_int);
  vm->bind(mod, "get_int(key: str, index: int | None = None) -> int", get_int);
  vm->bind(mod, "set_uint(key: str, value: int) -> None", set_uint);
  vm->bind(mod, "get_uint(key: str, index: int | None = None) -> int", get_uint);
  vm->bind(mod, "set_float(key: str, value: float) -> None", set_float);
  vm->bind(mod, "get_float(key: str, index: int | None = None) -> float", get_float);
  vm->bind(mod, "set_string(key: str, value: str) -> None", set_string);
  vm->bind(mod, "get_string(key: str, index: int | None = None) -> str", get_string);
  vm->bind(mod, "set_vector(key: str, value: Vector) -> None", set_vector);
  vm->bind(mod, "get_vector(key: str, index: int | None = None) -> Vector", get_vector);

  vm->bind(mod, "has_section(name: str) -> bool", has_section);
  vm->bind(mod, "has_value(key: str, check_spelling: bool = True) -> bool", has_value);

  vm->bind(mod, "clear_section(name: str) -> None", clear_section);
  vm->bind(mod, "clear_value(key: str) -> None", clear_value);
}

void orxPy_AddCommandModule(py::VM *vm)
{
  // Register command module
  py::PyVar mod = vm->new_module("command");

  using namespace pythonwrapper;

  // Bind command functions
  vm->bind(mod, "evaluate(command: str, guid: int | None = None) -> None", evaluate);
}

void orxPy_AddModules(py::VM *vm)
{
  orxPy_AddVectorModule(vm);
  orxPy_AddConfigModule(vm);
  orxPy_AddCommandModule(vm);
  orxPy_AddObjectModule(vm);
}

orxSTATUS orxPy_InitVM(py::VM *vm)
{
  orxSTATUS eResult = orxSTATUS_FAILURE;

  // Add core orx modules to the VM
  orxPy_AddModules(vm);

  // Get Python source location
  orxConfig_PushSection("Python");
  const orxSTRING zSource = orxConfig_GetString("Source");
  orxConfig_PopSection();

  // Read and compile Python source
  eResult = orxPy_ExecSource(vm, zSource);

  return eResult;
}

void orxPy_Exit(py::VM *vm)
{
  delete vm;
}

/** Init function, it is called when all orx's modules have been initialized
 */
orxSTATUS orxpy::Init()
{
  // Init extensions
  InitExtensions();

  pVM = new py::VM();
  orxASSERT(pVM != nullptr);

  orxSTATUS eResult = orxPy_InitVM(pVM);

  if (eResult == orxSTATUS_SUCCESS)
  {
    orxPy_InitCallbacks(pVM, &stPyCallbacks);
    orxPy_Call(pVM, stPyCallbacks.pyInit);
  }

  // Done!
  return eResult;
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
  orxPy_Call(pVM, stPyCallbacks.pyExit);

  // Exit from extensions
  ExitExtensions();

  // Exit and clean up Python VM
  orxPy_Exit(pVM);

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
