#include "ResourceManager.h"
#include "LR2/LR2Font.h"
#include <FreeImage.h>
#include <iostream>

namespace rhythmus
{
  
ImageAuto ResourceManager::LoadImage(const std::string& path)
{
  /* Most image won't be shared from scene to scene, so just load it now */
  Image* img = new Image();
  img->LoadFromPath(path);
  return ImageAuto(img);
}

FontAuto ResourceManager::LoadFont(const std::string& path, FontAttributes& attrs)
{
  for (const auto f : fonts_)
  {
    if (f->get_path() == path && f->is_ttf_font() && f->get_attribute() == attrs)
      return f;
  }

  FontAuto f(new Font());
  f->LoadFont(path.c_str(), attrs);
  fonts_.push_back(f);
  return f;
}

FontAuto ResourceManager::LoadLR2Font(const std::string& path)
{
  for (const auto f : fonts_)
  {
    if (f->get_path() == path && !f->is_ttf_font())
      return f;
  }

  LR2Font* f = new LR2Font();
  f->ReadLR2Font(path.c_str());
  FontAuto fa;
  fa.reset(f);
  return fa;
}

void ResourceManager::ReleaseImage(ImageAuto img)
{
}

void ResourceManager::ReleaseFont(FontAuto font)
{
  auto ii = fonts_.begin();
  for (; ii != fonts_.end(); ++ii)
  {
    if (*ii == font)
      break;
  }
  if (ii != fonts_.end())
    fonts_.erase(ii);
}

ResourceManager& ResourceManager::getInstance()
{
  static ResourceManager m;
  return m;
}

ResourceManager::ResourceManager()
{
}

ResourceManager::~ResourceManager()
{
}

}