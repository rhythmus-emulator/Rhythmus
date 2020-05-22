#pragma once

#include "Font.h"
#include "Util.h"

RHYTHMUS_NAMESPACE_BEGIN

/**
 * text alignment option
 * 0 : normal
 * 1 : left
 * 2 : right
 * 3 : offset -50% of text width
 * 4 : offset -100% of text width
 * (LR2 legacy option)
 */
enum TextAlignments
{
  kTextAlignLeft,
  kTextAlignRight,
  kTextAlignCenter,
  kTextAlignLR2Right,
  kTextAlignLR2Center,
};

/**
 * Text fitting option
 * LR2 uses Stretch to maxsize generally.
 */
enum TextFitting
{
  kTextFitNone,
  kTextFitMaxSize,
  kTextFitStretch,
};

class Text : public BaseObject
{
public:
  Text();
  virtual ~Text();

  virtual void Load(const MetricGroup& metric);

  void SetFont(const std::string& path);
  void SetFont(const MetricGroup &m);
  void SetSystemFont();
  void ClearFont();

  float GetTextWidth() const;
  virtual void SetText(const std::string& s);
  void ClearText();
  virtual void Refresh();
  void SetTextAlignment(TextAlignments align);
  void SetTextFitting(TextFitting fitting);
  void SetLineBreaking(bool enable_line_break);

  Font *font();

protected:
  virtual void SetLR2Alignment(int alignment);
  virtual void doRender();
  virtual void doUpdate(double);
  void UpdateTextRenderContext();
  TextVertexInfo& AddTextVertex(const TextVertexInfo &tvi);
  void SetTextVertexCycle(size_t cycle, size_t duration);
  void SetWidthMultiply(float multiply); /* special API for LR2 */

private:
  // Font.
  Font *font_;

  // text to be rendered
  std::string text_;

  // text_rendering related context
  struct {
    // glyph vertex to be rendered (generated by textglyph)
    std::vector<TextVertexInfo> textvertex;

    // calculated text width / height
    float width, height;

    // cycle attributes (optional)
    // if cycle exists, textvertex must be multiply of cycle.
    size_t cycles, duration, time;
  } text_render_ctx_;

  TextAlignments text_alignment_;
  TextFitting text_fitting_;

  // additional font attributes, which is set internally by font_alignment_ option.
  struct {
    // scale x / y
    float sx, sy;
    // translation x / y
    float tx, ty;
  } alignment_attrs_;

  float width_multiply_;

  int blending_;

  // is font texture is loaded?
  // (if not, need to be refreshed few moments.)
  bool is_texture_loaded_;

  // internal counter for updating Text object intervally.
  unsigned counter_;

  std::string *res_id_;

  // is line-breaking enabled?
  bool do_line_breaking_;
};

RHYTHMUS_NAMESPACE_END
