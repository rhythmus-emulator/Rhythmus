#pragma once
#include "Scene.h"
#include "Game.h"
#include "Event.h"

namespace rhythmus
{

class BaseObject;

class SceneManager : public InputEventReceiver
{
public:
  static void Initialize();
  static void Cleanup();

  void Update();
  void Render();

  Scene* get_current_scene();

  void ChangeScene(const std::string &scene_name);

  /* clear out focus */
  void ClearFocus();

  /* clear out focus of specific object
   * This must be called before deleting object.
   * If not, SceneManager may refer to dangling pointer. */
  void ClearFocus(BaseObject *obj);

  BaseObject *GetHoveredObject();
  BaseObject *GetFocusedObject();
  BaseObject *GetDraggingObject();

  /* from InputEventReceiver */
  virtual void OnInputEvent(const InputEvent& e);

private:
  SceneManager();
  ~SceneManager();

  // overlay-displayed scene
  // first element MUST be OverlayScene object.
  std::vector<Scene*> overlay_scenes_;

  // currently displaying scene
  Scene *current_scene_;

  // background scene (mostly for BGA)
  Scene* background_scene_;

  // next scene cached
  Scene *next_scene_;

  // currently hovered object
  BaseObject *hovered_obj_;

  // currently focused object
  BaseObject *focused_obj_;

  // currently dragging object
  BaseObject *dragging_obj_;

  // previous x, y coordinate (for dragging event)
  float px, py;
};

/* singleton object. */
extern SceneManager *SCENEMAN;

}

