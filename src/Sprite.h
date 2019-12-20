#pragma once
#include "BaseObject.h"
#include "Image.h"
#include <vector>
#include <string>

namespace rhythmus
{

/* @brief Renderable object with texture */
class Sprite : public BaseObject
{
public:
  Sprite();
  Sprite(const Sprite& spr) = default;
  virtual ~Sprite();

  /* Set sprite's image by path */
  void SetImageByPath(const std::string &path);

  /* Set sprite's image by prefetch image.
   * @warn The image is not owned by this sprite,
   * so be aware of texture leaking. */
  void SetImage(Image *img);
  
  void ReplaySprite();

  Image *image();

  /* @brief Load property(resource). */
  virtual void Load(const Metric &metric);
  virtual void LoadFromLR2SRC(const std::string &cmd);

protected:
  Image *img_;
  bool img_owned_;

  // sprite animation property
  // divx : sprite division by x pos
  // divy : sprite division by y pos
  // cnt : total frame (smaller than divx * divy)
  // interval : loop time for sprite animation (milisecond)
  int divx_, divy_, cnt_, interval_;

  // current sprite animation frame
  int idx_;

  // eclipsed time of sprite animation
  int eclipsed_time_;

  // texture coordination
  float sx_, sy_, sw_, sh_;

  // source sprite size
  int source_x, source_y, source_width, source_height;

  // texture attribute (TODO)
  float tex_attribute_;

  virtual void doUpdate(float delta);
  virtual void doRender();

  virtual const CommandFnMap& GetCommandFnMap();
};

/* Sprite may not need to be shared. */
using SpriteAuto = std::unique_ptr<Sprite>;

}