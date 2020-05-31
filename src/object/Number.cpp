#include "Number.h"
#include "Script.h"
#include "Util.h"
#include "config.h"
#include <string.h>
#include <algorithm>

namespace rhythmus
{

// ------------------------------- class Number

Number::Number() :
  img_(nullptr), font_(nullptr), blending_(0), tvi_glyphs_(nullptr),
  cycle_count_(0), cycle_time_(0), cycle_curr_time_(0), val_ptr_(nullptr), keta_(1)
{
  memset(&value_params_, 0, sizeof(value_params_));
  *num_chrs = 0;
  UpdateVertex();
}

Number::~Number()
{
  ClearAll();
}

void Number::Load(const MetricGroup& metric)
{
  BaseObject::Load(metric);

  // use font if exists
  // XXX: need to use 'path' attribute?
  if (metric.exist("font"))
    SetGlyphFromFont(metric);

  if (metric.exist("lr2src"))
  {
    SetGlyphFromLR2SRC(metric.get_str("lr2src"));
    if (metric.exist("lr2dst"))
    {
      std::string lr2dst = metric.get_str("lr2dst");
      CommandArgs args(lr2dst.substr(0, lr2dst.find('|')));
      blending_ = args.Get<int>(11);
    }
  }

  if (metric.exist("value"))
    SetNumber(metric.get<int>("value"));
}

void Number::SetGlyphFromFont(const MetricGroup &m)
{
  ClearAll();
  font_ = FONTMAN->Load(m);
  if (!font_)
    return;
  SleepUntilLoadFinish(font_);
  AllocNumberGlyph(1);

  std::vector<TextVertexInfo> textvertex;
  font_->GetTextVertexInfo("0123456789 +0123456789 -", textvertex, false);
  for (unsigned i = 0; i < 24; ++i)
    tvi_glyphs_[i] = textvertex[i];
}

void Number::SetGlyphFromLR2SRC(const std::string &lr2src)
{
#if USE_LR2_FEATURE == 1
  // (null),(image),(x),(y),(w),(h),(divx),(divy),(cycle),(timer),(num),(align),(keta)
  CommandArgs args(lr2src);

  ClearAll();
  img_ = IMAGEMAN->Load(args.Get_str(1));
  if (!img_)
    return;
  SleepUntilLoadFinish(img_);

  /* add glyphs */
  int divx = std::max(1, args.Get<int>(6));
  int divy = std::max(1, args.Get<int>(7));
  Vector4 imgcoord{
    args.Get<int>(2), args.Get<int>(3), args.Get<int>(4), args.Get<int>(5)
  };
  Vector2 gsize{ imgcoord.z / divx, imgcoord.w / divy };
  Vector2 texsize{ (float)img_->get_width(), (float)img_->get_height() };
  unsigned cnt = (unsigned)(divx * divy);
  unsigned multiply_mode = 10;
  if (cnt % 24 == 0) multiply_mode = 24;
  else if (cnt % 11 == 0) multiply_mode = 11;
  else if (cnt % 10 == 0) multiply_mode = 10;
  AllocNumberGlyph(cnt);
  for (unsigned j = 0; j < (unsigned)divy; ++j)
  {
    for (unsigned i = 0; i < (unsigned)divx; ++i)
    {
      unsigned idx = j * divx + i;
      auto &tvi = tvi_glyphs_[idx / multiply_mode * 24 + idx % multiply_mode];
      tvi.vi[0].t = Vector2(imgcoord.x + gsize.x * i, imgcoord.y + gsize.y * j);
      tvi.vi[1].t = Vector2(imgcoord.x + gsize.x * (i+1), imgcoord.y + gsize.y * j);
      tvi.vi[2].t = Vector2(imgcoord.x + gsize.x * (i+1), imgcoord.y + gsize.y * (j+1));
      tvi.vi[3].t = Vector2(imgcoord.x + gsize.x * i, imgcoord.y + gsize.y * (j+1));
      tvi.vi[0].t /= texsize;
      tvi.vi[1].t /= texsize;
      tvi.vi[2].t /= texsize;
      tvi.vi[3].t /= texsize;
      tvi.vi[0].c = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
      tvi.vi[1].c = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
      tvi.vi[2].c = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
      tvi.vi[3].c = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
      tvi.vi[0].p = Vector3(0.0f, 0.0f, 0.0f);
      tvi.vi[1].p = Vector3(gsize.x, 0.0f, 0.0f);
      tvi.vi[2].p = Vector3(gsize.x, gsize.y, 0.0f);
      tvi.vi[3].p = Vector3(0.0f, gsize.y, 0.0f);
      tvi.tex = img_->get_texture();
    }
  }

  // fill empty glyphs
  if (multiply_mode == 10)
  {
    for (int i = 0; i < cnt / multiply_mode; ++i)
    {
      auto &tvi = tvi_glyphs_[i * 24 + 10];
      memcpy(&tvi, &tvi_glyphs_[i * 24], sizeof(TextVertexInfo));
      tvi.tex = nullptr;
      memset(&tvi_glyphs_[i * 24 + 11], 0, sizeof(TextVertexInfo));
      for (int j = 12; j < 24; ++j)
      {
        memcpy(&tvi_glyphs_[i * 24 + 12 + j], &tvi_glyphs_[i * 24 + j],
          sizeof(TextVertexInfo));
      }
    }
  }
  else if (multiply_mode == 11)
  {
    for (int i = 0; i < cnt / multiply_mode; ++i)
    {
      memset(&tvi_glyphs_[i * 24 + 11], 0, sizeof(TextVertexInfo));
      for (int j = 12; j < 24; ++j)
      {
        memcpy(&tvi_glyphs_[i * 24 + 12 + j], &tvi_glyphs_[i * 24 + j],
          sizeof(TextVertexInfo));
      }
    }
  }

  cycle_count_ = cnt / multiply_mode;
  cycle_time_ = std::max(1, args.Get<int>(8));

  /* track change of number table */
  int eventid = args.Get<int>(9);
  std::string eventname = "Number" + args.Get<std::string>(9);
  AddCommand(eventname, "refresh");
  SubscribeTo(eventname);

  /* alignment (not use LR2 alignment here) */
  switch (args.Get<int>(11))
  {
  case 0:
    GetCurrentFrame().align.x = 0.0f;
    value_params_.fill_empty_zero = 1;
    break;
  case 1:
    GetCurrentFrame().align.x = 1.0f;
    value_params_.fill_empty_zero = 3;
    break;
  case 2:
    // XXX: different from LR2 if keta enabled
    GetCurrentFrame().align.x = 0.5f;
    value_params_.fill_empty_zero = 2;
    break;
  }

  /* keta processing: tween width multiply & set number formatter */
  int keta = args.Get<int>(12);
  keta_ = keta;
  value_params_.max_string = keta;
  value_params_.max_decimal = 0;

  /* set value instantly: TODO */
  //SetResourceId(eventid);
  //Refresh();
#endif
}

void Number::SetNumber(int number)
{
  for (int i = 0; i < value_params_.max_decimal; ++i)
    number *= 10;
  SetNumberInternal(number);
}

void Number::SetNumber(double number)
{
  for (int i = 0; i < value_params_.max_decimal; ++i)
    number *= 10;
  SetNumberInternal((int)number);
}

void Number::SetNumberInternal(int number)
{
  value_params_.start = value_params_.curr;
  value_params_.end = (int)number;
  if (value_params_.rollingtime == 0)
  {
    value_params_.curr = value_params_.end;
    UpdateNumberStr();
    UpdateVertex();
  }
}

void Number::SetText(const std::string &num)
{
  strcpy(num_chrs, num.c_str());
  UpdateVertex();
}

void Number::Refresh()
{
  /* kind of trick to compatible with LR2:
   * if value is UINT_MAX, then set with empty value. */
  if (val_ptr_)
  {
    if (*val_ptr_ == 0xFFFFFFFF)
      SetText(std::string());
    else
      SetNumber(*val_ptr_);
  }
}

void Number::ClearAll()
{
  AllocNumberGlyph(0);

  if (font_)
  {
    FONTMAN->Unload(font_);
    font_ = nullptr;
  }

  if (img_)
  {
    IMAGEMAN->Unload(img_);
    img_ = nullptr;
  }
}

void Number::AllocNumberGlyph(unsigned cycles)
{
  if (tvi_glyphs_)
  {
    free(tvi_glyphs_);
    tvi_glyphs_ = nullptr;
  }
  if (cycles == 0)
    return;
  tvi_glyphs_ = (TextVertexInfo*)calloc(24 * cycles, sizeof(TextVertexInfo));
  cycle_count_ = cycles;
}

void Number::UpdateNumberStr()
{
  // old code
  //char *p = itoa(value_params_.curr, num_chrs, 10);

  int len = 0;                    // actual length of number string
  int format_len = 0;             // formatted length of number string
  int val = value_params_.curr;
  char *sp = num_chrs;

  // check string size
  {
    int v = value_params_.curr;
    while (v > 0)
    {
      len++;
      v /= 10;
    }
    if (value_params_.max_decimal > 0)
      len += 1; /* dot */
    if (len > value_params_.max_string)
      len = value_params_.max_string;
    format_len = len;
  }

  // check fill_empty_zero
  if (value_params_.fill_empty_zero != 0)
  {
    format_len = value_params_.max_string;
    if (value_params_.max_decimal > 0)
      format_len += 1 /* dot */;
    if (len > format_len) len = format_len;
    switch (value_params_.fill_empty_zero)
    {
    case 1:
      sp = num_chrs;
      break;
    case 2:
      sp = num_chrs + (format_len - len) / 2;
      break;
    case 3:
      sp = num_chrs + (format_len - len);
      break;
    }
    for (char *p = num_chrs; p != num_chrs + format_len; ++p)
      *p = ' ';
  }
  num_chrs[format_len] = 0;

  // fill characters from decimal
  if (value_params_.max_decimal > 0)
  {
    for (int i = 0; i < value_params_.max_decimal; ++i)
    {
      sp[len - i - 1] = '0' + (val % 10);
      val /= 10;
    }
    sp[len - value_params_.max_decimal - 1] = '.';
  }
  for (int i = len - value_params_.max_decimal - 1; i >= 0; --i)
  {
    sp[i] = '0' + (val % 10);
    val /= 10;
  }
}

void Number::UpdateVertex()
{
  text_width_ = 0;
  text_height_ = 0;
  render_glyphs_count_ = 0;
  unsigned cycle_idx = (unsigned)(cycle_count_ * cycle_curr_time_ / cycle_time_);

  if (tvi_glyphs_ == nullptr)
    return;

  // copy glyphs to print
  float left = 0;
  while (num_chrs[render_glyphs_count_] > 0)
  {
    unsigned chridx = num_chrs[render_glyphs_count_];
    if (chridx >= '0' && chridx <= '9')
    {
      chridx -= '0';
      if (value_params_.curr < 0)
        chridx += 12;
    }
    else if (chridx == '+') chridx = 11;
    else if (chridx == '-') chridx = 23;
    else /*if (chridx == ' ') */ chridx = 10;
    chridx += cycle_idx * 24;
    for (unsigned i = 0; i < 4; ++i)
      render_glyphs_[render_glyphs_count_][i] = tvi_glyphs_[chridx].vi[i];
    for (unsigned i = 0; i < 4; ++i)
      render_glyphs_[render_glyphs_count_][i].p.x += left;
    left += tvi_glyphs_[chridx].vi[2].p.x;
    tex_[render_glyphs_count_] = tvi_glyphs_[chridx].tex;
    ++render_glyphs_count_;
  }
  text_width_ = left;
  text_height_ = render_glyphs_[0][2].p.y;

  // glyph vertex to centered
  for (unsigned i = 0; i < render_glyphs_count_; ++i)
  {
    for (unsigned j = 0; j < 4; ++j)
    {
      render_glyphs_[i][j].p.x -= text_width_ / 2;
      render_glyphs_[i][j].p.y -= text_height_ / 2;
    }
  }
}

void Number::doUpdate(double delta)
{
  bool updated = false;

  // update current number (rolling effect)
  if (value_params_.time > 0)
  {
    value_params_.time -= delta;
    if (value_params_.time < 0) value_params_.time = 0;
    UpdateNumberStr();
    UpdateVertex();
    updated = true;
  }

  // update cycle
  if (!updated && cycle_time_ > 0)
  {
    cycle_curr_time_ = fmod(cycle_curr_time_ + delta, (double)cycle_time_);
    UpdateVertex();
    updated = true;
  }

  // update alpha
  for (unsigned k = 0; k < render_glyphs_count_; ++k)
  {
    render_glyphs_[k][0].c.a =
      render_glyphs_[k][1].c.a =
      render_glyphs_[k][2].c.a =
      render_glyphs_[k][3].c.a = GetCurrentFrame().color.a;
  }
}

void Number::doRender()
{
  GRAPHIC->SetBlendMode(blending_);

  // Set scale before rendering to fit textbox
  Vector3 scale(
    GetWidth(GetCurrentFrame().pos) * keta_ / text_width_,
    GetHeight(GetCurrentFrame().pos) / text_height_,
    1.0f);
  if (scale.x != 1.0f || scale.y != 1.0f)
    GRAPHIC->Scale(scale);

  // optimizing: flush glyph with same texture at once
  for (unsigned i = 0; i < render_glyphs_count_;)
  {
    unsigned j = i + 1;
    for (; j < render_glyphs_count_; ++j)
      if (tex_[i] != tex_[j]) break;
    if (tex_[i])
    {
      GRAPHIC->SetTexture(0, **tex_[i]);
      GRAPHIC->DrawQuads(render_glyphs_[i], (j - i) * 4);
    }
    i = j;
  }
}

void Number::UpdateRenderingSize(Vector2 &d, Vector3 &p)
{
  d.x *= keta_;
}

}
