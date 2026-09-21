#pragma once

namespace npm {

class Application;

class Ui {
public:
    static void ApplyTheme();
    static void Draw(Application& app);
};

}  // namespace npm
