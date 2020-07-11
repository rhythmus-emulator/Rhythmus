#include "Sprite.h"
#include "Game.h"
#include "ResourceManager.h"
#include "Script.h"
#include "Util.h"
#include "common.h"
#include "config.h"
#include <sstream>

namespace rhythmus
{

Sprite::Sprite()
  : img_(nullptr), time_(0), frame_(0), res_id_(nullptr),
    blending_(0), use_texture_coord_(true), tex_attribute_(0)
{
  set_xy_as_center_ = true;
  sprani_.divx = sprani_.divy = sprani_.cnt = 1;
  sprani_.duration = 0;
  texcoord_ = Rect{ 0.0f, 0.0f, 1.0f, 1.0f };
}

Sprite::Sprite(const Sprite& spr) : BaseObject(spr),
  img_(nullptr), sprani_(spr.sprani_), time_(0), frame_(0), res_id_(spr.res_id_),
  blending_(spr.blending_), texcoord_(spr.texcoord_), use_texture_coord_(spr.use_texture_coord_),
  tex_attribute_(spr.tex_attribute_)
{
  if (spr.img_)
    img_ = (Image*)spr.img_->clone();
}

Sprite::~Sprite()
{
  if (img_)
    IMAGEMAN->Unload(img_);
}

BaseObject *Sprite::clone()
{
  return new Sprite(*this);
}

void Sprite::Load(const MetricGroup& metric)
{
  BaseObject::Load(metric);

  metric.get_safe("blend", blending_);

  if (metric.exist("path"))
    SetImage(metric.get_str("path"));
  else if (metric.exist("src"))
    SetImage(metric.get_str("src"));

  if (metric.exist("crop"))
  {
    CommandArgs args(metric.get_str("crop"), 4, true);
    SetImageCoord(Vector4{ args.Get<int>(0), args.Get<int>(1),
      args.Get<int>(2), args.Get<int>(3) });
  }
  else if (metric.exist("croptex"))
  {
    CommandArgs args(metric.get_str("croptex"), 4, true);
    SetTextureCoord(Vector4{ args.Get<int>(0), args.Get<int>(1),
      args.Get<int>(2), args.Get<int>(3) });
  }

#if USE_LR2_FEATURE == 1
  if (metric.exist("lr2src"))
  {
    std::string lr2src;
    std::string lr2dst;
    CommandArgs cmds, params;
    metric.get_safe("lr2src", lr2src);
    LoadLR2SRC(lr2src);

    metric.get_safe("lr2dst", lr2dst);
    cmds.set_separator('|');
    cmds.Parse(lr2dst);
    params.set_separator(',');
    params.Parse(cmds.Get_str(0), 20, true);

    // TODO: load resource id
    blending_ = params.Get<int>(11);
  }
#endif
}

void Sprite::LoadLR2SRC(const std::string &lr2src)
{
  /* (null),imgname,sx,sy,sw,sh,divx,divy,cycle,timer */
  CommandArgs args(lr2src, 10, true);

  SetImage("image" + args.Get<std::string>(1));

  Vector4 r{
    args.Get<int>(2), args.Get<int>(3), args.Get<int>(4), args.Get<int>(5)
  };
  // Use whole image if width/height is zero.
  if (r.z <= 0 || r.w <= 0)
    SetTextureCoord(Vector4{ 0.0f, 0.0f, 1.0f, 1.0f });
  else
    SetImageCoord(Vector4{ r.x, r.y, r.x + r.z, r.y + r.w });

  int divx = args.Get<int>(6);
  int divy = args.Get<int>(7);

  sprani_.divx = divx > 0 ? divx : 1;
  sprani_.divy = divy > 0 ? divy : 1;
  sprani_.cnt = sprani_.divx *  sprani_.divy;
  sprani_.duration = args.Get<int>(8);
}

void Sprite::OnReady()
{
  // TODO: use texture coord later
}

void Sprite::SetImage(const std::string &path)
{
  if (img_)
  {
    IMAGEMAN->Unload(img_);
    img_ = nullptr;
  }

  img_ = IMAGEMAN->Load(path);
  if (!img_)
    return;

  /* default image texture coord set */
  use_texture_coord_ = true;
  texcoord_ = Rect{ 0.0f, 0.0f, 1.0f, 1.0f };
}

Image *Sprite::image() { return img_; }

void Sprite::SetBlending(int blend)
{
  blending_ = blend;
}

void Sprite::SetImageCoord(const Rect &r)
{
  texcoord_ = r;
  use_texture_coord_ = false;
}

void Sprite::SetTextureCoord(const Rect &r)
{
  texcoord_ = r;
  use_texture_coord_ = true;
}

void Sprite::Replay()
{
  time_ = 0;
  frame_ = 0;
}

const SpriteAnimationInfo&
Sprite::GetSpriteAnimationInfo() { return sprani_; }

void Sprite::SetNumber(int number)
{
  if (sprani_.duration <= 0)
  {
    number = std::min(std::max(number, 0), sprani_.cnt - 1);
    frame_ = number;
  }
}

void Sprite::Refresh()
{
  if (res_id_)
    SetNumber(*res_id_);
}

// milisecond
void Sprite::doUpdate(double delta)
{
  // update sprite frame
  if (sprani_.duration > 0)
  {
    time_ += delta;
    frame_ = (int)(time_ * sprani_.divx * sprani_.divy / sprani_.duration)
      % sprani_.cnt;
    time_ = fmod(time_, (double)sprani_.duration);
  }
}

void Sprite::doRender()
{
  VertexInfo vi[4];
  Vector4 texcrop;
  Point imgsize;

  // If not loaded or hide, then not draw
  if (!img_ || !img_->is_loaded() || !IsVisible())
    return;

  // calculate texture crop area
  imgsize = Point{ (float)img_->get_width(), (float)img_->get_height() };
  if (use_texture_coord_)
    texcrop = texcoord_;
  else
  {
    texcrop.x = texcoord_.x / imgsize.x;
    texcrop.y = texcoord_.y / imgsize.y;
    texcrop.z = texcoord_.z / imgsize.x;
    texcrop.w = texcoord_.w / imgsize.y;
  }

  // texture animation
  if (sprani_.divx > 1 || sprani_.divy > 1)
  {
    float w = GetWidth(texcrop) / sprani_.divx;
    float h = GetHeight(texcrop) / sprani_.divy;
    int ix = frame_ % sprani_.divx;
    int iy = frame_ / sprani_.divx % sprani_.divy;
    texcrop.x += w * ix;
    texcrop.y += h * iy;
    texcrop.z = texcrop.x + w;
    texcrop.w = texcrop.y + h;
  }

  BaseObject::FillVertexInfo(vi);
  vi[0].t = Point{ texcrop.x, texcrop.y };
  vi[1].t = Point{ texcrop.z, texcrop.y };
  vi[2].t = Point{ texcrop.z, texcrop.w };
  vi[3].t = Point{ texcrop.x, texcrop.w };

  GRAPHIC->SetTexture(0, img_->get_texture_ID());
  GRAPHIC->SetBlendMode(blending_);
  GRAPHIC->DrawQuad(vi);
}

const char* Sprite::type() const { return "sprite"; }

std::string Sprite::toString() const
{
  std::stringstream ss;
  if (img_)
  {
    ss << "file: " << img_->get_path() << "," << img_->get_width() << "," << img_->get_height() << std::endl;
  }
  else {
    ss << "file is empty." << std::endl;
  }
  return BaseObject::toString() + ss.str();
}

}
