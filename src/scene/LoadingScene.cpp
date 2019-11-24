#include "LoadingScene.h"
#include "Song.h"
#include <iostream>

namespace rhythmus
{

// singletons used for loading context

std::string current_loading_file;

// ------------------------- class LoadingScene

LoadingScene::~LoadingScene()
{
  set_name("LoadingScene");
  next_scene_ = "SelectScene";
  prev_scene_ = "Exit";
}

void LoadingScene::LoadScene()
{
  current_file_text_.SetSystemFont();
  message_text_.SetSystemFont();

  message_text_.SetPos(
    320,
    Graphic::getInstance().width() - 160
  );
  current_file_text_.SetPos(
    320,
    Graphic::getInstance().height() - 120
  );
  loading_bar_.SetPos(
    240,
    Graphic::getInstance().height() - 120
  );

  // Register childs
  AddChild(&message_text_);
  AddChild(&current_file_text_);
  AddChild(&loading_bar_);
}

void LoadingScene::StartScene()
{
  SongList::getInstance().Load();
  message_text_.SetText("Song loading ...");
}

void LoadingScene::ProcessInputEvent(const InputEvent& e)
{
  if (e.type() == InputEvents::kOnKeyUp)
  {
    if (e.KeyCode() == GLFW_KEY_ESCAPE)
    {
      // cancel all loading thread and exit game instantly
      Game::Exit();
    }
    else if (SongList::getInstance().is_loaded())
    {
      CloseScene(true);
    }
  }
}

void LoadingScene::doUpdate(float)
{
  static bool check_loaded = false;

  if (!SongList::getInstance().is_loaded())
  {
    std::string path = SongList::getInstance().get_loading_filename();
    int prog = static_cast<int>(SongList::getInstance().get_progress() * 100);
    message_text_.SetText("Loading " + std::to_string(prog) + "%");
    current_file_text_.SetText(path);
  }
  else
  {
    if (!check_loaded)
    {
      // run first time when loading is done
      std::cout << "LoadingScene: Song list loading finished." << std::endl;
      SongList::getInstance().select(0);  // select first item
      EventManager::SendEvent("SongListLoadFinished");
      check_loaded = true;
    }

    current_file_text_.Clear();
    message_text_.SetText("Ready ...!");
#if 0
    Game::getInstance().SetNextGameMode(GameMode::kGameModeSelect);
    Game::getInstance().ChangeGameMode();
#endif
  }
}

}