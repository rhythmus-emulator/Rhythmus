#include "LR2Sprite.h"
#include "Timer.h"
#include "LR2Flag.h"

namespace rhythmus
{

// ---------------------------- class LR2Sprite

LR2Sprite::LR2Sprite() : timer_id_(0), src_timer_id_(0)
{
  set_name("LR2Sprite");
  memset(op_, 0, sizeof(op_));
}

void LR2Sprite::LoadProperty(const std::string& prop_name, const std::string& value)
{
  Sprite::LoadProperty(prop_name, value);

  if (prop_name == "#SRC_IMAGE")
  {
    // use SRC timer here
    src_timer_id_ = GetAttribute<int>("src_timer");
  }
  else if (prop_name == "#DST_IMAGE")
  {
    op_[0] = GetAttribute<int>("op0");
    op_[1] = GetAttribute<int>("op1");
    op_[2] = GetAttribute<int>("op2");
    timer_id_ = GetAttribute<int>("timer");
  }
}

bool LR2Sprite::IsVisible() const
{
  return Sprite::IsVisible() &&
    LR2Flag::GetFlag(op_[0]) && LR2Flag::GetFlag(op_[1]) &&
    LR2Flag::GetFlag(op_[2]) && LR2Flag::IsTimerActive(timer_id_);
}

}