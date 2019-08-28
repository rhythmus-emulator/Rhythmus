#include "BaseObject.h"
#include "LR2/LR2SceneLoader.h"   // XXX: atoi_op
#include "rutil.h"

namespace rhythmus
{

BaseObject::BaseObject()
  : parent_(nullptr), draw_order_(0), tween_length_(0), tween_loop_start_(0)
{
  memset(&current_prop_, 0, sizeof(DrawProperty));
  current_prop_.sw = current_prop_.sh = 1.0f;
  current_prop_.display = true;
  SetRGB(1.0f, 1.0f, 1.0f);
  SetAlpha(1.0f);
  SetScale(1.0f, 1.0f);
}

BaseObject::BaseObject(const BaseObject& obj)
{
  name_ = obj.name_;
  parent_ = obj.parent_;
  // XXX: childrens won't copy by now
  tween_ = obj.tween_;
  current_prop_ = obj.current_prop_;
}

BaseObject::~BaseObject()
{
  for (auto* p : owned_children_)
    delete p;
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
}

void BaseObject::RemoveChild(BaseObject* obj)
{
  auto it = std::find(children_.begin(), children_.end(), obj);
  if (it != children_.end())
    children_.erase(it);
}

void BaseObject::RegisterChild(BaseObject* obj)
{
  owned_children_.push_back(obj);
}

BaseObject* BaseObject::get_parent()
{
  return parent_;
}

BaseObject* BaseObject::FindChildByName(const std::string& name)
{
  for (const auto& spr : children_)
    if (spr->get_name() == name) return spr;
  return nullptr;
}

BaseObject* BaseObject::FindRegisteredChildByName(const std::string& name)
{
  for (const auto& spr : owned_children_)
    if (spr->get_name() == name) return spr;
  return nullptr;
}

BaseObject* BaseObject::GetLastChild()
{
  if (children_.size() > 0)
    return children_.back();
  else return nullptr;
}

bool BaseObject::IsAttribute(const std::string& key) const
{
  auto ii = attributes_.find(key);
  return (ii != attributes_.end());
}

void BaseObject::SetAttribute(const std::string& key, const std::string& value)
{
  attributes_[key] = value;
}

void BaseObject::DeleteAttribute(const std::string& key)
{
  auto ii = attributes_.find(key);
  if (ii != attributes_.end())
    attributes_.erase(ii);
}

template <>
std::string BaseObject::GetAttribute(const std::string& key) const
{
  const auto ii = attributes_.find(key);
  if (ii != attributes_.end())
    return ii->second;
  else
    return std::string();
}

template <>
int BaseObject::GetAttribute(const std::string& key) const
{
  return atoi_op( GetAttribute<std::string>(key).c_str() );
}

void BaseObject::LoadProperty(const std::map<std::string, std::string>& properties)
{
  for (const auto& ii : properties)
  {
    LoadProperty(ii.first, ii.second);
  }
}

// return true if property is set
void BaseObject::LoadProperty(const std::string& prop_name, const std::string& value)
{
  if (prop_name == "pos")
  {
    const auto it = std::find(value.begin(), value.end(), ',');
    if (it != value.end())
    {
      // TODO: change into 'setting'
      std::string v1 = value.substr(0, it - value.begin());
      const char *v2 = &(*it);
      SetPos(atoi(v1.c_str()), atoi(v2));
    }
  }
  /* Below is LR2 type commands */
  else if (strnicmp(prop_name.c_str(), "#DST_", 5) == 0)
  {
    std::vector<std::string> params;
    MakeParamCountSafe(value, params, 20);
    int attr = atoi(params[0].c_str());
    int time = atoi(params[1].c_str());
    int x = atoi(params[2].c_str());
    int y = atoi(params[3].c_str());
    int w = atoi(params[4].c_str());
    int h = atoi(params[5].c_str());
    int acc_type = atoi(params[6].c_str());
    int a = atoi(params[7].c_str());
    int r = atoi(params[8].c_str());
    int g = atoi(params[9].c_str());
    int b = atoi(params[10].c_str());
    int blend = atoi(params[11].c_str());
    int filter = atoi(params[12].c_str());
    int angle = atoi(params[13].c_str());
    int center = atoi(params[14].c_str());
    int loop = atoi(params[15].c_str());
    int timer = atoi(params[16].c_str());
    int op1 = atoi_op(params[17].c_str());
    int op2 = atoi_op(params[18].c_str());
    int op3 = atoi_op(params[19].c_str());

    SetTime(time);
    SetPos(x, y);
    SetSize(w, h);
    SetRGB(r, g, b);
    SetAlpha(a);
    SetRotation(0, 0, angle);

    if (loop > 0)
    {
      // trick: save loop information
      tween_loop_start_ = loop;
      tween_original_ = tween_;
    }

    // these attribute will be processed in LR2Objects
    if (!IsAttribute("timer")) SetAttribute("timer", params[16]);
    if (!IsAttribute("op1")) SetAttribute("op1", params[17]);
    if (!IsAttribute("op2")) SetAttribute("op2", params[18]);
    if (!IsAttribute("op3")) SetAttribute("op3", params[19]);
  }
}

void BaseObject::LoadDrawProperty(const BaseObject& other)
{
  LoadDrawProperty(other.current_prop_);
}

void BaseObject::LoadDrawProperty(const DrawProperty& draw_prop)
{
  current_prop_ = draw_prop;
}

void BaseObject::LoadTween(const BaseObject& other)
{
  LoadTween(other.tween_);
}

void BaseObject::LoadTween(const Tween& tween)
{
  tween_ = tween;
}

void BaseObject::AddTweenState(const DrawProperty &draw_prop, uint32_t time_duration, int ease_type, bool loop)
{
  tween_.emplace_back(TweenState{
    draw_prop, time_duration, 0, 0, loop, ease_type
    });
}

DrawProperty& BaseObject::GetDestDrawProperty()
{
  if (tween_.empty())
    return current_prop_;
  else
    return tween_.back().draw_prop;
}

DrawProperty& BaseObject::get_draw_property()
{
  return current_prop_;
}

void BaseObject::SetTime(int time_msec)
{
  if (tween_length_ <= time_msec) return;
  SetDeltaTime(time_msec - tween_length_);
}

void BaseObject::SetDeltaTime(int time_msec)
{
  tween_.push_back({
    GetDestDrawProperty(),
    (uint32_t)time_msec, 0, 0, false, EaseTypes::kEaseOut
    });
  tween_length_ += time_msec;
}

void BaseObject::SetPos(int x, int y)
{
  auto& p = GetDestDrawProperty();
  p.pi.x = x;
  p.pi.y = y;
}

void BaseObject::MovePos(int x, int y)
{
  auto& p = GetDestDrawProperty();
  p.pi.x += x;
  p.pi.y += y;
}

void BaseObject::SetSize(int w, int h)
{
  auto& p = GetDestDrawProperty();
  p.w = w;
  p.h = h;
}

void BaseObject::SetAlpha(float a)
{
  auto& p = GetDestDrawProperty();
  p.aBL = p.aBR = p.aTL = p.aTR = a;
}

void BaseObject::SetRGB(float r, float g, float b)
{
  auto& p = GetDestDrawProperty();
  p.r = r;
  p.g = g;
  p.b = b;
}

void BaseObject::SetScale(float x, float y)
{
  auto& p = GetDestDrawProperty();
  p.pi.sx = x;
  p.pi.sy = y;
}

void BaseObject::SetRotation(float x, float y, float z)
{
  auto& p = GetDestDrawProperty();
  p.pi.rotx = x;
  p.pi.roty = y;
  p.pi.rotz = z;
}

void BaseObject::SetCenter(float x, float y)
{
  auto& p = GetDestDrawProperty();
  p.pi.tx = x;
  p.pi.ty = y;
}

void BaseObject::Hide()
{
  current_prop_.display = false;
}

void BaseObject::Show()
{
  current_prop_.display = true;
}

void BaseObject::GetVertexInfo(VertexInfo* vi)
{
  const DrawProperty &ti = current_prop_;

  float x1, y1, x2, y2, sx1, sx2, sy1, sy2;

  x1 = ti.x;
  y1 = ti.y;
  x2 = x1 + ti.w;
  y2 = y1 + ti.h;
  sx1 = ti.sx;
  sy1 = ti.sy;
  sx2 = sx1 + ti.sw;
  sy2 = sy1 + ti.sh;

  // for predefined src width / height (-1 means use whole texture)
  if (ti.sw == -1) sx1 = 0.0, sx2 = 1.0;
  if (ti.sh == -1) sy1 = 0.0, sy2 = 1.0;

  vi[0].x = x1;
  vi[0].y = y1;
  vi[0].z = 0;
  vi[0].sx = sx1;
  vi[0].sy = sy1;
  vi[0].r = ti.r;
  vi[0].g = ti.g;
  vi[0].b = ti.b;
  vi[0].a = ti.aTL;

  vi[1].x = x1;
  vi[1].y = y2;
  vi[1].z = 0;
  vi[1].sx = sx1;
  vi[1].sy = sy2;
  vi[1].r = ti.r;
  vi[1].g = ti.g;
  vi[1].b = ti.b;
  vi[1].a = ti.aBL;

  vi[2].x = x2;
  vi[2].y = y2;
  vi[2].z = 0;
  vi[2].sx = sx2;
  vi[2].sy = sy2;
  vi[2].r = ti.r;
  vi[2].g = ti.g;
  vi[2].b = ti.b;
  vi[2].a = ti.aBR;

  vi[3].x = x2;
  vi[3].y = y1;
  vi[3].z = 0;
  vi[3].sx = sx2;
  vi[3].sy = sy1;
  vi[3].r = ti.r;
  vi[3].g = ti.g;
  vi[3].b = ti.b;
  vi[3].a = ti.aTR;
}

void BaseObject::SetDrawOrder(int order)
{
  draw_order_ = order;
}

int BaseObject::GetDrawOrder() const
{
  return draw_order_;
}

// milisecond
void BaseObject::UpdateTween(float delta)
{
  // TODO: process delta
  // TODO: calculate loop

  DrawProperty& ti = current_prop_;

  // DST calculation start.
  if (IsTweening())
  {
    if (tween_.size() == 1)
    {
      ti = tween_.front().draw_prop;
    }
    else
    {
      const TweenState &t1 = tween_.front();
      const TweenState &t2 = *std::next(tween_.begin());
      ti.display = t1.draw_prop.display;

      // If not display, we don't need to calculate further away.
      if (ti.display)
      {
        float r = (float)t1.time_eclipsed / t1.time_duration;
        MakeTween(ti, t1.draw_prop, t2.draw_prop, r, t1.ease_type);
      }
    }
  }
}

bool BaseObject::IsVisible() const
{
  return current_prop_.display &&
    current_prop_.aBL > 0 &&
    current_prop_.aBR > 0 &&
    current_prop_.aTL > 0 &&
    current_prop_.aTR > 0;
}

bool BaseObject::IsTweening() const
{
  return tween_.size() > 0;
}


// milisecond
void BaseObject::Update(float delta)
{
  UpdateTween(delta);
  doUpdate(delta);
  for (auto* p : children_)
    p->Update(delta);
}

void BaseObject::Render()
{
  doRender();
  for (auto* p : children_)
    p->Render();
}

void BaseObject::doUpdate(float delta)
{
  // Implement on your own
}

void BaseObject::doRender()
{
  // Implement on your own
}


#define TWEEN_ATTRS \
  TWEEN(x) \
  TWEEN(y) \
  TWEEN(w) \
  TWEEN(h) \
  TWEEN(r) \
  TWEEN(g) \
  TWEEN(b) \
  TWEEN(aTL) \
  TWEEN(aTR) \
  TWEEN(aBR) \
  TWEEN(aBL) \
  TWEEN(sx) \
  TWEEN(sy) \
  TWEEN(sw) \
  TWEEN(sh) \
  TWEEN(pi.rotx) \
  TWEEN(pi.roty) \
  TWEEN(pi.rotz) \
  TWEEN(pi.tx) \
  TWEEN(pi.ty) \
  TWEEN(pi.x) \
  TWEEN(pi.y) \
  TWEEN(pi.sx) \
  TWEEN(pi.sy) \


void MakeTween(DrawProperty& ti, const DrawProperty& t1, const DrawProperty& t2,
  double r, int ease_type)
{
  switch (ease_type)
  {
  case EaseTypes::kEaseLinear:
  {
#define TWEEN(attr) \
  ti.attr = t1.attr * (1 - r) + t2.attr * r;

    TWEEN_ATTRS;

#undef TWEEN
    break;
  }
  case EaseTypes::kEaseIn:
  {
    // use cubic function
    r = r * r * r;
#define TWEEN(attr) \
  ti.attr = t1.attr * (1 - r) + t2.attr * r;

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
  ti.attr = t1.attr * (1 - r) + t2.attr * r;

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
  ti.attr = t1.attr * (1 - r) + t2.attr * r;

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

void MakeParamCountSafe(const std::string& in,
  std::vector<std::string> &vsOut, char sep, int required_size)
{
  rutil::split(in, sep, vsOut);
  for (auto i = vsOut.size(); i < required_size; ++i)
    vsOut.emplace_back(std::string());
}

std::string GetFirstParam(const std::string& in, char sep)
{
  return in[0] != sep ? in.substr(0, in.find(',') - 1) : std::string();
}

}