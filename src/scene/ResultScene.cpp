#include "ResultScene.h"

namespace rhythmus
{

ResultScene::ResultScene()
{
  set_name("ResultScene");
  next_scene_mode_ = GameSceneMode::kGameSceneModeSelect;
}

void ResultScene::LoadScene()
{
  // TODO: place this code to Game setting
  Game::getInstance().SetAttribute(
    "ResultScene", "../themes/WMIX_HD/result/WMIX_RESULT.lr2skin"
  );

  Scene::LoadScene();
}

void ResultScene::StartScene()
{
}

void ResultScene::CloseScene()
{
  Scene::CloseScene();
}

bool ResultScene::ProcessEvent(const EventMessage& e)
{
  if (e.IsInput() && !IsEventValidTime(e))
    return true;

  if (e.IsKeyDown())
  {
    FinishScene();
  }

  return true;
}

}