#pragma once
#include "Scene.h"
#include "Sprite.h"
#include "Font.h"
#include "LR2/LR2Font.h"

namespace rhythmus
{

class TestScene : public Scene
{
public:
  TestScene();
  virtual ~TestScene();

  virtual void LoadScene();
  virtual void CloseScene();
  virtual void Render();
  virtual void ProcessEvent(const GameEvent& e);
private:
  Sprite spr_;
  Sprite spr2_;
  Sprite spr_bg_;
  Font font_;
  Text text_;
  ImageAuto img_movie_;
  LR2Font lr2font_;
};

}