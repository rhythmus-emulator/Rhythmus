/**
 * @brief
 *
 * This file declare BaseObject, which is common interface
 * to all renderable objects.
 *
 * It includes attribute with child/parent, coordinate value
 * just as DOM object, but without Texture attributes.
 * (Texture attribute is implemented in Sprite class)
 *
 * BaseObject itself is AST tree, which parses command to
 * fill rendering semantic.
 * TODO: detach parser from BaseObject and change input like
 * xml tree, instead of string command.
 *
 * Also, coordinate related utility functions declared in
 * this file.
 */

#pragma once

#include "Graphic.h"
#include "Event.h"
#include "Setting.h"

#include <string>
#include <vector>
#include <list>
#include <map>
#include <memory>
#include <functional>

namespace rhythmus
{

class BaseObject;
class CommandArgs;

/** @brief Drawing properties of Object */
struct DrawProperty
{
  float x, y, w, h;         // general object position
  float r, g, b;            // alpha masking
  float aTL, aTR, aBR, aBL; // alpha fade
  float sx, sy, sw, sh;     // texture src crop
  ProjectionInfo pi;
  bool display;             // display: show or hide
};

/** @brief Command function type for object. */
typedef std::function<void(void*, CommandArgs&)> CommandFn;

/** @brief Command mapping for object. */
typedef std::map<std::string, CommandFn> CommandFnMap;

/** @brief Tweens' ease type */
enum EaseTypes
{
  kEaseNone,
  kEaseLinear,
  kEaseIn,
  kEaseOut,
  kEaseInOut,
  kEaseInOutBack,
};

/**
 * @brief
 * Declaration for state of current tween
 * State includes:
 * - Drawing state (need to be animated)
 * - Tween itself information: Easetype, Time (duration), ...
 * - Loop
 * - Tween ease type
 * - Event to be called (kind of triggering)
 */
struct TweenState
{
  DrawProperty draw_prop;
  uint32_t time_duration;   // original duration time of this loop
  uint32_t time_loopstart;  // time to start when looping
  uint32_t time_eclipsed;   // current tween's eclipsed time
  bool loop;                // should this tween reused?
  int ease_type;            // tween ease type
  std::string commands;     // commands to be triggered when this tween starts
};

using Tween = std::list<TweenState>;

/**
 * @brief
 * Common interface to all renderable objects.
 * - which indicates containing its own frame coordinate value.
 */
class BaseObject : public EventReceiver
{
public:
  BaseObject();
  BaseObject(const BaseObject& obj);
  virtual ~BaseObject();

  void set_name(const std::string& name);
  const std::string& get_name() const;

  /* Add child to be updated / rendered. */
  void AddChild(BaseObject* obj);

  /* Remove child to be updated / rendered. */
  void RemoveChild(BaseObject* obj);

  void RemoveAllChild();

  /**
   * @brief
   * Make cacsading relationship to object.
   * If parent object is deleted, then registered children are also deleted.
   */
  void RegisterChild(BaseObject* obj);

  void set_parent(BaseObject* obj);
  BaseObject* get_parent();
  BaseObject* FindChildByName(const std::string& name);
  BaseObject* FindRegisteredChildByName(const std::string& name);
  BaseObject* GetLastChild();

  void RunCommandByName(const std::string &name);
  void RunCommand(std::string command);
  void ClearCommand(const std::string &name);
  void DeleteAllCommand();
  void QueueCommand(const std::string &command);
  void LoadCommand(const std::string &name, const std::string &command);
  void LoadCommand(const Metric& metric);
  void LoadCommandWithPrefix(const std::string &prefix, const Metric& metric);
  void LoadCommandWithNamePrefix(const Metric& metric);

  /* alias for LoadCommand */
  void AddCommand(const std::string &name, const std::string &command);

  /**
   * @brief
   * Run single command which mainly changes mutable attribute(e.g. tween)
   * of the object.
   * Types of executable command and function are mapped
   * in GetCommandFnMap() function, which is refered by this procedure.
   */
  void RunCommand(const std::string &commandname, const std::string& value);

  /* @brief Load property(resource). */
  virtual void Load(const Metric &metric);
  void LoadByText(const std::string &metric_text);
  void LoadByName();

  /* @brief Load from LR2SRC (for LR2) */
  virtual void LoadFromLR2SRC(const std::string &cmd);

  /* @brief Inherited from EventReceiver */
  virtual bool OnEvent(const EventMessage& msg);

  void AddTweenState(const DrawProperty &draw_prop, uint32_t time_duration,
    int ease_type = EaseTypes::kEaseOut, bool loop = false);
  void SetTweenTime(int time_msec);
  void SetDeltaTweenTime(int time_msec);
  void StopTween();
  uint32_t GetTweenLength() const;

  DrawProperty& GetDestDrawProperty();
  DrawProperty& get_draw_property();
  void SetX(int x);
  void SetY(int y);
  void SetWidth(int w);
  void SetHeight(int h);
  void SetOpacity(float opa);
  void SetClip(bool clip);
  void SetPos(int x, int y);
  void MovePos(int x, int y);
  void SetSize(int w, int h);
  void SetAlpha(unsigned a);
  void SetAlpha(float a);
  void SetRGB(unsigned r, unsigned g, unsigned b);
  void SetRGB(float r, float g, float b);
  void SetScale(float x, float y);
  void SetRotation(float x, float y, float z);
  void SetRotationAsRadian(float x, float y, float z);

  /**
   * -1: Use absolute coord
   * 0: Center
   * 1: Bottomleft
   * 2: Bottomcenter
   * ... (Same as numpad position)
   */
  void SetRotationCenter(int rot_center);
  void SetRotationCenterCoord(float x, float y);
  /**
   * refer: enum EaseTypes
   */
  void SetAcceleration(int acc);
  void SetVisibleGroup(int group0 = 0, int group1 = 0, int group2 = 0);
  void SetIgnoreVisibleGroup(bool ignore);
  void Hide();
  void Show();
  void SetDrawOrder(int order);
  int GetDrawOrder() const;
  void SetAllTweenPos(int x, int y);
  void SetAllTweenScale(float w, float h);
  virtual void SetText(const std::string &value);
  virtual void SetNumber(int number);
  virtual void SetNumber(double number);
  /* Refresh value from resource id (if it has) */
  virtual void Refresh();
  /* Set value automatically using Refresh() method from resource id */
  void SetResourceId(int id);
  /* -1: no resource */
  int GetResourceId() const;

  void SetFocusable(bool is_focusable);
  virtual bool IsEntered(float x, float y);
  void SetHovered(bool is_hovered);
  void SetFocused(bool is_focused);
  bool IsHovered() const;
  bool IsFocused() const;
  virtual void Click();

  /**
   * 0: Don't use blending (always 100% alpha)
   * 1: Use basic alpha blending (Default)
   * 2: Use color blending instead of alpha channel
   */
  void SetBlend(int blend_mode);

  void SetLR2DSTCommand(const std::string &lr2dst);

  bool IsTweening() const;
  bool IsVisible() const;

  void Update(float delta);
  void Render();

  bool operator==(const BaseObject& o) {
    return o.get_name() == get_name();
  }

private:
  void SetLR2DST(int time, int x, int y, int w, int h, int acc_type,
    int a, int r, int g, int b, int blend, int filter, int angle,
    int center);

protected:
  std::string name_;

  BaseObject *parent_;

  // children for rendering (not released when this object is destructed)
  std::vector<BaseObject*> children_;

  // owned children list (released when its destruction)
  std::vector<BaseObject*> owned_children_;

  // drawing order
  int draw_order_;

  // description of drawing motion
  Tween tween_;

  // current cached drawing state
  DrawProperty current_prop_;

  // rotation center property
  int rot_center_;

  // group for visibility
  // @warn 4th group: for special use (e.g. panel visibility of onmouse)
  int visible_group_[4];

  // ignoring visible group
  bool ignore_visible_group_;

  // Resource id for text/number value
  int resource_id_;

  // blending properties for image/text.
  int blending_;

  // is this object focusable?
  bool is_focusable_;

  // is this object currently focused?
  bool is_focused_;

  // is object is currently hovered?
  bool is_hovered_;

  // do clipping when rendering object, including children?
  bool do_clipping_;

  // commands to be called
  std::map<std::string, std::string> commands_;

  // information for debugging
  std::string debug_;

  // Tween
  void UpdateTween(float delta);
  void SetTweenLoopTime(int loopstart_time_msec);

  void FillVertexInfo(VertexInfo *vi);
  virtual void doUpdate(float delta);
  virtual void doRender();
  virtual void doUpdateAfter(float delta);
  virtual void doRenderAfter();

  virtual const CommandFnMap& GetCommandFnMap();
  
  void SetLR2DSTCommandInternal(const CommandArgs &args);
};

void MakeTween(DrawProperty& ti, const DrawProperty& t1, const DrawProperty& t2,
  double r, int ease_type);

}