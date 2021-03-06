#include "BaseObject.h"
#include "SceneManager.h"
#include "Setting.h"
#include "Util.h"
#include "Script.h"
#include "Logger.h"
#include "KeyPool.h"
#include "common.h"
#include "config.h"

namespace rhythmus
{

#define TWEEN_ATTRS \
  TWEEN(pos) \
  TWEEN(color) \
  TWEEN(rotate) \
  TWEEN(align) \
  TWEEN(scale)


void MakeTween(DrawProperty& ti, const DrawProperty& t1, const DrawProperty& t2,
  float r, int ease_type)
{
  switch (ease_type)
  {
  case EaseTypes::kEaseLinear:
  {
#define TWEEN(attr) \
  ti.attr = t1.attr * (1.0f - r) + t2.attr * r;

    TWEEN_ATTRS;

#undef TWEEN
    break;
  }
  case EaseTypes::kEaseIn:
  {
    // use cubic function
    r = r * r * r;
#define TWEEN(attr) \
  ti.attr = t1.attr * (1.0f - r) + t2.attr * r;

    TWEEN_ATTRS;

#undef TWEEN
    break;
  }
  case EaseTypes::kEaseOut:
  {
    // use cubic function
    r = 1 - r;
    r = r * r * r;
    r = 1 - r;
#define TWEEN(attr) \
  ti.attr = t1.attr * (1.0f - r) + t2.attr * r;

    TWEEN_ATTRS;

#undef TWEEN
    break;
  }
  case EaseTypes::kEaseInOut:
  {
    // use cubic function
    r = 2 * r - 1;
    r = r * r * r;
    r = 0.5f + r / 2;
#define TWEEN(attr) \
  ti.attr = t1.attr * (1.0f - r) + t2.attr * r;

    TWEEN_ATTRS;

#undef TWEEN
    break;
  }
  case EaseTypes::kEaseNone:
  default:
  {
#define TWEEN(attr) \
  ti.attr = t1.attr;

    TWEEN_ATTRS;

#undef TWEEN
    break;
  }
  }
}

// ---------------------------------------------------------------- class Tween

Animation::Animation(const DrawProperty *initial_state)
  : current_frame_(-1), current_frame_time_(0), frame_time_(0),
    is_finished_(false), repeat_(false), paused_(false), repeat_start_time_(0)
{
  if (initial_state)
    AddFrame(*initial_state, 0, EaseTypes::kEaseLinear);
}

void Animation::Clear()
{
  frames_.clear();
}

void Animation::DuplicateFrame(double duration)
{
  if (frames_.empty()) return;
  if (duration > 0) frames_.push_back(frames_.back());
  frames_.back().time += duration;
}

void Animation::AddFrame(const AnimationFrame &frame)
{
  if (!frames_.empty() && frames_.back().time >= frame.time)
    frames_.back() = frame;
  else
    frames_.push_back(frame);
}

void Animation::AddFrame(AnimationFrame &&frame)
{
  if (!frames_.empty() && frames_.back().time >= frame.time)
    frames_.back() = frame;
  else
    frames_.emplace_back(frame);
}

void Animation::AddFrame(const DrawProperty &draw_prop, double time, int ease_type)
{
  if (!frames_.empty() && frames_.back().time >= time)
  {
    frames_.back().time = time;
    frames_.back().draw_prop = draw_prop;
    frames_.back().ease_type = ease_type;
  }
  else
    frames_.emplace_back(AnimationFrame{ draw_prop, time, ease_type });
}

void Animation::SetCommand(const std::string &cmd)
{
  // set command to invoke when animation finished.
  command = cmd;
}

void Animation::Update(double &ms, std::string &command_to_invoke, DrawProperty *out)
{
  // don't do anything if empty.
  if (frames_.empty() || paused_) return;

  // find out which frame is it in.
  frame_time_ += ms;
  if (repeat_)
  {
    const double loop_time = frames_.back().time;
    const double actual_loop_time = loop_time - (double)repeat_start_time_;
    if (actual_loop_time <= 0)
      frame_time_ = std::min(frame_time_, loop_time);
    else if (frame_time_ > loop_time)
      frame_time_ = fmod(frame_time_ - loop_time, actual_loop_time) + (double)repeat_start_time_;
    ms = 0;
  }
  for (current_frame_ = -1; current_frame_ < (int)frames_.size() - 1; ++current_frame_)
  {
    if (frame_time_ < frames_[current_frame_ + 1].time)
      break;
  }
  if (current_frame_ < 0)
  {
    // if not yet to first frame, then exit.
    current_frame_time_ = 0;
    ms = 0;
    return;
  }
  else if (!repeat_ && frame_time_ >= frames_.back().time)
  {
    // stop animation as it had reached its end.
    current_frame_ = (unsigned)frames_.size() - 1;
    current_frame_time_ = 0;
    ms = frame_time_ - frames_.back().time;
    command_to_invoke = command;
    is_finished_ = true;
  }
  else
  {
    current_frame_time_ = frame_time_ - frames_[current_frame_].time;
    ms = 0;
  }

  // return calculated drawproperty.
  if (out)
    GetDrawProperty(*out);
}

void Animation::Replay()
{
  current_frame_ = 0;
  current_frame_time_ = 0;
  frame_time_ = 0;
  is_finished_ = false;
  paused_ = false;
}

void Animation::Play()
{
  paused_ = false;
}

void Animation::Pause()
{
  paused_ = true;
}

void Animation::HurryTween()
{
  R_ASSERT(!frames_.empty());
  current_frame_ = frames_.size() - 1;
}

void Animation::GetDrawProperty(DrawProperty &out)
{
  if (current_frame_ == frames_.size() - 1)
    out = frames_.back().draw_prop;
  else
    MakeTween(out,
      frames_[current_frame_].draw_prop,
      frames_[current_frame_ + 1].draw_prop,
      (float)(current_frame_time_ / (frames_[current_frame_ + 1].time - frames_[current_frame_].time)),
      frames_[current_frame_].ease_type);
}

void Animation::SetEaseType(int ease_type)
{
  if (!frames_.empty())
    frames_.back().ease_type = ease_type;
}

void Animation::SetLoop(unsigned repeat_start_time)
{
  repeat_ = true;
  repeat_start_time_ = repeat_start_time;
}

void Animation::DeleteLoop()
{
  repeat_ = false;
  repeat_start_time_ = 0;
}

const DrawProperty &Animation::LastFrame() const
{
  return frames_.back().draw_prop;
}

DrawProperty &Animation::LastFrame()
{
  return frames_.back().draw_prop;
}

double Animation::GetTweenLength() const
{
  if (frames_.empty()) return 0;
  else return frames_.back().time;
}

size_t Animation::size() const { return frames_.size(); }
bool Animation::empty() const { return frames_.empty(); }
bool Animation::is_finished() const { return is_finished_; }

bool Animation::is_tweening() const
{
  return !frames_.empty() && current_frame_ >= 0;
}


// ----------------------------------------------------------- class BaseObject

BaseObject::BaseObject()
  : parent_(nullptr), is_allocated_(false), propagate_event_(false), draw_order_(0), position_prop_(0),
    set_xy_as_center_(false), visible_(true), hide_if_not_tweening_(false),
    ignore_visible_group_(true), is_draggable_(false), is_focusable_(false),
    is_focused_(false), is_hovered_(false), do_clipping_(false)
{
  memset(&frame_, 0, sizeof(DrawProperty));
  SetRGB(1.0f, 1.0f, 1.0f);
  SetAlpha(1.0f);
  SetScale(1.0f, 1.0f);
  SetCenter(0.5f, 0.5f);
  SetRotation(0, 0, 0);
  memset(visible_flag_, 0, sizeof(visible_flag_));
  memset(&bg_color_, 0, sizeof(Vector4));
}

BaseObject::BaseObject(const BaseObject& obj)
  : name_(obj.name_), is_allocated_(obj.is_allocated_), propagate_event_(obj.propagate_event_),
    parent_(obj.parent_), ani_(obj.ani_), position_prop_(obj.position_prop_),
    draw_order_(obj.draw_order_), set_xy_as_center_(obj.set_xy_as_center_), visible_(obj.visible_),
    hide_if_not_tweening_(obj.hide_if_not_tweening_),
    ignore_visible_group_(obj.ignore_visible_group_),
    is_focusable_(obj.is_focusable_), is_draggable_(false), is_focused_(false),
    is_hovered_(false), do_clipping_(false)
{
  frame_ = obj.frame_;
  memcpy(visible_flag_, obj.visible_flag_, sizeof(visible_flag_));
  bg_color_ = obj.bg_color_;

  for (auto *o : obj.children_)
  {
    if (o->is_allocated_)
      AddChild(o->clone());
    else
      AddChild(o);
  }
}

BaseObject::~BaseObject()
{
  SCENEMAN->ClearFocus(this);
  RemoveAllChild();
}

BaseObject *BaseObject::clone()
{
  return new BaseObject(*this);
}

void BaseObject::set_name(const std::string& name)
{
  name_ = name;
}

const std::string& BaseObject::get_name() const
{
  return name_;
}

void BaseObject::AddChild(BaseObject* obj)
{
  children_.push_back(obj);
  obj->parent_ = this;
}

void BaseObject::RemoveChild(BaseObject* obj)
{
  auto it = std::find(children_.begin(), children_.end(), obj);
  if (it != children_.end())
    children_.erase(it);
  if (obj->is_allocated_) delete obj;
  else obj->parent_ = nullptr;
}

void BaseObject::RemoveAllChild()
{
  for (auto *o : children_)
    if (o->is_allocated_)
      delete o;
  children_.clear();
}

BaseObject* BaseObject::FindChildByName(const std::string& name)
{
  for (auto *child : children_)
    if (child->get_name() == name) return child;
  return nullptr;
}

void BaseObject::set_parent(BaseObject* obj)
{
  parent_ = obj;
}

BaseObject* BaseObject::get_parent()
{
  return parent_;
}

BaseObject* BaseObject::GetLastChild()
{
  if (children_.size() > 0)
    return children_.back();
  else return nullptr;
}

BaseObject *BaseObject::GetChildAtPosition(float x, float y)
{
  for (auto i = children_.rbegin(); i != children_.rend(); ++i)
  {
    auto *obj = *i;
    if (/*obj->IsVisible() &&*/ obj->IsEntered(x, y))
    {
      auto *childobj = obj->GetChildAtPosition(x - obj->GetX(), y - obj->GetY());
      if (childobj) return childobj;
      else return obj;
    }
  }
  return nullptr;
}

bool BaseObject::IsHeapAllocated(bool v) const
{
  return is_allocated_;
}

void BaseObject::Load(const MetricGroup &m)
{
  // Load command
  LoadCommand(m);

  // Load values
  m.get_safe("name", name_);
  m.get_safe("zindex", draw_order_);
  m.get_safe("focus", is_focusable_);
  m.get_safe("clipping", do_clipping_);
  if (m.exist("background"))
    FillColorFromString(bg_color_, m.get_str("background"));
  m.get_safe("x", frame_.pos.x);
  m.get_safe("y", frame_.pos.y);
  if (m.exist("w"))
    frame_.pos.z = frame_.pos.x + m.get<float>("w");
  if (m.exist("h"))
    frame_.pos.w = frame_.pos.y + m.get<float>("h");
}

void BaseObject::LoadFromName()
{
  if (get_name().empty())
    return;

  MetricGroup *m = METRIC->get_group(get_name());
  if (!m)
    return;

  Load(*m);

  for (auto *o : children_)
    o->LoadFromName();
}

void BaseObject::OnReady()
{
  // propagate OnReady() to children
  for (auto *o : children_)
    o->OnReady();
}

void BaseObject::RunCommandByName(const std::string &event_name)
{
  auto it = commands_.find(event_name);
  if (it != commands_.end())
    RunCommand(it->second);
}

void BaseObject::RunCommand(std::string command)
{
  // @warn
  // RunCommand parameter must be passed by value, not by reference
  // since original command string might be destroyed while processing command
  // e.g. command 'flush' clears event early and it destroys original command.

  if (command.empty()) return;
  size_t ia = 0, ib = 0;
  std::string cmd_type, value;
  while (ib <= command.size())
  {
    if (command[ib] == ';' || command[ib] == 0)
    {
      Split(command.substr(ia, ib - ia), ':', cmd_type, value);
      RunCommand(cmd_type, value);
      ia = ib = ib + 1;
    }
    else ++ib;
  }
}

void BaseObject::RunCommand(const std::string &command, const std::string& value)
{
  auto &fnmap = GetCommandFnMap();
  auto it = fnmap.find(command);
  if (it == fnmap.end())
    return;
  else
  {
    try
    {
      CommandArgs args(value);
      it->second(this, args, value);
    }
    catch (std::out_of_range&)
    {
      std::cerr << "Error: Command parameter is not enough to execute " << command << std::endl;
    }
  }
  if (propagate_event_)
  {
    for (auto *obj : children_)
      obj->RunCommand(command, value);
  }
}

/* just clear out command, without event unregister. */
void BaseObject::ClearCommand(const std::string &name)
{
  auto it = commands_.find(name);
  if (it != commands_.end())
    commands_[name].clear();
}

void BaseObject::DeleteAllCommand()
{
  commands_.clear();
  UnsubscribeAll();
}

void BaseObject::QueueCommand(const std::string &command)
{
  if (ani_.empty())
  {
    //std::cerr << "Warning: tried to queue command without tween.";
    //return;
    RunCommand(command);
  }
  else
  {
    ani_.back().SetCommand(command);
  }
}

void BaseObject::AddCommand(const std::string &name, const std::string &command)
{
  // Append command if already exist.
  auto it = commands_.find(name);
  if (it == commands_.end())
  {
    commands_[name] = command;

    // add event handler if not registered
    SubscribeTo(name);
  }
  else
  {
    if (it->second.empty())
      it->second = command;
    else
      it->second += ";" + command;
  }
}

/* @warn Load() methods automatically calls LoadCommand(). */
void BaseObject::LoadCommand(const MetricGroup& metric)
{
  // process event handler registering
  // XXX: is this code is good enough?
  for (auto it = metric.cbegin(); it != metric.cend(); ++it)
  {
    if (it->first.size() >= 5 /* XXX: 'On' prefix? */
      && strnicmp(it->first.c_str(), "On", 2) == 0)
    {
      AddCommand(it->first.substr(2), it->second);
    }
  }
}

void BaseObject::LoadCommandWithPrefix(const std::string &prefix, const MetricGroup& metric)
{
  for (auto it = metric.cbegin(); it != metric.cend(); ++it)
  {
    if (strnicmp(it->first.c_str(), prefix.c_str(), prefix.size()) == 0)
    {
      AddCommand(it->first.substr(prefix.size()), it->second);
    }
  }
}

void BaseObject::HurryTween()
{
  if (!ani_.empty())
  {
    ani_.back().HurryTween();
    ani_.back().GetDrawProperty(frame_);
    ani_.clear();
    OnAnimation(frame_);
  }
}

bool BaseObject::OnEvent(const EventMessage& msg)
{
  RunCommandByName(msg.GetEventName());
  return true;
}

DrawProperty& BaseObject::GetLastFrame()
{
  if (ani_.empty())
    return frame_;
  else
    return ani_.back().LastFrame();
}

DrawProperty& BaseObject::GetCurrentFrame()
{
  return frame_;
}

void BaseObject::SetX(float x)
{
  float width = GetWidth();
  GetLastFrame().pos.x = x;
  GetLastFrame().pos.z = x + width;
}

void BaseObject::SetY(float y)
{
  float height = GetHeight();
  GetLastFrame().pos.y = y;
  GetLastFrame().pos.w = y + height;
}

void BaseObject::SetWidth(float w)
{
  GetLastFrame().pos.z = GetLastFrame().pos.x + w;
}

void BaseObject::SetHeight(float h)
{
  GetLastFrame().pos.w = GetLastFrame().pos.y + h;
}

void BaseObject::SetOpacity(float opa)
{
  GetLastFrame().color.a = opa;
}

void BaseObject::SetClip(bool clip)
{
  do_clipping_ = clip;
}

void BaseObject::SetPos(int x, int y)
{
  SetX(x);
  SetY(y);
}

void BaseObject::SetPos(const Vector4& pos)
{
  auto &p = GetLastFrame().pos;
  p = pos;
  p.z += p.x;
  p.w += p.y;
}

void BaseObject::MovePos(int x, int y)
{
  auto &p = GetLastFrame();
  p.pos.x += x;
  p.pos.y += y;
  p.pos.z += x;
  p.pos.w += y;
}

void BaseObject::SetSize(int w, int h)
{
  SetWidth(w);
  SetHeight(h);
}

void BaseObject::SetAlpha(unsigned a)
{
  SetAlpha(a / 255.0f);
}

void BaseObject::SetAlpha(float a)
{
  auto& p = GetLastFrame();
  p.color.a = a;
}

void BaseObject::SetRGB(unsigned r, unsigned g, unsigned b)
{
  SetRGB(r / 255.0f, g / 255.0f, b / 255.0f);
}

void BaseObject::SetRGB(float r, float g, float b)
{
  auto& p = GetLastFrame();
  p.color.r = r;
  p.color.g = g;
  p.color.b = b;
}

void BaseObject::SetScale(float x, float y)
{
  auto& p = GetLastFrame();
  p.scale.x = x;
  p.scale.y = y;
}

void BaseObject::SetRotation(float x, float y, float z)
{
  auto& p = GetLastFrame();
  p.rotate.x = x;
  p.rotate.y = y;
  p.rotate.z = z;
}

void BaseObject::SetRotationAsDegree(float x, float y, float z)
{
  SetRotation(
    glm::radians(x),
    glm::radians(y),
    glm::radians(z)
  );
}

void BaseObject::SetRepeat(bool repeat)
{
  if (!ani_.empty())
  {
    if (repeat)
      ani_.back().SetLoop(0);
    else
      ani_.back().DeleteLoop();
  }
}

void BaseObject::SetLoop(unsigned loop_start_time)
{
  ani_.back().SetLoop(loop_start_time);
}

void BaseObject::SetCenter(float x, float y)
{
  auto& p = GetLastFrame();
  p.align.x = x;
  p.align.y = y;
}

void BaseObject::SetCenter(int type)
{
  auto& p = GetLastFrame();
  float px[] = { 0.5f,
    1.0f,1.0f,1.0f,
    0.5f,0.5f,0.5f,
    0.0f,0.0f,0.0f
  };
  float py[] = { 0.5f,
    0.0f, 0.5f, 1.0f,
    0.0f, 0.5f, 1.0f,
    0.0f, 0.5f, 1.0f,
  };
  p.align.x = px[type];
  p.align.y = py[type];
}

void BaseObject::SetAcceleration(int acc)
{
  if (ani_.empty())
    return;
  ani_.back().SetEaseType(acc);
}

float BaseObject::GetX() const
{
  return frame_.pos.x;
}

float BaseObject::GetY() const
{
  return frame_.pos.y;
}

float BaseObject::GetWidth() const { return frame_.pos.z - frame_.pos.x; }
float BaseObject::GetHeight() const { return frame_.pos.w - frame_.pos.y; }

void BaseObject::SetDebug(const std::string &debug_msg)
{
  debug_ = debug_msg;
}

void BaseObject::BringToTop()
{
  if (!parent_) return;
  auto b = parent_->children_.begin();
  auto e = parent_->children_.end();
  auto it = std::find(b, e, this);
  if (it != e) {
    std::rotate(it, it + 1, e);
  }
}

void BaseObject::AddFrameByLR2command(const std::string &cmd)
{
  CommandArgs params(cmd, 20, true);
  const char *args[20];
  for (unsigned i = 0; i < 20; ++i)
    args[i] = params.Get_str(i);
  AddFrameByLR2command(args);
}

static inline int _atoi_n(const char *v)
{
  return v[0] == '-' ? -atoi(v + 1) : atoi(v);
}

void BaseObject::AddFrameByLR2command(const char **argv)
{
  if (ani_.empty()) ani_.emplace_back(Animation(nullptr));
  Animation &ani = ani_.back();

  int time = _atoi_n(argv[1]);
  int x = _atoi_n(argv[2]);
  int y = _atoi_n(argv[3]);
  int w = _atoi_n(argv[4]);
  int h = _atoi_n(argv[5]);
  int lr2acc = _atoi_n(argv[6]);
  int a = _atoi_n(argv[7]);
  int r = _atoi_n(argv[8]);
  int g = _atoi_n(argv[9]);
  int b = _atoi_n(argv[10]);
  //int blend = params.Get<int>(11);
  //int filter = params.Get<int>(12);
  int angle = _atoi_n(argv[13]);
  int center = _atoi_n(argv[14]);
  //int loop = _atoi_n(argv[15]);
  int acc = 0;
  // timer/op code is ignored here.

  // set attributes
  DrawProperty f;
  f.pos = Vector4{ x, y, x + w, y + h };
  f.color = Vector4{ r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f };
  f.rotate = Vector3{ 0.0f, 0.0f, glm::radians((float)angle) };
  f.scale = Vector2{ 1.0f, 1.0f };

  switch (lr2acc)
  {
  case 0:
    acc = EaseTypes::kEaseLinear;
    break;
  case 1:
    acc = EaseTypes::kEaseIn;
    break;
  case 2:
    acc = EaseTypes::kEaseOut;
    break;
  case 3:
    acc = EaseTypes::kEaseInOut;
    break;
  }

  // add new animation
  ani.AddFrame(f, time, acc);

  SetCenter(center);
}

void BaseObject::SetVisibleFlag(const std::string& group0, const std::string& group1,
  const std::string& group2, const std::string& group3)
{
  static const int alwaystrue = 1;
  ignore_visible_group_ = false;
  visible_flag_[0] = &alwaystrue;
  visible_flag_[1] = &alwaystrue;
  visible_flag_[2] = &alwaystrue;
  visible_flag_[3] = &alwaystrue;
  if (!group0.empty())
  {
    KeyData<int> k1 = KEYPOOL->GetInt(group0);
    visible_flag_[0] = &*k1;
  }
  if (!group1.empty())
  {
    KeyData<int> k2 = KEYPOOL->GetInt(group1);
    visible_flag_[1] = &*k2;
  }
  if (!group2.empty())
  {
    KeyData<int> k3 = KEYPOOL->GetInt(group2);
    visible_flag_[2] = &*k3;
  }
  if (!group3.empty())
  {
    KeyData<int> k4 = KEYPOOL->GetInt(group3);
    visible_flag_[3] = &*k4;
  }
}

void BaseObject::UnsetVisibleFlag()
{
  ignore_visible_group_ = true;
}

void BaseObject::Hide()
{
  visible_ = false;
}

void BaseObject::Show()
{
  visible_ = true;
}

void BaseObject::SetDrawOrder(int order)
{
  draw_order_ = order;
}

int BaseObject::GetDrawOrder() const
{
  return draw_order_;
}

void BaseObject::SetText(const std::string &value) {}

void BaseObject::SetNumber(int number) { SetText(std::to_string(number)); }

void BaseObject::SetNumber(double number) { SetText(std::to_string(number)); }

void BaseObject::Refresh() {}

void BaseObject::SetFocusable(bool is_focusable)
{
  is_focusable_ = is_focusable;
}

void BaseObject::SetDraggable(bool is_draggable)
{
  is_draggable_ = is_draggable;
}

bool BaseObject::IsEntered(float x, float y)
{
  // TODO: in case of rotation?
  return (x >= frame_.pos.x && x <= frame_.pos.z
       && y >= frame_.pos.y && y <= frame_.pos.w);
}

void BaseObject::SetHovered(bool is_hovered)
{
  if (is_hovered_ == is_hovered)
    return;

  if (is_hovered)
    RunCommandByName("hover");
  else
    RunCommandByName("hoverout");
  is_hovered_ = is_hovered;
}

void BaseObject::SetFocused(bool is_focused)
{
  if (!is_focusable_ || is_focused_ == is_focused)
    return;

  if (is_focused)
    RunCommandByName("focus");
  else
    RunCommandByName("focusout");
  is_focused_ = is_focused;
}

bool BaseObject::IsHovered() const
{
  return is_hovered_;
}

bool BaseObject::IsFocused() const
{
  return is_focused_;
}

bool BaseObject::IsDragging() const
{
  return this == SCENEMAN->GetDraggingObject();
}

bool BaseObject::IsFocusable() const
{
  return is_focusable_;
}

bool BaseObject::IsDraggable() const
{
  return is_draggable_;
}

void BaseObject::Click()
{
  if (!is_focusable_)
    return;

  // XXX: same as SetFocused(true) ...?
  SetFocused(true);

  RunCommandByName("click");
}

void BaseObject::OnDrag(float dx, float dy)
{
  frame_.pos.x += dx;
  frame_.pos.z += dx;
  frame_.pos.y += dy;
  frame_.pos.w += dy;
}

void BaseObject::OnText(uint32_t codepoint)
{
}

void BaseObject::OnAnimation(DrawProperty &frame)
{
}

bool BaseObject::IsVisible() const
{
  if (!ignore_visible_group_ && (*visible_flag_[0] == 0
                              || *visible_flag_[1] == 0
                              || *visible_flag_[2] == 0
                              || *visible_flag_[3] == 0))
    return false;

  if (hide_if_not_tweening_ && (ani_.empty() || !ani_.front().is_tweening()))
    return false;

  return visible_/* &&
    current_prop_.aBL > 0 &&
    current_prop_.aBR > 0 &&
    current_prop_.aTL > 0 &&
    current_prop_.aTR > 0*/;
}

void BaseObject::SetDeltaTime(double time_msec)
{
  ani_.back().DuplicateFrame(time_msec);
}

void BaseObject::Stop()
{
  ani_.clear();
}

void BaseObject::Replay()
{
  if (ani_.empty()) return;
  ani_.back().Replay();
}

void BaseObject::Pause()
{
  if (ani_.empty()) return;
  ani_.back().Pause();
}

double BaseObject::GetTweenLength() const
{
  if (ani_.empty()) return 0;
  else return ani_.back().GetTweenLength();
}

bool BaseObject::IsTweening() const
{
  return ani_.size() > 0;
}


// milisecond
void BaseObject::Update(double delta)
{
  // update tweening
  double t = delta;
  std::string cmd;
  if (!ani_.empty())
  {
    while (!ani_.empty() && t > 0)
    {
      auto &ani = ani_.front();
      ani.Update(t, cmd, &frame_);
      if (ani.is_finished())
        ani_.pop_front();
    }
    OnAnimation(frame_);
  }

  doUpdate(delta);
  for (auto* p : children_)
    p->Update(delta);
  doUpdateAfter();
}

void BaseObject::Render()
{
  if (!IsVisible())
    return;

  Vector2 size(frame_.pos.z - frame_.pos.x,
               frame_.pos.w - frame_.pos.y);

  // set matrix for rotation/translation
  // remind vertex's center coord is (0,0).
  // (see BaseObject::FillVertexInfo(...) for more detail)
  GRAPHIC->PushMatrix();
  /*
  if (position_prop_ == 0)
  {
  }*/
  GRAPHIC->Translate(Vector3{
    frame_.pos.x + size.x * frame_.align.x,
    frame_.pos.y + size.y * frame_.align.y,
    0 });
  if (frame_.rotate.x != 0 || frame_.rotate.y != 0 || frame_.rotate.z != 0)
    GRAPHIC->Rotate(frame_.rotate);
  GRAPHIC->Scale(Vector3{ frame_.scale.x, frame_.scale.y, 1.0f });
  if (set_xy_as_center_)
  {
    if (frame_.align.x != 0.5f || frame_.align.y != 0.5f)
    {
      GRAPHIC->Translate(Vector3{
        (0.5f-frame_.align.x) * size.x,
        (0.5f-frame_.align.y) * size.y,
        0 });
    }
  }
  else
  {
    if (frame_.align.x != 0.0f || frame_.align.y != 0.0f)
    {
      GRAPHIC->Translate(Vector3{
        -frame_.align.x * size.x,
        -frame_.align.y * size.y,
        0 });
    }
  }

  // TODO: texture translate

  // clip implementation (prevent content overflowing)
  // XXX: nested clipping support?
  if (do_clipping_)
    GRAPHIC->ClipViewArea(frame_.pos);

  // render background if necessary.
  // XXX: you can check rendering area by using background property.
  if (bg_color_.a > 0)
  {
    VertexInfo vi[4];
    vi[0].p = Vector3(-size.x / 2, -size.y / 2, 0.0f);
    vi[1].p = Vector3(size.x / 2, -size.y / 2, 0.0f);
    vi[2].p = Vector3(size.x / 2, size.y / 2, 0.0f);
    vi[3].p = Vector3(-size.x / 2, size.y / 2, 0.0f);
    vi[0].t = Vector2(0.0f, 0.0f);
    vi[1].t = Vector2(1.0f, 0.0f);
    vi[2].t = Vector2(1.0f, 1.0f);
    vi[3].t = Vector2(0.0f, 1.0f);
    vi[0].c = vi[1].c = vi[2].c = vi[3].c = bg_color_;
    GRAPHIC->SetBlendMode(1);
    GRAPHIC->SetTexture(0, 1);
    GRAPHIC->DrawQuad(vi);
  }

  // render vertices
  doRender();

  for (auto* p : children_)
    p->Render();

  doRenderAfter();

  // reset clipping
  if (do_clipping_)
    GRAPHIC->ResetViewArea();

  GRAPHIC->PopMatrix();
}

void BaseObject::FillVertexInfo(VertexInfo *vi)
{
  const DrawProperty &f = GetCurrentFrame();

  // we use center of vertex as base coord.
  // remind final drawing position will be determined by world matrix.

  float w = f.pos.z - f.pos.x;
  float h = f.pos.w - f.pos.y;

  vi[0].p = Vector3{ -w / 2, -h / 2, 0 };
  vi[0].c = f.color;

  vi[1].p = Vector3{ w / 2, -h / 2, 0 };
  vi[1].c = f.color;

  vi[2].p = Vector3{ w / 2, h / 2, 0 };
  vi[2].c = f.color;

  vi[3].p = Vector3{ -w / 2, h / 2, 0 };
  vi[3].c = f.color;
}

void BaseObject::doUpdate(double delta) {}
void BaseObject::doRender() {}
void BaseObject::doUpdateAfter() {}
void BaseObject::doRenderAfter() {}


/* ---------------------------- Command related */

const CommandFnMap& BaseObject::GetCommandFnMap()
{
  static CommandFnMap cmdfnmap_;
  if (cmdfnmap_.empty())
  {
    static auto fn_X = [](void *o, CommandArgs& args, const std::string &) {
      auto *obj = static_cast<BaseObject*>(o);
      auto vi = args.Get<float>(0);
      obj->SetX(vi);
    };
    static auto fn_Y = [](void *o, CommandArgs& args, const std::string &) {
      auto *obj = static_cast<BaseObject*>(o);
      auto vi = args.Get<float>(0);
      obj->SetY(vi);
    };
    static auto fn_W = [](void *o, CommandArgs& args, const std::string &) {
      static_cast<BaseObject*>(o)->SetWidth(args.Get<float>(0));
    };
    static auto fn_H = [](void *o, CommandArgs& args, const std::string &) {
      static_cast<BaseObject*>(o)->SetHeight(args.Get<float>(0));
    };
    static auto fn_Pos = [](void *o, CommandArgs& args, const std::string &) {
      static_cast<BaseObject*>(o)->SetPos(args.Get<int>(0), args.Get<int>(1));
    };
    static auto fn_Scale = [](void *o, CommandArgs& args, const std::string &) {
      if (args.size() == 1)
        static_cast<BaseObject*>(o)->SetScale(args.Get<float>(0), args.Get<float>(0));
      else
        static_cast<BaseObject*>(o)->SetScale(args.Get<float>(0), args.Get<float>(1));
    };
    static auto fn_Opacity = [](void *o, CommandArgs& args, const std::string &) {
      static_cast<BaseObject*>(o)->SetOpacity(args.Get<float>(0));
    };
    static auto fn_acc = [](void *o, CommandArgs& args, const std::string &) {
      if (args.size() == 0)
        return;
      std::string v = args.Get<std::string>(0);
      if (v[0] >= '0' && v[0] <= '9')
        static_cast<BaseObject*>(o)->SetAcceleration(args.Get<int>(0));
      else
      {
        const char *ease_names[] = {
          "none", "linear", "easein", "easeout", "easeinout", "easeinoutback", 0
        };
        for (int i = 0; ease_names[i]; ++i)
        {
          if (stricmp(v.c_str(), ease_names[i]) == 0)
          {
            static_cast<BaseObject*>(o)->SetAcceleration(i);
            return;
          }
        }
      }
    };
    static auto fn_time = [](void *o, CommandArgs& args, const std::string &) {
      static_cast<BaseObject*>(o)->SetDeltaTime(args.Get<int>(0));
    };
    static auto fn_stop = [](void *o, CommandArgs& args, const std::string &) {
      static_cast<BaseObject*>(o)->Stop();
    };
    static auto fn_loop = [](void *o, CommandArgs& args, const std::string &) {
      static_cast<BaseObject*>(o)->SetLoop(args.Get<int>(0));
    };
    static auto fn_repeat = [](void *o, CommandArgs& args, const std::string &) {
      static_cast<BaseObject*>(o)->SetRepeat(true);
    };
    static auto fn_rotate = [](void *o, CommandArgs& args, const std::string &) {
      static_cast<BaseObject*>(o)->SetRotationAsDegree(0.0f, 0.0f, args.Get<float>(0));
    };
    static auto fn_Show = [](void *o, CommandArgs& args, const std::string &) {
      static_cast<BaseObject*>(o)->Show();
    };
    static auto fn_Hide = [](void *o, CommandArgs& args, const std::string &) {
      static_cast<BaseObject*>(o)->Hide();
    };
    static auto fn_Replay = [](void *o, CommandArgs& args, const std::string &) {
      static_cast<BaseObject*>(o)->Replay();
    };
    static auto fn_Pause = [](void *o, CommandArgs& args, const std::string &) {
      static_cast<BaseObject*>(o)->Pause();
    };
    static auto fn_Text = [](void *o, CommandArgs& args, const std::string &) {
      static_cast<BaseObject*>(o)->SetText(args.Get<std::string>(0));
    };
    static auto fn_Number = [](void *o, CommandArgs& args, const std::string &) {
      static_cast<BaseObject*>(o)->SetNumber(args.Get<int>(0));
    };
    static auto fn_NumberF = [](void *o, CommandArgs& args, const std::string &) {
      static_cast<BaseObject*>(o)->SetNumber(args.Get<double>(0));
    };
    static auto fn_Refresh = [](void *o, CommandArgs& args, const std::string &) {
      static_cast<BaseObject*>(o)->Refresh();
    };
    static auto fn_name = [](void *o, CommandArgs& args, const std::string &) {
      static_cast<BaseObject*>(o)->set_name(args.Get<std::string>(0));
    };
    static auto fn_Focusable = [](void *o, CommandArgs& args, const std::string &) {
      static_cast<BaseObject*>(o)->SetFocusable(args.Get<int>(0));
    };
    static auto fn_SendEvent = [](void *o, CommandArgs& args, const std::string &) {
      EVENTMAN->SendEvent(args.Get<std::string>(0));
    };
    cmdfnmap_["x"] = fn_X;
    cmdfnmap_["y"] = fn_Y;
    cmdfnmap_["w"] = fn_W;
    cmdfnmap_["h"] = fn_H;
    cmdfnmap_["scale"] = fn_Scale;
    cmdfnmap_["opacity"] = fn_Opacity;
    cmdfnmap_["acc"] = fn_acc;
    cmdfnmap_["time"] = fn_time;
    cmdfnmap_["stop"] = fn_stop;
    cmdfnmap_["loop"] = fn_loop;
    cmdfnmap_["repeat"] = fn_repeat;
    cmdfnmap_["rotate"] = fn_rotate;
    cmdfnmap_["pos"] = fn_Pos;
    cmdfnmap_["show"] = fn_Show;
    cmdfnmap_["hide"] = fn_Hide;
    cmdfnmap_["replay"] = fn_Replay;
    cmdfnmap_["pause"] = fn_Pause;
    cmdfnmap_["text"] = fn_Text;
    cmdfnmap_["number"] = fn_Number;
    cmdfnmap_["numberf"] = fn_NumberF;
    cmdfnmap_["refresh"] = fn_Refresh;
    cmdfnmap_["name"] = fn_name;
    cmdfnmap_["focusable"] = fn_Focusable;
    cmdfnmap_["sendevent"] = fn_SendEvent;
  }

  return cmdfnmap_;
}

}

#include "Sprite.h"
#include "object/Text.h"
#include "object/Number.h"
#include "object/Slider.h"
#include "object/Bargraph.h"
#include "object/Button.h"
#include "object/OnMouse.h"
#include "object/Dialog.h"
#include "object/ListView.h"

namespace rhythmus
{

BaseObject* BaseObject::CreateObject(const std::string &type)
{
  BaseObject *object = nullptr;

  // create object from type string
  if (type == "image" || type == "sprite") {
    object = new Sprite();
  }
  else if (type == "text") {
    object = new Text();
  }
  else if (type == "number") {
    object = new Number();
  }
  else if (type == "slider") {
    object = new Slider();
  }
  else if (type == "bargraph") {
    object = new Bargraph();
  }
  else if (type == "onmouse") {
    object = new OnMouse();
  }
  else if (type == "button") {
    object = new Button();
  }
  else if (type == "dialog") {
    // TODO: add command process handler
    //object = new Dialog();
  }
  else if (type == "listview") {
    // TODO: add command process handler
    //object = new ListView();
  }
  else if (type == "frame") {
    // TODO: use BaseFrame() object
    object = new BaseObject();
  }
  else if (type == "line") {

  }

  if (object) {
    object->is_allocated_ = true;
  }

  return object;
}

const char* BaseObject::type() const { return "BaseObject"; }

std::string BaseObject::toString() const
{
  std::stringstream ss;
  ss << "type: " << type() << std::endl;
  if (commands_.empty())
    ss << "events: (empty)" << std::endl;
  else {
    ss << "events:" << std::endl;
    for (auto &i : commands_)
    {
      ss << " - " << i.first << " : " << i.second << std::endl;
    }
  }
  ss << "pos (rect) : " << frame_.pos.x << "," << frame_.pos.y << "," << frame_.pos.w << "," << frame_.pos.z << std::endl;
  ss << "draw_order : " << draw_order_ << std::endl;
  ss << "is_focusable? : " << (is_focusable_ ? "true" : "false") << std::endl;

  if (!debug_.empty())
    ss << "debug message: " << debug_ << std::endl;
  return ss.str();
}

bool BaseObject::is_dynamic() const { return is_allocated_; }

// ------------------------------------------------------------------ Loader/Helper

// TODO: Number::OnStart 메서드 구현하기 (Load에서 Async 코드 부분 제거하기)

class XMLObjectHandlers
{
public:
  static void sprite(XMLExecutor *e, XMLContext *ctx)
  {
    auto *o = BaseObject::CreateObject("sprite");
    auto *parent = (BaseObject*)e->GetParent();
    parent->AddChild(o);
    e->SetCurrentObject(o);
    o->Load(ctx->GetCurrentMetric());
  }
  static void text(XMLExecutor *e, XMLContext *ctx)
  {
    auto *o = BaseObject::CreateObject("text");
    auto *parent = (BaseObject*)e->GetParent();
    parent->AddChild(o);
    e->SetCurrentObject(o);
    o->Load(ctx->GetCurrentMetric());
  }
  static void slider(XMLExecutor *e, XMLContext *ctx)
  {
    auto *o = BaseObject::CreateObject("slider");
    auto *parent = (BaseObject*)e->GetParent();
    parent->AddChild(o);
    e->SetCurrentObject(o);
    o->Load(ctx->GetCurrentMetric());
  }
  XMLObjectHandlers()
  {
    XMLExecutor::AddHandler("sprite", (XMLCommandHandler)&sprite);
    XMLExecutor::AddHandler("image", (XMLCommandHandler)&sprite);
    XMLExecutor::AddHandler("text", (XMLCommandHandler)&text);
    XMLExecutor::AddHandler("slider", (XMLCommandHandler)&slider);
  }
};

class LR2CSVBaseHandlers
{
public:
  static bool src_base(BaseObject*& o, LR2CSVExecutor* loader, LR2CSVContext* ctx)
  {
    if (!ctx->get_str(0)) return false;
    const std::string type(ctx->get_str(0));

    // if current object is reserved one, then don't overwrite.
    if (o && !o->is_dynamic())
      return true;
    else
      o = nullptr;

    // create new object
    if (type == "#SRC_IMAGE")
      o = BaseObject::CreateObject("sprite");
    else if (type == "#SRC_TEXT")
      o = BaseObject::CreateObject("text");
    else if (type == "#SRC_NUMBER")
      o = BaseObject::CreateObject("number");
    else if (type == "#SRC_SLIDER")
      o = BaseObject::CreateObject("slider");
    else if (type == "#SRC_BARGRAPH")
      o = BaseObject::CreateObject("bargraph");
    else if (type == "#SRC_ONMOUSE")
      o = BaseObject::CreateObject("onmouse");
    else if (type == "#SRC_BUTTON")
      o = BaseObject::CreateObject("button");

    if (!o) {
      Logger::Warn("Warning: invalid command \"%s\".", ctx->get_str(0));
      return false;
    }

    // add to scene
    auto* scene = ((BaseObject*)loader->get_object("scene"));
    R_ASSERT(scene != nullptr);
    scene->AddChild(o);

    // set debug text
    o->SetDebug(format_string("LR2SRC-%u", ctx->get_str(21)));
    return true;
  }
  static bool dst_base(BaseObject*& o, LR2CSVExecutor* loader, LR2CSVContext* ctx)
  {
    const char* args[21];

    if (!o) {
      Logger::Warn("Warning: invalid command \"%s\".", ctx->get_str(0));
      return false;
    }

    for (unsigned i = 0; i < 21; ++i) args[i] = ctx->get_str(i);
    o->AddFrameByLR2command(args + 1);

    // these attributes are only affective for first run
    if (loader->get_command_index() == 0) {
      const int loop = ctx->get_int(16);
      const int timer = ctx->get_int(17);

      // LR2 needs to keep its animation queue, so don't use stop.
      o->AddCommand(format_string("LR%d", timer), "replay");
      o->AddCommand(format_string("LR%dOff", timer), "hide");
      if (loop >= 0)
        o->SetLoop(loop);

      // XXX: move this flag to Sprite,
      // as LR2_TEXT object don't work with this..?
      o->SetVisibleFlag(
        format_string("F%s", ctx->get_str(18)),
        format_string("F%s", ctx->get_str(19)),
        format_string("F%s", ctx->get_str(20)),
        std::string()
      );
    }
    return true;
  }
  LR2CSVBaseHandlers()
  {
    LR2CSVExecutor::AddHandler("#SRC_BASE_", (LR2CSVHandlerFunc)& src_base);
    LR2CSVExecutor::AddHandler("#DST_BASE_", (LR2CSVHandlerFunc)& dst_base);
  }
private:
};

// register handler
XMLObjectHandlers _XMLObjectHandlers;
LR2CSVBaseHandlers _LR2CSVBaseHandlers;

}
