#include "Font.h"
#include "SceneManager.h"
#include "Setting.h"
#include "Image.h"
#include "Util.h"
#include "Logger.h"
#include "rparser.h"  /* rutil Text encoding to UTF32 */
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_STROKER_H
#include "common.h"
#include "config.h"

#if USE_LR2_FONT == 1
#include "LR2/exdxa.h"
#include "LR2/LR2JIS.h"
#endif

namespace rhythmus
{

/* Global FreeType font context. */
FT_Library ftLib;

/* How many font context exists? */
int ftLibCnt;

constexpr int defFontCacheWidth = 2048;
constexpr int defFontCacheHeight = 2048;

inline uint32_t HexToUint(const char *hex)
{
  if (hex[0] == '0' && hex[1] == 'x')
    hex += 2;
  return (uint32_t)strtoll(hex, NULL, 16);
}


void ClearFontAttribute(FontAttribute &fontattr)
{
  fontattr.name.clear();
  fontattr.height = 10;
  fontattr.baseline_offset = 0;
  fontattr.color = 0xFF000000;
  fontattr.outline_width = 0;
  fontattr.outline_color = 0x00000000;
  memset(&fontattr.tex, 0, sizeof(FontFillBitmap));
  memset(&fontattr.outline_tex, 0, sizeof(FontFillBitmap));
}

void SetFontAttributeFromCommand(FontAttribute &fontattr, const std::string &command)
{
  std::vector<std::string> params;
  std::string a, b;
  Split(command, ';', params);
  for (const auto &s : params)
  {
    Split(s, ':', a, b);
    CommandArgs args(b);
    if (a == "size")
    {
      fontattr.height = args.Get<int>(0) * 4;
    }
    else if (a == "color")
    {
      fontattr.color = HexToUint(args.Get<std::string>(0).c_str());
    }
    else if (a == "border")
    {
      fontattr.outline_width = args.Get<int>(0);
      fontattr.outline_color = HexToUint(args.Get<std::string>(1).c_str());
    }
    else if (a == "texture")
    {
      // TODO
    }
    else if (a == "bordertexture")
    {
      // TODO
    }
  }
}

// --------------------------- class FontBitmap

FontBitmap::FontBitmap(int w, int h)
  : bitmap_(0), error_code_(0), error_msg_(0)
{
  width_ = w;
  height_ = h;
  cur_x_ = cur_y_ = 0;
  cur_line_height_ = 0;

  bitmap_ = (uint32_t*)malloc(width_ * height_ * sizeof(uint32_t));
  R_ASSERT(bitmap_);
  memset(bitmap_, 0, width_ * height_ * sizeof(uint32_t));
}

// bitmap is moved into memory.
FontBitmap::FontBitmap(uint32_t* bitmap, int w, int h)
  : bitmap_(bitmap), width_(w), height_(h), cur_x_(0), cur_y_(0),
    cur_line_height_(0), error_code_(0), error_msg_(0)
{
}

// bitmap is copied into memory.
FontBitmap::FontBitmap(const uint32_t* bitmap, int w, int h)
  : bitmap_(0), width_(w), height_(h), cur_x_(0), cur_y_(0),
    cur_line_height_(0), error_code_(0), error_msg_(0)
{
  bitmap_ = (uint32_t*)malloc(width_ * height_ * sizeof(uint32_t));
  memcpy(bitmap_, bitmap, width_ * height_ * sizeof(uint32_t));
}

FontBitmap::~FontBitmap()
{
  if (*texture_)
  {
    GRAPHIC->DeleteTexture(*texture_);
    texture_.set(0);
  }

  if (bitmap_)
  {
    free(bitmap_);
    bitmap_ = 0;
  }
}

void FontBitmap::Write(uint32_t* bitmap, int w, int h, FontGlyph &glyph_out)
{
  // writable?
  if (!IsWritable(w, h)) return;

  // need to break line?
  if (cur_x_ + w > width_)
  {
    cur_y_ += cur_line_height_ + 1 /* padding */;
    cur_x_ = 0;
    cur_line_height_ = 0;
  }

  // update new max height
  if (cur_line_height_ < h) cur_line_height_ = h;

  // write bitmap to destination
  for (int x = 0; x < w; ++x)
    for (int y = 0; y < h; ++y)
      bitmap_[(y + cur_y_)*width_ + x + cur_x_] = bitmap[w*y + x];

  // set texture pos before updating x pos
  GetGlyphTexturePos(glyph_out);

  // update x pos
  cur_x_ += w + 1 /* padding */;
}

/* Get designated area(current x, y with given width / height) as texture position. */
void FontBitmap::GetGlyphTexturePos(FontGlyph &glyph_out)
{
  const int w = glyph_out.width;
  const int h = glyph_out.height;
  glyph_out.sx1 = (float)cur_x_ / width_;
  glyph_out.sx2 = (float)(cur_x_ + w) / width_;
  glyph_out.sy1 = (float)cur_y_ / height_;
  glyph_out.sy2 = (float)(cur_y_ + h) / height_;
}

bool FontBitmap::Update()
{
  if (!bitmap_) return false;

  // commit bitmap
  if (*texture_ == 0)
    texture_.set(GRAPHIC->CreateTexture((uint8_t*)bitmap_, width_, height_));
  else
    GRAPHIC->UpdateTexture(*texture_, (uint8_t*)bitmap_, 0, 0, width_, height_);

  if (*texture_ == 0)
  {
    Logger::Error("Font - Allocating texture failed.");
    return false;
  }

  return true;
}

bool FontBitmap::IsWritable(int w, int h) const
{
  if (!bitmap_) return false;
  /* if x is already bigger then width,
   * and next line height also overflows,
   * then cannot write to this bitmap. */
  if (cur_x_ + w >= width_ && cur_y_ + cur_line_height_ + h >= height_) return false;
  else return true;
}

int FontBitmap::width() const { return width_; }
int FontBitmap::height() const { return height_; }

unsigned FontBitmap::get_texid() const
{
  return *texture_;
}

const Texture* FontBitmap::get_texture() const
{
  return &texture_;
}

int FontBitmap::get_error_code() const { return error_code_; }
const char *FontBitmap::get_error_msg() const { return error_msg_; }

void FontBitmap::SetToReadOnly()
{
  // update before remove cache
  Update();

  if (bitmap_)
  {
    free(bitmap_);
    bitmap_ = 0;
  }
}



// --------------------------------- class Font

Font::Font()
  : is_ttf_font_(false), ftface_count_(0), ftstroker_(0),
    error_code_(0), error_msg_(0)
{
  memset(&ftface_, 0, sizeof(ftface_));
  if (ftLibCnt++ == 0)
  {
    if (FT_Init_FreeType(&ftLib))
    {
      std::cerr << "ERROR: Could not init FreeType Library." << std::endl;
      ftLibCnt = 0;
      return;
    }
  }

  ClearFontAttribute(fontattr_);
}

Font::~Font()
{
  ClearGlyph();
  ReleaseFont();

  // Freetype done is only called when Font object releasing.
  if (--ftLibCnt == 0)
    FT_Done_FreeType(ftLib);
}

void Font::Load(const std::string &path)
{
  Clear();
  error_msg_ = 0;
  error_code_ = 0;

  std::string ext = GetExtension(path);
  path_ = path;
  if (ext == "ttf" || ext == "ttc" || ext == "otf" || ext == "woff")
  {
    LoadFreetypeFont(path);
  }
  else if (ext == "dxa")
  {
    LoadLR2BitmapFont(path);
  }
  else if (ext == "lr2font" && path.size() > 13)
  {
    // /font.lr2font file
    std::string newpath = path.substr(0, path.size() - 13) + ".dxa";
    LoadLR2BitmapFont(newpath);
  }
  else
  {
    error_msg_ = "Unsupported type of font.";
    error_code_ = -1;
  }
}

void Font::Load(const char *p, size_t len, const char *ext_hint_opt)
{
  error_msg_ = "Font load from memory is not supported feature yet.";
  error_code_ = -1;
}

uint32_t StringToColor(const std::string &color_str)
{
  if (!color_str.empty() && color_str[0] == '#')
  {
    return HexStringToColor(color_str.c_str() + 1);
  }
  return 0;
}

void Font::Load(const MetricGroup& metrics)
{
  error_msg_ = 0;
  error_code_ = 0;
  std::string s_color;
  std::string s_border_color;

  // set font attributes from metrics
  metrics.get_safe("size", fontattr_.height);
  metrics.get_safe("color", s_color);
  metrics.get_safe("border-color", s_border_color);
  metrics.get_safe("border-width", fontattr_.outline_width);
  fontattr_.color = StringToColor(s_color);
  fontattr_.outline_color = StringToColor(s_border_color);

  // set font name if exists
  if (metrics.exist("name"))
    set_name(metrics.get_str("name"));

  Load(metrics.get_str("path"));
}

void Font::Clear()
{
  ClearGlyph();
  ReleaseFont();
}

void Font::Update(float)
{
  if (fbitmap_commitlist_.size() != 0) {
    std::lock_guard<std::mutex> lock(fbitmap_commitlock_);
    for (auto* b : fbitmap_commitlist_)
    {
      b->Update();
    }
    fbitmap_commitlist_.clear();
  }
}

// Callable from sub-thread.
void Font::CommitBitmap(FontBitmap *fbitmap)
{
  std::lock_guard<std::mutex> lock(fbitmap_commitlock_);
  for (auto *b : fbitmap_commitlist_)
    if (b == fbitmap) return;   // duplication check
  fbitmap_commitlist_.push_back(fbitmap);
}

bool Font::is_empty() const
{
  return ftface_[0] == 0 && glyph_.empty();
}

void Font::LoadFreetypeFont(const std::string &path)
{
  R_ASSERT(is_empty());
  std::vector<std::string> fontpaths;

  if (fontattr_.height <= 0)
  {
    error_msg_ = "Invalid font size (0)";
    error_code_ = -1;
    return;
  }
  if (ftLibCnt == 0)
  {
    error_msg_ = "Freetype is not initalized.";
    error_code_ = -1;
    return;
  }

  is_ttf_font_ = true;
  const int fntsize_pixel = fontattr_.height;

  /* Create freetype fonts */
  Split(path, ';', fontpaths);
  ftface_count_ = (unsigned)fontpaths.size();
  if (ftface_count_ > kMaxFallbackFonts)
  {
    error_msg_ = "Too many fallback fonts are specified.";
    error_code_ = -1;
    return;
  }
  for (unsigned i = 0; i < ftface_count_; ++i)
  {
    const char *ttfpath = fontpaths[i].c_str();
    int r = FT_New_Face(ftLib, ttfpath, 0, (FT_Face*)&ftface_[i]);
    if (r)
    {
      error_msg_ = "Cannot read font file.";
      error_code_ = -1;
      return;
    }
    // TODO: cleanup previous loaded ftface
    // Set size to load glyphs as
    FT_Set_Pixel_Sizes((FT_Face)ftface_[i], 0, fntsize_pixel);
  }

  /* Now, all fallback font properties are set by first one. */
  FT_Face ftface = (FT_Face)ftface_[0];

  /* Create freetype stroker (if necessary) */
  if (fontattr_.outline_width > 0)
  {
    FT_Stroker stroker;
    FT_Stroker_New(ftLib, &stroker);
    FT_Stroker_Set(stroker, fontattr_.outline_width * 32 /* XXX: is it good enough? */,
      FT_STROKER_LINECAP_ROUND, FT_STROKER_LINEJOIN_ROUND, 0);
    ftstroker_ = stroker;
  }
  else ftstroker_ = 0;

  // Set font baseline
  if (fontattr_.baseline_offset == 0)
  {
    fontattr_.baseline_offset = static_cast<int>(
      fntsize_pixel * (1.0f + (float)ftface->descender / ftface->height)
      );
  }

  // Prepare some basic glyphs at first ...
  uint32_t glyph_init[128];
  for (int i = 0; i < 128; i++)
    glyph_init[i] = i;
  PrepareGlyph(glyph_init, 128);
}

void Font::LoadLR2BitmapFont(const std::string &path)
{
  R_ASSERT(is_empty());

#if USE_LR2_FONT == 1
  DXAExtractor dxa;
  if (!dxa.Open(path.c_str()))
  {
    error_msg_ = "LR2Font : Cannot open file.";
    error_code_ = -1;
    return;
  }

  std::map<std::string, const DXAExtractor::DXAFile*> dxa_file_mapper;
  const DXAExtractor::DXAFile* dxa_file_lr2font = nullptr;
  for (const auto& f : dxa) {
    // make file mapper & find font file (LR2FONT)
    std::string fn = f.filename;
    if (fn.size() > 2 && fn[0] == '.' && fn[1] == '/')
      fn = fn.substr(2);
    dxa_file_mapper[Upper(fn)] = &f;
    if (Upper(GetExtension(fn)) == "LR2FONT")
      dxa_file_lr2font = &f;
  }

  if (!dxa_file_lr2font)
  {
    error_msg_ = "LR2Font : Invalid dxa file (no font info found)";
    error_code_ = -1;
    return;
  }

  // -- parse LR2FONT file --

  const char *p, *p_old, *p_end;
  p = p_old = dxa_file_lr2font->p;
  p_end = p + dxa_file_lr2font->len;
  std::vector<std::string> col;
  while (p < p_end)
  {
    while (p < p_end && *p != '\n' && *p != '\r')
      ++p;
    std::string cmd(p_old, p - p_old);
    while (*p == '\n' || *p == '\r') ++p;
    p_old = p;

    if (cmd.size() <= 2) continue;
    if (cmd[0] == '/' && cmd[1] == '/') continue;
    Split(cmd, ',', col);
    if (col[0] == "#S")
    {
      fontattr_.height = atoi(col[1].c_str());
      fontattr_.baseline_offset = fontattr_.height;
    }
    else if (col[0] == "#M")
    {
      // XXX: I don't know what is this ...
    }
    else if (col[0] == "#T")
    {
      const auto* f = dxa_file_mapper[Upper(col[2])];
      if (!f)
      {
        error_msg_ = "LR2Font : Cannot find texture.";  // col[1]
        error_code_ = -1;
        return;
      }
      std::unique_ptr<Image> img = std::make_unique<Image>();
      img->Load(f->p, f->len, nullptr);
      if (img->get_error_code() != 0)
      {
        error_msg_ = "LR2Font : Cannot load texture.";
        error_code_ = -1;
        return;
      }
      FontBitmap *fbitmap = new FontBitmap(
        (const uint32_t*)img->get_ptr(), img->get_width(), img->get_height()
      );
      fontbitmap_.push_back(fbitmap);
      CommitBitmap(fbitmap);
    }
    else if (col[0] == "#R")
    {
      /* glyphnum, texid, x, y, width, height */
      unsigned bitmapidx = atoi(col[2].c_str());
      FontGlyph g;
      g.codepoint = atoi(col[1].c_str());
      g.codepoint = ConvertLR2JIStoUTF16(g.codepoint);
      g.texture = 0;
      g.srcx = atoi(col[3].c_str());
      g.srcy = atoi(col[4].c_str());
      g.width = atoi(col[5].c_str());
      g.height = atoi(col[6].c_str());
      g.adv_x = g.width;
      g.pos_x = 0;
      g.pos_y = g.height;

      /* in case of unexpected texture */
      if (bitmapidx >= fontbitmap_.size())
        continue;
      g.texture = fontbitmap_[bitmapidx]->get_texture();

      /* need to calculate src pos by texture, so need to get texture. */
      float tex_width = (float)fontbitmap_[bitmapidx]->width();
      float tex_height = (float)fontbitmap_[bitmapidx]->height();

      /* if width / height is zero, indicates invalid glyph (due to image loading failed) */
      if (tex_width == 0 || tex_height == 0)
        continue;

      g.sx1 = (float)g.srcx / tex_width;
      g.sx2 = (float)(g.srcx + g.width) / tex_width;
      g.sy1 = (float)g.srcy / tex_height;
      g.sy2 = (float)(g.srcy + g.height) / tex_height;

      glyph_.push_back(g);
    }
  }
#else
  error_msg_ = "LR2font is not supported.";
  error_code_ = -1;
#endif
}

#if 0
void LR2Font::UploadTextureFile(const char* p, size_t len)
{
  FIMEMORY *memstream = FreeImage_OpenMemory((BYTE*)p, len);
  FREE_IMAGE_FORMAT fmt = FreeImage_GetFileTypeFromMemory(memstream);
  if (fmt == FREE_IMAGE_FORMAT::FIF_UNKNOWN)
  {
    std::cerr << "LR2Font open failed: invalid image internally." << std::endl;
    FreeImage_CloseMemory(memstream);
    return;
  }

  // from here, font bitmap texture must uploaded to memory.
  FIBITMAP *bitmap, *temp;
  temp = FreeImage_LoadFromMemory(fmt, memstream);
  // image need to be flipped
  FreeImage_FlipVertical(temp);
  bitmap = FreeImage_ConvertTo32Bits(temp);
  FreeImage_Unload(temp);

  int width = FreeImage_GetWidth(bitmap);
  int height = FreeImage_GetHeight(bitmap);
  const uint8_t* data_ptr = FreeImage_GetBits(bitmap);

  FontBitmap *fbitmap = new FontBitmap((const uint32_t*)data_ptr, width, height);
  fontbitmap_.push_back(fbitmap);

  // done, release all FreeImage resource
  FreeImage_Unload(bitmap);
}
#endif

#define BLEND_RGBA
#ifdef BLEND_RGBA
inline void __blend(unsigned char result[4], unsigned char fg[4], unsigned char bg[4])
{
  unsigned int alpha = fg[3] + 1;
  unsigned int inv_alpha = 256 - fg[3];
  result[0] = (unsigned char)((alpha * fg[0] + inv_alpha * bg[0]) >> 8);
  result[1] = (unsigned char)((alpha * fg[1] + inv_alpha * bg[1]) >> 8);
  result[2] = (unsigned char)((alpha * fg[2] + inv_alpha * bg[2]) >> 8);
  result[3] = bg[3];
}
#else
inline void __blend(unsigned char result[4], unsigned char fg[4], unsigned char bg[4])
{
  unsigned int alpha = fg[0] + 1;
  unsigned int inv_alpha = 256 - fg[0];
  result[1] = (unsigned char)((alpha * fg[1] + inv_alpha * bg[1]) >> 8);
  result[2] = (unsigned char)((alpha * fg[2] + inv_alpha * bg[2]) >> 8);
  result[3] = (unsigned char)((alpha * fg[3] + inv_alpha * bg[3]) >> 8);
  result[0] = bg[0];
}
#endif

void Font::PrepareText(const std::string& s)
{
  // TODO: stress test; PrepareText(...) as much as it can
  // (bitmap overflow check)
  uint32_t s32[1024];
  int s32len = 0;

  /* only available when TTF font */
  if (!ftface_[0]) return;

  ConvertStringToCodepoint(s, s32, s32len, 1024);
  PrepareGlyph(s32, s32len);
}

void Font::PrepareGlyph(uint32_t *chrs, int count)
{
  /* only available when TTF font */
  if (!ftface_[0]) return;

  FT_Face ftface = nullptr;
  FT_Stroker ftStroker = (FT_Stroker)ftstroker_;
  FT_Glyph glyph;
  FT_BitmapGlyph bglyph;
  FontGlyph g;
  unsigned gindex;
  bool existing_glyph = false;
  bool found_glyph = false;

  for (int i = 0; i < count; ++i)
  {
    /* check is glyph exists already before rendering */
    existing_glyph = false;
    for (auto &g : glyph_)
    {
      if (g.codepoint == chrs[i])
      {
        existing_glyph = true;
        break;
      }
    }
    if (existing_glyph)
      continue;

    /* Use FT_Load_Char, which calls and find Glyph index internally ... */
    /* and... just ignore failed character. */
    found_glyph = false;
    for (unsigned j = 0; j < ftface_count_; ++j)
    {
      ftface = (FT_Face)ftface_[j];
      gindex = FT_Get_Char_Index(ftface, chrs[i]);
      if (gindex == 0) continue;
      if (FT_Load_Glyph(ftface, gindex, FT_LOAD_NO_BITMAP) == 0)
      {
        found_glyph = true;
        break;
      }
    }
    if (!found_glyph)
      continue;

    FT_Get_Glyph(ftface->glyph, &glyph);
    FT_Glyph_To_Bitmap(&glyph, FT_RENDER_MODE_NORMAL, 0, 1);
    bglyph = (FT_BitmapGlyph)glyph;

    g.codepoint = chrs[i];
    g.width = bglyph->bitmap.width;
    g.height = bglyph->bitmap.rows;
    g.pos_x = bglyph->left;
    g.pos_y = bglyph->top;
    g.adv_x = ftface->glyph->advance.x >> 6;
    g.srcx = g.srcy = 0;
    g.texture = 0;

    /* generate bitmap only when size is valid */
    if (bglyph->bitmap.width > 0)
    {
      // generate foreground bitmap instantly
      // XXX: need texture option
      uint32_t* bitmap = (uint32_t*)malloc(g.width * g.height * sizeof(uint32_t));
      for (unsigned x = 0; x < g.width; ++x) {
        for (unsigned y = 0; y < g.height; ++y) {
          char a = bglyph->bitmap.buffer[x + y * g.width];
          bitmap[x + y * g.width] = a << 24 | (0x00ffffff & fontattr_.color);
        }
      }
      FT_Done_Glyph(glyph);

      // create outline if necessary.
      // XXX: need to check error code
      if (ftStroker)
      {
        FT_Get_Glyph(ftface->glyph, &glyph);
        FT_Glyph_Stroke(&glyph, ftStroker, 1);
        FT_Glyph_To_Bitmap(&glyph, FT_RENDER_MODE_NORMAL, 0, 1);
        bglyph = (FT_BitmapGlyph)glyph;

        // blending to bitmap
        // XXX: it's quite corpse code, but ... it works! anyway.
        uint32_t bit_, fgbit_;
        int border_offset_x = g.pos_x - bglyph->left;
        int border_offset_y = bglyph->top - g.pos_y;
        for (int x = 0; x < (int)bglyph->bitmap.width && x - border_offset_x < (int)g.width; ++x) {
          for (int y = 0; y < (int)bglyph->bitmap.rows && y - border_offset_y < (int)g.height; ++y) {
            if (x < border_offset_x || y < border_offset_y) continue;
            char a = bglyph->bitmap.buffer[x + y * bglyph->bitmap.width];
            fgbit_ = a << 24 | (0x00ffffff & fontattr_.outline_color);
            const size_t bitmap_offset = x - border_offset_x + (y - border_offset_y) * g.width;
            __blend((unsigned char*)&bit_, (unsigned char*)&fgbit_, (unsigned char*)&bitmap[bitmap_offset]);
            bitmap[bitmap_offset] = bit_;
          }
        }

        FT_Done_Glyph(glyph);
      }

      /* cut glyph if it's too big height */
      if (g.height > fontattr_.height)
        g.height = fontattr_.height;

      /* upload bitmap */
      auto* cache = GetWritableBitmapCache(g.width, g.height);
      R_ASSERT(cache);
      cache->Write(bitmap, g.width, g.height, g);
      g.texture = cache->get_texture();
      free(bitmap);
      CommitBitmap(cache);
    }

    glyph_.push_back(g);
  }
}

const FontAttribute &Font::GetAttribute() const { return fontattr_; }

/**
 * Get glyph from codepoint.
 * NullGlyph is returned if codepoint not found(cached).
 */
const FontGlyph* Font::GetGlyph(uint32_t chr) const
{
  // XXX: linear search, but is there other way to search glyphs faster?
  for (unsigned i = 0; i < glyph_.size(); ++i)
    if (glyph_[i].codepoint == chr)
      return &glyph_[i];
  return &null_glyph_;
}

/* Check is given glyph is NullGlyph */
bool Font::IsNullGlyph(const FontGlyph* g) const
{
  return g == &null_glyph_;
}

/* Set NullGlyph as specified codepoint */
void Font::SetNullGlyphAsCodePoint(uint32_t chr)
{
  const FontGlyph* g = GetGlyph(chr);
  if (!IsNullGlyph(g))
    null_glyph_ = *g;
}

float Font::GetTextWidth(const std::string& s)
{
  float r = 0;
  const FontGlyph* g = 0;
  uint32_t s32[1024];
  int s32len = 0;
  ConvertStringToCodepoint(s, s32, s32len, 1024);

  for (int i = 0; i < s32len; ++i)
  {
    g = GetGlyph(s32[i]);
    r += g->width;
  }

  return r;
}

void Font::GetTextVertexInfo(const std::string& s,
  std::vector<TextVertexInfo>& vtvi,
  bool do_line_breaking) const
{
  uint32_t s32[1024];
  int s32len = 0;
  ConvertStringToCodepoint(s, s32, s32len, 1024);

  // create textglyph
  std::vector<const FontGlyph*> textglyph;
  for (int i = 0; i < s32len; ++i)
    textglyph.push_back(GetGlyph(s32[i]));

  // create textvertex
  TextVertexInfo tvi;
  VertexInfo* vi = tvi.vi;
  int cur_x = 0, cur_y = 0, line_y = 0;
  for (const auto* g : textglyph)
  {
    // line-breaking
    if (g->codepoint == '\n')
    {
      if (do_line_breaking)
      {
        cur_x = 0;
        line_y += fontattr_.height;
      }
      continue;
    }

    // no-size glyph is ignored
    if (g->codepoint == 0 || g->texture == 0)
      continue;

    cur_x += g->pos_x;
    cur_y = line_y + fontattr_.baseline_offset - g->pos_y;

    vi[0].c = Vector4{ 1.0f };
    vi[0].p = Vector3{ cur_x, cur_y, .0f };
    vi[0].t = Vector2{ g->sx1, g->sy1 };

    vi[1].c = Vector4{ 1.0f };
    vi[1].p = Vector3{ cur_x + g->width, cur_y, .0f };
    vi[1].t = Vector2{ g->sx2, g->sy1 };

    vi[2].c = Vector4{ 1.0f };
    vi[2].p = Vector3{ cur_x + g->width, cur_y + g->height, .0f };
    vi[2].t = Vector2{ g->sx2, g->sy2 };

    vi[3].c = Vector4{ 1.0f };
    vi[3].p = Vector3{ cur_x, cur_y + g->height, .0f };
    vi[3].t = Vector2{ g->sx1, g->sy2 };

    cur_x += g->adv_x - g->pos_x;
    tvi.tex = g->texture;

    // skip in case of special character
    if (g->codepoint == ' ' || g->codepoint == '\r')
      continue;

    vtvi.push_back(tvi);
  }
}

void Font::ConvertStringToCodepoint(const std::string& s,
  uint32_t *s32, int& lenout, int maxlen) const
{
  if (maxlen < 0)
    maxlen = 0x7fffffff;

  // convert utf8 string to uint32 (utf32-le)
  std::string enc_str = rutil::ConvertEncoding(s, rutil::E_UTF32, rutil::E_UTF8);
  maxlen = std::min(maxlen - 1 /* last null character */, static_cast<int>(enc_str.size() / 4));
  const uint32_t *cp = reinterpret_cast<const uint32_t*>(enc_str.c_str());
  
  int i = 0;
  for (; i < maxlen; ++i)
    s32[i] = cp[i];
  s32[i] = 0;
  lenout = maxlen;
}

FontBitmap* Font::GetWritableBitmapCache(int w, int h)
{
  if (fontbitmap_.empty() || !fontbitmap_.back()->IsWritable(w, h))
  {
    if (!fontbitmap_.empty())
      fontbitmap_.back()->SetToReadOnly();
    fontbitmap_.push_back(new FontBitmap(defFontCacheWidth, defFontCacheHeight));
  }
  return fontbitmap_.back();
}

void Font::ClearGlyph()
{
  std::lock_guard<std::mutex> lock(fbitmap_commitlock_);

  // release texture and bitmap
  fbitmap_commitlist_.clear();
  for (auto *b : fontbitmap_)
    delete b;
  fontbitmap_.clear();

  // release glyph cache
  glyph_.clear();
  
  // reset null_glyph_ codepoint
  null_glyph_.codepoint = 0;
}

void Font::ReleaseFont()
{
  if (ftstroker_)
  {
    FT_Stroker ftstroker = (FT_Stroker)ftstroker_;
    FT_Stroker_Done(ftstroker);
    ftstroker_ = 0;
  }

  for (unsigned i = 0; i < ftface_count_; ++i)
  {
    FT_Face ft = (FT_Face)ftface_[i];
    FT_Done_Face(ft);
    ftface_[i] = 0;
  }
  ftface_count_ = 0;

  path_.clear();
  is_ttf_font_ = false;
  fontattr_.name.clear(); // clear name only
  error_code_ = 0;
  error_msg_ = 0;
}

const std::string& Font::get_path() const
{
  return path_;
}

bool Font::is_ttf_font() const
{
  return is_ttf_font_;
}

int Font::height() const
{
  return fontattr_.height;
}

void Font::DrawText(float x, float y, const std::string &text_utf8)
{
  std::vector<TextVertexInfo> tvi;

  if (x != .0f || y != .0f) {
    Vector3 t(x, y, 0.0f);
    GRAPHIC->PushMatrix();
    GRAPHIC->Translate(t);
  }

  PrepareText(text_utf8);
  GetTextVertexInfo(text_utf8, tvi, true /* line-break */);
  for (unsigned i = 0; i < tvi.size(); ++i)
  {
    GRAPHIC->SetTexture(0, **tvi[i].tex);
    GRAPHIC->DrawQuads(tvi[i].vi, 4);
  }

  if (x != .0f || y != .0f) {
    GRAPHIC->PopMatrix();
  }
}

}
