#pragma once

namespace ml
{
  class scene
  {
  public:


    virtual void on_attach() {}
    virtual void on_detach() {}

    virtual void on_before_update() {};
    virtual void on_update() {};
    virtual void on_after_update() {};

    virtual ~scene() = default;
  };
}