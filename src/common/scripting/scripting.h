#ifndef BRANEENGINE_SCRPTING_H
#define BRANEENGINE_SCRPTING_H

#include "runtime/module.h"

class Scripting : public Module
{

  public:
    Scripting() = default;

    void start() override;

    void stop() override;
};

#endif // BRANEENGINE_SCRPTING_H
