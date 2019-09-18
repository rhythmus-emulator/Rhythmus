#include "Scene.h"
#include "SceneManager.h"       /* preference */
#include "LR2/LR2SceneLoader.h" /* for lr2skin file load */
#include "LR2/LR2Sprite.h"
#include "LR2/LR2Font.h"
#include "LR2/LR2Text.h"
#include "rutil.h"              /* for string modification */
#include "Util.h"
#include <iostream>

namespace rhythmus
{

// -------------------------------- Scene

Scene::Scene()
  : fade_time_(0), fade_duration_(0), input_available_time_(0), focused_object_(nullptr)
{
  memset(&theme_param_, 0, sizeof(theme_param_));
}

void Scene::LoadScene()
{
  // Check is preference file exist for current scene
  // If so, load that file to actor.
  std::string scene_name = get_name();
  if (!scene_name.empty())
  {
    std::string scene_path =
      Game::getInstance().GetAttribute<std::string>(scene_name);
    if (!scene_path.empty())
    {
      if (rutil::endsWith(scene_path, ".lr2skin", false))
      {
        LoadFromCsv(scene_path);
      }
      else
      {
        std::cerr << "[Error] Scene " << scene_name <<
          " does not support file: " << scene_path << std::endl;
      }
    }
    LoadOptions();

    // TODO: check for invalid option, and fix if it's invalid
    // (e.g. invalid file path)
  }
  else
  {
    std::cerr << "[Warning] Scene has no name." << std::endl;
  }

  for (auto *obj : children_)
    obj->Load();
}

void Scene::StartScene()
{
  // Prepare to trigger scenetime if necessary
  if (theme_param_.next_scene_time > 0)
  {
    QueueSceneEvent(theme_param_.next_scene_time, Events::kEventSceneTimeout);
  }

  // set input avail time
  input_available_time_ = Timer::GetGameTimeInMillisecond() + theme_param_.begin_input_time;
}

void Scene::CloseScene()
{
  if (!get_name().empty())
    SaveOptions();

  // Trigger FadeOut & ChangeScene event
  TriggerFadeOut(theme_param_.fade_out_time);
  QueueSceneEvent(theme_param_.fade_out_time, Events::kEventSceneChange);
}

void Scene::RegisterImage(ImageAuto img)
{
  images_.push_back(img);
}

ImageAuto Scene::GetImageByName(const std::string& name)
{
  for (const auto& img : images_)
    if (img->get_name() == name) return img;
  return nullptr;
}

FontAuto Scene::GetFontByName(const std::string& name)
{
  for (const auto& font : fonts_)
    if (font->get_name() == name) return font;
  return nullptr;
}

void Scene::LoadOptions()
{
  ASSERT(get_name().size() > 0);
  auto& setting = SceneManager::getSetting();
  setting.SetPreferenceGroup(get_name());

  SettingList slist;
  setting.GetAllPreference(slist);
  for (auto ii : slist)
  {
    SetThemeConfig(ii.name, ii.value);
  }
}

void Scene::SaveOptions()
{
  ASSERT(get_name().size() > 0);
  auto& setting = SceneManager::getSetting();
  setting.SetPreferenceGroup(get_name());

  for (auto& t : theme_options_)
  {
    setting.SetOption(t.name, t.type, t.desc, t.options, t.selected);
  }
}

bool Scene::IsEventValidTime(const EventMessage& e) const
{
  return input_available_time_ < e.GetTimeInMilisecond();
}

void Scene::doUpdate(float delta)
{
  // Update images
  for (const auto& img: images_)
    img->Update(delta);

  // scheduled events
  eventqueue_.Update(delta);

  // fade in/out processing
  if (fade_duration_ != 0)
  {
    fade_time_ += delta;
    if (fade_duration_ > 0 && fade_time_ > fade_duration_)
    {
      fade_duration_ = 0;
      fade_time_ = 0;
    }
  }
}

void Scene::doRenderAfter()
{
  static VertexInfo vi[4] = {
    {0, 0, 0.1f, 0, 0, 1, 1, 1, 1},
    {0, 0, 0.1f, 1, 0, 1, 1, 1, 1},
    {0, 0, 0.1f, 1, 1, 1, 1, 1, 1},
    {0, 0, 0.1f, 0, 1, 1, 1, 1, 1}
  };

  // implementation of fadeout effect
  if (fade_duration_ != 0)
  {
    float fade_alpha_ = fade_duration_ > 0 ?
      1.0f - fade_time_ / fade_duration_ :
      fade_time_ / -fade_duration_;
    if (fade_alpha_ > 1)
      fade_alpha_ = 1;

    float w = Game::getInstance().get_window_width();
    float h = Game::getInstance().get_window_height();
    vi[1].x = w;
    vi[2].x = w;
    vi[2].y = h;
    vi[3].y = h;
    vi[0].a = vi[1].a = vi[2].a = vi[3].a = fade_alpha_;
    Graphic::SetTextureId(0);
    glColor3f(0, 0, 0);
    memcpy(Graphic::get_vertex_buffer(), vi, sizeof(VertexInfo) * 4);
    Graphic::RenderQuad();
  }
}

void Scene::TriggerFadeIn(float duration)
{
  if (fade_duration_ != 0) return;
  fade_duration_ = duration;
  fade_time_ = 0;
}

void Scene::TriggerFadeOut(float duration)
{
  if (fade_duration_ != 0) return;
  fade_duration_ = -duration;
  fade_time_ = 0;
}

void Scene::QueueSceneEvent(float delta, int event_id)
{
  eventqueue_.QueueEvent(event_id, delta);
}

constexpr char* kLR2SubstitutePath = "LR2files/Theme";
constexpr char* kSubstitutePath = "../themes";

void Scene::LoadProperty(const std::string& prop_name, const std::string& value)
{
  static std::string _prev_prop_name = prop_name;
  std::vector<std::string> params;

  // LR2 type properties
  if (strncmp(prop_name.c_str(), "#SRC_", 5) == 0)
  {
    // If #SRC came in serial, we need to reuse last object...
    // kinda trick: check previous command
    BaseObject* obj = nullptr;
    if (_prev_prop_name == prop_name)
      obj = GetLastChild();
    else
    {
      std::string type_name = prop_name.substr(5);
      if (type_name == "IMAGE")
        obj = new LR2Sprite();
      else if (type_name == "TEXT")
        obj = new LR2Text();
      else
      {
        obj = new BaseObject();
        obj->set_name(type_name);
      }
      RegisterChild(obj);
      AddChild(obj);
    }

    if (obj)
      obj->LoadProperty(prop_name, value);
  }
  else if (strncmp(prop_name.c_str(), "#DST_", 5) == 0)
  {
    BaseObject* obj = GetLastChild();
    if (!obj)
    {
      std::cout << "LR2Skin Load warning : DST command found without SRC, ignored." << std::endl;
      return;
    }
    obj->LoadProperty(prop_name, value);
  }
  // XXX: register such objects first?
  else if (prop_name == "#IMAGE")
  {
    ImageAuto img;
    std::string imgname = GetFirstParam(value);
    std::string imgpath;
    ThemeOption* option = GetThemeOption(imgname);
    if (option)
    {
      // create img path from option
      imgpath = option->selected;
    }
    else
    {
      imgpath = imgname;
    }
    imgpath = Substitute(imgpath, kLR2SubstitutePath, kSubstitutePath);
    img = ResourceManager::getInstance().LoadImage(imgpath);
    // TODO: set colorkey
    img->CommitImage();
    char name[10];
    itoa(images_.size(), name, 10);
    img->set_name(name);
    images_.push_back(img);
  }
  else if (prop_name == "#LR2FONT")
  {
    std::string fntname = GetFirstParam(value);
    std::string fntpath = Substitute(fntname, kLR2SubstitutePath, kSubstitutePath);
    // convert filename path to .dxa
    auto ri = fntpath.rfind('/');
    if (ri != std::string::npos && stricmp(fntpath.substr(ri).c_str(), "/font.lr2font") == 0)
      fntpath = fntpath.substr(0, ri) + ".dxa";
    char name[10];
    itoa(fonts_.size(), name, 10);
    fonts_.push_back(ResourceManager::getInstance().LoadLR2Font(fntpath));
    fonts_.back()->set_name(name);
  }
  else if (prop_name == "#BAR_CENTER" || prop_name == "#BAR_AVAILABLE")
  {
    // TODO: set attribute to theme_param_
  }
  else if (prop_name == "#INFORMATION")
  {
    MakeParamCountSafe(value, params, 4);
    theme_param_.gamemode = params[0];
    theme_param_.title = params[1];
    theme_param_.maker = params[2];
    theme_param_.preview = params[3];
  }
  else if (prop_name == "#CUSTOMOPTION")
  {
    MakeParamCountSafe(value, params, 4);
    std::string name = params[1];
    ThemeOption *saved_option = GetThemeOption(name);
    std::string options_str;
    std::string option_default;
    {
      int i;
      for (i = 2; i < params.size() - 1; ++i)
        options_str += params[i] + ",";
      options_str.pop_back();
      option_default = params[i];
    }

    if (saved_option)
    {
      saved_option->type = "option";
      saved_option->desc = params[0];
      saved_option->options = options_str;
    }
    else
    {
      ThemeOption options;
      options.type = "option";
      options.name = name;
      options.desc = params[0];
      options.options = options_str;
      options.selected = option_default;
      theme_options_.push_back(options);
    }
  }
  else if (prop_name == "#CUSTOMFILE")
  {
    MakeParamCountSafe(value, params, 3);
    std::string name = params[1];
    ThemeOption *saved_option = GetThemeOption(name);
    std::string path_prefix = Substitute(params[1], kLR2SubstitutePath, kSubstitutePath);

    if (saved_option)
    {
      saved_option->type = "file";
      saved_option->name = name;
      saved_option->desc = params[0];
      saved_option->options = path_prefix;
    }
    else
    {
      ThemeOption options;
      options.type = "file";
      options.name = name;
      options.desc = params[0];
      options.options = path_prefix;
      options.selected = Replace(options.options, "*", params[2]);
      theme_options_.push_back(options);
    }
  }
  else if (prop_name == "#TRANSCLOLR")
  {
    MakeParamCountSafe(value, params, 3);
    theme_param_.transcolor[0] = atoi(params[0].c_str());
    theme_param_.transcolor[1] = atoi(params[1].c_str());
    theme_param_.transcolor[2] = atoi(params[2].c_str());
  }
  else if (prop_name == "#STARTINPUT" || prop_name == "#IGNOREINPUT")
  {
    std::string v = GetFirstParam(value);
    theme_param_.begin_input_time = atoi(v.c_str());
  }
  else if (prop_name == "#FADEOUT")
  {
    theme_param_.fade_out_time = atoi(GetFirstParam(value).c_str());
  }
  else if (prop_name == "#FADEIN")
  {
    theme_param_.fade_in_time = atoi(GetFirstParam(value).c_str());
  }
  else if (prop_name == "#SCENETIME")
  {
    theme_param_.next_scene_time = atoi(GetFirstParam(value).c_str());
  }
  /*
  else if (prop_name == "#HELPFILE")
  {
  }*/
}

void Scene::LoadFromCsv(const std::string& filepath)
{
  LR2SceneLoader loader;
  loader.SetSubStitutePath("LR2files/Theme", kSubstitutePath);
  loader.Load(filepath);

  for (auto &v : loader)
  {
    LoadProperty(v.first, v.second);
  }
}

void Scene::SetThemeConfig(const std::string& key, const std::string& value)
{
  for (auto& t : theme_options_)
  {
    if (t.name == key)
      t.selected = value;
    return;
  }
}

ThemeOption* Scene::GetThemeOption(const std::string& key)
{
  for (auto& t : theme_options_)
  {
    if (t.name == key)
      return &t;
  }
  return nullptr;
}

}