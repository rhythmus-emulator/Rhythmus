#include "Text.h"
#include "ResourceManager.h"
#include "Script.h"
#include "KeyPool.h"
#include "common.h"
#include "config.h"

RHYTHMUS_NAMESPACE_BEGIN

// --------------------------------- class Text

Text::Text()
  : font_(nullptr),
    text_fitting_(TextFitting::kTextFitNone), set_xy_aligncenter_(false), use_height_as_font_height_(false),
    width_multiply_(1.0f), blending_(0), counter_(0), is_texture_loaded_(true),
    res_id_(nullptr), do_line_breaking_(true)
{
  alignment_attrs_.sx = alignment_attrs_.sy = 1.0f;
  alignment_attrs_.tx = alignment_attrs_.ty = .0f;
  text_render_ctx_.width = text_render_ctx_.height = .0f;
  text_alignment_ = Vector2(0.0f, 0.0f);  // TOPLEFT
}

Text::~Text()
{
  ClearFont();
}

void Text::Load(const MetricGroup &m)
{
  BaseObject::Load(m);

  if (m.exist("path"))
    SetFont(m);
  if (m.exist("text"))
    SetText(m.get_str("text"));

#if USE_LR2_FEATURE == 1
  if (m.exist("lr2src"))
  {
    std::string lr2src;
    m.get_safe("lr2src", lr2src);
    CommandArgs args(lr2src);

    /* fetch font size from lr2dst ... */
    SetFont(args.Get<std::string>(0));

    /* track change of text table */
    std::string eventname = "Text" + args.Get<std::string>(1);
    AddCommand(eventname, "refresh");
    SubscribeTo(eventname);
    std::string resname = "S" + args.Get<std::string>(1);
    KeyData<std::string> kdata = KEYPOOL->GetString(resname);
    res_id_ = &*kdata;
    Refresh();

    /* alignment */
    const int lr2align = args.Get<int>(2);
    switch (lr2align)
    {
    case 0:
      // topleft
      SetTextFitting(TextFitting::kTextFitMaxSize);
      GetCurrentFrame().align.x = 0.0;
      GetCurrentFrame().align.y = 0.0;
      break;
    case 1:
      // topcenter
      SetTextFitting(TextFitting::kTextFitMaxSize);
      GetCurrentFrame().align.x = 0.5;
      GetCurrentFrame().align.y = 0.0;
      break;
    case 2:
      // topright
      SetTextFitting(TextFitting::kTextFitMaxSize);
      GetCurrentFrame().align.x = 1.0;
      GetCurrentFrame().align.y = 0.0;
      break;
    }
    set_xy_aligncenter_ = true;
    use_height_as_font_height_ = true;

    /* editable */
    //args.Get<int>(2);
  }

  // TODO: load blending from LR2DST
#endif
}

void Text::SetFont(const std::string& path)
{
  ClearFont();
  font_ = FONTMAN->Load(path);

  /* if text previously exists, call SetText() internally */
  if (!text_.empty())
    SetText(text_);
}

void Text::SetFont(const MetricGroup &m)
{
  ClearFont();
  font_ = FONTMAN->Load(m);

  /* if text previously exists, call SetText() internally */
  if (!text_.empty())
    SetText(text_);
}

/* @DEPRECIATED */
void Text::SetSystemFont()
{
  MetricGroup sysfont;
  sysfont.set("path", "system/default.ttf");
  sysfont.set("size", 16);
  sysfont.set("color", "#FFFFFFFF");
  sysfont.set("border-size", 1);
  sysfont.set("border-color", "#FF000000");
  SetFont(sysfont);
}

void Text::ClearFont()
{
  if (font_)
  {
    FONTMAN->Unload(font_);
    font_ = nullptr;
  }
}

float Text::GetTextWidth() const
{
  return text_render_ctx_.width;
}

void Text::SetText(const std::string& s)
{
  if (!font_) return;

  text_ = s;
  font_->PrepareText(s);
  UpdateTextRenderContext();
}

void Text::ClearText()
{
  text_render_ctx_.textvertex.clear();
  text_render_ctx_.vi.clear();
  text_render_ctx_.height = text_render_ctx_.width = 0;
  text_.clear();
}

void Text::UpdateTextRenderContext()
{
  text_render_ctx_.textvertex.clear();
  text_render_ctx_.vi.clear();

  if (!font_ || text_.empty())
  {
    // don't attempt again.
    is_texture_loaded_ = true;
    return;
  }

  is_texture_loaded_ = font_ && font_->is_loaded();
  if (!is_texture_loaded_) return;

  font_->GetTextVertexInfo(text_, text_render_ctx_.textvertex, do_line_breaking_);
  text_render_ctx_.width = 0;
  text_render_ctx_.height = 0;
  text_render_ctx_.drawsize = Vector2(
    GetWidth(GetCurrentFrame().pos),
    GetHeight(GetCurrentFrame().pos)
  );

  // 1. calculate whole text width and height.
  // XXX: breaking loop if first cycle is over should be better?
  for (const auto& tvi : text_render_ctx_.textvertex)
  {
    text_render_ctx_.width = std::max(text_render_ctx_.width, (float)tvi.vi[2].p.x);
    text_render_ctx_.height = std::max(text_render_ctx_.height, (float)tvi.vi[2].p.y);
  }

  R_ASSERT(text_render_ctx_.width != 0 && text_render_ctx_.height != 0);

  // 2. resize textsize area (if necessary)
  if (use_height_as_font_height_)
  {
    text_render_ctx_.height = (float)font_->GetAttribute().height;
  }

  Vector3 scale(1.0f, 1.0f, 1.0f);
  Vector3 centerpos(text_render_ctx_.width / 2.0f, text_render_ctx_.height / 2.0f, 0);
  switch (text_fitting_)
  {
  case TextFitting::kTextFitMaxSize:
    if (text_render_ctx_.drawsize.x != 0)
      scale.x = std::min(1.0f, text_render_ctx_.drawsize.x / text_render_ctx_.width);
    if (text_render_ctx_.drawsize.y != 0)
      scale.y = std::min(1.0f, text_render_ctx_.drawsize.y / text_render_ctx_.height);
    break;
  case TextFitting::kTextFitStretch:
    if (text_render_ctx_.drawsize.x != 0)
      scale.x = text_render_ctx_.drawsize.x / text_render_ctx_.width;
    if (text_render_ctx_.drawsize.y != 0)
      scale.y = text_render_ctx_.drawsize.y / text_render_ctx_.height;
    break;
  case TextFitting::kTextFitNone:
  default:
    text_render_ctx_.drawsize.x = text_render_ctx_.width;
    text_render_ctx_.drawsize.y = text_render_ctx_.height;
    break;
  }

  // 3. move text vertices (alignment)
  // consider newline if x == 0.
  unsigned newline_idx = 0;
  if (text_alignment_.x != 0)
  {
    for (unsigned i = 0; i < text_render_ctx_.textvertex.size(); ++i)
    {
      bool end_of_line = false;
      if (i == text_render_ctx_.textvertex.size() - 1
        || text_render_ctx_.textvertex[i + 1].vi[0].p.x == 0)
        end_of_line = true;
      if (end_of_line)
      {
        float m =
          (text_render_ctx_.width - text_render_ctx_.textvertex[i].vi[1].p.x)
          * text_alignment_.x;
        for (unsigned j = newline_idx; j <= i; ++j)
        {
          auto &tvi = text_render_ctx_.textvertex[j];
          for (unsigned k = 0; k < 4; ++k)
            tvi.vi[k].p.x += m;
        }
        newline_idx = i + 1;
      }
    }
  }
  if (text_alignment_.y != 0)
  {
    float m =
      (text_render_ctx_.height- text_render_ctx_.textvertex.back().vi[2].p.y)
      * text_alignment_.y;
    for (auto& tvi : text_render_ctx_.textvertex)
    {
      for (unsigned k = 0; k < 4; ++k)
        tvi.vi[k].p.y += m;
    }
  }

  // 4. move (centering) and resize text vertices.
  for (auto& tvi : text_render_ctx_.textvertex)
  {
    for (unsigned i = 0; i < 4; ++i)
    {
      tvi.vi[i].p -= centerpos;
      tvi.vi[i].p *= scale;
      text_render_ctx_.vi.push_back(tvi.vi[i]);
    }
  }
}

TextVertexInfo& Text::AddTextVertex(const TextVertexInfo &tvi)
{
  text_render_ctx_.textvertex.push_back(tvi);
  return text_render_ctx_.textvertex.back();
}

void Text::SetWidthMultiply(float multiply)
{
  width_multiply_ = multiply;
}

void Text::Refresh()
{
  if (res_id_)
    SetText(*res_id_);
}

void Text::SetTextFitting(TextFitting fitting)
{
  text_fitting_ = fitting;
}

void Text::SetLineBreaking(bool enable_line_break)
{
  do_line_breaking_ = enable_line_break;
}

Font *Text::font() { return font_; }

void Text::doUpdate(double delta)
{
  counter_ = (counter_ + 1) % 30;

  // attempt to reload font texture periodically if not completely loaded
  if (counter_ == 0 && !is_texture_loaded_ && font_)
  {
    UpdateTextRenderContext();
  }
}

void Text::doRender()
{
  GRAPHIC->PushMatrix();
  GRAPHIC->SetBlendMode(blending_);

  // Draw vertex by given quad.
  float alpha = GetCurrentFrame().color.a;
  unsigned last_i = 0, last_texid = 0;
  for (unsigned i = 0; i < text_render_ctx_.textvertex.size(); ++i)
  {
    auto &tvi = text_render_ctx_.textvertex[i];
    text_render_ctx_.vi[i * 4 + 0].c.a = alpha;
    text_render_ctx_.vi[i * 4 + 1].c.a = alpha;
    text_render_ctx_.vi[i * 4 + 2].c.a = alpha;
    text_render_ctx_.vi[i * 4 + 3].c.a = alpha;
    if (last_texid != tvi.texid || i == text_render_ctx_.textvertex.size() - 1)
    {
      if (i > 0)
      {
        GRAPHIC->SetTexture(0, last_texid);
        GRAPHIC->DrawQuads(&text_render_ctx_.vi[last_i * 4], (i - last_i) * 4);
      }
      last_texid = tvi.texid;
      last_i = i;
    }
  }

  GRAPHIC->PopMatrix();

  /* TESTCODE for rendering whole glyph texture */
#if 0
  if (text_render_ctx_.textvertex.empty())
    return;
  Graphic::SetTextureId(text_render_ctx_.textvertex[0].texid);
  VertexInfo *vi = Graphic::get_vertex_buffer();
  vi[0].x = 10;
  vi[0].y = 10;
  vi[0].sx = .0f;
  vi[0].sy = .0f;

  vi[1].x = 200;
  vi[1].y = 10;
  vi[1].sx = 1.0f;
  vi[1].sy = .0f;

  vi[2].x = 200;
  vi[2].y = 200;
  vi[2].sx = 1.0f;
  vi[2].sy = 1.0f;

  vi[3].x = 10;
  vi[3].y = 200;
  vi[3].sx = .0f;
  vi[3].sy = 1.0f;

  vi[0].r = vi[0].g = vi[0].b = vi[0].a = 1.0f;
  vi[1].r = vi[1].g = vi[1].b = vi[1].a = 1.0f;
  vi[2].r = vi[2].g = vi[2].b = vi[2].a = 1.0f;
  vi[3].r = vi[3].g = vi[3].b = vi[3].a = 1.0f;

  Graphic::RenderQuad();
#endif
}

void Text::UpdateRenderingSize(Vector2 &d, Vector3 &p)
{
  // calculate scale and apply
  // and transition in case of width is smaller than text size
  const float width = GetWidth(GetCurrentFrame().pos);
  const float height = GetHeight(GetCurrentFrame().pos);
  if (text_render_ctx_.drawsize.x > width)
    p.x = (text_render_ctx_.drawsize.x - width) * (0.5f - text_alignment_.x);
  if (text_render_ctx_.drawsize.y > height)
    p.y = (text_render_ctx_.drawsize.y - height) * (0.5f - text_alignment_.y);

  // -- don't consider about scale here
  // as width/height purposes for alignment, not size of text.

  // simulate align center as topleft by making drawing size as zero
  if (set_xy_aligncenter_)
  {
    d.x = d.y = 0.0f;
  }
}

RHYTHMUS_NAMESPACE_END
