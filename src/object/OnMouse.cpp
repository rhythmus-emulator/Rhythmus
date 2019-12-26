#include "OnMouse.h"
#include "Util.h"

namespace rhythmus
{

OnMouse::OnMouse() : panel_(0)
{
  memset(&onmouse_rect_, 0, sizeof(Rect));
}

OnMouse::~OnMouse() {}

void OnMouse::Load(const Metric &metric)
{
  Sprite::Load(metric);
}

void OnMouse::LoadFromLR2SRC(const std::string &cmd)
{
  Sprite::LoadFromLR2SRC(cmd);

  CommandArgs args(cmd);

  panel_ = args.Get<int>(9);
  if (panel_ > 0 || panel_ == -1)
  {
    if (panel_ == -1) panel_ = 0;
    AddCommand("Panel" + std::to_string(panel_), "focusable:1");
    AddCommand("Panel" + std::to_string(panel_) + "Off", "focusable:0");
    visible_group_[3] = 20 + panel_;
  }

  onmouse_rect_.x = args.Get<int>(10);
  onmouse_rect_.y = args.Get<int>(11);
  onmouse_rect_.w = args.Get<int>(12);
  onmouse_rect_.h = args.Get<int>(13);

  debug_ += format_string("called with %d\n", panel_);
}

bool OnMouse::IsEntered(float x, float y)
{
  x -= current_prop_.pi.x + current_prop_.x + onmouse_rect_.x;
  y -= current_prop_.pi.y + current_prop_.y + onmouse_rect_.y;
  return (x >= 0 && x <= onmouse_rect_.w
    && y >= 0 && y <= onmouse_rect_.h);
}

void OnMouse::doRender()
{
  if (!is_hovered_)
    return;
  Sprite::doRender();
}

}