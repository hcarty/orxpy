/**
 * @file orxpy.h
 * @date 22-Jun-2024
 */

#ifndef __orxpy_H__
#define __orxpy_H__

#include "Scroll.h"

/** Game Class
 */
class orxpy : public Scroll<orxpy>
{
public:
private:
  orxSTATUS Bootstrap() const;

  void Update(const orxCLOCK_INFO &_rstInfo);

  orxSTATUS Init();
  orxSTATUS Run();
  void Exit();
  void BindObjects();

private:
};

#endif // __orxpy_H__
